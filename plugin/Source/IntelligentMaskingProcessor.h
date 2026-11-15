#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <atomic>
#include <cmath>
#include <complex>

class IntelligentMaskingProcessor final
{
public:
    // Twenty-four logarithmically spaced bands from the low vocal range to air.
    // This is an original adaptive spectral-balance design, not Soundtheory code.
    static constexpr int bandCount = 24;
    static constexpr std::array<float, bandCount> centres
    {
        65.0f, 83.5f, 107.0f, 137.0f, 176.0f, 226.0f,
        290.0f, 372.0f, 477.0f, 612.0f, 785.0f, 1006.0f,
        1290.0f, 1654.0f, 2122.0f, 2722.0f, 3491.0f, 4478.0f,
        5742.0f, 7369.0f, 9457.0f, 12129.0f, 15561.0f, 20000.0f
    };

    void prepare (double sampleRate) noexcept
    {
        sr = sampleRate > 1000.0 ? sampleRate : 44100.0;
        reset();
    }

    void reset() noexcept
    {
        for (auto& channel : states)
            for (auto& state : channel)
                state = {};

        smoothedDb.fill (0.0f);
        for (auto& value : displayedResponseDb)
            value.store (0.0f, std::memory_order_relaxed);

        for (int band = 0; band < bandCount; ++band)
            updateCoefficients (band, 0.0f);
    }

    void setRecover (float value) noexcept { recover = juce::jlimit (0.0f, 200.0f, value); }
    void setTame (float value) noexcept { tame = juce::jlimit (0.0f, 200.0f, value); }
    void setBias (float value) noexcept { bias = juce::jlimit (-100.0f, 100.0f, value); }
    void setBrighten (float value) noexcept { brighten = juce::jlimit (-100.0f, 100.0f, value); }
    void setBoost (float value) noexcept { boost = juce::jlimit (-50.0f, 50.0f, value); }
    void setEnabled (bool value) noexcept { enabled = value; }

    void process (juce::AudioBuffer<float>& vocal,
                  const std::array<float, bandCount>& bandLevels,
                  const std::array<float, bandCount>& resonanceScores) noexcept
    {
        const auto channels = juce::jmin (2, vocal.getNumChannels());
        const auto numSamples = vocal.getNumSamples();
        if (channels == 0 || numSamples == 0)
            return;

        std::array<float, bandCount> targetDb {};
        float totalEnergy = 0.0f;

        for (const auto level : bandLevels)
            totalEnergy += level;

        const bool hasSignal = totalEnergy > 1.0e-7f;
        if (enabled && hasSignal)
        {
            std::array<float, bandCount> levelDb {};
            for (int band = 0; band < bandCount; ++band)
                levelDb[static_cast<size_t> (band)] =
                    juce::Decibels::gainToDecibels (
                        juce::jmax (bandLevels[static_cast<size_t> (band)], 1.0e-9f), -120.0f);

            // Estimate the local spectral neighbourhood without using an external
            // reference signal. A band above its neighbourhood is dominant;
            // a band below it is potentially masked by nearby spectral energy.
            for (int band = 0; band < bandCount; ++band)
            {
                float neighbourhood = 0.0f;
                float weightSum = 0.0f;

                for (int offset = -3; offset <= 3; ++offset)
                {
                    if (offset == 0)
                        continue;

                    const int neighbour = band + offset;
                    if (neighbour < 0 || neighbour >= bandCount)
                        continue;

                    const float weight = offset == -1 || offset == 1 ? 1.0f
                                       : offset == -2 || offset == 2 ? 0.65f : 0.35f;
                    neighbourhood += levelDb[static_cast<size_t> (neighbour)] * weight;
                    weightSum += weight;
                }

                const float localDb = weightSum > 0.0f ? neighbourhood / weightSum
                                                       : levelDb[static_cast<size_t> (band)];
                const float prominence = levelDb[static_cast<size_t> (band)] - localDb;

                // Positive Bias gives Recover more classification space;
                // negative Bias gives Tame more classification space.
                const float biasDb = bias * 0.045f;
                const float recoverClass = -prominence + biasDb;

                const float recoverNeed = juce::jlimit (0.0f, 1.0f,
                    (recoverClass - 2.0f) / 7.0f);

                // A separate narrow-peak detector is used for Tame. It compares
                // the strongest FFT bin in this band with its immediate spectral
                // neighbours, rather than treating every naturally strong vocal
                // formant as a harsh resonance.
                const float resonanceNeed = juce::jlimit (0.0f, 1.0f,
                    (resonanceScores[static_cast<size_t> (band)] - 1.5f) / 4.5f);

                const float recoverDb = recoverNeed * (recover / 100.0f) * 1.5f;
                const float tameDb = resonanceNeed * (tame / 100.0f) * 4.0f;
                targetDb[static_cast<size_t> (band)] = recoverDb - tameDb;
            }

            // Brighten shifts the unmasking balance toward darker or brighter
            // regions. It is intentionally secondary to Recover and Tame.
            if (recover > 0.0f || tame > 0.0f)
            {
                for (int band = 0; band < bandCount; ++band)
                {
                    const float octavesFrom1k = std::log2 (centres[static_cast<size_t> (band)] / 1000.0f);
                    const float frequencyPosition = juce::jlimit (-1.0f, 1.0f, octavesFrom1k / 4.0f);
                    targetDb[static_cast<size_t> (band)] +=
                        (brighten / 100.0f) * frequencyPosition * 0.6f;
                }
            }

            // Boost is an independent bass/mid tonal-balance control.
            const float boostAmount = boost / 50.0f;
            for (int band = 0; band < bandCount; ++band)
            {
                const float hz = centres[static_cast<size_t> (band)];
                const float bassOctaves = std::log2 (hz / 100.0f);
                const float midOctaves = std::log2 (hz / 1400.0f);
                const float bass = std::exp (-0.5f * bassOctaves * bassOctaves / (1.15f * 1.15f));
                const float mids = std::exp (-0.5f * midOctaves * midOctaves / (1.35f * 1.35f));
                targetDb[static_cast<size_t> (band)] += boostAmount * (3.0f * bass - 1.5f * mids);
            }

            // Remove the energy-weighted mean correction to avoid turning the
            // adaptive EQ into a hidden master-volume control.
            float weightedGain = 0.0f;
            float weightTotal = 0.0f;
            for (int band = 0; band < bandCount; ++band)
            {
                const float weight = bandLevels[static_cast<size_t> (band)];
                weightedGain += targetDb[static_cast<size_t> (band)] * weight;
                weightTotal += weight;
            }

            if (weightTotal > 1.0e-8f)
            {
                const float meanGain = weightedGain / weightTotal;
                for (auto& gain : targetDb)
                    gain = juce::jlimit (-4.0f, 1.2f, gain - meanGain);
            }
        }

        const float blockSeconds = static_cast<float> (numSamples) / static_cast<float> (sr);
        for (int band = 0; band < bandCount; ++band)
        {
            auto& current = smoothedDb[static_cast<size_t> (band)];
            const float target = targetDb[static_cast<size_t> (band)];
            const float timeConstant = target < current ? 0.018f : 0.120f;
            const float smoothing = 1.0f - std::exp (-blockSeconds / timeConstant);
            current += smoothing * (target - current);

            updateCoefficients (band, current);
        }
        updateDisplayedResponse();

        for (int channel = 0; channel < channels; ++channel)
        {
            auto* data = vocal.getWritePointer (channel);
            for (int sample = 0; sample < numSamples; ++sample)
            {
                float x = data[sample];
                for (int band = 0; band < bandCount; ++band)
                {
                    auto& state = states[static_cast<size_t> (channel)][static_cast<size_t> (band)];
                    const auto& coeff = coefficients[static_cast<size_t> (band)];
                    const float y = coeff.b0 * x + state.z1;
                    state.z1 = coeff.b1 * x - coeff.a1 * y + state.z2;
                    state.z2 = coeff.b2 * x - coeff.a2 * y;
                    x = y;
                }
                data[sample] = x;
            }
        }
    }

    float getResponseDb (int band) const noexcept
    {
        return band >= 0 && band < bandCount
            ? displayedResponseDb[static_cast<size_t> (band)].load (std::memory_order_relaxed)
            : 0.0f;
    }

    // Evaluate the complete cascade of active biquad filters at one frequency.
    // This is the actual magnitude response, not the per-band control values.
    float getTotalResponseDb (float frequencyHz) const noexcept
    {
        using Complex = std::complex<float>;
        const float w = juce::MathConstants<float>::twoPi
                      * juce::jlimit (1.0f, static_cast<float> (sr * 0.49), frequencyHz)
                      / static_cast<float> (sr);
        const Complex z1 (std::cos (w), -std::sin (w));
        const Complex z2 = z1 * z1;
        Complex total (1.0f, 0.0f);
        for (const auto& c : coefficients)
        {
            const Complex numerator = c.b0 + c.b1 * z1 + c.b2 * z2;
            const Complex denominator = 1.0f + c.a1 * z1 + c.a2 * z2;
            if (std::abs (denominator) > 1.0e-12f)
                total *= numerator / denominator;
        }
        return juce::Decibels::gainToDecibels (std::abs (total), -120.0f);
    }

private:
    struct Coefficients { float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f, a1 = 0.0f, a2 = 0.0f; };
    struct State { float z1 = 0.0f, z2 = 0.0f; };

    void updateCoefficients (int band, float gainDb) noexcept
    {
        constexpr float q = 5.0f;
        const float frequency = juce::jlimit (20.0f, static_cast<float> (sr * 0.45),
                                               centres[static_cast<size_t> (band)]);
        const float amplitude = std::pow (10.0f, gainDb / 40.0f);
        const float w = juce::MathConstants<float>::twoPi * frequency / static_cast<float> (sr);
        const float cosine = std::cos (w);
        const float sine = std::sin (w);
        const float alpha = sine / (2.0f * q);
        const float a0 = 1.0f + alpha / amplitude;

        auto& c = coefficients[static_cast<size_t> (band)];
        c.b0 = (1.0f + alpha * amplitude) / a0;
        c.b1 = (-2.0f * cosine) / a0;
        c.b2 = (1.0f - alpha * amplitude) / a0;
        c.a1 = (-2.0f * cosine) / a0;
        c.a2 = (1.0f - alpha / amplitude) / a0;
    }

    void updateDisplayedResponse() noexcept
    {
        // Show the live per-band correction commands, referenced to 0 dB.
        // Positive values are boosts; negative values are cuts. Plotting the
        // individual adaptive bands keeps both directions visible instead
        // of letting overlapping filter responses hide the signed decisions.
        for (int band = 0; band < bandCount; ++band)
            displayedResponseDb[static_cast<size_t> (band)].store (
                smoothedDb[static_cast<size_t> (band)], std::memory_order_relaxed);
    }

    double sr = 44100.0;
    float recover = 0.0f, tame = 0.0f, bias = 0.0f, brighten = 0.0f, boost = 0.0f;
    bool enabled = false;
    std::array<Coefficients, bandCount> coefficients {};
    std::array<std::array<State, bandCount>, 2> states {};
    std::array<float, bandCount> smoothedDb {};
    std::array<std::atomic<float>, bandCount> displayedResponseDb {};
};
