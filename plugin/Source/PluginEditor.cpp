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

    auto plot = graph.withTrimmedTop (54).reduced (14, 10);
    for (int i = 0; i <= 8; ++i)
    {
        const float x = plot.getX() + plot.getWidth() * (float) i / 8.0f;
        g.setColour (grid.withAlpha (0.75f));
        g.drawVerticalLine (juce::roundToInt (x), plot.getY(), plot.getBottom());
    }
    for (int i = 0; i <= 8; ++i)
    {
        const float y = plot.getY() + plot.getHeight() * (float) i / 8.0f;
        g.setColour (grid.withAlpha (0.65f));
        g.drawHorizontalLine (juce::roundToInt (y), plot.getX(), plot.getRight());
    }

    juce::Path vocal, beat;
    constexpr int points = 256;
    for (int i = 0; i < points; ++i)
    {
        const float x = plot.getX() + plot.getWidth() * (float) i / (float) (points - 1);
        const float vm = audioProcessor.getVocalMagnitude (i * 4);
        const float bm = audioProcessor.getBeatMagnitude (i * 4);
        const float vy = plot.getBottom() - juce::jlimit (0.0f, 1.0f, vm * 3.0f) * plot.getHeight();
        const float by = plot.getBottom() - juce::jlimit (0.0f, 1.0f, bm * 3.0f) * plot.getHeight();
        if (i == 0) { vocal.startNewSubPath (x, vy); beat.startNewSubPath (x, by); }
        else { vocal.lineTo (x, vy); beat.lineTo (x, by); }
    }
    g.setColour (cyan); g.strokePath (vocal, juce::PathStrokeType (2.0f));
    g.setColour (orange); g.strokePath (beat, juce::PathStrokeType (1.8f));

    const int sideX = w - 150;
    g.setColour (panel);
    g.fillRoundedRectangle ((float) sideX, 112.0f, 112.0f, graph.getHeight(), 8.0f);
    label (g, "OUTPUT", { sideX + 12, 128, 88, 18 }, 10, white, true, juce::Justification::centred);
    g.setColour (juce::Colour (10, 20, 28));
    g.fillRoundedRectangle ((float) sideX + 42, 164.0f, 28.0f, graph.getHeight() - 92.0f, 5.0f);
    g.setColour (green);
    g.fillRoundedRectangle ((float) sideX + 46, 250.0f, 20.0f, juce::jmax (12.0f, graph.getHeight() - 180.0f), 3.0f);
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
        g.setColour (green);
        g.fillRoundedRectangle (105.0f, (float) bottomY + 10.0f + (float) row * 27.0f, (float) (w - 235) * 0.62f, 7.0f, 3.0f);
    }
    label (g, "272 MUSIC RECORDS  ·  SIDECHAIN IS ANALYSIS ONLY", { 30, h - 23, w - 60, 14 }, 8, muted, true);
}

void IntelligenceAudioProcessorEditor::resized()
{
    const int topY = 27;
    intelligenceButton.setBounds (getWidth() - 190, topY, 160, 30);
    const int startX = 330, gap = 18, cellW = (getWidth() - startX - 220) / 3;
    auto place = [cellW] (juce::Label& l, juce::Slider& s, int index)
    {
        const int x = 330 + index * (cellW + 18);
        l.setBounds (x, 24, 78, 22);
        s.setBounds (x + 78, 23, cellW - 78, 28);
    };
    place (recoverLabel, recoverSlider, 0);
    place (tameLabel, tameSlider, 1);
    place (depthLabel, depthSlider, 2);
}

void IntelligenceAudioProcessorEditor::timerCallback() { repaint(); }
