#include "Wavetable.h"

#include <juce_dsp/juce_dsp.h>

namespace
{
    constexpr int fftOrder = 11;
    static_assert ((1 << fftOrder) == WavetableBank::tableSize);

    enum Bank { basic, growl, digital, organ };

    int maxHarmonic (int level)
    {
        return juce::jmax (1, (WavetableBank::tableSize / 2 - 1) >> level);
    }

    float sawSign (int n) { return n % 2 == 1 ? 1.0f : -1.0f; }

    // Bump centred on harmonic `centre`, `width` octaves wide.
    float bump (int n, float centre, float width)
    {
        const auto octaves = std::log2 ((float) n / centre) / width;
        return std::exp (-0.5f * octaves * octaves);
    }

    // Sine-series amplitude of harmonic n for each bank and frame.
    float harmonicAmplitude (int bank, int frame, int n)
    {
        const auto pi = juce::MathConstants<float>::pi;
        const auto odd = (n % 2) == 1;

        switch (bank)
        {
            case basic: // sine, triangle, saw, square, pulse 25%
                switch (frame)
                {
                    case 0:  return n == 1 ? 1.0f : 0.0f;
                    case 1:  return odd ? (8.0f / (pi * pi)) * (((n - 1) / 2) % 2 == 0 ? 1.0f : -1.0f) / (float) (n * n) : 0.0f;
                    case 2:  return (2.0f / (pi * (float) n)) * sawSign (n);
                    case 3:  return odd ? 4.0f / (pi * (float) n) : 0.0f;
                    default: return (2.0f / (pi * (float) n)) * std::sin ((float) n * pi * 0.25f);
                }

            case growl:
            {
                // Vowel formants U, O, A, E, I as harmonic numbers (pitch-relative,
                // placed for a bass note around C2), on a saw-like base.
                constexpr float formants[5][2] { { 5, 11 }, { 8, 12 }, { 12, 18 }, { 6, 26 }, { 5, 35 } };
                const auto shape = bump (n, formants[frame][0], 0.3f) + 0.7f * bump (n, formants[frame][1], 0.3f) + 0.03f;
                return (shape + (n == 1 ? 0.5f : 0.0f)) / std::sqrt ((float) n) * sawSign (n);
            }

            case digital:
            {
                // Hard-sync-like sweep: a resonant peak moving up a saw spectrum.
                constexpr float peaks[5] { 1.0f, 2.0f, 3.5f, 5.5f, 8.0f };
                const auto r = peaks[frame];
                const auto d = ((float) n - r) / (0.6f + 0.15f * r);
                return (1.0f / (float) n) * (1.0f + 3.0f * std::exp (-0.5f * d * d)) * sawSign (n);
            }

            default: // organ drawbars on harmonics 1, 2, 3, 4, 6, 8
            {
                constexpr int harmonics[6] { 1, 2, 3, 4, 6, 8 };
                constexpr float drawbars[5][6] { { 1.0f, 0.5f, 0.0f, 0.0f, 0.0f, 0.0f },
                                                 { 1.0f, 0.8f, 0.6f, 0.0f, 0.0f, 0.0f },
                                                 { 1.0f, 1.0f, 0.8f, 0.5f, 0.0f, 0.0f },
                                                 { 1.0f, 1.0f, 1.0f, 0.8f, 0.5f, 0.0f },
                                                 { 1.0f, 1.0f, 1.0f, 1.0f, 0.8f, 0.6f } };
                for (int i = 0; i < 6; ++i)
                    if (harmonics[i] == n)
                        return drawbars[frame][i];
                return 0.0f;
            }
        }
    }
}

WavetableBank::WavetableBank()
    : data ((size_t) (numBanks * numFrames * numLevels * (tableSize + 1)))
{
    juce::dsp::FFT fft (fftOrder);
    std::vector<float> work ((size_t) tableSize * 2);

    for (int bank = 0; bank < numBanks; ++bank)
    {
        for (int frame = 0; frame < numFrames; ++frame)
        {
            float peak = 0.0f;

            for (int level = 0; level < numLevels; ++level)
            {
                std::fill (work.begin(), work.end(), 0.0f);
                for (int n = 1; n <= maxHarmonic (level); ++n)
                    work[(size_t) (2 * n + 1)] = -harmonicAmplitude (bank, frame, n);

                fft.performRealOnlyInverseTransform (work.data());

                auto* t = table (bank, frame, level);
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

float WavetableBank::lookup (int bank, int level, float position, double phase) const
{
    const auto framePos = juce::jlimit (0.0f, 1.0f, position) * (float) (numFrames - 1);
    const auto frameA = juce::jmin ((int) framePos, numFrames - 2);
    const auto frameMix = framePos - (float) frameA;

    const auto index = phase * tableSize;
    const auto i = (int) index;
    const auto frac = (float) (index - i);

    auto read = [&] (int frame)
    {
        const auto* t = table (bank, frame, level);
        return t[i] + frac * (t[i + 1] - t[i]);
    };

    const auto a = read (frameA);
    return frameMix > 0.0f ? a + frameMix * (read (frameA + 1) - a) : a;
}
