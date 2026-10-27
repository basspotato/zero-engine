#include "Wavetable.h"

#include <juce_dsp/juce_dsp.h>

namespace
{
    constexpr int fftOrder = 11;
    static_assert ((1 << fftOrder) == WavetableBank::tableSize);

    int maxHarmonic (int level)
    {
        return juce::jmax (1, (WavetableBank::tableSize / 2 - 1) >> level);
    }

    // Sine-series amplitude of harmonic n for each frame.
    float harmonicAmplitude (int frame, int n)
    {
        const auto pi = juce::MathConstants<float>::pi;
        const auto odd = (n % 2) == 1;

        switch (frame)
        {
            case 0:  return n == 1 ? 1.0f : 0.0f;
            case 1:  return odd ? (8.0f / (pi * pi)) * (((n - 1) / 2) % 2 == 0 ? 1.0f : -1.0f) / (float) (n * n) : 0.0f;
            case 2:  return (2.0f / (pi * (float) n)) * (n % 2 == 1 ? 1.0f : -1.0f);
            case 3:  return odd ? 4.0f / (pi * (float) n) : 0.0f;
            default: return (2.0f / (pi * (float) n)) * std::sin ((float) n * pi * 0.25f);
        }
    }
}

WavetableBank::WavetableBank()
    : data ((size_t) (numFrames * numLevels * (tableSize + 1)))
{
    juce::dsp::FFT fft (fftOrder);
    std::vector<float> work ((size_t) tableSize * 2);

    for (int frame = 0; frame < numFrames; ++frame)
    {
        float peak = 0.0f;

        for (int level = 0; level < numLevels; ++level)
        {
            std::fill (work.begin(), work.end(), 0.0f);
            for (int n = 1; n <= maxHarmonic (level); ++n)
                work[(size_t) (2 * n + 1)] = -harmonicAmplitude (frame, n);

            fft.performRealOnlyInverseTransform (work.data());

            auto* t = table (frame, level);
            std::copy (work.begin(), work.begin() + tableSize, t);
            t[tableSize] = t[0];

            // Normalise every level by the full-bandwidth peak so levels match in loudness.
            if (level == 0)
                for (int i = 0; i < tableSize; ++i)
                    peak = juce::jmax (peak, std::abs (t[i]));

            for (int i = 0; i <= tableSize; ++i)
                t[i] /= peak;
        }
    }
}

const WavetableBank& WavetableBank::get()
{
    static const WavetableBank bank;
    return bank;
}

int WavetableBank::levelFor (double frequency, double sampleRate)
{
    const auto allowed = sampleRate * 0.5 / juce::jmax (1.0, frequency);
    int level = 0;
    while (level < numLevels - 1 && maxHarmonic (level) > allowed)
        ++level;
    return level;
}

float WavetableBank::lookup (int level, float position, double phase) const
{
    const auto framePos = juce::jlimit (0.0f, 1.0f, position) * (float) (numFrames - 1);
    const auto frameA = juce::jmin ((int) framePos, numFrames - 2);
    const auto frameMix = framePos - (float) frameA;

    const auto index = phase * tableSize;
    const auto i = (int) index;
    const auto frac = (float) (index - i);

    auto read = [&] (int frame)
    {
        const auto* t = table (frame, level);
        return t[i] + frac * (t[i + 1] - t[i]);
    };

    const auto a = read (frameA);
    return frameMix > 0.0f ? a + frameMix * (read (frameA + 1) - a) : a;
}
