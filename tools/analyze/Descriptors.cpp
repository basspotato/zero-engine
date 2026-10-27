#include "Descriptors.h"
#include <juce_dsp/juce_dsp.h>

namespace
{
    struct Biquad
    {
        double b0, b1, b2, a1, a2;
        double z1 = 0, z2 = 0;

        double process (double x)
        {
            const auto y = b0 * x + z1;
            z1 = b1 * x - a1 * y + z2;
            z2 = b2 * x - a2 * y;
            return y;
        }
    };

    // BS.1770 K-weighting at any sample rate (pre-filter shelf, then RLB high-pass),
    // using the analogue prototypes behind the standard's 48 kHz coefficients.
    std::pair<Biquad, Biquad> kWeighting (double fs)
    {
        Biquad shelf {};
        {
            const auto f0 = 1681.974450955533, gainDb = 3.999843853973347, q = 0.7071752369554196;
            const auto k = std::tan (juce::MathConstants<double>::pi * f0 / fs);
            const auto vh = std::pow (10.0, gainDb / 20.0);
            const auto vb = std::pow (vh, 0.4996667741545416);
            const auto a0 = 1.0 + k / q + k * k;
            shelf.b0 = (vh + vb * k / q + k * k) / a0;
            shelf.b1 = 2.0 * (k * k - vh) / a0;
            shelf.b2 = (vh - vb * k / q + k * k) / a0;
            shelf.a1 = 2.0 * (k * k - 1.0) / a0;
            shelf.a2 = (1.0 - k / q + k * k) / a0;
        }

        Biquad highPass {};
        {
            const auto f0 = 38.13547087602444, q = 0.5003270373238773;
            const auto k = std::tan (juce::MathConstants<double>::pi * f0 / fs);
            const auto a0 = 1.0 + k / q + k * k;
            highPass.b0 = 1.0; highPass.b1 = -2.0; highPass.b2 = 1.0;
            highPass.a1 = 2.0 * (k * k - 1.0) / a0;
            highPass.a2 = (1.0 - k / q + k * k) / a0;
        }

        return { shelf, highPass };
    }

    double integratedLoudness (const juce::AudioBuffer<float>& audio, double fs)
    {
        const auto numCh = juce::jmin (2, audio.getNumChannels());
        const auto n = audio.getNumSamples();

        // K-weighted squared signal, summed over channels (L and R weigh 1.0).
        std::vector<double> power ((size_t) n, 0.0);
        for (int ch = 0; ch < numCh; ++ch)
        {
            auto [shelf, highPass] = kWeighting (fs);
            const auto* data = audio.getReadPointer (ch);
            for (int i = 0; i < n; ++i)
            {
                const auto y = highPass.process (shelf.process (data[i]));
                power[(size_t) i] += y * y;
            }
        }

        // A mono file plays from both speakers; BS.1770 would count it once, but
        // matching a stereo render needs it counted as L+R.
        if (numCh == 1)
            for (auto& p : power)
                p *= 2.0;

        // 400 ms blocks, 75% overlap; a short one-shot becomes one block.
        const auto blockLen = juce::jmin (n, (int) std::round (0.4 * fs));
        const auto hop = juce::jmax (1, (int) std::round (0.1 * fs));
        if (blockLen <= 0)
            return -100.0;

        std::vector<double> blocks;
        for (int start = 0; start + blockLen <= n; start += hop)
        {
            double sum = 0.0;
            for (int i = start; i < start + blockLen; ++i)
                sum += power[(size_t) i];
            blocks.push_back (sum / blockLen);
        }

        auto toLufs = [] (double z) { return -0.691 + 10.0 * std::log10 (juce::jmax (z, 1.0e-20)); };

        auto gatedMean = [&] (double thresholdLufs)
        {
            double sum = 0.0;
            int count = 0;
            for (auto z : blocks)
                if (toLufs (z) > thresholdLufs)
                {
                    sum += z;
                    ++count;
                }
            return count > 0 ? sum / count : 0.0;
        };

        const auto absoluteGated = gatedMean (-70.0);
        if (absoluteGated <= 0.0)
            return -100.0;

        const auto relativeGate = toLufs (absoluteGated) - 10.0;
        const auto gated = gatedMean (relativeGate);
        return gated > 0.0 ? toLufs (gated) : -100.0;
    }
}

Descriptors analyse (const juce::AudioBuffer<float>& audio, double fs)
{
    Descriptors d;
    const auto n = audio.getNumSamples();
    const auto stereo = audio.getNumChannels() > 1;
    const auto* left = audio.getReadPointer (0);
    const auto* right = audio.getReadPointer (stereo ? 1 : 0);

    d.seconds = n / fs;
    d.lufs = integratedLoudness (audio, fs);

    float peak = 0.0f;
    for (int ch = 0; ch < audio.getNumChannels(); ++ch)
        peak = juce::jmax (peak, audio.getMagnitude (ch, 0, n));
    d.peakDb = juce::Decibels::gainToDecibels ((double) peak, -100.0);
    d.plr = d.lufs > -99.0 ? d.peakDb - d.lufs : 0.0;

    // Spectrum and width over the active frames only, so silence and long
    // quiet tails don't drag the averages.
    constexpr int order = 12, size = 1 << order, hop = size / 4;
    juce::dsp::FFT fft (order);
    juce::dsp::WindowingFunction<float> window ((size_t) size, juce::dsp::WindowingFunction<float>::hann, false);
    std::vector<float> frame ((size_t) size * 2);
    std::vector<double> spectrum ((size_t) size / 2 + 1, 0.0);

    // Frame RMS first, to find the loudest one.
    std::vector<double> frameRms;
    for (int start = 0; start + size <= juce::jmax (n, size); start += hop)
    {
        double sum = 0.0;
        for (int i = start; i < juce::jmin (n, start + size); ++i)
        {
            const auto m = 0.5 * (left[i] + right[i]);
            sum += m * m;
        }
        frameRms.push_back (std::sqrt (sum / size));
    }

    const auto loudest = frameRms.empty() ? 0.0 : *std::max_element (frameRms.begin(), frameRms.end());
    const auto activeThreshold = juce::jmax (loudest * 0.01, 3.0e-4); // within 40 dB of the loudest, above -70 dBFS

    double midEnergy = 0.0, sideEnergy = 0.0, lr = 0.0, ll = 0.0, rr = 0.0;

    for (size_t f = 0; f < frameRms.size(); ++f)
    {
        if (frameRms[f] < activeThreshold)
            continue;

        const auto start = (int) f * hop;
        std::fill (frame.begin(), frame.end(), 0.0f);
        for (int i = 0; i < size && start + i < n; ++i)
        {
            const auto l = (double) left[start + i], r = (double) right[start + i];
            const auto m = 0.5 * (l + r), s = 0.5 * (l - r);
            frame[(size_t) i] = (float) m;

            // Width uses each sample once (the non-overlapping first hop of the frame).
            if (i < hop)
            {
                midEnergy += m * m; sideEnergy += s * s;
                lr += l * r; ll += l * l; rr += r * r;
            }
        }

        window.multiplyWithWindowingTable (frame.data(), (size_t) size);
        fft.performFrequencyOnlyForwardTransform (frame.data(), true);
        for (size_t b = 0; b < spectrum.size(); ++b)
            spectrum[b] += (double) frame[b] * frame[b];
    }

    double total = 0.0, weighted = 0.0, high = 0.0, bass = 0.0, sub = 0.0;
    for (size_t b = 1; b < spectrum.size(); ++b)
    {
        const auto hz = b * fs / size;
        if (hz < 20.0 || hz > 20000.0)
            continue;

        const auto p = spectrum[b];
        total += p;
        weighted += p * hz;
        if (hz > 4000.0) high += p;
        if (hz < 150.0)  bass += p;
        if (hz < 60.0)   sub += p;
    }

    if (total > 0.0)
    {
        d.centroidHz = weighted / total;
        d.highPct = 100.0 * high / total;
        d.bassPct = 100.0 * bass / total;
        d.subPct = 100.0 * sub / total;
    }

    if (midEnergy + sideEnergy > 0.0)
        d.sidePct = 100.0 * sideEnergy / (midEnergy + sideEnergy);
    if (ll > 0.0 && rr > 0.0)
        d.correlation = lr / std::sqrt (ll * rr);

    return d;
}
