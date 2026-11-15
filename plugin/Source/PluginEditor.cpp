#include "PluginEditor.h"

namespace
{
    const juce::Colour bg { 17, 22, 32 }, panel { 29, 40, 57 }, panel2 { 22, 30, 43 };
    const juce::Colour grid { 48, 75, 101 }, cyan { 70, 218, 238 }, orange { 255, 169, 83 };
    const juce::Colour white { 238, 244, 250 }, muted { 150, 169, 190 }, green { 32, 224, 139 };

    void label (juce::Graphics& g, const juce::String& s, juce::Rectangle<int> r, float size,
                juce::Colour colour, bool bold = false, juce::Justification align = juce::Justification::centredLeft)
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

    auto setup = [this] (juce::Slider& s, juce::Label& l, const juce::String& name)
    {
        s.setSliderStyle (juce::Slider::LinearHorizontal);
        s.setTextBoxStyle (juce::Slider::TextBoxRight, false, 48, 24);
        s.setTextValueSuffix (" %");
        s.setColour (juce::Slider::trackColourId, cyan);
        s.setColour (juce::Slider::thumbColourId, white);
        s.setColour (juce::Slider::textBoxTextColourId, white);
        s.setColour (juce::Slider::textBoxBackgroundColourId, panel);
        s.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        addAndMakeVisible (s);
        l.setText (name, juce::dontSendNotification);
        l.setColour (juce::Label::textColourId, muted);
        l.setJustificationType (juce::Justification::centredLeft);
        addAndMakeVisible (l);
    };
    setup (recoverSlider, recoverLabel, "RECOVER");
    setup (tameSlider, tameLabel, "TAME");
    setup (depthSlider, depthLabel, "DEPTH");
    recoverAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.parameters, "recover", recoverSlider);
    tameAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.parameters, "tame", tameSlider);
    depthAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.parameters, "depth", depthSlider);
    setup (biasSlider, biasLabel, "BIAS");
    biasSlider.setRange (-1.0, 1.0, 0.01);
    biasSlider.setTextValueSuffix ("");
    setup (brightenSlider, brightenLabel, "BRIGHTEN");
    setup (boostSlider, boostLabel, "BOOST");
    boostSlider.setRange (-6.0, 6.0, 0.01);
    boostSlider.setTextValueSuffix (" dB");
    setup (outputSlider, outputLabel, "OUTPUT");
    outputSlider.setRange (-24.0, 12.0, 0.01);
    outputSlider.setTextValueSuffix (" dB");
    biasAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.parameters, "bias", biasSlider);
    brightenAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.parameters, "brighten", brightenSlider);
    boostAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.parameters, "boost", boostSlider);
    outputAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.parameters, "output", outputSlider);
    bypassButton.setColour (juce::ToggleButton::textColourId, white);
    bypassButton.setColour (juce::ToggleButton::tickColourId, orange);
    addAndMakeVisible (bypassButton);
    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (audioProcessor.parameters, "bypass", bypassButton);
    startTimerHz (24);
}

void IntelligenceAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (bg);
    const int w = getWidth(), h = getHeight();
    g.setColour (panel2);
    g.fillRoundedRectangle (12.0f, 12.0f, (float) w - 24.0f, (float) h - 24.0f, 12.0f);

    label (g, "272", { 28, 20, 55, 40 }, 29, cyan, true);
    label (g, "MUSIC RECORDS", { 88, 22, 180, 18 }, 11, white, true);
    label (g, "INTELLIGENCE", { 88, 40, 210, 23 }, 17, white, true);
    label (g, "VOCAL / BEAT INTELLIGENT PROCESSOR", { 28, 70, 340, 18 }, 9, muted, true);

    auto graph = juce::Rectangle<float> (24, 112, (float) w - 190.0f, (float) h - 230.0f);
    g.setColour (panel);
    g.fillRoundedRectangle (graph, 8.0f);
    label (g, "SPECTRAL INTELLIGENCE", { 40, 121, 260, 22 }, 11, white, true);
    label (g, "VOCAL", { 40, 147, 60, 16 }, 9, cyan, true);
    label (g, "BEAT SIDECHAIN", { 104, 147, 120, 16 }, 9, orange, true);
    label (g, "DYNAMIC EQ", { 235, 147, 100, 16 }, 9, green, true);
    label (g, "dB", { 28, 174, 24, 14 }, 8, muted, false);

    auto plot = graph.withTrimmedTop (54).reduced (14, 20);
    const auto sampleRate = audioProcessor.getAnalysisSampleRate();
    const float minHz = 20.0f;
    const float maxHz = juce::jmin (20000.0f, static_cast<float> (sampleRate * 0.48));
    // Use a robust percentile instead of the single loudest FFT bin.
    // This prevents DC-adjacent rumble or one narrow peak from flattening the whole plot.
    std::array<float, SpectralAnalyzer::spectrumSize> visibleLevels {};
    int visibleCount = 0;
    const auto firstVisibleBin = juce::jmax (2, juce::roundToInt (40.0f * SpectralAnalyzer::fftSize
                                                                   / static_cast<float> (sampleRate)));
    const auto lastVisibleBin = juce::jmin (SpectralAnalyzer::spectrumSize - 1,
        juce::roundToInt (18000.0f * SpectralAnalyzer::fftSize / static_cast<float> (sampleRate)));
    for (int bin = firstVisibleBin; bin <= lastVisibleBin; ++bin)
    {
        const auto vocalDb = juce::Decibels::gainToDecibels (audioProcessor.getVocalMagnitude (bin) * 4.0f, -120.0f);
        const auto beatDb = juce::Decibels::gainToDecibels (audioProcessor.getBeatMagnitude (bin) * 4.0f, -120.0f);
        visibleLevels[static_cast<size_t> (visibleCount++)] = juce::jmax (vocalDb, beatDb);
    }
    std::sort (visibleLevels.begin(), visibleLevels.begin() + visibleCount);
    const auto percentileIndex = juce::jmax (0, static_cast<int> (visibleCount * 0.92f) - 1);
    const float referenceDb = visibleCount > 0 ? visibleLevels[static_cast<size_t> (percentileIndex)] : -72.0f;
    const float minDb = juce::jmax (-108.0f, referenceDb - 48.0f);
    const float maxDb = juce::jmax (-6.0f, referenceDb + 12.0f);
    const auto xForFrequency = [&] (float hz)
    {
        const auto norm = std::log (juce::jlimit (minHz, maxHz, hz) / minHz)
                        / std::log (maxHz / minHz);
        return plot.getX() + norm * plot.getWidth();
    };
    const auto yForDb = [&] (float db)
    {
        const auto norm = (juce::jlimit (minDb, maxDb, db) - minDb) / (maxDb - minDb);
        return plot.getBottom() - norm * plot.getHeight();
    };

    const std::array<float, 10> frequencyTicks { 20.0f, 50.0f, 100.0f, 200.0f, 500.0f,
                                                 1000.0f, 2000.0f, 5000.0f, 10000.0f, 20000.0f };
    for (const auto hz : frequencyTicks)
    {
        if (hz > maxHz) continue;
        const auto x = xForFrequency (hz);
        g.setColour (grid.withAlpha (0.75f));
        g.drawVerticalLine (juce::roundToInt (x), plot.getY(), plot.getBottom());
        const auto tickText = hz >= 1000.0f
            ? juce::String (hz / 1000.0f, hz >= 10000.0f ? 0 : 1) + "k"
            : juce::String (juce::roundToInt (hz));
        label (g, tickText, { juce::roundToInt (x) - 20, juce::roundToInt (plot.getBottom()) + 3, 40, 14 },
               8, muted, false, juce::Justification::centred);
    }
    for (int db = -90; db <= 0; db += 18)
    {
        const auto y = yForDb (static_cast<float> (db));
        g.setColour (grid.withAlpha (0.65f));
        g.drawHorizontalLine (juce::roundToInt (y), plot.getX(), plot.getRight());
        label (g, juce::String (db), { juce::roundToInt (plot.getX()) - 34, juce::roundToInt (y) - 7, 28, 14 },
               8, muted, false, juce::Justification::centredRight);
    }

    juce::Path vocal, beat;
    constexpr int points = 512;
    for (int i = 0; i < points; ++i)
    {
        const auto norm = static_cast<float> (i) / static_cast<float> (points - 1);
        const auto hz = minHz * std::pow (maxHz / minHz, norm);
        const auto bin = juce::jlimit (1, SpectralAnalyzer::spectrumSize - 1,
            juce::roundToInt (hz * static_cast<float> (SpectralAnalyzer::fftSize) / static_cast<float> (sampleRate)));
        const auto x = xForFrequency (hz);
        const auto vocalDb = juce::Decibels::gainToDecibels (audioProcessor.getVocalMagnitude (bin) * 4.0f, minDb);
        const auto beatDb = juce::Decibels::gainToDecibels (audioProcessor.getBeatMagnitude (bin) * 4.0f, minDb);
        const auto vy = yForDb (vocalDb);
        const auto by = yForDb (beatDb);
        if (i == 0) { vocal.startNewSubPath (x, vy); beat.startNewSubPath (x, by); }
        else { vocal.lineTo (x, vy); beat.lineTo (x, by); }
    }
    g.setColour (cyan.withAlpha (0.95f));
    g.strokePath (vocal, juce::PathStrokeType (2.0f));
    g.setColour (orange.withAlpha (0.90f));
    g.strokePath (beat, juce::PathStrokeType (1.8f));

    // Actual six-band gain-change telemetry, shown as a separate lower overlay.
    juce::Path correction;
    for (int band = 0; band < IntelligentMaskingProcessor::bandCount; ++band)
    {
        const auto hz = IntelligentMaskingProcessor::centres[static_cast<size_t> (band)];
        const auto x = xForFrequency (hz);
        const auto gainDb = audioProcessor.getReductionDb (band);
        const auto y = plot.getBottom() - (juce::jlimit (-4.5f, 1.2f, gainDb) + 4.5f)
                                           / 5.7f * (plot.getHeight() * 0.28f);
        if (band == 0) correction.startNewSubPath (x, y);
        else correction.lineTo (x, y);
        g.setColour (green);
        g.fillEllipse (x - 3.0f, y - 3.0f, 6.0f, 6.0f);
    }
    g.setColour (green.withAlpha (0.9f));
    g.strokePath (correction, juce::PathStrokeType (1.6f));

    const int sideX = w - 150;
    g.setColour (panel);
    g.fillRoundedRectangle ((float) sideX, 112.0f, 112.0f, graph.getHeight(), 8.0f);
    label (g, "OUTPUT", { sideX + 12, 128, 88, 18 }, 10, white, true, juce::Justification::centred);
    g.setColour (juce::Colour (10, 20, 28));
    g.fillRoundedRectangle ((float) sideX + 42, 164.0f, 28.0f, graph.getHeight() - 92.0f, 5.0f);
    g.setColour (green);
    const auto outputDb = juce::Decibels::gainToDecibels (audioProcessor.getOutputPeak (0), -60.0f);
    const auto outputNorm = juce::jlimit (0.0f, 1.0f, (outputDb + 60.0f) / 60.0f);
    const auto meterHeight = (graph.getHeight() - 92.0f) * outputNorm;
    g.fillRoundedRectangle ((float) sideX + 46, graph.getBottom() - 18.0f - meterHeight,
                            20.0f, juce::jmax (2.0f, meterHeight), 3.0f);
    label (g, "dB", { sideX + 12, juce::roundToInt (graph.getBottom() - 30.0f), 88, 16 }, 9, muted, false, juce::Justification::centred);

    const int bottomY = h - 96;
    g.setColour (panel);
    g.fillRoundedRectangle (24.0f, (float) bottomY, (float) w - 48.0f, 66.0f, 7.0f);
    label (g, "INPUT  L", { 40, bottomY + 8, 58, 16 }, 9, muted, true);
    label (g, "INPUT  R", { 40, bottomY + 35, 58, 16 }, 9, muted, true);
    for (int row = 0; row < 2; ++row)
    {
        g.setColour (juce::Colour (11, 24, 30));
        g.fillRoundedRectangle (105.0f, (float) bottomY + 10.0f + (float) row * 27.0f, (float) w - 235.0f, 7.0f, 3.0f);
        const auto peak = row == 0 ? audioProcessor.getInputPeak (0) : audioProcessor.getInputPeak (1);
        const auto peakDb = juce::Decibels::gainToDecibels (peak, -60.0f);
        const auto meterNorm = juce::jlimit (0.0f, 1.0f, (peakDb + 60.0f) / 60.0f);
        g.setColour (green);
        g.fillRoundedRectangle (105.0f, (float) bottomY + 10.0f + (float) row * 27.0f,
                                (float) (w - 235) * meterNorm, 7.0f, 3.0f);
    }
    label (g, "272 MUSIC RECORDS  ·  SIDECHAIN IS ANALYSIS ONLY", { 30, h - 23, w - 60, 14 }, 8, muted, true);
}

void IntelligenceAudioProcessorEditor::resized()
{
    const int topY = 26;
    intelligenceButton.setBounds (getWidth() - 170, topY, 145, 28);
    bypassButton.setBounds (getWidth() - 170, topY + 30, 145, 24);
    const int left = 300;
    const int available = getWidth() - left - 190;
    const int cell = available / 4;
    auto place = [cell] (juce::Label& l, juce::Slider& s, int index)
    {
        const int x = 24 + index * cell;
        l.setBounds (x, 24, 78, 20);
        s.setBounds (x, 43, cell - 10, 25);
    };
    place (recoverLabel, recoverSlider, 0);
    place (tameLabel, tameSlider, 1);
    place (depthLabel, depthSlider, 2);
    place (biasLabel, biasSlider, 3);
    place (brightenLabel, brightenSlider, 4);
    place (boostLabel, boostSlider, 5);
    place (outputLabel, outputSlider, 6);
}

void IntelligenceAudioProcessorEditor::timerCallback() { repaint(); }
