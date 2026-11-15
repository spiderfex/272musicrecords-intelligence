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
    label (g, "SPECTRAL INTELLIGENCE", { 40, 121, 260, 22 }, 11, white, true);
    label (g, "DYNAMIC EQ RESPONSE", { 40, 147, 260, 16 }, 9, cyan, true);

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

    // Plot the applied EQ gains from the same values used to update the filters.
    juce::Path response;
    juce::Path fill;
    const float zeroY = yForDb (0.0f);
    const auto& centres = IntelligentMaskingProcessor::centres;
    std::array<juce::Point<float>, IntelligentMaskingProcessor::bandCount> points {};

    for (int band = 0; band < IntelligentMaskingProcessor::bandCount; ++band)
    {
        const float hz = centres[static_cast<size_t> (band)];
        const float gainDb = audioProcessor.getCorrectionDb (band);
        points[static_cast<size_t> (band)] = { xForFrequency (hz), yForDb (gainDb) };
    }

    response.startNewSubPath (plot.getX(), zeroY);
    response.lineTo (points.front().x, points.front().y);
    for (int band = 1; band < IntelligentMaskingProcessor::bandCount; ++band)
    {
        const auto previous = points[static_cast<size_t> (band - 1)];
        const auto current = points[static_cast<size_t> (band)];
        const float midX = (previous.x + current.x) * 0.5f;
        response.cubicTo (midX, previous.y, midX, current.y, current.x, current.y);
    }
    response.lineTo (plot.getRight(), zeroY);
    fill = response;
    fill.closeSubPath();

    g.setColour (cyan.withAlpha (0.10f));
    g.fillPath (fill);

    juce::Path curve;
    curve.startNewSubPath (points.front());
    for (int band = 1; band < IntelligentMaskingProcessor::bandCount; ++band)
    {
        const auto previous = points[static_cast<size_t> (band - 1)];
        const auto current = points[static_cast<size_t> (band)];
        const float midX = (previous.x + current.x) * 0.5f;
        curve.cubicTo (midX, previous.y, midX, current.y, current.x, current.y);
    }

    g.setColour (white);
    g.strokePath (curve, juce::PathStrokeType (2.2f));
    g.setColour (cyan);
    for (const auto& point : points)
        g.fillEllipse (point.x - 2.2f, point.y - 2.2f, 4.4f, 4.4f);

    const int meterX = w - 150;
    g.setColour (panel);
    g.fillRoundedRectangle (static_cast<float> (meterX), 166.0f, 112.0f, graph.getHeight(), 8.0f);
    label (g, "OUTPUT", { meterX + 12, 128, 88, 18 }, 10, white, true, juce::Justification::centred);
    g.setColour (juce::Colour (10, 20, 28));
    g.fillRoundedRectangle (static_cast<float> (meterX) + 42.0f, 218.0f,
                            28.0f, graph.getHeight() - 92.0f, 5.0f);

    g.setColour (green);
    const float outputDb = juce::Decibels::gainToDecibels (audioProcessor.getOutputPeak (0), -60.0f);
    const float outputNorm = juce::jlimit (0.0f, 1.0f, (outputDb + 60.0f) / 60.0f);
    const float meterHeight = (graph.getHeight() - 92.0f) * outputNorm;
    g.fillRoundedRectangle (static_cast<float> (meterX) + 46.0f,
                            graph.getBottom() - 18.0f - meterHeight,
                            20.0f, juce::jmax (2.0f, meterHeight), 3.0f);
    label (g, "dB", { meterX + 12, juce::roundToInt (graph.getBottom() - 30.0f), 88, 16 },
           9, muted, false, juce::Justification::centred);

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
