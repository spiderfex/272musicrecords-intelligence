#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    constexpr std::array<float, IntelligentMaskingProcessor::bandCount> bandCentres
        { 180.0f, 350.0f, 700.0f, 1400.0f, 2800.0f, 5600.0f };
}

juce::AudioProcessorValueTreeState::ParameterLayout IntelligenceAudioProcessor::createParameterLayout()
{
    using Float = juce::AudioParameterFloat;
    using Bool = juce::AudioParameterBool;
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add (std::make_unique<Bool> ("enabled", "Intelligence", false));
    layout.add (std::make_unique<Float> ("depth", "Depth", juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f));
    layout.add (std::make_unique<Float> ("tame", "Tame", juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.7f));
    layout.add (std::make_unique<Float> ("recover", "Recover", juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.25f));
    return layout;
}

IntelligenceAudioProcessor::IntelligenceAudioProcessor()
    : AudioProcessor (BusesProperties()
          .withInput ("Vocal", juce::AudioChannelSet::stereo(), true)
          .withInput ("Beat Sidechain", juce::AudioChannelSet::stereo(), false)
          .withOutput ("Vocal Output", juce::AudioChannelSet::stereo(), true)),
      parameters (*this, nullptr, "PARAMETERS", createParameterLayout())
{
}

void IntelligenceAudioProcessor::prepareToPlay (double sampleRate, int)
{
    vocalAnalyzer.reset();
    beatAnalyzer.reset();
    maskingProcessor.prepare (sampleRate);
}

void IntelligenceAudioProcessor::releaseResources() {}

bool IntelligenceAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto mainInput = layouts.getMainInputChannelSet();
    const auto mainOutput = layouts.getMainOutputChannelSet();
    if (mainInput != mainOutput) return false;
    if (mainInput != juce::AudioChannelSet::mono() && mainInput != juce::AudioChannelSet::stereo()) return false;
    if (layouts.inputBuses.size() > 1)
    {
        const auto sidechain = layouts.getChannelSet (true, 1);
        if (sidechain != juce::AudioChannelSet::disabled()
            && sidechain != juce::AudioChannelSet::mono()
            && sidechain != juce::AudioChannelSet::stereo()) return false;
    }
    return true;
}

std::array<float, IntelligentMaskingProcessor::bandCount>
IntelligenceAudioProcessor::getBandLevels (const SpectralAnalyzer& analyzer) const noexcept
{
    std::array<float, IntelligentMaskingProcessor::bandCount> levels {};
    const auto binHz = 44100.0f / static_cast<float> (SpectralAnalyzer::fftSize);
    for (size_t band = 0; band < levels.size(); ++band)
    {
        const auto centreBin = juce::roundToInt (bandCentres[band] / binHz);
        const auto radius = juce::jmax (2, centreBin / 5);
        float sum = 0.0f;
        int count = 0;
        for (int bin = juce::jmax (1, centreBin - radius);
             bin <= juce::jmin (SpectralAnalyzer::spectrumSize - 1, centreBin + radius); ++bin)
        {
            sum += analyzer.getMagnitude (bin);
            ++count;
        }
        levels[band] = count > 0 ? sum / static_cast<float> (count) : 0.0f;
    }
    return levels;
}

void IntelligenceAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    auto vocalInput = getBusBuffer (buffer, true, 0);
    auto vocalOutput = getBusBuffer (buffer, false, 0);
    vocalAnalyzer.processBlock (vocalInput);

    bool hasBeat = false;
    if (auto* sidechainBus = getBus (true, 1); sidechainBus != nullptr && sidechainBus->isEnabled())
    {
        auto beatInput = getBusBuffer (buffer, true, 1);
        beatAnalyzer.processBlock (beatInput);
        hasBeat = beatInput.getNumChannels() > 0;
    }

    const auto channels = juce::jmin (vocalInput.getNumChannels(), vocalOutput.getNumChannels());
    const auto samples = juce::jmin (vocalInput.getNumSamples(), vocalOutput.getNumSamples());
    for (int channel = 0; channel < channels; ++channel)
        if (vocalInput.getReadPointer (channel) != vocalOutput.getWritePointer (channel))
            vocalOutput.copyFrom (channel, 0, vocalInput, channel, 0, samples);
    for (int channel = channels; channel < vocalOutput.getNumChannels(); ++channel)
        vocalOutput.clear (channel, 0, samples);

    // TEMPORARY DIAGNOSTIC: prove that the enabled parameter reaches the vocal output.
    // Remove this fixed attenuation after the signal-path test.
    if (parameters.getRawParameterValue ("enabled")->load() > 0.5f)
        vocalOutput.applyGain (juce::Decibels::decibelsToGain (-12.0f));

    maskingProcessor.setEnabled (hasBeat && parameters.getRawParameterValue ("enabled")->load() > 0.5f);
    maskingProcessor.setDepth (parameters.getRawParameterValue ("depth")->load());
    maskingProcessor.setTame (parameters.getRawParameterValue ("tame")->load());
    maskingProcessor.setRecover (parameters.getRawParameterValue ("recover")->load());

    if (hasBeat)
        maskingProcessor.process (vocalOutput, getBandLevels (vocalAnalyzer), getBandLevels (beatAnalyzer));
}

juce::AudioProcessorEditor* IntelligenceAudioProcessor::createEditor()
{
    return new IntelligenceAudioProcessorEditor (*this);
}

void IntelligenceAudioProcessor::getStateInformation (juce::MemoryBlock& destinationData)
{
    if (auto xml = parameters.copyState().createXml())
        copyXmlToBinary (*xml, destinationData);
}

void IntelligenceAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (parameters.state.getType()))
            parameters.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new IntelligenceAudioProcessor();
}
