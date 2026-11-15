#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "SpectralAnalyzer.h"
#include "IntelligentMaskingProcessor.h"
#include <atomic>

class IntelligenceAudioProcessor final : public juce::AudioProcessor
{
public:
    IntelligenceAudioProcessor();
    ~IntelligenceAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destinationData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    float getVocalMagnitude (int bin) const noexcept { return vocalAnalyzer.getMagnitude (bin); }
    float getCorrectionDb (int band) const noexcept { return maskingProcessor.getResponseDb (band); }
    float getInputPeak (int channel) const noexcept { return inputPeak[static_cast<size_t> (juce::jlimit (0, 1, channel))].load(); }
    float getOutputPeak (int channel) const noexcept { return outputPeak[static_cast<size_t> (juce::jlimit (0, 1, channel))].load(); }
    double getAnalysisSampleRate() const noexcept { return currentSampleRate.load (std::memory_order_relaxed); }

    juce::AudioProcessorValueTreeState parameters;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    std::array<float, IntelligentMaskingProcessor::bandCount> getVocalBandLevels() const noexcept;

    SpectralAnalyzer vocalAnalyzer;
    IntelligentMaskingProcessor maskingProcessor;
    std::atomic<double> currentSampleRate { 44100.0 };
    std::array<std::atomic<float>, 2> inputPeak {}, outputPeak {};
    std::array<float, 2> inputPeakSmooth {}, outputPeakSmooth {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (IntelligenceAudioProcessor)
};
