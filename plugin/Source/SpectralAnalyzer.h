#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

#include <array>
#include <atomic>
#include <cmath>

class SpectralAnalyzer final
{
public:
    static constexpr int fftOrder = 11;
    static constexpr int fftSize = 1 << fftOrder;
    static constexpr int spectrumSize = fftSize / 2 + 1;

    SpectralAnalyzer() : fft (fftOrder)
    {
        reset();
    }

    void reset() noexcept
    {
        ring.fill (0.0f);
        fftData.fill (0.0f);
        writePosition = 0;
        for (auto& value : magnitudes)
            value.store (0.0f, std::memory_order_relaxed);
    }

    void processBlock (const juce::AudioBuffer<float>& input) noexcept
    {
        const auto channels = input.getNumChannels();
        const auto samples = input.getNumSamples();

        if (channels <= 0 || samples <= 0)
            return;

        for (int sample = 0; sample < samples; ++sample)
        {
            float monoSample = 0.0f;
            for (int channel = 0; channel < channels; ++channel)
                monoSample += input.getSample (channel, sample);

            ring[static_cast<size_t> (writePosition)] = monoSample / static_cast<float> (channels);
            ++writePosition;

            if (writePosition == fftSize)
            {
                analyseFrame();
                writePosition = 0;
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
        for (int i = 0; i < fftSize; ++i)
        {
            const auto phase = juce::MathConstants<float>::twoPi
                             * static_cast<float> (i)
                             / static_cast<float> (fftSize - 1);
            const auto window = 0.5f - 0.5f * std::cos (phase);
            fftData[static_cast<size_t> (i)] = ring[static_cast<size_t> (i)] * window;
        }

        for (int i = fftSize; i < static_cast<int> (fftData.size()); ++i)
            fftData[static_cast<size_t> (i)] = 0.0f;

        fft.performFrequencyOnlyForwardTransform (fftData.data());

        for (int bin = 0; bin < spectrumSize; ++bin)
        {
            const auto magnitude = fftData[static_cast<size_t> (bin)]
                                 / static_cast<float> (fftSize);
            magnitudes[static_cast<size_t> (bin)].store (magnitude,
                                                         std::memory_order_relaxed);
        }
    }

    juce::dsp::FFT fft;
    std::array<float, fftSize> ring {};
    std::array<float, fftSize * 2> fftData {};
    std::array<std::atomic<float>, spectrumSize> magnitudes {};
    int writePosition = 0;
};
