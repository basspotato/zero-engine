#include "SoundTraits.h"
#include <juce_dsp/juce_dsp.h>
#include <vector>

SoundTraits SoundTraits::of (const juce::AudioBuffer<float>& audio, double sampleRate)
{
    SoundTraits t;
    const auto channels = juce::jmin (2, audio.getNumChannels());
    const auto total = audio.getNumSamples();
    if (channels <= 0 || total <= 0 || sampleRate <= 0.0)
        return t;
    auto sample = [&] (int ch, int i) { return audio.getSample (juce::jmin (ch, channels - 1), i); };

    // From the first sample within 40 dB of the peak, up to 1 s.
    float peak = 0.0f;
    for (int ch = 0; ch < channels; ++ch)
        peak = juce::jmax (peak, audio.getMagnitude (ch, 0, total));
    if (peak <= 1.0e-6f)
        return t;
    int start = 0;
    while (start < total && std::abs (sample (0, start)) < 0.01f * peak && std::abs (sample (1, start)) < 0.01f * peak)
        ++start;
    const auto length = juce::jmin (total - start, (int) sampleRate);
    if (length < 2048)
        return t;

    double mid2 = 0.0, side2 = 0.0;
    float windowPeak = 0.0f;
    for (int i = start; i < start + length; ++i)
    {
        const auto l = sample (0, i), r = sample (1, i);
        mid2 += 0.25 * (l + r) * (l + r);
        side2 += 0.25 * (l - r) * (l - r);
        windowPeak = juce::jmax (windowPeak, juce::jmax (std::abs (l), std::abs (r)));
    }
    const auto rms = std::sqrt ((mid2 + side2) / length);
    t.levelDb = (float) juce::Decibels::gainToDecibels (rms, -100.0);
    t.sidePct = (float) (100.0 * side2 / juce::jmax (1.0e-12, mid2 + side2));
    t.crestDb = (float) juce::Decibels::gainToDecibels (windowPeak / juce::jmax (1.0e-9, rms), 0.0);

    // Spectra of the mid channel, 4096-point frames every 2048 (at 48 kHz:
    // 85 ms frames every 43 ms), summed for the long-term shape and compared
    // frame to frame for movement, in third-octave bands from 25 Hz.
    constexpr int order = 12, size = 1 << order, hop = size / 2;
    constexpr int numBands = 30; // 25 Hz up by thirds of an octave to about 20 kHz
    juce::dsp::FFT fft (order);
    std::vector<float> frame ((size_t) size * 2);
    std::vector<double> spectrum ((size_t) size / 2, 0.0);
    std::vector<std::array<double, numBands>> bandFrames;
    const auto binHz = sampleRate / size;
    auto bandOf = [] (double hz) { return hz < 25.0 ? -1 : (int) std::floor (3.0 * std::log2 (hz / 25.0)); };

    for (int at = start; at + size <= start + length || bandFrames.empty(); at += hop)
    {
        std::fill (frame.begin(), frame.end(), 0.0f);
        for (int i = 0; i < size && at + i < total; ++i)
            frame[(size_t) i] = 0.5f * (sample (0, at + i) + sample (1, at + i))
                              * (0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * (float) i / (float) size));
        fft.performFrequencyOnlyForwardTransform (frame.data());
        std::array<double, numBands> bands {};
        for (int bin = 1; bin < size / 2; ++bin)
        {
            const auto e = (double) frame[(size_t) bin] * frame[(size_t) bin];
            spectrum[(size_t) bin] += e;
            if (const auto b = bandOf (bin * binHz); b >= 0 && b < numBands)
                bands[(size_t) b] += e;
        }
        bandFrames.push_back (bands);
        if (at + size > start + length)
            break;
    }

    double energy = 0.0, weighted = 0.0, bass = 0.0, body = 0.0, logSum = 0.0, logSquares = 0.0;
    for (int bin = 1; bin < size / 2; ++bin)
    {
        const auto hz = bin * binHz, e = spectrum[(size_t) bin];
        energy += e;
        weighted += e * hz;
        if (hz < 150.0) bass += e;
        else if (hz < 600.0) body += e;
        if (hz >= 20.0)
        {
            const auto octave = std::log2 (hz);
            logSum += e * octave;
            logSquares += e * octave * octave;
        }
    }
    if (energy <= 0.0)
        return t;
    t.centroidHz = (float) (weighted / energy);
    t.bassPct = (float) (100.0 * bass / energy);
    t.bodyPct = (float) (100.0 * body / energy);
    const auto meanOctave = logSum / energy;
    t.spreadOct = (float) std::sqrt (juce::jmax (0.0, logSquares / energy - meanOctave * meanOctave));

    // Movement: mean absolute change in dB per band between successive frames,
    // over the bands within 40 dB of the loudest (silent bands don't count).
    std::array<double, numBands> longTerm {};
    for (const auto& f : bandFrames)
        for (int b = 0; b < numBands; ++b)
            longTerm[(size_t) b] += f[(size_t) b];
    const auto loudestBand = *std::max_element (longTerm.begin(), longTerm.end());
    double change = 0.0;
    int changes = 0;
    for (size_t f = 1; f < bandFrames.size(); ++f)
        for (int b = 0; b < numBands; ++b)
            if (longTerm[(size_t) b] > loudestBand * 1.0e-4)
            {
                auto db = [] (double e) { return 10.0 * std::log10 (e + 1.0e-12); };
                change += std::abs (db (bandFrames[f][(size_t) b]) - db (bandFrames[f - 1][(size_t) b]));
                ++changes;
            }
    t.movementDb = changes > 0 ? (float) (change / changes) : 0.0f;

    // Richness: bands from 100 Hz to 10 kHz within 20 dB of the strongest of them.
    const auto from = bandOf (100.0), to = bandOf (10000.0);
    double strongest = 0.0;
    for (int b = from; b <= to; ++b)
        strongest = juce::jmax (strongest, longTerm[(size_t) b]);
    int rich = 0;
    for (int b = from; b <= to; ++b)
        rich += longTerm[(size_t) b] > strongest * 0.01;
    t.richness = (float) rich / (float) (to - from + 1);
    return t;
}
