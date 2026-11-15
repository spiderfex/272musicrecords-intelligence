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
    setResizeLimits (680, 470, 1200, 760);
    setSize (820, 560);
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
    drawLabel (g, "ANALYSIS ACTIVE", { getWidth() - 143, 30, 120, 24 }, 10.5f, text, true);

    auto graph = juce::Rectangle<float> (22.0f, 91.0f, (float) getWidth() - 44.0f,
                                         (float) getHeight() - 190.0f);
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

    const auto cardY = getHeight() - 78;
    const auto cardW = (getWidth() - 56) / 3;
    const juce::StringArray names { "ENGINE", "SIDECHAIN", "PROCESSING" };
    const juce::StringArray values { "Spectral analysis", "External beat input", "Monitor only · V0.1" };
    for (int i = 0; i < 3; ++i)
    {
        auto r = juce::Rectangle<float> (22.0f + i * (cardW + 6), (float) cardY,
                                         (float) cardW, 58.0f);
        g.setColour (panel); g.fillRoundedRectangle (r, 10.0f);
        drawLabel (g, names[i], r.toNearestInt().reduced (12).withHeight (16), 9.5f, muted, true);
        drawLabel (g, values[i], r.toNearestInt().reduced (12).withTrimmedTop (18), 11.5f, text);
    }
    drawLabel (g, "272MUSICRECORDS  •  VOCAL INTELLIGENCE", { 24, getHeight() - 16, getWidth() - 48, 12 },
               8.5f, muted, true);
}

void IntelligenceAudioProcessorEditor::resized() {}
void IntelligenceAudioProcessorEditor::timerCallback() { repaint(); }
