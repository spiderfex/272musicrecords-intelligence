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
    layout.add (std::make_unique<Float> ("bias", "Bias", juce::NormalisableRange<float> (-1.0f, 1.0f, 0.01f), 0.0f));
    layout.add (std::make_unique<Float> ("brighten", "Brighten", juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));
    layout.add (std::make_unique<Float> ("boost", "Boost", juce::NormalisableRange<float> (-6.0f, 6.0f, 0.01f), 0.0f));
    layout.add (std::make_unique<Float> ("output", "Output Gain", juce::NormalisableRange<float> (-24.0f, 12.0f, 0.01f), 0.0f));
    layout.add (std::make_unique<Bool> ("bypass", "Bypass", false));
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
    currentSampleRate.store (sampleRate > 1000.0 ? sampleRate : 44100.0, std::memory_order_relaxed);
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
    const auto binHz = static_cast<float> (currentSampleRate.load (std::memory_order_relaxed)) / static_cast<float> (SpectralAnalyzer::fftSize);
    for (size_t band = 0; band < levels.size(); ++band)
    {
        const auto centreBin = juce::roundToInt (bandCentres[band] / binHz);
        const auto radius = juce::jmax (2, centreBin / 5);
        float peak = 0.0f;
        for (int bin = juce::jmax (1, centreBin - radius);
             bin <= juce::jmin (SpectralAnalyzer::spectrumSize - 1, centreBin + radius); ++bin)
            peak = juce::jmax (peak, analyzer.getMagnitude (bin));
        levels[band] = peak;
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

    maskingProcessor.setEnabled (hasBeat && parameters.getRawParameterValue ("enabled")->load() > 0.5f);
    maskingProcessor.setDepth (parameters.getRawParameterValue ("depth")->load());
    maskingProcessor.setTame (parameters.getRawParameterValue ("tame")->load());
    maskingProcessor.setRecover (parameters.getRawParameterValue ("recover")->load());
    maskingProcessor.setBias (parameters.getRawParameterValue ("bias")->load());
    maskingProcessor.setBrighten (parameters.getRawParameterValue ("brighten")->load());

    const bool bypassed = parameters.getRawParameterValue ("bypass")->load() > 0.5f;
    if (hasBeat && ! bypassed)
        maskingProcessor.process (vocalOutput, getBandLevels (vocalAnalyzer), getBandLevels (beatAnalyzer));

    const auto outputGain = juce::Decibels:: decibelsToGain (
        parameters.getRawParameterValue ("output")->load()
        + parameters.getRawParameterValue ("boost")->load());
    for (int channel = 0; channel < channels; ++channel)
    {
        auto* data = vocalOutput.getWritePointer (channel);
        float inPeak = 0.0f, outPeak = 0.0f;
        const auto* input = vocalInput.getReadPointer (channel);
        for (int sample = 0; sample < samples; ++sample)
        {
            inPeak = juce::jmax (inPeak, std::abs (input[sample]));
            data[sample] *= outputGain;
            outPeak = juce::jmax (outPeak, std::abs (data[sample]));
        }
        const auto inSmooth = 0.25f * inPeak + 0.75f * inputPeakSmooth[static_cast<size_t> (channel)];
        const auto outSmooth = 0.25f * outPeak + 0.75f * outputPeakSmooth[static_cast<size_t> (channel)];
        inputPeakSmooth[static_cast<size_t> (channel)] = inSmooth;
        outputPeakSmooth[static_cast<size_t> (channel)] = outSmooth;
        inputPeak[static_cast<size_t> (channel)].store (inSmooth, std::memory_order_relaxed);
        outputPeak[static_cast<size_t> (channel)].store (outSmooth, std::memory_order_relaxed);
    }
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
