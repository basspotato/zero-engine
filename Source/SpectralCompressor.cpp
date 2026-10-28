#include "SpectralCompressor.h"

namespace
{
    constexpr int bins = SpectralCompressor::size / 2 + 1;
    constexpr float ratio = 4.0f;          // both directions, scaled by amount
    constexpr float maxCutDb = 24.0f, maxLiftDb = 12.0f;
    constexpr float floorBelowPeakDb = 50.0f; // bins this far under the loudest are noise: not in the reference
    constexpr float liftBelowPeakDb = 30.0f;  // lift only bins within this of the loudest, fading out toward it,
                                              // or the gaps between harmonics come up as hiss
    constexpr float overlapGain = 1.0f / 1.5f; // Hann analysis x Hann synthesis at 75 % overlap sums to 1.5
}

void SpectralCompressor::prepare (double sampleRate, int numChannels)
{
    channels = juce::jlimit (1, 2, numChannels);

    window.resize ((size_t) size);
    for (int k = 0; k < size; ++k)
        window[(size_t) k] = 0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * (float) k / (float) size); // periodic Hann

    levelDb.assign ((size_t) bins, -120.0f);
    gainDb.assign ((size_t) bins, 0.0f);
    appliedDb.assign ((size_t) bins, 0.0f);
    binHz.resize ((size_t) bins);
    for (int b = 0; b < bins; ++b)
        binHz[(size_t) b] = (float) (b * sampleRate / size);
    firstBin = juce::jmax (1, (int) std::ceil (150.0 * size / sampleRate)); // the low end is left alone

    for (int ch = 0; ch < 2; ++ch)
    {
        input[ch].assign ((size_t) size, 0.0f);
        output[ch].assign ((size_t) size, 0.0f);
        frame[ch].assign ((size_t) size * 2, 0.0f);
    }

    // Per-bin smoothing in frames: a quick 5 ms attack, a 60 ms release.
    const auto framesPerSecond = sampleRate / hop;
    attack = (float) (1.0 - std::exp (-1.0 / (0.005 * framesPerSecond)));
    release = (float) (1.0 - std::exp (-1.0 / (0.060 * framesPerSecond)));

    reset();
}

void SpectralCompressor::reset() noexcept
{
    for (int ch = 0; ch < 2; ++ch)
    {
        std::fill (input[ch].begin(), input[ch].end(), 0.0f);
        std::fill (output[ch].begin(), output[ch].end(), 0.0f);
    }
    std::fill (gainDb.begin(), gainDb.end(), 0.0f);
    pos = hopCount = 0;
}

void SpectralCompressor::process (juce::AudioBuffer<float>& buffer, int numSamples, float amount) noexcept
{
    const auto numCh = juce::jmin (channels, buffer.getNumChannels());

    for (int i = 0; i < numSamples; ++i)
    {
        for (int ch = 0; ch < numCh; ++ch)
        {
            auto* data = buffer.getWritePointer (ch);
            input[ch][(size_t) pos] = data[i];
            data[i] = output[ch][(size_t) pos]; // finished: every overlapping frame has added to it
            output[ch][(size_t) pos] = 0.0f;
        }

        pos = (pos + 1) % size;
        if (++hopCount == hop)
        {
            hopCount = 0;
            processFrame (amount);
        }
    }
}

void SpectralCompressor::processFrame (float amount) noexcept
{
    const auto numCh = channels;

    // Windowed frame, oldest sample first, into the spectrum.
    for (int ch = 0; ch < numCh; ++ch)
    {
        auto& f = frame[ch];
        for (int k = 0; k < size; ++k)
            f[(size_t) k] = input[ch][(size_t) ((pos + k) % size)] * window[(size_t) k];
        std::fill (f.begin() + size, f.end(), 0.0f);
        fft.performRealOnlyForwardTransform (f.data()); // full spectrum: the inverse needs the mirrored half
    }

    if (amount > 1.0e-4f)
    {
        // Stereo-linked level per bin, and the loudest bin.
        auto peakDb = -120.0f;
        for (int b = firstBin; b < bins; ++b)
        {
            auto power = 0.0f;
            for (int ch = 0; ch < numCh; ++ch)
            {
                const auto re = frame[ch][(size_t) (2 * b)], im = frame[ch][(size_t) (2 * b + 1)];
                power += re * re + im * im;
            }
            levelDb[(size_t) b] = 10.0f * std::log10 (power / (float) numCh + 1.0e-12f);
            peakDb = juce::jmax (peakDb, levelDb[(size_t) b]);
        }

        // Reference: a pink (-3 dB/oct) curve through the frame's own average,
        // so the stage evens out the spectrum without brightening or darkening it.
        auto sum = 0.0f;
        int counted = 0;
        // A valley: well under both neighbours (between two harmonics).
        auto isValley = [this] (int b)
        {
            return b > 0 && b + 1 < bins && levelDb[(size_t) b] < levelDb[(size_t) b - 1] - 6.0f && levelDb[(size_t) b] < levelDb[(size_t) b + 1] - 6.0f;
        };
        for (int b = firstBin; b < bins; ++b)
            if (levelDb[(size_t) b] > peakDb - floorBelowPeakDb)
            {
                sum += levelDb[(size_t) b] + 3.0f * std::log2 (binHz[(size_t) b] / 1000.0f);
                ++counted;
            }
        const auto reference = counted > 0 ? sum / (float) counted : -120.0f;
        const auto slope = (1.0f - 1.0f / (1.0f + (ratio - 1.0f) * amount)); // 0 .. 0.75

        for (int b = firstBin; b < bins; ++b)
        {
            const auto target = reference - 3.0f * std::log2 (binHz[(size_t) b] / 1000.0f);
            const auto over = levelDb[(size_t) b] - target;
            auto wanted = 0.0f;
            if (over > 0.0f)
                wanted = -juce::jmin (maxCutDb, over * slope);
            else if (const auto headroom = levelDb[(size_t) b] - (peakDb - liftBelowPeakDb); headroom > 0.0f && ! isValley (b))
                wanted = juce::jmin (maxLiftDb, -over * slope) * juce::jmin (1.0f, headroom / 10.0f);

            auto& g = gainDb[(size_t) b];
            g += (wanted < g ? attack : release) * (wanted - g);
        }
    }
    else
    {
        for (auto& g : gainDb)
            g += release * (0.0f - g);
    }

    // Smooth the gains over nine neighbouring bins: a harmonic spreads over
    // several, and different gains across one harmonic sound like fizz.
    for (int b = firstBin; b < bins; ++b)
    {
        auto sum = 0.0f;
        int n = 0;
        for (int k = juce::jmax (firstBin, b - 4); k <= juce::jmin (bins - 1, b + 4); ++k, ++n)
            sum += gainDb[(size_t) k];
        appliedDb[(size_t) b] = sum / (float) n;
    }

    // Level-neutral: shift every gain so the frame's total energy is unchanged.
    // Lifting thousands of quiet bins would otherwise add up to a big level jump;
    // the stage should reshape the spectrum, not turn it up.
    auto neutralDb = 0.0f;
    if (amount > 1.0e-4f)
    {
        double before = 0.0, after = 0.0;
        for (int b = firstBin; b < bins; ++b)
        {
            const auto power = std::pow (10.0, levelDb[(size_t) b] / 10.0);
            before += power;
            after += power * std::pow (10.0, appliedDb[(size_t) b] / 10.0);
        }
        if (before > 1.0e-12 && after > 1.0e-12)
            neutralDb = (float) juce::jlimit (-12.0, 12.0, 10.0 * std::log10 (after / before));
    }

    // Apply the gains, back to time, and overlap-add.
    for (int ch = 0; ch < numCh; ++ch)
    {
        auto& f = frame[ch];
        for (int b = firstBin; b < bins; ++b)
        {
            const auto gain = juce::Decibels::decibelsToGain (appliedDb[(size_t) b] - neutralDb);
            f[(size_t) (2 * b)] *= gain;
            f[(size_t) (2 * b + 1)] *= gain;
            if (b < size / 2) // and its mirror, so the result stays real
            {
                f[(size_t) (2 * (size - b))] *= gain;
                f[(size_t) (2 * (size - b) + 1)] *= gain;
            }
        }
        fft.performRealOnlyInverseTransform (f.data());

        for (int k = 0; k < size; ++k)
            output[ch][(size_t) ((pos + k) % size)] += f[(size_t) k] * window[(size_t) k] * overlapGain;
    }
}
