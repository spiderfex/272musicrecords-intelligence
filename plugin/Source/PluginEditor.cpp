#include "PluginEditor.h"

namespace
{
    const juce::Colour bg { 17, 22, 32 }, panel { 29, 40, 57 }, panel2 { 22, 30, 43 };
    const juce::Colour grid { 48, 75, 101 }, cyan { 70, 218, 238 };
    const juce::Colour white { 238, 244, 250 }, muted { 150, 169, 190 }, green { 32, 224, 139 };

    void label (juce::Graphics& g, const juce::String& s, juce::Rectangle<int> r, float size,
                juce::Colour colour, bool bold = false,
                juce::Justification align = juce::Justification::centredLeft)
    {
        g.setColour (colour);
        g.setFont (juce::Font (juce::FontOptions (size).withStyle (bold ? "Bold" : "Regular")));
        g.drawText (s, r, align, true);
    }
}

IntelligenceAudioProcessorEditor::IntelligenceAudioProcessorEditor (IntelligenceAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setResizable (true, true);
    setResizeLimits (900, 560, 1500, 900);
    setSize (1120, 680);

    intelligenceButton.setColour (juce::ToggleButton::textColourId, white);
    intelligenceButton.setColour (juce::ToggleButton::tickColourId, cyan);
    intelligenceButton.setColour (juce::ToggleButton::tickDisabledColourId, muted);
    addAndMakeVisible (intelligenceButton);
    enabledAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>
        (audioProcessor.parameters, "enabled", intelligenceButton);

    auto setup = [this] (juce::Slider& slider, juce::Label& controlLabel, const juce::String& name)
    {
        slider.setSliderStyle (juce::Slider::LinearHorizontal);
        slider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 54, 24);
        slider.setTextValueSuffix (" %");
        slider.setColour (juce::Slider::trackColourId, cyan);
        slider.setColour (juce::Slider::thumbColourId, white);
        slider.setColour (juce::Slider::textBoxTextColourId, white);
        slider.setColour (juce::Slider::textBoxBackgroundColourId, panel);
        slider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        addAndMakeVisible (slider);

        controlLabel.setText (name, juce::dontSendNotification);
        controlLabel.setColour (juce::Label::textColourId, muted);
        controlLabel.setJustificationType (juce::Justification::centredLeft);
        addAndMakeVisible (controlLabel);
    };

    setup (recoverSlider, recoverLabel, "RECOVER");
    recoverSlider.setRange (0.0, 200.0, 0.1);
    setup (tameSlider, tameLabel, "TAME");
    tameSlider.setRange (0.0, 200.0, 0.1);
    setup (biasSlider, biasLabel, "BIAS");
    biasSlider.setRange (-100.0, 100.0, 0.1);
    setup (brightenSlider, brightenLabel, "BRIGHTEN");
    brightenSlider.setRange (-100.0, 100.0, 0.1);
    setup (boostSlider, boostLabel, "BOOST");
    boostSlider.setRange (-50.0, 50.0, 0.1);
    boostSlider.setTextValueSuffix (" dB");
    setup (outputSlider, outputLabel, "OUTPUT");
    outputSlider.setRange (-24.0, 12.0, 0.01);
    outputSlider.setTextValueSuffix (" dB");

    recoverAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>
        (audioProcessor.parameters, "recover", recoverSlider);
    tameAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>
        (audioProcessor.parameters, "tame", tameSlider);
    biasAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>
        (audioProcessor.parameters, "bias", biasSlider);
    brightenAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>
        (audioProcessor.parameters, "brighten", brightenSlider);
    boostAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>
        (audioProcessor.parameters, "boost", boostSlider);
    outputAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>
        (audioProcessor.parameters, "output", outputSlider);

    bypassButton.setColour (juce::ToggleButton::textColourId, white);
    bypassButton.setColour (juce::ToggleButton::tickColourId, cyan);
    addAndMakeVisible (bypassButton);
    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>
        (audioProcessor.parameters, "bypass", bypassButton);

    startTimerHz (30);
}

void IntelligenceAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (bg);
    const int w = getWidth(), h = getHeight();

    g.setColour (panel2);
    g.fillRoundedRectangle (12.0f, 12.0f, static_cast<float> (w) - 24.0f,
                            static_cast<float> (h) - 24.0f, 12.0f);

    label (g, "272", { 28, 20, 55, 40 }, 29, cyan, true);
    label (g, "MUSIC RECORDS", { 88, 22, 180, 18 }, 11, white, true);
    label (g, "INTELLIGENCE", { 88, 40, 210, 23 }, 17, white, true);
    label (g, "VOCAL-ONLY ADAPTIVE EQUALIZER", { 28, 55, 340, 16 }, 9, muted, true);

    auto graph = juce::Rectangle<float> (24, 166, static_cast<float> (w) - 190.0f,
                                         static_cast<float> (h) - 284.0f);
    g.setColour (panel);
    g.fillRoundedRectangle (graph, 8.0f);
    label (g, "VOCAL SPECTRUM  ·  PRE", { 40, 121, 260, 22 }, 11, white, true);
    label (g, "TOTAL EQ RESPONSE", { 40, 147, 260, 16 }, 9, cyan, true);

    auto plot = graph.withTrimmedTop (54).reduced (14, 20);
    constexpr float minDb = -12.0f;
    constexpr float maxDb = 12.0f;
    const auto sampleRate = audioProcessor.getAnalysisSampleRate();
    constexpr float minHz = 20.0f;
    const float maxHz = juce::jmin (20000.0f, static_cast<float> (sampleRate * 0.48));

    const auto xForFrequency = [&] (float hz)
    {
        const float norm = std::log (juce::jlimit (minHz, maxHz, hz) / minHz)
                         / std::log (maxHz / minHz);
        return plot.getX() + norm * plot.getWidth();
    };
    const auto yForDb = [&] (float db)
    {
        const float norm = (juce::jlimit (minDb, maxDb, db) - minDb) / (maxDb - minDb);
        return plot.getBottom() - norm * plot.getHeight();
    };
    const float zeroY = yForDb (0.0f);

    const std::array<float, 10> frequencyTicks { 20.0f, 50.0f, 100.0f, 200.0f, 500.0f,
                                                 1000.0f, 2000.0f, 5000.0f, 10000.0f, 20000.0f };
    for (const auto hz : frequencyTicks)
    {
        if (hz > maxHz)
            continue;

        const auto x = xForFrequency (hz);
        g.setColour (grid.withAlpha (0.75f));
        g.drawVerticalLine (juce::roundToInt (x), plot.getY(), plot.getBottom());

        const auto tickText = hz >= 1000.0f
            ? juce::String (hz / 1000.0f, hz >= 10000.0f ? 0 : 1) + "k"
            : juce::String (juce::roundToInt (hz));

        label (g, tickText,
               { juce::roundToInt (x) - 20, juce::roundToInt (plot.getBottom()) + 3, 40, 14 },
               8, muted, false, juce::Justification::centred);
    }

    for (int db = -12; db <= 12; db += 3)
    {
        const auto y = yForDb (static_cast<float> (db));
        g.setColour (db == 0 ? cyan.withAlpha (0.8f) : grid.withAlpha (0.65f));
        g.drawHorizontalLine (juce::roundToInt (y), plot.getX(), plot.getRight());

        if (db % 6 == 0)
            label (g, juce::String (db > 0 ? "+" : "") + juce::String (db) + " dB",
                   { juce::roundToInt (plot.getX()) - 46, juce::roundToInt (y) - 7, 42, 14 },
                   8, muted, false, juce::Justification::centredRight);
    }

    // Real-time pre-processing vocal spectrum from the FFT analyzer.
    juce::Path spectrum;
    constexpr int spectrumPoints = 220;
    for (int i = 0; i < spectrumPoints; ++i)
    {
        const float norm = static_cast<float> (i) / static_cast<float> (spectrumPoints - 1);
        const float hz = minHz * std::pow (maxHz / minHz, norm);
        const int bin = juce::jlimit (1, SpectralAnalyzer::spectrumSize - 1,
                                      juce::roundToInt (hz * static_cast<float> (SpectralAnalyzer::fftSize)
                                                        / static_cast<float> (sampleRate)));
        // Hann-window coherent-gain compensation for an approximate dBFS display.
        const float magnitude = audioProcessor.getVocalMagnitude (bin) * 4.0f;
        const float db = juce::jlimit (-90.0f, 0.0f,
            juce::Decibels::gainToDecibels (magnitude, -90.0f));
        const float spectrumY = plot.getBottom()
            - ((db + 90.0f) / 90.0f) * plot.getHeight();
        const juce::Point<float> point (xForFrequency (hz), spectrumY);
        if (i == 0) spectrum.startNewSubPath (point);
        else spectrum.lineTo (point);
    }
    g.setColour (juce::Colour (150, 157, 169).withAlpha (0.78f));
    g.strokePath (spectrum, juce::PathStrokeType (1.25f));

    // Draw the measured magnitude response of the complete active biquad cascade.
    juce::Path response;
    constexpr int responsePoints = 240;
    for (int i = 0; i < responsePoints; ++i)
    {
        const float norm = static_cast<float> (i) / static_cast<float> (responsePoints - 1);
        const float hz = minHz * std::pow (maxHz / minHz, norm);
        const float gainDb = audioProcessor.getTotalResponseDb (hz);
        const juce::Point<float> point (xForFrequency (hz), yForDb (gainDb));
        if (i == 0) response.startNewSubPath (point);
        else response.lineTo (point);
    }
    g.setColour (cyan.withAlpha (0.12f));
    juce::Path responseFill = response;
    responseFill.lineTo (plot.getRight(), zeroY);
    responseFill.lineTo (plot.getX(), zeroY);
    responseFill.closeSubPath();
    g.fillPath (responseFill);
    g.setColour (juce::Colour (255, 190, 70));
    g.strokePath (response, juce::PathStrokeType (2.0f));

    const int meterX = w - 150;
    g.setColour (panel);
    g.fillRoundedRectangle (static_cast<float> (meterX), 166.0f, 112.0f, graph.getHeight(), 8.0f);
    label (g, "OUTPUT", { meterX + 12, 128, 88, 18 }, 10, white, true, juce::Justification::centred);
    g.setColour (juce::Colour (10, 20, 28));
    g.fillRoundedRectangle (static_cast<float> (meterX) + 42.0f, 218.0f,
                            28.0f, graph.getHeight() - 92.0f, 5.0f);

    const float meterTop = 218.0f;
    const float meterBottom = graph.getBottom() - 18.0f;
    const float meterH = juce::jmax (10.0f, meterBottom - meterTop);
    for (int ch = 0; ch < 2; ++ch)
    {
        const float x = static_cast<float> (meterX) + 25.0f + static_cast<float> (ch) * 42.0f;
        const float peak = audioProcessor.getOutputPeak (ch);
        const float peakDb = juce::Decibels::gainToDecibels (peak, -60.0f);
        const float norm = juce::jlimit (0.0f, 1.0f, (peakDb + 60.0f) / 60.0f);
        g.setColour (green);
        g.fillRoundedRectangle (x, meterBottom - meterH * norm, 16.0f,
                                juce::jmax (2.0f, meterH * norm), 3.0f);
        const float holdDb = juce::Decibels::gainToDecibels (audioProcessor.getOutputPeakHold (ch), -60.0f);
        const float holdY = meterBottom - meterH * juce::jlimit (0.0f, 1.0f, (holdDb + 60.0f) / 60.0f);
        g.setColour (juce::Colour (255, 190, 70));
        g.drawHorizontalLine (juce::roundToInt (holdY), x - 2.0f, x + 18.0f);
        label (g, ch == 0 ? "L" : "R", { juce::roundToInt (x - 2), 196, 20, 14 }, 8, muted, true,
               juce::Justification::centred);
        label (g, juce::String (peakDb, 1), { juce::roundToInt (x - 10), juce::roundToInt (meterBottom + 2), 38, 14 },
               8, white, false, juce::Justification::centred);
    }
    label (g, "dBFS", { meterX + 12, juce::roundToInt (graph.getBottom() - 12.0f), 88, 14 },
           8, muted, false, juce::Justification::centred);

    const int bottomY = h - 96;
    g.setColour (panel);
    g.fillRoundedRectangle (24.0f, static_cast<float> (bottomY),
                            static_cast<float> (w) - 48.0f, 66.0f, 7.0f);
    label (g, "VOCAL L", { 40, bottomY + 8, 58, 16 }, 9, muted, true);
    label (g, "VOCAL R", { 40, bottomY + 35, 58, 16 }, 9, muted, true);

    for (int row = 0; row < 2; ++row)
    {
        g.setColour (juce::Colour (11, 24, 30));
        g.fillRoundedRectangle (105.0f, static_cast<float> (bottomY) + 10.0f
                                + static_cast<float> (row) * 27.0f,
                                static_cast<float> (w - 235), 7.0f, 3.0f);

        const float peak = audioProcessor.getInputPeak (row);
        const float peakDb = juce::Decibels::gainToDecibels (peak, -60.0f);
        const float meterNorm = juce::jlimit (0.0f, 1.0f, (peakDb + 60.0f) / 60.0f);

        g.setColour (green);
        g.fillRoundedRectangle (105.0f, static_cast<float> (bottomY) + 10.0f
                                + static_cast<float> (row) * 27.0f,
                                static_cast<float> (w - 235) * meterNorm, 7.0f, 3.0f);
    }

    label (g, "272 MUSIC RECORDS  ·  VOCAL INPUT / SELF-ANALYSIS",
           { 30, h - 23, w - 60, 14 }, 8, muted, true);
}

void IntelligenceAudioProcessorEditor::resized()
{
    // Keep the brand and mode switches in the header; controls occupy two
    // dedicated rows below it, so nothing overlaps the logo or graph.
    intelligenceButton.setBounds (getWidth() - 170, 20, 145, 26);
    bypassButton.setBounds (getWidth() - 170, 48, 145, 24);

    const int cell = (getWidth() - 48) / 3;
    auto place = [cell] (juce::Label& controlLabel, juce::Slider& slider, int index)
    {
        const int x = 24 + (index % 3) * cell;
        const int y = 78 + (index / 3) * 44;
        controlLabel.setBounds (x, y, 90, 16);
        slider.setBounds (x, y + 16, cell - 18, 24);
    };

    place (recoverLabel, recoverSlider, 0);
    place (tameLabel, tameSlider, 1);
    place (biasLabel, biasSlider, 2);
    place (brightenLabel, brightenSlider, 3);
    place (boostLabel, boostSlider, 4);
    place (outputLabel, outputSlider, 5);
}

void IntelligenceAudioProcessorEditor::timerCallback()
{
    repaint();
}
