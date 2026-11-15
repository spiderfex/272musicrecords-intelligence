#include "PluginEditor.h"

IntelligenceAudioProcessorEditor::IntelligenceAudioProcessorEditor (
    IntelligenceAudioProcessor& processor)
    : AudioProcessorEditor (&processor), audioProcessor (processor)
{
    setSize (640, 420);
    startTimerHz (20);
}

void IntelligenceAudioProcessorEditor::paint (juce::Graphics& graphics)
{
    graphics.fillAll (juce::Colour::fromRGB (18, 20, 25));

    graphics.setColour (juce::Colours::white);
    graphics.setFont (juce::Font (25.0f, juce::Font::bold));
    graphics.drawText ("272MusicRecords", 24, 20, getWidth() - 48, 34,
                       juce::Justification::centredLeft);

    graphics.setColour (juce::Colour::fromRGB (93, 190, 255));
    graphics.setFont (juce::Font (16.0f, juce::Font::bold));
    graphics.drawText ("INTELLIGENCE  |  V0.1 SPECTRAL ANALYSIS", 24, 57,
                       getWidth() - 48, 24, juce::Justification::centredLeft);

    const auto graph = juce::Rectangle<float> (24.0f, 112.0f,
                                               static_cast<float> (getWidth() - 48),
                                               238.0f);
    graphics.setColour (juce::Colour::fromRGB (34, 39, 48));
    graphics.fillRoundedRectangle (graph, 8.0f);

    for (int line = 1; line < 4; ++line)
    {
        const auto y = graph.getY() + graph.getHeight() * static_cast<float> (line) / 4.0f;
        graphics.setColour (juce::Colour::fromRGB (54, 60, 71));
        graphics.drawHorizontalLine (juce::roundToInt (y), graph.getX() + 8.0f,
                                     graph.getRight() - 8.0f);
    }

    juce::Path vocalPath;
    juce::Path beatPath;
    constexpr int displayBins = 256;

    for (int i = 0; i < displayBins; ++i)
    {
        const auto x = graph.getX() + 8.0f
                     + (graph.getWidth() - 16.0f) * static_cast<float> (i)
                       / static_cast<float> (displayBins - 1);

        const auto vocalDb = juce::Decibels::gainToDecibels (
            audioProcessor.getVocalMagnitude (i * 4) + 1.0e-8f, -80.0f);
        const auto beatDb = juce::Decibels::gainToDecibels (
            audioProcessor.getBeatMagnitude (i * 4) + 1.0e-8f, -80.0f);

        const auto vocalY = graph.getBottom() - 8.0f
                          - juce::jlimit (0.0f, 1.0f, (vocalDb + 80.0f) / 80.0f)
                            * (graph.getHeight() - 16.0f);
        const auto beatY = graph.getBottom() - 8.0f
                         - juce::jlimit (0.0f, 1.0f, (beatDb + 80.0f) / 80.0f)
                           * (graph.getHeight() - 16.0f);

        if (i == 0)
        {
            vocalPath.startNewSubPath (x, vocalY);
            beatPath.startNewSubPath (x, beatY);
        }
        else
        {
            vocalPath.lineTo (x, vocalY);
            beatPath.lineTo (x, beatY);
        }
    }

    graphics.setColour (juce::Colour::fromRGB (93, 190, 255));
    graphics.strokePath (vocalPath, juce::PathStrokeType (2.0f));
    graphics.setColour (juce::Colour::fromRGB (255, 174, 83));
    graphics.strokePath (beatPath, juce::PathStrokeType (2.0f));

    graphics.setFont (juce::Font (13.0f));
    graphics.setColour (juce::Colour::fromRGB (93, 190, 255));
    graphics.drawText ("VOCAL", 28, 82, 70, 20, juce::Justification::centredLeft);
    graphics.setColour (juce::Colour::fromRGB (255, 174, 83));
    graphics.drawText ("BEAT SIDECHAIN", 112, 82, 150, 20,
                       juce::Justification::centredLeft);

    graphics.setColour (juce::Colour::fromRGB (205, 210, 220));
    graphics.setFont (juce::Font (12.0f));
    graphics.drawText ("Independent spectra · 2048-point FFT · audio remains unchanged",
                       24, 278, getWidth() - 48, 20, juce::Justification::centredLeft);

    graphics.setColour (juce::Colour::fromRGB (145, 151, 163));
    graphics.drawText ("Analysis only — masking detection and dynamic correction are next.",
                       24, getHeight() - 40, getWidth() - 48, 20,
                       juce::Justification::centredLeft);
}

void IntelligenceAudioProcessorEditor::resized()
{
}

void IntelligenceAudioProcessorEditor::timerCallback()
{
    repaint();
}
