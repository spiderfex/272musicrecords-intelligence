#include "PluginEditor.h"

namespace
{
    const juce::Colour background { 12, 15, 22 };
    const juce::Colour panel { 23, 29, 40 };
    const juce::Colour muted { 137, 151, 170 };
    const juce::Colour text { 235, 241, 250 };
    const juce::Colour cyan { 75, 207, 231 };
    const juce::Colour orange { 255, 173, 91 };

    void drawLabel (juce::Graphics& g, const juce::String& value, juce::Rectangle<int> area,
                    float size, juce::Colour colour, bool bold = false)
    {
        g.setColour (colour);
        g.setFont (juce::Font (juce::FontOptions (size).withStyle (bold ? "Bold" : "Regular")));
        g.drawText (value, area, juce::Justification::centredLeft, true);
    }
}

IntelligenceAudioProcessorEditor::IntelligenceAudioProcessorEditor (IntelligenceAudioProcessor& processor)
    : AudioProcessorEditor (&processor), audioProcessor (processor)
{
    setResizable (true, true);
    setResizeLimits (680, 570, 1200, 820);
    setSize (820, 650);

    intelligenceButton.setColour (juce::ToggleButton::textColourId, text);
    intelligenceButton.setColour (juce::ToggleButton::tickColourId, cyan);
    intelligenceButton.setColour (juce::ToggleButton::tickDisabledColourId, muted);
    addAndMakeVisible (intelligenceButton);
    enabledAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>
        (audioProcessor.parameters, "enabled", intelligenceButton);

    auto setupSlider = [this] (juce::Slider& slider, juce::Label& label, const juce::String& name)
    {
        slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 64, 20);
        slider.setTextValueSuffix (" %");
        slider.setColour (juce::Slider::rotarySliderFillColourId, cyan);
        slider.setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colour (48, 60, 76));
        slider.setColour (juce::Slider::thumbColourId, text);
        slider.setColour (juce::Slider::textBoxTextColourId, text);
        slider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        addAndMakeVisible (slider);
        label.setText (name, juce::dontSendNotification);
        label.setColour (juce::Label::textColourId, muted);
        label.setJustificationType (juce::Justification::centred);
        addAndMakeVisible (label);
    };
    setupSlider (depthSlider, depthLabel, "DEPTH");
    setupSlider (tameSlider, tameLabel, "TAME");
    setupSlider (recoverSlider, recoverLabel, "RECOVER");

    depthAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.parameters, "depth", depthSlider);
    tameAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.parameters, "tame", tameSlider);
    recoverAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.parameters, "recover", recoverSlider);
    startTimerHz (20);
}

void IntelligenceAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (background);
    auto area = getLocalBounds().reduced (22);
    drawLabel (g, "272", area.removeFromTop (32).withWidth (48), 24.0f, cyan, true);
    drawLabel (g, "MUSIC RECORDS", { 78, 23, 210, 25 }, 13.0f, text, true);
    drawLabel (g, "INTELLIGENCE", { 78, 44, 250, 27 }, 18.0f, text, true);

    g.setColour (juce::Colour (36, 49, 64));
    g.fillRoundedRectangle (getWidth() - 174.0f, 27.0f, 150.0f, 30.0f, 15.0f);
    g.setColour (cyan);
    g.fillEllipse (getWidth() - 158.0f, 38.0f, 8.0f, 8.0f);
    drawLabel (g, "VOCAL INTELLIGENCE", { getWidth() - 143, 30, 135, 24 }, 9.5f, text, true);

    auto graph = juce::Rectangle<float> (22.0f, 91.0f, (float) getWidth() - 44.0f, 245.0f);
    g.setColour (panel);
    g.fillRoundedRectangle (graph, 14.0f);
    drawLabel (g, "SPECTRUM MONITOR", { 42, 105, 240, 22 }, 12.0f, text, true);
    drawLabel (g, "VOCAL", { 42, 132, 65, 18 }, 10.0f, cyan, true);
    drawLabel (g, "BEAT SIDECHAIN", { 116, 132, 130, 18 }, 10.0f, orange, true);
    auto plot = graph.reduced (18.0f, 48.0f);
    for (int line = 1; line < 5; ++line)
    {
        const auto y = plot.getY() + plot.getHeight() * (float) line / 5.0f;
        g.setColour (juce::Colour (43, 54, 69));
        g.drawHorizontalLine (juce::roundToInt (y), plot.getX(), plot.getRight());
    }
    juce::Path vocalPath, beatPath;
    constexpr int bins = 256;
    for (int i = 0; i < bins; ++i)
    {
        const auto x = plot.getX() + plot.getWidth() * (float) i / (float) (bins - 1);
        const auto v = juce::Decibels::gainToDecibels (audioProcessor.getVocalMagnitude (i * 4) + 1.0e-8f, -80.0f);
        const auto b = juce::Decibels::gainToDecibels (audioProcessor.getBeatMagnitude (i * 4) + 1.0e-8f, -80.0f);
        const auto vy = plot.getBottom() - juce::jlimit (0.0f, 1.0f, (v + 80.0f) / 80.0f) * plot.getHeight();
        const auto by = plot.getBottom() - juce::jlimit (0.0f, 1.0f, (b + 80.0f) / 80.0f) * plot.getHeight();
        if (i == 0) { vocalPath.startNewSubPath (x, vy); beatPath.startNewSubPath (x, by); }
        else { vocalPath.lineTo (x, vy); beatPath.lineTo (x, by); }
    }
    g.setColour (cyan); g.strokePath (vocalPath, juce::PathStrokeType (2.2f));
    g.setColour (orange); g.strokePath (beatPath, juce::PathStrokeType (2.0f));

    g.setColour (panel);
    g.fillRoundedRectangle (juce::Rectangle<float> (22.0f, 350.0f, (float) getWidth() - 44.0f, 220.0f), 14.0f);
    drawLabel (g, "INTELLIGENT PROCESSING", { 42, 362, 300, 20 }, 12.0f, text, true);
    drawLabel (g, "Automatic vocal correction · Beat-guided", { 42, 385, 360, 18 }, 10.5f, muted);
    drawLabel (g, "SIDECHAIN IS A REFERENCE — BEAT IS NOT MIXED TO OUTPUT", { 42, getHeight() - 28, getWidth() - 84, 14 }, 8.5f, muted, true);
}

void IntelligenceAudioProcessorEditor::resized()
{
    intelligenceButton.setBounds (getWidth() - 230, 53, 205, 28);
    const int top = 410;
    const int width = (getWidth() - 100) / 3;
    auto place = [top, width] (juce::Slider& slider, juce::Label& label, int index)
    {
        const int x = 38 + index * (width + 12);
        label.setBounds (x, top, width, 22);
        slider.setBounds (x + (width - 112) / 2, top + 20, 112, 145);
    };
    place (depthSlider, depthLabel, 0);
    place (tameSlider, tameLabel, 1);
    place (recoverSlider, recoverLabel, 2);
}

void IntelligenceAudioProcessorEditor::timerCallback() { repaint(); }
