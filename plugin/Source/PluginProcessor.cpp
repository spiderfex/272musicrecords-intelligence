#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::AudioProcessorValueTreeState::ParameterLayout IntelligenceAudioProcessor::createParameterLayout()
{
    using Float = juce::AudioParameterFloat;
    using Bool = juce::AudioParameterBool;
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<Bool> ("enabled", "Intelligence", true));
    layout.add (std::make_unique<Float> ("recover", "Recover", juce::NormalisableRange<float> (0.0f, 200.0f, 0.1f), 0.0f));
    layout.add (std::make_unique<Float> ("tame", "Tame", juce::NormalisableRange<float> (0.0f, 200.0f, 0.1f), 0.0f));
    layout.add (std::make_unique<Float> ("bias", "Bias", juce::NormalisableRange<float> (-100.0f, 100.0f, 0.1f), 0.0f));
    layout.add (std::make_unique<Float> ("brighten", "Brighten", juce::NormalisableRange<float> (-100.0f, 100.0f, 0.1f), 0.0f));
    layout.add (std::make_unique<Float> ("boost", "Boost", juce::NormalisableRange<float> (-50.0f, 50.0f, 0.1f), 0.0f));
    layout.add (std::make_unique<Float> ("output", "Output Gain", juce::NormalisableRange<float> (-24.0f, 12.0f, 0.01f), 0.0f));
    layout.add (std::make_unique<Bool> ("bypass", "Bypass", false));

    return layout;
}

IntelligenceAudioProcessor::IntelligenceAudioProcessor()
    : AudioProcessor (BusesProperties()
          .withInput ("Vocal", juce::AudioChannelSet::stereo(), true)
          .withOutput ("Vocal Output", juce::AudioChannelSet::stereo(), true)),
      parameters (*this, nullptr, "PARAMETERS", createParameterLayout())
{
}

void IntelligenceAudioProcessor::prepareToPlay (double sampleRate, int)
{
    currentSampleRate.store (sampleRate > 1000.0 ? sampleRate : 44100.0, std::memory_order_relaxed);
    vocalAnalyzer.reset();
    maskingProcessor.prepare (sampleRate);
    inputPeakSmooth.fill (0.0f);
    outputPeakSmooth.fill (0.0f);
}

void IntelligenceAudioProcessor::releaseResources() {}

bool IntelligenceAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto mainInput = layouts.getMainInputChannelSet();
    const auto mainOutput = layouts.getMainOutputChannelSet();
    if (mainInput != mainOutput)
        return false;

    return mainInput == juce::AudioChannelSet::mono()
        || mainInput == juce::AudioChannelSet::stereo();
}

std::array<float, IntelligentMaskingProcessor::bandCount>
IntelligenceAudioProcessor::getVocalBandLevels() const noexcept
{
    std::array<float, IntelligentMaskingProcessor::bandCount> levels {};
    const auto sampleRate = static_cast<float> (currentSampleRate.load (std::memory_order_relaxed));
    const auto binHz = sampleRate / static_cast<float> (SpectralAnalyzer::fftSize);
    const auto& centres = IntelligentMaskingProcessor::centres;

    for (int band = 0; band < IntelligentMaskingProcessor::bandCount; ++band)
    {
        const float centre = centres[static_cast<size_t> (band)];
        const float lower = band == 0
            ? centre / std::sqrt (centres[1] / centre)
            : std::sqrt (centres[static_cast<size_t> (band - 1)] * centre);
        const float upper = band == IntelligentMaskingProcessor::bandCount - 1
            ? centre * std::sqrt (centre / centres[static_cast<size_t> (band - 1)])
            : std::sqrt (centre * centres[static_cast<size_t> (band + 1)]);

        const int firstBin = juce::jlimit (1, SpectralAnalyzer::spectrumSize - 1,
                                           juce::roundToInt (lower / binHz));
        const int lastBin = juce::jlimit (firstBin, SpectralAnalyzer::spectrumSize - 1,
                                          juce::roundToInt (upper / binHz));

        float power = 0.0f;
        int count = 0;
        for (int bin = firstBin; bin <= lastBin; ++bin)
        {
            const float magnitude = vocalAnalyzer.getMagnitude (bin);
            power += magnitude * magnitude;
            ++count;
        }

        levels[static_cast<size_t> (band)] = count > 0
            ? std::sqrt (power / static_cast<float> (count)) : 0.0f;
    }

    return levels;
}

void IntelligenceAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    auto vocalInput = getBusBuffer (buffer, true, 0);
    auto vocalOutput = getBusBuffer (buffer, false, 0);
    vocalAnalyzer.processBlock (vocalInput);

    const int channels = juce::jmin (vocalInput.getNumChannels(), vocalOutput.getNumChannels());
    const int samples = juce::jmin (vocalInput.getNumSamples(), vocalOutput.getNumSamples());

    for (int channel = 0; channel < channels; ++channel)
    {
        if (vocalInput.getReadPointer (channel) != vocalOutput.getWritePointer (channel))
            vocalOutput.copyFrom (channel, 0, vocalInput, channel, 0, samples);
    }
    for (int channel = channels; channel < vocalOutput.getNumChannels(); ++channel)
        vocalOutput.clear (channel, 0, samples);

    const bool bypassed = parameters.getRawParameterValue ("bypass")->load() > 0.5f;
    const bool intelligenceOn = parameters.getRawParameterValue ("enabled")->load() > 0.5f;

    maskingProcessor.setEnabled (intelligenceOn);
    maskingProcessor.setRecover (parameters.getRawParameterValue ("recover")->load());
    maskingProcessor.setTame (parameters.getRawParameterValue ("tame")->load());
    maskingProcessor.setBias (parameters.getRawParameterValue ("bias")->load());
    maskingProcessor.setBrighten (parameters.getRawParameterValue ("brighten")->load());
    maskingProcessor.setBoost (parameters.getRawParameterValue ("boost")->load());

    // The main vocal input is the only analysis source. There is no beat bus
    // and no sidechain dependency in this processor.
    if (! bypassed)
        maskingProcessor.process (vocalOutput, getVocalBandLevels());

    const float outputGain = bypassed ? 1.0f : juce::Decibels::decibelsToGain (
        parameters.getRawParameterValue ("output")->load());

    for (int channel = 0; channel < channels; ++channel)
    {
        auto* data = vocalOutput.getWritePointer (channel);
        const auto* input = vocalInput.getReadPointer (channel);
        float inPeak = 0.0f;
        float outPeak = 0.0f;

        for (int sample = 0; sample < samples; ++sample)
        {
            inPeak = juce::jmax (inPeak, std::abs (input[sample]));
            data[sample] *= outputGain;
            outPeak = juce::jmax (outPeak, std::abs (data[sample]));
        }

        const auto index = static_cast<size_t> (channel);
        const float inSmooth = 0.25f * inPeak + 0.75f * inputPeakSmooth[index];
        const float outSmooth = 0.25f * outPeak + 0.75f * outputPeakSmooth[index];
        inputPeakSmooth[index] = inSmooth;
        outputPeakSmooth[index] = outSmooth;
        inputPeak[index].store (inSmooth, std::memory_order_relaxed);
        outputPeak[index].store (outSmooth, std::memory_order_relaxed);
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
