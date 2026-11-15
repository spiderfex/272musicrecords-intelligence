#include "PluginEditor.h"

IntelligenceAudioProcessorEditor::IntelligenceAudioProcessorEditor (
    IntelligenceAudioProcessor& processor)
    : AudioProcessorEditor (&processor), audioProcessor (processor)
{
    setSize (560, 320);
}

void IntelligenceAudioProcessorEditor::paint (juce::Graphics& graphics)
{
    graphics.fillAll (juce::Colour::fromRGB (18, 20, 25));

    graphics.setColour (juce::Colours::white);
    graphics.setFont (juce::Font (25.0f, juce::Font::bold));
    graphics.drawText ("272MusicRecords", 24, 24, getWidth() - 48, 34,
                       juce::Justification::centredLeft);

    graphics.setColour (juce::Colour::fromRGB (93, 190, 255));
    graphics.setFont (juce::Font (17.0f, juce::Font::bold));
    graphics.drawText ("INTELLIGENCE  |  V0.1 FOUNDATION", 24, 67,
                       getWidth() - 48, 26, juce::Justification::centredLeft);

    graphics.setColour (juce::Colour::fromRGB (205, 210, 220));
    graphics.setFont (juce::Font (14.0f));
    graphics.drawText ("Vocal input: pass-through shell", 24, 126,
                       getWidth() - 48, 24, juce::Justification::centredLeft);
    graphics.drawText ("Beat sidechain: auxiliary input bus declared", 24, 154,
                       getWidth() - 48, 24, juce::Justification::centredLeft);

    graphics.setColour (juce::Colour::fromRGB (145, 151, 163));
    graphics.setFont (juce::Font (12.0f));
    graphics.drawText ("Analysis and dynamic correction are not implemented yet.",
                       24, getHeight() - 48, getWidth() - 48, 20,
                       juce::Justification::centredLeft);
}

void IntelligenceAudioProcessorEditor::resized()
{
    // Controls will be added after the audio and sidechain paths are validated.
}
