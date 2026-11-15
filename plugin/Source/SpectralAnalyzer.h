#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>

class SpectralAnalyzer final
{
public:
    static constexpr int fftOrder = 11;
    static constexpr int fftSize = 1 << fftOrder;
    static constexpr int spectrumSize = fftSize / 2 + 1;
    static constexpr int maxChannels = 2;
    static constexpr int hopSize = 512;

    SpectralAnalyzer() : fft (fftOrder)
    {
        reset();
    }

    void reset() noexcept
    {
        for (auto& channel : ring)
            channel.fill (0.0f);

        for (auto& channel : fftData)
            channel.fill (0.0f);

        writePosition = 0;
        samplesSinceAnalysis = 0;
        hasFullFrame = false;
        activeChannels = 1;
        smoothedMagnitudes.fill (0.0f);

        for (auto& value : magnitudes)
            value.store (0.0f, std::memory_order_relaxed);
    }

    void processBlock (const juce::AudioBuffer<float>& input) noexcept
    {
        const auto channels = juce::jmin (input.getNumChannels(), maxChannels);
        const auto samples = input.getNumSamples();

        if (channels <= 0 || samples <= 0)
            return;

        activeChannels = channels;

        for (int sample = 0; sample < samples; ++sample)
        {
            for (int channel = 0; channel < channels; ++channel)
                ring[static_cast<size_t> (channel)][static_cast<size_t> (writePosition)]
                    = input.getSample (channel, sample);

            writePosition = (writePosition + 1) % fftSize;
            ++samplesSinceAnalysis;

            if ((! hasFullFrame && samplesSinceAnalysis >= fftSize)
                || (hasFullFrame && samplesSinceAnalysis >= hopSize))
            {
                analyseFrame();
                hasFullFrame = true;
                samplesSinceAnalysis = 0;
            }
        }
    }

    float getMagnitude (int bin) const noexcept
    {
        if (bin < 0 || bin >= spectrumSize)
            return 0.0f;

        return magnitudes[static_cast<size_t> (bin)].load (std::memory_order_relaxed);
    }

private:
    void analyseFrame() noexcept
    {
        for (int channel = 0; channel < activeChannels; ++channel)
        {
            auto& channelData = fftData[static_cast<size_t> (channel)];
            const auto& channelRing = ring[static_cast<size_t> (channel)];

            // writePosition points to the oldest sample in the circular buffer.
            for (int i = 0; i < fftSize; ++i)
            {
                const auto phase = juce::MathConstants<float>::twoPi
                                 * static_cast<float> (i)
                                 / static_cast<float> (fftSize - 1);
                const auto window = 0.5f - 0.5f * std::cos (phase);
                const auto ringIndex = (writePosition + i) % fftSize;
                channelData[static_cast<size_t> (i)]
                    = channelRing[static_cast<size_t> (ringIndex)] * window;
            }

            for (int i = fftSize; i < static_cast<int> (channelData.size()); ++i)
                channelData[static_cast<size_t> (i)] = 0.0f;

            fft.performFrequencyOnlyForwardTransform (channelData.data());
        }

        for (int bin = 0; bin < spectrumSize; ++bin)
        {
            float peakMagnitude = 0.0f;

            for (int channel = 0; channel < activeChannels; ++channel)
            {
                const auto magnitude = fftData[static_cast<size_t> (channel)]
                                           [static_cast<size_t> (bin)]
                                     / static_cast<float> (fftSize);
                peakMagnitude = juce::jmax (peakMagnitude, magnitude);
            }

            auto& smoothed = smoothedMagnitudes[static_cast<size_t> (bin)];
            const auto coefficient = peakMagnitude > smoothed ? 0.55f : 0.16f;
            smoothed += coefficient * (peakMagnitude - smoothed);
            magnitudes[static_cast<size_t> (bin)].store (smoothed, std::memory_order_relaxed);
        }
    }

    juce::dsp::FFT fft;
    std::array<std::array<float, fftSize>, maxChannels> ring {};
    std::array<std::array<float, fftSize * 2>, maxChannels> fftData {};
    std::array<std::atomic<float>, spectrumSize> magnitudes {};
    std::array<float, spectrumSize> smoothedMagnitudes {};
    int writePosition = 0;
    int samplesSinceAnalysis = 0;
    int activeChannels = 1;
    bool hasFullFrame = false;
};
