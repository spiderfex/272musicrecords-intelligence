#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"

class IntelligenceAudioProcessorEditor final : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit IntelligenceAudioProcessorEditor (IntelligenceAudioProcessor&);
    void paint (juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    IntelligenceAudioProcessor& audioProcessor;
    juce::ToggleButton intelligenceButton { "INTELLIGENCE ON" };
    juce::Slider depthSlider, tameSlider, recoverSlider;
    juce::Label depthLabel, tameLabel, recoverLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> enabledAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> depthAttachment, tameAttachment, recoverAttachment;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (IntelligenceAudioProcessorEditor)
};
