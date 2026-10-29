#include "Wavetable.h"
#include "JupiterHarmonics.h"

#include <juce_dsp/juce_dsp.h>

namespace
{
    constexpr int fftOrder = 11;
    static_assert ((1 << fftOrder) == WavetableBank::tableSize);

    enum Bank { basic, growl, digital, organ, fm, wavefold, pwm, neuro, bell, choir, harsh, analog, jupiter };

    // These are easier to describe as a waveform than as harmonics; the
    // constructor analyses them with a forward FFT before band-limiting.
    bool isTimeDomain (int bank)
    {
        return bank == fm || bank == wavefold || bank == pwm || bank == harsh;
    }

    // One cycle, x in [0, 1).
    float timeDomainSample (int bank, int frame, float x)
    {
        const auto t = juce::MathConstants<float>::twoPi * x;

        switch (bank)
        {
            case fm: // two-operator FM, modulator at the carrier's pitch, rising index
            {
                constexpr float index[5] { 0.3f, 1.0f, 2.0f, 3.5f, 6.0f };
                return std::sin (t + index[frame] * std::sin (t));
            }

            case wavefold: // sine folded back on itself harder each frame
            {
                constexpr float gain[5] { 1.0f, 2.0f, 3.5f, 5.5f, 8.0f };
                return std::sin (gain[frame] * std::sin (t));
            }

            case pwm: // pulse narrowing from square to a thin spike
            {
                constexpr float width[5] { 0.5f, 0.35f, 0.2f, 0.1f, 0.04f };
                return x < width[frame] ? 1.0f : -1.0f; // DC is removed afterwards
            }

            default: // harsh: hard-clipped sine, then bit-crushed in the last frames
            {
                constexpr float gain[5] { 1.0f, 2.0f, 4.0f, 8.0f, 16.0f };
                constexpr float steps[5] { 0.0f, 0.0f, 0.0f, 8.0f, 4.0f };
                auto y = juce::jlimit (-1.0f, 1.0f, gain[frame] * std::sin (t));
                if (steps[frame] > 0.0f)
                    y = std::round (y * steps[frame]) / steps[frame];
                return y;
            }
        }
    }

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

            case neuro:
            {
                // Bitier, higher formant pairs with a square-ish odd-harmonic lift.
                constexpr float formants[5][2] { { 10, 16 }, { 14, 24 }, { 18, 32 }, { 9, 40 }, { 22, 48 } };
                const auto shape = bump (n, formants[frame][0], 0.18f) + bump (n, formants[frame][1], 0.18f) + 0.05f;
                return shape * (odd ? 1.3f : 0.7f) / std::sqrt ((float) n) * sawSign (n);
            }

            case bell:
            {
                // Sparse, widely spaced partials build up for a metallic ring.
                constexpr int partials[5] { 4, 7, 11, 17, 25 };
                if (n == 1)
                    return 1.0f;
                for (int i = 0; i <= frame; ++i)
                    if (partials[i] == n)
                        return 0.6f / std::sqrt ((float) (i + 1));
                return 0.0f;
            }

            case choir:
            {
                // Three-formant vowels ooh, oh, ah, eh, ee, placed for a note around C4.
                constexpr float formants[5][3] { { 1.2f, 3.0f, 9.0f }, { 1.9f, 3.0f, 9.5f }, { 3.0f, 4.4f, 11.0f },
                                                 { 2.0f, 6.5f, 10.0f }, { 1.1f, 8.8f, 11.5f } };
                const auto& f = formants[frame];
                const auto shape = bump (n, f[0], 0.35f) + 0.6f * bump (n, f[1], 0.35f) + 0.3f * bump (n, f[2], 0.35f) + 0.02f;
                return shape / std::pow ((float) n, 0.7f);
            }

            case analog:
            {
                // Saw with a soft high roll-off, dark to bright.
                constexpr float rolloff[5] { 8.0f, 15.0f, 30.0f, 60.0f, 200.0f };
                return (2.0f / (pi * (float) n)) * std::exp (-(float) n / rolloff[frame]) * sawSign (n);
            }

            case jupiter: // measured from the owner's Jupiter-8 recordings, dark to bright
                return n <= jupiterHarmonicCount ? jupiterHarmonics[frame][n - 1] * sawSign (n) : 0.0f;

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
    : banksHeld (numBanks), data ((size_t) (numBanks * numFrames * numLevels * (tableSize + 1)))
{
    for (int bank = 0; bank < numBanks; ++bank)
        build (bank, nullptr);
}

WavetableBank::WavetableBank (const Harmonics& harmonics)
    : banksHeld (1), data ((size_t) (numFrames * numLevels * (tableSize + 1)))
{
    build (sampleBank, &harmonics);
}

void WavetableBank::build (int bank, const Harmonics* measured)
{
    juce::dsp::FFT fft (fftOrder);
    std::vector<float> spectrum ((size_t) tableSize * 2), work ((size_t) tableSize * 2);

    {
        for (int frame = 0; frame < numFrames; ++frame)
        {
            // Full spectrum as interleaved (re, im) bins.
            std::fill (spectrum.begin(), spectrum.end(), 0.0f);
            if (isTimeDomain (bank))
            {
                for (int i = 0; i < tableSize; ++i)
                    spectrum[(size_t) i] = timeDomainSample (bank, frame, (float) i / (float) tableSize);
                fft.performRealOnlyForwardTransform (spectrum.data());
                spectrum[0] = spectrum[1] = 0.0f; // no DC
            }
            else if (bank == sampleBank) // measured harmonics, saw-phased; a plain saw when none
            {
                for (int n = 1; n <= numSampleHarmonics; ++n)
                    spectrum[(size_t) (2 * n + 1)] = -(measured != nullptr ? (*measured)[(size_t) frame][(size_t) n - 1] : 1.0f / (float) n) * sawSign (n);
                if (measured == nullptr)
                    for (int n = numSampleHarmonics + 1; n < tableSize / 2; ++n)
                        spectrum[(size_t) (2 * n + 1)] = -sawSign (n) / (float) n;
            }
            else
            {
                for (int n = 1; n < tableSize / 2; ++n)
                    spectrum[(size_t) (2 * n + 1)] = -harmonicAmplitude (bank, frame, n);
            }

            float peak = 0.0f;

            for (int level = 0; level < numLevels; ++level)
            {
                // Keep only the harmonics this octave's mip level can hold.
                std::fill (work.begin(), work.end(), 0.0f);
                const auto keep = (size_t) (2 * (maxHarmonic (level) + 1));
                std::copy (spectrum.begin(), spectrum.begin() + (std::ptrdiff_t) keep, work.begin());

                fft.performRealOnlyInverseTransform (work.data());

                auto* t = table (bank, frame, level);
                std::copy (work.begin(), work.begin() + tableSize, t);
                t[tableSize] = t[0];

                // Normalise every level by the full-bandwidth peak so levels match in loudness.
                if (level == 0)
                    for (int i = 0; i < tableSize; ++i)
                        peak = juce::jmax (peak, std::abs (t[i]));

                for (int i = 0; i <= tableSize; ++i)
                    t[i] /= juce::jmax (1.0e-9f, peak);
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
