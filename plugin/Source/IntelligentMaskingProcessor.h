#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <atomic>
#include <cmath>

class IntelligentMaskingProcessor final
{
public:
    static constexpr int bandCount = 6;
    static constexpr std::array<float, bandCount> centres { 180.0f, 350.0f, 700.0f, 1400.0f, 2800.0f, 5600.0f };

    void prepare (double sampleRate) noexcept
    {
        sr = sampleRate > 1000.0 ? sampleRate : 44100.0;
        reset();
    }
    void reset() noexcept
    {
        for (auto& channel : states) for (auto& state : channel) state = {};
        for (auto& gain : smoothedDb) gain = 0.0f;
        for (auto& gain : displayedReductionDb) gain.store (0.0f, std::memory_order_relaxed);
    }
    void setDepth (float value) noexcept { depth = juce::jlimit (0.0f, 1.0f, value); }
    void setTame (float value) noexcept { tame = juce::jlimit (0.0f, 1.0f, value); }
    void setRecover (float value) noexcept { recover = juce::jlimit (0.0f, 1.0f, value); }
    void setEnabled (bool value) noexcept { enabled = value; }

    void process (juce::AudioBuffer<float>& vocal, const std::array<float, 6>& vocalBands,
                  const std::array<float, 6>& beatBands) noexcept
    {
        const auto channels = juce::jmin (2, vocal.getNumChannels());
        if (! enabled || channels == 0) return;
        std::array<float, bandCount> targetDb {};
        for (int band = 0; band < bandCount; ++band)
        {
            const auto v = juce::jmax (vocalBands[static_cast<size_t> (band)], 1.0e-7f);
            const auto b = juce::jmax (beatBands[static_cast<size_t> (band)], 1.0e-7f);
            const auto ratioDb = juce::Decibels::gainToDecibels (b / v, -36.0f);
            const auto overlap = juce::jlimit (0.0f, 1.0f, (ratioDb + 18.0f) / 24.0f);
            const auto vocalPresence = juce::jlimit (0.0f, 1.0f,
                (juce::Decibels::gainToDecibels (v, -72.0f) + 54.0f) / 36.0f);
            const auto tameCut = overlap * vocalPresence * tame * depth * 4.5f;
            const auto recoverLift = (1.0f - overlap) * vocalPresence * recover * depth * 1.2f;
            targetDb[static_cast<size_t> (band)] = juce::jlimit (-4.5f, 1.2f, recoverLift - tameCut);
        }
        const auto smoothing = 1.0f - std::exp (-static_cast<float> (vocal.getNumSamples())
                                                  / (static_cast<float> (sr) * 0.035f));
        for (int band = 0; band < bandCount; ++band)
        {
            smoothedDb[static_cast<size_t> (band)] += smoothing
                * (targetDb[static_cast<size_t> (band)] - smoothedDb[static_cast<size_t> (band)]);
            displayedReductionDb[static_cast<size_t> (band)].store (
                smoothedDb[static_cast<size_t> (band)], std::memory_order_relaxed);
        }
        for (int band = 0; band < bandCount; ++band)
            updateCoefficients (band, smoothedDb[static_cast<size_t> (band)]);
        for (int channel = 0; channel < channels; ++channel)
        {
            auto* data = vocal.getWritePointer (channel);
            for (int sample = 0; sample < vocal.getNumSamples(); ++sample)
            {
                auto x = data[sample];
                for (int band = 0; band < bandCount; ++band)
                {
                    auto& s = states[static_cast<size_t> (channel)][static_cast<size_t> (band)];
                    const auto& c = coeffs[static_cast<size_t> (band)];
                    const auto y = c.b0 * x + s.z1;
                    s.z1 = c.b1 * x - c.a1 * y + s.z2;
                    s.z2 = c.b2 * x - c.a2 * y;
                    x = y;
                }
                data[sample] = x;
            }
        }
    }
    float getReductionDb (int band) const noexcept
    {
        return band >= 0 && band < bandCount
            ? displayedReductionDb[static_cast<size_t> (band)].load (std::memory_order_relaxed) : 0.0f;
    }
private:
    struct Coefficients { float b0=1, b1=0, b2=0, a1=0, a2=0; };
    struct State { float z1=0, z2=0; };
    void updateCoefficients (int band, float gainDb) noexcept
    {
        constexpr float q = 0.85f;
        const auto A = std::pow (10.0f, gainDb / 40.0f);
        const auto w = juce::MathConstants<float>::twoPi * centres[static_cast<size_t> (band)] / static_cast<float> (sr);
        const auto cs = std::cos (w), sn = std::sin (w), alpha = sn / (2.0f * q);
        const auto a0 = 1.0f + alpha / A;
        auto& c = coeffs[static_cast<size_t> (band)];
        c.b0 = (1.0f + alpha * A) / a0; c.b1 = (-2.0f * cs) / a0;
        c.b2 = (1.0f - alpha * A) / a0; c.a1 = (-2.0f * cs) / a0;
        c.a2 = (1.0f - alpha / A) / a0;
    }
    double sr = 44100.0;
    float depth = 0.5f, tame = 0.7f, recover = 0.25f;
    bool enabled = false;
    std::array<Coefficients, bandCount> coeffs {};
    std::array<std::array<State, bandCount>, 2> states {};
    std::array<float, bandCount> smoothedDb {};
    std::array<std::atomic<float>, bandCount> displayedReductionDb {};
};
