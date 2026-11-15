#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

#include <array>
#include <atomic>
#include <cmath>

class SpectralAnalyzer final
{
public:
    static constexpr int fftOrder = 12;
    static constexpr int fftSize = 1 << fftOrder;
    static constexpr int spectrumSize = fftSize / 2 + 1;
    static constexpr int maxChannels = 2;
    static constexpr int hopSize = 256;

    SpectralAnalyzer() : fft (fftOrder)
    {
        for (int i = 0; i < fftSize; ++i)
        {
            const float phase = juce::MathConstants<float>::twoPi
                              * static_cast<float> (i) / static_cast<float> (fftSize - 1);
            window[static_cast<size_t> (i)] = 0.5f - 0.5f * std::cos (phase);
        }
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
        const int channels = juce::jmin (input.getNumChannels(), maxChannels);
        const int samples = input.getNumSamples();
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
            auto& data = fftData[static_cast<size_t> (channel)];
            const auto& source = ring[static_cast<size_t> (channel)];

            for (int i = 0; i < fftSize; ++i)
            {
                const int ringIndex = (writePosition + i) % fftSize;
                data[static_cast<size_t> (i)] =
                    source[static_cast<size_t> (ringIndex)] * window[static_cast<size_t> (i)];
            }

            for (int i = fftSize; i < static_cast<int> (data.size()); ++i)
                data[static_cast<size_t> (i)] = 0.0f;

            fft.performFrequencyOnlyForwardTransform (data.data());
        }

        for (int bin = 0; bin < spectrumSize; ++bin)
        {
            float power = 0.0f;
            for (int channel = 0; channel < activeChannels; ++channel)
            {
                const float magnitude = fftData[static_cast<size_t> (channel)]
                                               [static_cast<size_t> (bin)]
                                      / static_cast<float> (fftSize);
                power += magnitude * magnitude;
            }

            const float frameMagnitude = std::sqrt (power / static_cast<float> (activeChannels));
            auto& smoothed = smoothedMagnitudes[static_cast<size_t> (bin)];
            const float coefficient = frameMagnitude > smoothed ? 0.62f : 0.20f;
            smoothed += coefficient * (frameMagnitude - smoothed);
            magnitudes[static_cast<size_t> (bin)].store (smoothed, std::memory_order_relaxed);
        }
    }

    juce::dsp::FFT fft;
    std::array<float, fftSize> window {};
    std::array<std::array<float, fftSize>, maxChannels> ring {};
    std::array<std::array<float, fftSize * 2>, maxChannels> fftData {};
    std::array<std::atomic<float>, spectrumSize> magnitudes {};
    std::array<float, spectrumSize> smoothedMagnitudes {};
    int writePosition = 0;
    int samplesSinceAnalysis = 0;
    int activeChannels = 1;
    bool hasFullFrame = false;
};
