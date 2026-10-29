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


    double melOf (double hz) { return 2595.0 * std::log10 (1.0 + hz / 700.0); }
    double hzOfMel (double m) { return 700.0 * (std::pow (10.0, m / 2595.0) - 1.0); }

    // The centre of mel band b, on the one scale every sound shares (40 Hz to 16 kHz).
    double bandCentreHz (int b)
    {
        const auto lo = melOf (40.0), hi = melOf (16000.0);
        return hzOfMel (lo + (hi - lo) * (b + 1) / (SoundPrint::melBands + 1));
    }

    // How many bands (from the bottom) both sounds can hold: a 16 kHz recording
    // has nothing above 7.2 kHz, so the bands above it are left out of the comparison.
    int sharedBands (const SoundPrint& a, const SoundPrint& b)
    {
        const auto top = juce::jmin (a.maxHz, b.maxHz);
        int n = 0;
        while (n < SoundPrint::melBands && bandCentreHz (n) < top)
            ++n;
        return juce::jmax (1, n);
    }

    // Log-mel frames of a mono signal: 40 bands from 40 Hz to 16 kHz whatever the
    // sample rate (bands above a low rate's Nyquist stay empty), in dB under the
    // loudest cell, floor -80. Zero-padded so even a short note gets a frame.
    std::vector<std::array<float, SoundPrint::melBands>> melFrames (const std::vector<float>& x, double sampleRate, int order, int hop)
    {
        constexpr auto bands = SoundPrint::melBands;
        const auto size = 1 << order;
        auto mel = [] (double hz) { return melOf (hz); };
        auto hzOf = [] (double m) { return hzOfMel (m); };
        const auto lo = mel (40.0), hi = mel (16000.0);
        std::vector<std::array<float, bands>> weights ((size_t) size / 2);
        for (int b = 0; b < bands; ++b)
        {
            const auto left = hzOf (lo + (hi - lo) * b / (bands + 1)), centre = hzOf (lo + (hi - lo) * (b + 1) / (bands + 1)),
                       right = hzOf (lo + (hi - lo) * (b + 2) / (bands + 1));
            for (int bin = 1; bin < size / 2; ++bin)
            {
                const auto hz = bin * sampleRate / size;
                weights[(size_t) bin][(size_t) b] = (float) juce::jmax (0.0, hz < centre ? (hz - left) / (centre - left) : (right - hz) / (right - centre));
            }
        }

        juce::dsp::FFT fft (order);
        std::vector<float> frame ((size_t) size * 2);
        std::vector<std::array<float, bands>> out;
        float loudest = -200.0f;
        for (int start = 0; start == 0 || start + size / 2 <= (int) x.size(); start += hop)
        {
            std::fill (frame.begin(), frame.end(), 0.0f);
            for (int i = 0; i < size && start + i < (int) x.size(); ++i)
                frame[(size_t) i] = x[(size_t) (start + i)] * (0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * (float) i / (float) size));
            fft.performFrequencyOnlyForwardTransform (frame.data());

            std::array<float, bands> cell {};
            for (int b = 0; b < bands; ++b)
            {
                double power = 0.0;
                for (int bin = 1; bin < size / 2; ++bin)
                    if (const auto w = weights[(size_t) bin][(size_t) b]; w > 0.0f)
                        power += w * frame[(size_t) bin] * frame[(size_t) bin];
                cell[(size_t) b] = (float) (10.0 * std::log10 (power + 1.0e-12));
                loudest = juce::jmax (loudest, cell[(size_t) b]);
            }
            out.push_back (cell);
        }
        for (auto& cell : out)
            for (auto& v : cell)
                v = juce::jmax (-80.0f, v - loudest);
        return out;
    }

    // Mean dB difference of two spectrograms over the frames both have and the
    // lowest `bands` bands (cells under -70 dB in both skipped).
    float melDistance (bool overLonger, int bands, const std::vector<std::array<float, SoundPrint::melBands>>& a,
                       const std::vector<std::array<float, SoundPrint::melBands>>& b)
    {
        double sum = 0.0;
        int n = 0;
        const auto frames = overLonger ? juce::jmax (a.size(), b.size()) : juce::jmin (a.size(), b.size());
        for (size_t f = 0; f < frames; ++f)
            for (size_t band = 0; band < (size_t) bands; ++band)
            {
                const auto x = f < a.size() ? a[f][band] : -80.0f, y = f < b.size() ? b[f][band] : -80.0f; // past the end: silence
                if (x > -70.0f || y > -70.0f)
                {
                    sum += std::abs (x - y);
                    ++n;
                }
            }
        return n > 0 ? (float) (sum / n) : 0.0f;
    }

    // Per-frame perceptual features of a log-mel frame (dB): its timbre as MFCCs
    // 1-13 (orthonormal DCT-II of the band levels, so the overall level, c0, is
    // left out), its brightness as the centroid in mel bands, and its loudness.
    struct FrameFeatures { std::array<float, 13> mfcc {}; float centroid = 0.0f, loudnessDb = -80.0f; };

    FrameFeatures featuresOf (const std::array<float, SoundPrint::melBands>& frame, int bands)
    {
        FrameFeatures f;
        for (int k = 1; k <= 13; ++k)
        {
            double sum = 0.0;
            for (int b = 0; b < bands; ++b)
                sum += frame[(size_t) b] * std::cos (juce::MathConstants<double>::pi * k * (b + 0.5) / bands);
            f.mfcc[(size_t) k - 1] = (float) (sum * std::sqrt (2.0 / bands));
        }
        double power = 0.0, weighted = 0.0;
        for (int b = 0; b < bands; ++b)
        {
            const auto p = std::pow (10.0, frame[(size_t) b] / 10.0);
            power += p;
            weighted += b * p;
        }
        f.loudnessDb = (float) (10.0 * std::log10 (power + 1.0e-12));
        f.centroid = power > 0.0 ? (float) (weighted / power) : 0.0f;
        return f;
    }

    // The composite terms over the frames (silence past an end, when comparing
    // whole sounds): mean MFCC difference (dB), centroid difference (mel bands)
    // over frames where both sound, and loudness difference (dB).
    struct Composite { float mfcc = 0.0f, centroid = 0.0f, loudness = 0.0f; };

    Composite compositeDistance (bool overLonger, int bands, const std::vector<std::array<float, SoundPrint::melBands>>& a,
                                 const std::vector<std::array<float, SoundPrint::melBands>>& b)
    {
        std::array<float, SoundPrint::melBands> silence;
        silence.fill (-80.0f);
        const auto frames = overLonger ? juce::jmax (a.size(), b.size()) : juce::jmin (a.size(), b.size());
        double mfcc = 0.0, centroid = 0.0, loudness = 0.0;
        int n = 0, sounding = 0;
        for (size_t i = 0; i < frames; ++i)
        {
            const auto x = featuresOf (i < a.size() ? a[i] : silence, bands), y = featuresOf (i < b.size() ? b[i] : silence, bands);
            if (x.loudnessDb < -60.0f && y.loudnessDb < -60.0f)
                continue;
            double m = 0.0;
            for (size_t k = 0; k < x.mfcc.size(); ++k)
                m += std::abs (x.mfcc[k] - y.mfcc[k]);
            mfcc += m / (double) x.mfcc.size();
            loudness += std::abs (juce::jmax (-60.0f, x.loudnessDb) - juce::jmax (-60.0f, y.loudnessDb));
            ++n;
            if (x.loudnessDb > -60.0f && y.loudnessDb > -60.0f)
            {
                centroid += std::abs (x.centroid - y.centroid);
                ++sounding;
            }
        }
        Composite c;
        if (n > 0)
        {
            c.mfcc = (float) (mfcc / n);
            c.loudness = (float) (loudness / n);
        }
        if (sounding > 0)
            c.centroid = (float) (centroid / sounding);
        return c;
    }

    // A repeating wobble in a series of 10 ms dB values: the series minus its
    // straight-line trend (the decay), its depth (RMS, dB) and its rate (Hz) by
    // autocorrelation over 1 to 12 Hz, or 0 when nothing repeats clearly.
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

        std::vector<double> r (101, 0.0);
        for (int lag = 1; lag <= 100 && lag < n / 2; ++lag)
        {
            for (int i = 0; i + lag < n; ++i)
                r[(size_t) lag] += series[(size_t) i] * series[(size_t) (i + lag)];
            r[(size_t) lag] /= power * (n - lag) / n;
        }
        for (int lag = 8; lag < 100 && lag + 1 < n / 2; ++lag)
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

    // Measured at 48 kHz, the rate candidates render at: frames are counted in
    // samples, so a 44.1 kHz reference's frames would otherwise drift 9 % against
    // a render's over the phrase. Bands its own rate can't hold stay excluded.
    constexpr double measureRate = 48000.0;
    if (std::abs (sampleRate - measureRate) > 1.0)
    {
        const auto ratio = sampleRate / measureRate;
        const auto length = (int) std::floor (total / ratio);
        juce::AudioBuffer<float> resampled (channels, juce::jmax (1, length));
        for (int ch = 0; ch < channels; ++ch)
        {
            juce::WindowedSincInterpolator interpolator;
            interpolator.process (ratio, audio.getReadPointer (ch), resampled.getWritePointer (ch), length, total, 0);
        }
        auto measured = of (resampled, measureRate, findPitch);
        measured.maxHz = juce::jmin (measured.maxHz, (float) (0.45 * sampleRate));
        return measured;
    }

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

    // Energy per 10 ms; the onset is the first block within 30 dB of the loudest.
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
    if (overall <= 1.0e-12)
        return print; // silent: nothing to measure
    auto onset = (int) (std::find_if (energy.begin(), energy.end(), [&] (double e) { return e > overall * 1.0e-3; }) - energy.begin());
    // A loop or phrase is split into notes (references only; an engine render
    // is always taken whole). A note starts where the spectrum gains energy all
    // of a sudden (positive spectral flux, a peak well over its neighbourhood,
    // at least 60 ms after the last), which finds arp notes even when one rings
    // into the next. Each note's pitch is the median YIN reading over the first
    // part of it, before the next note's bleed.
    const auto pitchFrame = (int) (0.06 * sampleRate);
    auto pitchAt = [&] (int b)
    {
        std::vector<float> slice ((size_t) pitchFrame);
        for (int i = 0; i < pitchFrame; ++i)
            slice[(size_t) i] = b * block + i >= 0 && b * block + i < total ? 0.5f * (sample (0, b * block + i) + sample (1, b * block + i)) : 0.0f;
        return pitchOf (slice, sampleRate);
    };

    auto last = juce::jmin ((int) energy.size(), onset + (int) (span * 100.0));
    bool cut = false; // the first note ended by the next (local: bounds the wobble measurement)
    float notePitch = 0.0f;
    if (findPitch)
    {
        const auto limit = juce::jmin ((int) energy.size(), onset + 200);
        std::vector<int> starts { onset };

        constexpr int order = 10, size = 1 << order, hop = 256;
        juce::dsp::FFT fft (order);
        std::vector<float> frame ((size_t) size * 2), previous ((size_t) size / 2, 0.0f);
        std::vector<float> flux;
        const auto from = onset * block;
        for (int start = from; start + size <= juce::jmin (total, limit * block); start += hop)
        {
            std::fill (frame.begin(), frame.end(), 0.0f);
            for (int i = 0; i < size; ++i)
                frame[(size_t) i] = 0.5f * (sample (0, start + i) + sample (1, start + i))
                                  * (0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * (float) i / (float) size));
            fft.performFrequencyOnlyForwardTransform (frame.data());
            float rise = 0.0f;
            for (int bin = 1; bin < size / 2; ++bin)
            {
                const auto level = std::log1p (100.0f * frame[(size_t) bin]);
                rise += juce::jmax (0.0f, level - previous[(size_t) bin]);
                previous[(size_t) bin] = level;
            }
            flux.push_back (rise);
        }

        const auto biggest = flux.empty() ? 0.0f : *std::max_element (flux.begin(), flux.end());
        const auto spacing = (int) (0.06 * sampleRate / hop);
        int lastPeak = 0;
        for (int n = 1; n + 1 < (int) flux.size(); ++n)
        {
            std::vector<float> around;
            for (int k = juce::jmax (0, n - 10); k <= juce::jmin ((int) flux.size() - 1, n + 10); ++k)
                around.push_back (flux[(size_t) k]);
            std::nth_element (around.begin(), around.begin() + (long) around.size() / 2, around.end());
            const auto isPeak = flux[(size_t) n] >= flux[(size_t) n - 1] && flux[(size_t) n] > flux[(size_t) n + 1]
                             && flux[(size_t) n] > around[around.size() / 2] + 0.15f * biggest;
            if (isPeak && n - lastPeak >= spacing)
            {
                // The frame's window ends where the new energy arrived.
                const auto at = (from + n * hop + size / 2) / block;
                if (at > starts.back() + 5 && at < limit)
                    starts.push_back (at);
                lastPeak = n;
            }
        }

        // A note's pitch from the energy it brings: the spectrum just after its
        // start minus the one just before (the last note still ringing cancels
        // out), then the fundamental whose harmonics carry most of that new
        // energy (weighted 0.8 per harmonic, so an octave below, which also
        // collects the even ones, scores lower). YIN refines it when it agrees.
        constexpr int pitchOrder = 12, pitchSize = 1 << pitchOrder;
        juce::dsp::FFT pitchFft (pitchOrder);
        auto spectrumAt = [&] (int startSample)
        {
            std::vector<float> s ((size_t) pitchSize * 2, 0.0f);
            for (int i = 0; i < pitchSize; ++i)
                if (const auto at = startSample + i; at >= 0 && at < total)
                    s[(size_t) i] = 0.5f * (sample (0, at) + sample (1, at))
                                  * (0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * (float) i / (float) pitchSize));
            pitchFft.performFrequencyOnlyForwardTransform (s.data());
            s.resize ((size_t) pitchSize / 2);
            return s;
        };
        auto newNotePitch = [&] (int beginBlock, int endBlock)
        {
            const auto after = spectrumAt (beginBlock * block + block);
            const auto before = spectrumAt (beginBlock * block - pitchSize);
            const auto binHz = sampleRate / pitchSize;
            std::vector<float> arrived (after.size());
            for (size_t b = 0; b < after.size(); ++b)
                arrived[b] = juce::jmax (0.0f, after[b] - (beginBlock == onset ? 0.0f : before[b]));

            auto energyNear = [&] (double hz)
            {
                const auto centre = (int) std::round (hz / binHz);
                auto best = 0.0f;
                for (int b = centre - 1; b <= centre + 1; ++b)
                    if (b > 0 && b < (int) arrived.size())
                        best = juce::jmax (best, arrived[(size_t) b]);
                return best;
            };
            auto strongest = [&] (double& bestHz)
            {
                double bestScore = 0.0;
                for (double hz = 50.0; hz <= 1000.0; hz *= std::exp2 (1.0 / 48.0))
                {
                    double score = 0.0, weight = 1.0;
                    for (int h = 1; h <= 8 && h * hz < 0.45 * sampleRate; ++h, weight *= 0.8)
                        score += weight * energyNear (h * hz);
                    if (score > bestScore)
                    {
                        bestScore = score;
                        bestHz = hz;
                    }
                }
                return bestScore;
            };
            double bestHz = 0.0;
            const auto bestScore = strongest (bestHz);

            // A chord: take the found note's harmonics out and look again; a
            // pitch that still carries half as much is another note of it.
            std::vector<float> also;
            auto removeHarmonics = [&] (double hz)
            {
                for (int h = 1; h <= 16 && h * hz < 0.45 * sampleRate; ++h)
                    for (int b = (int) std::round (h * hz / binHz) - 2; b <= (int) std::round (h * hz / binHz) + 2; ++b)
                        if (b > 0 && b < (int) arrived.size())
                            arrived[(size_t) b] = 0.0f;
            };
            removeHarmonics (bestHz);
            for (int tries = 0; tries < 6 && also.size() < 3 && bestScore > 0.0; ++tries)
            {
                double hz = 0.0;
                if (strongest (hz) < 0.5 * bestScore)
                    break;
                removeHarmonics (hz);
                // Under 1.5 semitones from a found note: its detuned unison, not a note.
                auto apart = std::abs (12.0 * std::log2 (hz / bestHz)) >= 1.5;
                for (const auto other : also)
                    apart = apart && std::abs (12.0 * std::log2 (hz / other)) >= 1.5;
                if (apart)
                    also.push_back ((float) hz);
            }

            std::vector<float> readings;
            for (int b = beginBlock + 2; b <= beginBlock + juce::jmax (3, (int) (0.6 * (endBlock - beginBlock))) && b < endBlock; b += 2)
                if (const auto p = pitchAt (b); p > 0.0f && std::abs (12.0 * std::log2 (p / bestHz)) < 1.0)
                    readings.push_back (p);
            auto pitch = (float) bestHz;
            if (! readings.empty())
            {
                std::nth_element (readings.begin(), readings.begin() + (long) readings.size() / 2, readings.end());
                pitch = readings[readings.size() / 2];
            }
            return std::make_pair (pitch, also);
        };

        std::vector<std::vector<float>> shapes; // per note, for telling echoes from notes
        for (size_t i = 0; i < starts.size() && i < 24; ++i)
        {
            const auto begin = starts[i], end = i + 1 < starts.size() ? starts[i + 1] : limit;
            const auto [pitch, also] = newNotePitch (begin, end);
            print.notes.push_back ({ 0.01f * (float) (begin - onset), 0.01f * (float) (end - begin), pitch, also });
            auto shape = spectrumAt (begin * block + block); // the note just after it starts, up to 5 kHz, unit length
            shape.resize ((size_t) (5000.0 / (sampleRate / pitchSize)));
            const auto norm = std::sqrt (std::inner_product (shape.begin(), shape.end(), shape.begin(), 0.0));
            for (auto& v : shape)
                v = (float) (v / juce::jmax (1.0e-12, norm));
            shapes.push_back (std::move (shape));
        }

        // An echo is not a note: one at the last note's pitch (or an octave of
        // it, as its arrived energy can read) and over 6 dB quieter is the delay
        // repeating it, so it joins the note before; so does anything over 9 dB
        // under the loudest note (an echo of an earlier one, a reverb swell), and
        // anything with the last note's spectrum but 4 dB quieter (its echo, or
        // a wobble opening the filter again), and the same pitch again with no
        // dip between (a held note).
        auto loudestIn = [&] (const Note& n)
        {
            const auto from = onset + juce::roundToInt (n.start * 100.0f), to = from + juce::jmax (1, juce::roundToInt (n.length * 100.0f));
            double loudest = 0.0;
            for (int b = from; b < juce::jmin (to, (int) energy.size()); ++b)
                loudest = juce::jmax (loudest, energy[(size_t) b]);
            return loudest;
        };
        // Whether the level fell 6 dB under the note before's loudest in the 50 ms
        // before this one: a re-played note has a gap or a decay first; a held
        // note whose wobble reads as an onset does not.
        auto dipBefore = [&] (const Note& n, const Note& before)
        {
            const auto at = onset + juce::roundToInt (n.start * 100.0f);
            auto quietest = 1.0e30;
            for (int b = juce::jmax (onset, at - 5); b < at && b < (int) energy.size(); ++b)
                quietest = juce::jmin (quietest, energy[(size_t) b]);
            return quietest < 0.25 * loudestIn (before);
        };
        // Whether the pitch just before an onset is the pitch just after (plain
        // YIN readings): no new note, whatever the arrived energy reads as.
        auto pitchHolds = [&] (const Note& n)
        {
            const auto at = onset + juce::roundToInt (n.start * 100.0f);
            const auto x = pitchAt (at - 7), y = pitchAt (at + 2);
            return x > 0.0f && y > 0.0f && std::abs (12.0f * std::log2 (x / y)) < 0.7f;
        };
        double loudestNote = 0.0;
        for (const auto& n : print.notes)
            loudestNote = juce::jmax (loudestNote, loudestIn (n));
        for (size_t i = 1; i < print.notes.size();)
        {
            auto& before = print.notes[i - 1];
            const auto& n = print.notes[i];
            const auto semitones = n.pitchHz > 0.0f && before.pitchHz > 0.0f ? std::abs (12.0f * std::log2 (n.pitchHz / before.pitchHz)) : 99.0f;
            const auto sameNote = std::abs (semitones - 12.0f * std::round (semitones / 12.0f)) < 0.7f && semitones < 25.0f;
            const auto similar = std::inner_product (shapes[i].begin(), shapes[i].end(), shapes[i - 1].begin(), 0.0) > 0.9;
            if ((sameNote && loudestIn (n) < 0.25 * loudestIn (before)) || loudestIn (n) < loudestNote / 8.0
                || (similar && loudestIn (n) < 0.63 * loudestIn (before)) || (! dipBefore (n, before) && (sameNote || pitchHolds (n))))
            {
                before.length = n.start + n.length - before.start;
                print.notes.erase (print.notes.begin() + (long) i);
                shapes.erase (shapes.begin() + (long) i);
            }
            else
                ++i;
        }
        // A loop that opens quietly (the last bar's tail wrapped round, a
        // pickup) starts at its first real note: over 9 dB under the loudest note
        // is not one, and candidates can't play it quietly enough.
        while (print.notes.size() > 1 && loudestIn (print.notes.front()) < loudestNote / 8.0)
        {
            const auto skip = print.notes[1].start;
            print.notes.erase (print.notes.begin());
            shapes.erase (shapes.begin());
            for (auto& n : print.notes)
                n.start -= skip;
            onset += juce::roundToInt (skip * 100.0f);
        }
        starts.clear();
        for (const auto& n : print.notes)
            starts.push_back (onset + juce::roundToInt (n.start * 100.0f));

        notePitch = print.notes.front().pitchHz;
        last = juce::jmin ((int) energy.size(), onset + (int) (span * 100.0)); // the onset may have moved
        if (starts.size() > 1)
        {
            last = starts[1];
            cut = true;
        }
    }
    last = juce::jmax (last, juce::jmin ((int) energy.size(), onset + 1));
    const auto peakBlock = (int) (std::max_element (energy.begin() + onset, energy.begin() + last) - energy.begin());
    const auto peakEnergy = juce::jmax (1.0e-12, energy[(size_t) peakBlock]);

    // The sound over time: every note of a phrase and the release after, from
    // the onset to where it dies away (up to 2 s; a longer sound is cut there).
    // Candidates play the same notes, so the whole of it is compared.
    auto end = juce::jmin ((int) energy.size(), onset + 200);
    // Still sounding at the window's end: the reference goes on past what is
    // measured, so candidates are compared only that far (cut).
    print.cut = energy[(size_t) end - 1] >= overall * 1.0e-5; // or the file ends mid-tail
    while (end > onset + 1 && energy[(size_t) end - 1] < overall * 1.0e-5)
        --end;
    end = juce::jmin ((int) energy.size(), end + 3);
    print.noteSeconds = 0.01f * (float) (end - onset);
    {
        std::vector<float> note ((size_t) juce::jmax (0, juce::jmin (total, end * block) - onset * block));
        for (size_t i = 0; i < note.size(); ++i)
            note[i] = 0.5f * (sample (0, onset * block + (int) i) + sample (1, onset * block + (int) i));
        print.melTone = melFrames (note, sampleRate, 11, 512);
        print.melAttack = melFrames (note, sampleRate, 9, 128);
    }

    // Movement after the attack settles, over the first note (from 100 ms past
    // its loudest moment, while within 30 dB of it): level and brightness
    // wobbles; whichever is deeper speaks for the sound.
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

    // Width and level.
    std::vector<float> mono ((size_t) length);
    double mid2 = 0.0, side2 = 0.0;
    for (int i = 0; i < length; ++i)
    {
        const auto l = sample (0, from + i), r = sample (1, from + i);
        mono[(size_t) i] = 0.5f * (l + r);
        mid2 += 0.25 * (l + r) * (l + r);
        side2 += 0.25 * (l - r) * (l - r);
    }
    const auto rms = std::sqrt ((mid2 + side2) / length);
    print.levelDb = (float) juce::Decibels::gainToDecibels (rms, -100.0);
    print.sidePct = (float) (100.0 * side2 / juce::jmax (1.0e-12, mid2 + side2));
    print.maxHz = (float) (0.45 * sampleRate);
    print.pitchHz = findPitch ? (notePitch > 0.0f ? notePitch : pitchOf (mono, sampleRate)) : 0.0f;

    // Octave bands (for checking the pitch against the spectrum): Hann-windowed
    // frames, half overlapping.
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

    // The first note's harmonics, at the pitch it is played back at, 30 ms to
    // 500 ms in (within the note): each harmonic's peak within 3 % of where it
    // should be, 4096-point frames, scaled to the frame's loudest.
    if (findPitch && ! print.notes.empty() && print.notes.front().pitchHz > 0.0f)
    {
        const auto f0 = (double) print.notes.front().pitchHz;
        constexpr int order = 12, size = 1 << order;
        juce::dsp::FFT harmonicFft (order);
        std::vector<float> cycle ((size_t) size * 2);
        const auto noteFrom = onset * block, noteTo = juce::jmin (total, last * block);
        constexpr double moments[WavetableBank::numFrames] { 0.03, 0.1, 0.2, 0.35, 0.5 };
        WavetableBank::Harmonics h {};
        auto found = false;
        for (int m = 0; m < WavetableBank::numFrames; ++m)
        {
            const auto at = juce::jmax (0, juce::jmin (noteFrom + (int) (moments[m] * sampleRate), noteTo - size));
            std::fill (cycle.begin(), cycle.end(), 0.0f);
            for (int i = 0; i < size && at + i < total; ++i)
                cycle[(size_t) i] = 0.5f * (sample (0, at + i) + sample (1, at + i))
                                  * (0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * (float) i / (float) size));
            harmonicFft.performFrequencyOnlyForwardTransform (cycle.data());
            const auto binHz = sampleRate / size;
            float strongest = 0.0f;
            for (int n = 1; n <= WavetableBank::numSampleHarmonics && n * f0 < 0.45 * sampleRate && n * f0 < print.maxHz; ++n)
            {
                const auto centre = n * f0 / binHz, reach = juce::jmax (1.0, 0.03 * centre);
                float peak = 0.0f;
                for (auto b = (int) std::floor (centre - reach); b <= (int) std::ceil (centre + reach); ++b)
                    if (b > 0 && b < size / 2)
                        peak = juce::jmax (peak, cycle[(size_t) b]);
                h[(size_t) m][(size_t) n - 1] = peak;
                strongest = juce::jmax (strongest, peak);
            }
            if (strongest > 0.0f)
            {
                found = true;
                for (auto& a : h[(size_t) m])
                    a = a / strongest > 1.0e-3f ? a / strongest : 0.0f; // under -60 dB: nothing
            }
            else if (m > 0)
                h[(size_t) m] = h[(size_t) m - 1]; // the note has gone quiet: hold the last shape
        }
        if (found)
            print.harmonics = h;
    }
    return print;
}

float SoundPrint::distance (const SoundPrint& a, const SoundPrint& b, juce::String* explain)
{
    // Spectrograms are normalised to their own loudest cell, so a near-silent
    // sound (a high-pass at 20 kHz, say) would show its noise floor as a full
    // spectrum: one 40 dB quieter than the other is simply not a match. Nor is
    // a sound too short or quiet to have been measured at all.
    if (std::abs (a.levelDb - b.levelDb) > 40.0f || a.melTone.empty() || b.melTone.empty())
        return 100.0f;

    // How the two evolve over the whole phrase: the mean dB difference of their
    // spectrograms, tone and attack resolution alike (4 dB off counts as 1);
    // then, as the composite loss research recommends, timbre (MFCCs),
    // brightness over time and the loudness envelope, each scaled so about one
    // unit means "clearly different"; and stereo width, which the mono
    // spectrograms can't see.
    const auto overLonger = ! a.cut && ! b.cut; // whole sounds: a tail one has and the other lacks counts
    const auto bands = sharedBands (a, b); // only the frequencies both can hold
    const auto mel = 0.5f * (melDistance (overLonger, bands, a.melTone, b.melTone) + melDistance (overLonger, bands, a.melAttack, b.melAttack)) / 4.0f;
    const auto c = compositeDistance (overLonger, bands, a.melTone, b.melTone);
    const auto width = (a.sidePct - b.sidePct) / 15.0f;
    if (explain != nullptr)
        *explain = juce::String::formatted ("mel %.2f mfcc %.2f centroid %.2f loudness %.2f width %.2f", mel, c.mfcc / 4.0f, c.centroid / 2.0f,
                                            c.loudness / 3.0f, 0.5f * width * width);
    return weights.mel * mel + weights.mfcc * c.mfcc / 4.0f + weights.centroid * c.centroid / 2.0f
         + weights.loudness * c.loudness / 3.0f + 0.5f * width * width;
}
