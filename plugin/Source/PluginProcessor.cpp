#include "PluginProcessor.h"
#include "PluginEditor.h"

IntelligenceAudioProcessor::IntelligenceAudioProcessor()
    : AudioProcessor (BusesProperties()
          .withInput ("Vocal", juce::AudioChannelSet::stereo(), true)
          .withInput ("Beat Sidechain", juce::AudioChannelSet::stereo(), true)
          .withOutput ("Vocal Output", juce::AudioChannelSet::stereo(), true))
{
}

void IntelligenceAudioProcessor::prepareToPlay (double, int)
{
    // DSP state and preallocated analysis buffers will be initialized here.
}

void IntelligenceAudioProcessor::releaseResources()
{
    // Release non-real-time resources here.
}

bool IntelligenceAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto mainInput = layouts.getMainInputChannelSet();
    const auto mainOutput = layouts.getMainOutputChannelSet();

    if (mainInput != mainOutput)
        return false;

    if (mainInput != juce::AudioChannelSet::mono()
        && mainInput != juce::AudioChannelSet::stereo())
        return false;

    if (layouts.inputBuses.size() > 1)
    {
        const auto sidechain = layouts.getChannelSet (true, 1);
        if (sidechain != juce::AudioChannelSet::disabled()
            && sidechain != juce::AudioChannelSet::mono()
            && sidechain != juce::AudioChannelSet::stereo())
            return false;
    }

    return true;
}

void IntelligenceAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                                juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    // V0.1 shell: pass the vocal input through unchanged.
    // The beat sidechain is a separate auxiliary input and is never mixed into the output.
    // Future analysis must use preallocated memory and must not block the audio thread.
    auto vocalInput = getBusBuffer (buffer, true, 0);
    auto vocalOutput = getBusBuffer (buffer, false, 0);

    const auto channelsToCopy = juce::jmin (vocalInput.getNumChannels(),
                                            vocalOutput.getNumChannels());
    const auto samples = juce::jmin (vocalInput.getNumSamples(),
                                     vocalOutput.getNumSamples());

    for (int channel = 0; channel < channelsToCopy; ++channel)
        if (vocalInput.getReadPointer (channel) != vocalOutput.getWritePointer (channel))
            vocalOutput.copyFrom (channel, 0, vocalInput, channel, 0, samples);

    for (int channel = channelsToCopy; channel < vocalOutput.getNumChannels(); ++channel)
        vocalOutput.clear (channel, 0, samples);
}

juce::AudioProcessorEditor* IntelligenceAudioProcessor::createEditor()
{
    return new IntelligenceAudioProcessorEditor (*this);
}

void IntelligenceAudioProcessor::getStateInformation (juce::MemoryBlock& destinationData)
{
    destinationData.reset();
}

void IntelligenceAudioProcessor::setStateInformation (const void*, int)
{
    // No persistent parameters exist in the initial shell.
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new IntelligenceAudioProcessor();
}
