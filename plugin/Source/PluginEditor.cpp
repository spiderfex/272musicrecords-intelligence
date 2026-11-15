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
    label (g, "DYNAMIC EQ RESPONSE", { 40, 147, 260, 16 }, 9, cyan, true);
    label (g, "+12 dB", { 25, 174, 42, 14 }, 8, muted);
    label (g, "0 dB", { 25, 0, 42, 14 }, 8, muted);
    label (g, "-12 dB", { 25, 0, 42, 14 }, 8, muted);

    auto plot = graph.withTrimmedTop (54).reduced (14, 20);
    constexpr float minDb = -12.0f;
    constexpr float maxDb = 12.0f;
    const auto sampleRate = audioProcessor.getAnalysisSampleRate();
    const float minHz = 20.0f;
    const float maxHz = juce::jmin (20000.0f, static_cast<float> (sampleRate * 0.48));
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

    // The display is a fixed-scale EQ response, not a moving input spectrum.
    // Each point is fed by the gain actually applied by the DSP for that band.
    juce::Path response;
    juce::Path fill;
    const auto zeroY = yForDb (0.0f);
    const auto& centres = IntelligentMaskingProcessor::centres;
    std::array<juce::Point<float>, IntelligentMaskingProcessor::bandCount> points {};
    for (int band = 0; band < IntelligentMaskingProcessor::bandCount; ++band)
    {
        const auto hz = centres[static_cast<size_t> (band)];
        const auto gainDb = audioProcessor.getReductionDb (band);
        points[static_cast<size_t> (band)] = { xForFrequency (hz), yForDb (gainDb) };
    }
    response.startNewSubPath (plot.getX(), zeroY);
    response.lineTo (points.front().x, points.front().y);
    for (int band = 1; band < IntelligentMaskingProcessor::bandCount; ++band)
    {
        const auto previous = points[static_cast<size_t> (band - 1)];
        const auto current = points[static_cast<size_t> (band)];
        const auto midX = (previous.x + current.x) * 0.5f;
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
        const auto midX = (previous.x + current.x) * 0.5f;
        curve.cubicTo (midX, previous.y, midX, current.y, current.x, current.y);
    }
    g.setColour (white);
    g.strokePath (curve, juce::PathStrokeType (2.2f));
    g.setColour (cyan);
    for (const auto& point : points)
        g.fillEllipse (point.x - 2.2f, point.y - 2.2f, 4.4f, 4.4f);

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
    const int cell = (getWidth() - 220) / 4;
    auto place = [cell] (juce::Label& l, juce::Slider& s, int index)
    {
        const int x = 24 + (index % 4) * cell;
        const int y = 20 + (index / 4) * 42;
        l.setBounds (x, y, 78, 18);
        s.setBounds (x, y + 18, cell - 10, 22);
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
