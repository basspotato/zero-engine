#include "SoundPrint.h"
#include <juce_dsp/juce_dsp.h>
#include <algorithm>

namespace
{
    constexpr int fftOrder = 12, fftSize = 1 << fftOrder;

    // Fundamental of the mono window (50 Hz to 1 kHz) by YIN (de Cheveigné &
    // Kawahara 2002): the cumulative-mean-normalised difference function, first
    // dip under the threshold, refined by parabola. 0 when nothing repeats
    // clearly (noise, FX). Plain autocorrelation reads any dark, smooth sound as
    // its highest allowed pitch.
    float pitchOf (const std::vector<float>& mono, double sampleRate)
    {
        const auto minLag = juce::jmax (2, (int) (sampleRate / 1000.0)), maxLag = (int) (sampleRate / 50.0);
        const auto window = juce::jmin ((int) (0.05 * sampleRate), (int) mono.size() - maxLag - 1);
        if (window < maxLag)
            return 0.0f;

        std::vector<double> d ((size_t) maxLag + 2, 0.0);
        for (int lag = 1; lag <= maxLag + 1; ++lag)
            for (int i = 0; i < window; ++i)
            {
                const auto diff = (double) mono[(size_t) i] - mono[(size_t) (i + lag)];
                d[(size_t) lag] += diff * diff;
            }

        // Normalise: d'(lag) = d(lag) * lag / sum of d up to lag.
        double running = 0.0;
        d[0] = 1.0;
        for (int lag = 1; lag <= maxLag + 1; ++lag)
        {
            running += d[(size_t) lag];
            d[(size_t) lag] = running > 0.0 ? d[(size_t) lag] * lag / running : 1.0;
        }

        int found = 0;
        for (int lag = minLag; lag <= maxLag; ++lag)
            if (d[(size_t) lag] < 0.15)
            {
                while (lag < maxLag && d[(size_t) lag + 1] < d[(size_t) lag])
                    ++lag;
                found = lag;
                break;
            }
        if (found == 0) // no clear dip: take the deepest, if it is deep enough
        {
            auto deepest = minLag;
            for (int lag = minLag; lag <= maxLag; ++lag)
                if (d[(size_t) lag] < d[(size_t) deepest])
                    deepest = lag;
            if (d[(size_t) deepest] > 0.35)
                return 0.0f;
            // The deepest dip can sit at two or three periods; the first dip
            // nearly as deep is the period itself.
            found = deepest;
            for (int lag = minLag + 1; lag < deepest; ++lag)
                if (d[(size_t) lag] <= d[(size_t) lag - 1] && d[(size_t) lag] <= d[(size_t) lag + 1] && d[(size_t) lag] < d[(size_t) deepest] + 0.1)
                {
                    found = lag;
                    break;
                }
        }

        const auto a = d[(size_t) found - 1], b = d[(size_t) found], c = d[(size_t) found + 1];
        const auto shift = std::abs (a - 2.0 * b + c) > 1.0e-12 ? 0.5 * (a - c) / (a - 2.0 * b + c) : 0.0;
        return (float) (sampleRate / (found + juce::jlimit (-0.5, 0.5, shift)));
    }

    // A repeating wobble in a series of 10 ms dB values: the series minus its
    // straight-line trend (the decay), its depth (RMS, dB) and its rate (Hz) by
    // autocorrelation over 2 to 12 Hz, or 0 when nothing repeats clearly.
    struct Wobble { float rateHz = 0.0f, depthDb = 0.0f; };
    Wobble wobbleOf (std::vector<double> series)
    {
        Wobble w;
        const auto n = (int) series.size();
        if (n < 40)
            return w;

        double meanX = 0.5 * (n - 1), meanY = 0.0, sxy = 0.0, sxx = 0.0;
        for (auto v : series) meanY += v;
        meanY /= n;
        for (int i = 0; i < n; ++i)
        {
            sxy += (i - meanX) * (series[(size_t) i] - meanY);
            sxx += (i - meanX) * (i - meanX);
        }
        const auto trend = sxy / sxx;
        double power = 0.0;
        for (int i = 0; i < n; ++i)
        {
            series[(size_t) i] -= meanY + trend * (i - meanX);
            power += series[(size_t) i] * series[(size_t) i];
        }
        w.depthDb = (float) std::sqrt (power / n);
        if (w.depthDb < 1.0f)
            return w; // under 1 dB: no audible wobble, whatever repeats
        if (power < 1.0e-6)
            return w;

        std::vector<double> r (51, 0.0);
        for (int lag = 1; lag <= 50 && lag < n / 2; ++lag)
        {
            for (int i = 0; i + lag < n; ++i)
                r[(size_t) lag] += series[(size_t) i] * series[(size_t) (i + lag)];
            r[(size_t) lag] /= power * (n - lag) / n;
        }
        for (int lag = 8; lag < 50 && lag + 1 < n / 2; ++lag)
            if (r[(size_t) lag] > 0.3 && r[(size_t) lag] >= r[(size_t) lag - 1] && r[(size_t) lag] >= r[(size_t) lag + 1])
            {
                w.rateHz = 100.0f / (float) lag;
                break;
            }
        return w;
    }
}

SoundPrint SoundPrint::of (const juce::AudioBuffer<float>& audio, double sampleRate, bool findPitch)
{
    SoundPrint print;
    const auto total = audio.getNumSamples();
    const auto channels = juce::jmin (2, audio.getNumChannels());
    if (total <= 0 || channels <= 0)
        return print;

    // DC removed: recordings often carry an offset that would read as deep sub.
    float dc[2] {};
    for (int ch = 0; ch < channels; ++ch)
    {
        double sum = 0.0;
        for (int i = 0; i < total; ++i)
            sum += audio.getSample (ch, i);
        dc[ch] = (float) (sum / total);
    }
    auto sample = [&] (int ch, int i) { ch = juce::jmin (ch, channels - 1); return audio.getSample (ch, i) - dc[ch]; };

    // Energy per 10 ms; the onset is the first block within 30 dB of the loudest,
    // and only the 1.2 s after it are measured.
    const auto block = juce::jmax (1, (int) (0.01 * sampleRate));
    std::vector<double> energy;
    std::vector<double> slope; // energy of the sample-to-sample change: rises with brightness
    for (int start = 0; start + block <= total; start += block)
    {
        double e = 0.0, s = 0.0;
        for (int i = start; i < start + block; ++i)
        {
            e += sample (0, i) * sample (0, i) + sample (1, i) * sample (1, i);
            if (i > 0)
            {
                const auto d = sample (0, i) + sample (1, i) - sample (0, i - 1) - sample (1, i - 1);
                s += d * d;
            }
        }
        energy.push_back (e);
        slope.push_back (s);
    }
    if (energy.empty())
        return print;

    const auto overall = *std::max_element (energy.begin(), energy.end());
    const auto onset = (int) (std::find_if (energy.begin(), energy.end(), [&] (double e) { return e > overall * 1.0e-3; }) - energy.begin());
    // A loop or phrase: only the first note counts. The next note starts at a
    // pitch change (an arp; references only, tracked every 20 ms), or at a sharp
    // re-attack: the level up 9 dB within 10 ms after falling 12 dB under the
    // loudest so far (a filter wobble moves too smoothly to count).
    auto last = juce::jmin ((int) energy.size(), onset + (int) (span * 100.0));
    bool cut = false; // ended by the next note: what comes after is unknown, not silence
    float notePitch = 0.0f; // the first note's pitch, from readings across it (references)
    {
        auto loudestSoFar = energy[(size_t) onset];
        for (int b = onset + 2; b < last; ++b)
        {
            if (energy[(size_t) b] > 8.0 * energy[(size_t) b - 1] && energy[(size_t) b - 1] < loudestSoFar / 16.0)
            {
                last = b - 1;
                cut = true;
                break;
            }
            loudestSoFar = juce::jmax (loudestSoFar, energy[(size_t) b]);
        }
    }
    if (findPitch)
    {
        const auto frame = (int) (0.06 * sampleRate);
        auto pitchAt = [&] (int b)
        {
            std::vector<float> slice ((size_t) frame);
            for (int i = 0; i < frame; ++i)
                slice[(size_t) i] = b * block + i < total ? 0.5f * (sample (0, b * block + i) + sample (1, b * block + i)) : 0.0f;
            return pitchOf (slice, sampleRate);
        };
        auto semitonesApart = [] (float x, float y)
        {
            auto s = std::abs (12.0f * std::log2 (x / y));
            return std::abs (s - 12.0f) < 0.6f ? 0.0f : s; // an octave slip in the reading, not a new note
        };

        // A new note: two readings in a row a semitone or more from the first
        // note's and within a third of a semitone of each other (a wobble
        // makes readings wander; an arp note holds steady). The note's own
        // pitch is the median of its readings.
        std::vector<float> readings;
        float pending = 0.0f;
        int pendingAt = -1;
        for (int b = onset + 2; b + 2 < last; b += 2)
        {
            const auto p = pitchAt (b);
            if (p <= 0.0f)
                continue;
            if (readings.empty() || semitonesApart (p, readings.front()) < 1.0f)
            {
                readings.push_back (p);
                pendingAt = -1;
                continue;
            }
            if (pendingAt >= 0 && semitonesApart (p, pending) < 0.35f)
            {
                last = pendingAt;
                cut = true;
                break;
            }
            pending = p;
            pendingAt = b;
        }
        if (! readings.empty())
        {
            std::nth_element (readings.begin(), readings.begin() + (long) readings.size() / 2, readings.end());
            notePitch = readings[readings.size() / 2];
        }
    }
    const auto peakBlock = (int) (std::max_element (energy.begin() + onset, energy.begin() + last) - energy.begin());
    const auto peakEnergy = juce::jmax (1.0e-12, energy[(size_t) peakBlock]);

    // Attack: until the first block within 3 dB of the loudest (a sound that
    // swells slowly after a quick start still counts as quick).
    auto reached = onset;
    while (reached < peakBlock && energy[(size_t) reached] < 0.5 * peakEnergy)
        ++reached;
    print.attackMs = 10.0f * (float) (reached - onset);

    constexpr int pointBlocks[] { 10, 25, 50, 90 };
    for (int p = 0; p < numEnvelopePoints; ++p)
    {
        const auto b = onset + pointBlocks[p];
        double sum = 0.0; // 50 ms around the point, so a wobble doesn't make it jumpy
        for (int i = b - 2; i <= b + 2; ++i)
            sum += i < last ? energy[(size_t) i] : 0.0;
        print.envelopeDb[(size_t) p] = b + 2 < last ? (float) (10.0 * std::log10 (sum / 5.0 / peakEnergy + 1.0e-9))
                                     : cut ? unknown : -60.0f;
    }

    // Movement after the attack settles (from 100 ms past the loudest moment,
    // while within 30 dB of it):
    // level and brightness wobbles; whichever is deeper speaks for the sound.
    {
        std::vector<double> level, bright;
        for (int b = peakBlock + 10; b < last; ++b)
        {
            const auto e = energy[(size_t) b];
            if (e < peakEnergy * 1.0e-3)
                break; // a decay into the noise floor is not a wobble
            level.push_back (10.0 * std::log10 (e));
            bright.push_back (10.0 * std::log10 (juce::jmax (1.0e-12, slope[(size_t) b]) / e));
        }
        const auto a = wobbleOf (level), b = wobbleOf (bright);
        const auto& w = (b.rateHz > 0.0f && (a.rateHz <= 0.0f || b.depthDb > a.depthDb)) ? b : a;
        print.moveRateHz = w.rateHz;
        print.moveDepthDb = juce::jmax (a.depthDb, b.depthDb);
    }

    // Spectrum window: from 20 ms after the loudest 10 ms, 0.4 s long (shorter if the sound is).
    const auto loudest = peakBlock * block;
    const auto noteEnd = juce::jmin (total, last * block);
    auto from = juce::jmin (loudest + (int) (0.02 * sampleRate), juce::jmax (0, noteEnd - fftSize));
    auto length = juce::jmin ((int) (0.4 * sampleRate), noteEnd - from);
    if (length < 1024) // a very short note: take what follows it too
    {
        from = loudest;
        length = juce::jmin ((int) (0.4 * sampleRate), total - from);
    }
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
    print.pitchHz = findPitch ? (notePitch > 0.0f ? notePitch : pitchOf (mono, sampleRate)) : 0.0f;

    // Spectral shape: Hann-windowed frames, half overlapping, summed into octave bands.
    juce::dsp::FFT fft (fftOrder);
    std::vector<float> frame ((size_t) fftSize * 2);
    std::array<double, numBands> bandPower {};
    double weightedHz = 0.0, centroidPower = 0.0;
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
            const auto power = (double) frame[(size_t) bin] * frame[(size_t) bin];
            if (band >= 0 && band < numBands)
                bandPower[(size_t) band] += power;
            if (hz >= 20.0 && hz <= 20000.0)
            {
                weightedHz += hz * power;
                centroidPower += power;
            }
        }
        if (start + fftSize >= length)
            break;
    }

    double sum = 0.0;
    for (auto p : bandPower)
        sum += p;
    for (int b = 0; b < numBands; ++b)
        print.bandsDb[(size_t) b] = sum > 0.0 ? (float) (10.0 * std::log10 (bandPower[(size_t) b] / sum + 1.0e-9)) : -90.0f;
    print.centroidHz = centroidPower > 0.0 ? (float) (weightedHz / centroidPower) : 0.0f;

    // The pitch against the spectrum: a period shared by several partials (a
    // chord, a layered patch) can read far under the note you hear. When the
    // pitch's own octave band is weak, step up through its multiples to the
    // first one whose band carries real energy.
    if (print.pitchHz > 0.0f)
    {
        const auto strongest = *std::max_element (print.bandsDb.begin(), print.bandsDb.end());
        auto bandOf = [] (float hz) { return (int) std::floor (std::log2 (hz / 31.25) + 0.5); };
        for (int multiple = 1; multiple <= 4; ++multiple)
        {
            const auto band = bandOf (print.pitchHz * (float) multiple);
            if (band >= 0 && band < numBands && print.bandsDb[(size_t) band] > strongest - 12.0f)
            {
                print.pitchHz *= (float) multiple;
                break;
            }
        }
    }
    return print;
}

float SoundPrint::distance (const SoundPrint& a, const SoundPrint& b)
{
    // Spectral shape matters most. Quiet top bands are where "air" lives, so
    // from 2 kHz up they count down to -60 dB of the total; lower bands under
    // -40 dB are rumble and count as -40. Below the floor in both: ignored.
    double shape = 0.0;
    int counted = 0;
    for (int band = 0; band < numBands; ++band)
    {
        const auto x = a.bandsDb[(size_t) band], y = b.bandsDb[(size_t) band];
        const auto floor = band >= 6 ? -60.0f : -40.0f; // 2 kHz and up: quiet is air; below, rumble
        if (x < floor && y < floor)
            continue;
        const auto d = (juce::jmax (x, floor) - juce::jmax (y, floor)) / 6.0f;
        shape += d * d;
        ++counted;
    }
    shape = counted > 0 ? shape / counted : 0.0;

    // Brightness: the spectral centroid (half an octave apart counts like two
    // bands 6 dB off).
    double brightness = 0.0;
    if (a.centroidHz > 0.0f && b.centroidHz > 0.0f)
        brightness = std::pow (std::log2 (a.centroidHz / b.centroidHz) / 0.5, 2.0);

    // Movement: the level after the peak (6 dB apart counts like a band), and
    // the attack on a log scale (twice as slow counts about as much). Points a
    // loop's next note hid are skipped.
    double envelope = 0.0;
    int points = 0;
    for (int p = 0; p < numEnvelopePoints; ++p)
    {
        if (a.envelopeDb[(size_t) p] <= unknown || b.envelopeDb[(size_t) p] <= unknown)
            continue;
        const auto d = (juce::jmax (a.envelopeDb[(size_t) p], -40.0f) - juce::jmax (b.envelopeDb[(size_t) p], -40.0f)) / 6.0f;
        envelope += d * d;
        ++points;
    }
    envelope = points > 0 ? envelope / points : 0.0;
    const auto attack = std::log2 ((a.attackMs + 10.0f) / (b.attackMs + 10.0f));

    // Wobble: its depth (2 dB apart counts like a band), and when both wobble,
    // its rate (a wobble 40% too fast counts about like a band 6 dB off).
    const auto depth = (a.moveDepthDb - b.moveDepthDb) / 2.0f;
    auto movement = 0.5 * depth * depth;
    if (a.moveRateHz > 0.0f && b.moveRateHz > 0.0f)
        movement += 4.0f * std::pow (std::log2 (a.moveRateHz / b.moveRateHz), 2.0f);

    const auto width = (a.sidePct - b.sidePct) / 15.0f;
    const auto density = (a.crestDb - b.crestDb) / 4.0f;
    return (float) (shape + brightness + 0.5 * envelope + 0.3 * attack * attack + movement + 0.5 * width * width + 0.5 * density * density);
}
