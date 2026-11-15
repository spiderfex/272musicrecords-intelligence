#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"

class IntelligenceAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit IntelligenceAudioProcessorEditor (IntelligenceAudioProcessor&);
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    IntelligenceAudioProcessor& audioProcessor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (IntelligenceAudioProcessorEditor)
};
