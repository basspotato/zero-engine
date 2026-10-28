#include "SoundPrint.h"
#include <juce_dsp/juce_dsp.h>

namespace
{
    constexpr int fftOrder = 12, fftSize = 1 << fftOrder;

    // Fundamental by autocorrelation of the mono window (50 Hz to 1 kHz), or 0
    // when nothing repeats clearly (noise, FX).
    float pitchOf (const std::vector<float>& mono, double sampleRate)
    {
        const auto minLag = (int) (sampleRate / 1000.0), maxLag = (int) (sampleRate / 50.0);
        const auto n = (int) mono.size() - maxLag;
        if (n <= 0)
            return 0.0f;

        double energy = 0.0;
        for (int i = 0; i < n; ++i)
            energy += (double) mono[(size_t) i] * mono[(size_t) i];
        if (energy < 1.0e-9)
            return 0.0f;

        int bestLag = 0;
        double best = 0.0;
        for (int lag = minLag; lag <= maxLag; ++lag)
        {
            double sum = 0.0;
            for (int i = 0; i < n; ++i)
                sum += (double) mono[(size_t) i] * mono[(size_t) (i + lag)];
            sum /= energy;
            if (sum > best * 1.02) // first clear peak wins over its octave-down twins
            {
                best = sum;
                bestLag = lag;
            }
        }
        return best > 0.5 && bestLag > 0 ? (float) (sampleRate / bestLag) : 0.0f;
    }
}

SoundPrint SoundPrint::of (const juce::AudioBuffer<float>& audio, double sampleRate, bool findPitch)
{
    SoundPrint print;
    const auto total = audio.getNumSamples();
    const auto channels = juce::jmin (2, audio.getNumChannels());
    if (total <= 0 || channels <= 0)
        return print;

    auto sample = [&] (int ch, int i) { return audio.getSample (juce::jmin (ch, channels - 1), i); };

    // Window: from 20 ms after the loudest 10 ms, 0.4 s long (shorter if the sound is).
    const auto block = juce::jmax (1, (int) (0.01 * sampleRate));
    int loudest = 0;
    double loudestEnergy = -1.0;
    for (int start = 0; start + block <= total; start += block)
    {
        double e = 0.0;
        for (int i = start; i < start + block; ++i)
            e += sample (0, i) * sample (0, i) + sample (1, i) * sample (1, i);
        if (e > loudestEnergy) { loudestEnergy = e; loudest = start; }
    }
    const auto from = juce::jmin (loudest + (int) (0.02 * sampleRate), juce::jmax (0, total - fftSize));
    const auto length = juce::jmin ((int) (0.4 * sampleRate), total - from);
    if (length < 1024)
        return print;

    // Width, density and level.
    std::vector<float> mono ((size_t) length);
    double mid2 = 0.0, side2 = 0.0;
    float peak = 0.0f;
    for (int i = 0; i < length; ++i)
    {
        const auto l = sample (0, from + i), r = sample (1, from + i);
        mono[(size_t) i] = 0.5f * (l + r);
        mid2 += 0.25 * (l + r) * (l + r);
        side2 += 0.25 * (l - r) * (l - r);
        peak = juce::jmax (peak, std::abs (l), std::abs (r));
    }
    const auto rms = std::sqrt ((mid2 + side2) / length);
    print.levelDb = (float) juce::Decibels::gainToDecibels (rms, -100.0);
    print.sidePct = (float) (100.0 * side2 / juce::jmax (1.0e-12, mid2 + side2));
    print.crestDb = (float) juce::Decibels::gainToDecibels (peak / juce::jmax (1.0e-9, rms), 0.0);
    print.pitchHz = findPitch ? pitchOf (mono, sampleRate) : 0.0f;

    // Spectral shape: Hann-windowed frames, half overlapping, summed into octave bands.
    juce::dsp::FFT fft (fftOrder);
    std::vector<float> frame ((size_t) fftSize * 2);
    std::array<double, numBands> bandPower {};
    for (int start = 0; start + fftSize <= length || start == 0; start += fftSize / 2)
    {
        std::fill (frame.begin(), frame.end(), 0.0f);
        for (int i = 0; i < fftSize && start + i < length; ++i)
            frame[(size_t) i] = mono[(size_t) (start + i)] * (0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * (float) i / (float) fftSize));
        fft.performFrequencyOnlyForwardTransform (frame.data());

        for (int bin = 1; bin < fftSize / 2; ++bin)
        {
            const auto hz = bin * sampleRate / fftSize;
            const auto band = (int) std::floor (std::log2 (hz / 31.25) + 0.5);
            if (band >= 0 && band < numBands)
                bandPower[(size_t) band] += (double) frame[(size_t) bin] * frame[(size_t) bin];
        }
        if (start + fftSize >= length)
            break;
    }

    double sum = 0.0;
    for (auto p : bandPower)
        sum += p;
    for (int b = 0; b < numBands; ++b)
        print.bandsDb[(size_t) b] = sum > 0.0 ? (float) (10.0 * std::log10 (bandPower[(size_t) b] / sum + 1.0e-9)) : -90.0f;
    return print;
}

float SoundPrint::distance (const SoundPrint& a, const SoundPrint& b)
{
    // Spectral shape matters most; bands far below both sounds' energy (under
    // -40 dB of the total) are ignored so noise floors don't count.
    double shape = 0.0;
    int counted = 0;
    for (int band = 0; band < numBands; ++band)
    {
        const auto x = a.bandsDb[(size_t) band], y = b.bandsDb[(size_t) band];
        if (x < -40.0f && y < -40.0f)
            continue;
        const auto d = (juce::jmax (x, -40.0f) - juce::jmax (y, -40.0f)) / 6.0f;
        shape += d * d;
        ++counted;
    }
    shape = counted > 0 ? shape / counted : 0.0;

    const auto width = (a.sidePct - b.sidePct) / 15.0f;
    const auto density = (a.crestDb - b.crestDb) / 4.0f;
    return (float) (shape + 0.5 * width * width + 0.5 * density * density);
}
