#include "Harmonics.h"
#include "Descriptors.h"
#include "../../Source/Wavetable.h"
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_dsp/juce_dsp.h>

namespace
{
    constexpr int fftOrder = 15;              // 32768-point FFT, zero-padded
    constexpr int fftSize = 1 << fftOrder;
    constexpr int maxHarmonics = WavetableBank::tableSize / 2 - 1;

    struct Frame
    {
        juce::String name;
        double f0 = 0, brightness = 0;
        std::vector<float> amplitude; // harmonic n at [n - 1], 0..1
    };

    // Harmonic amplitudes of one steady stretch of a recorded note.
    bool measure (const juce::File& file, Frame& frame)
    {
        juce::AudioFormatManager formats;
        formats.registerBasicFormats();
        std::unique_ptr<juce::AudioFormatReader> reader (formats.createReaderFor (file));
        if (reader == nullptr)
            return false;

        const auto sr = reader->sampleRate;
        const auto length = (int) juce::jmin<juce::int64> (reader->lengthInSamples, (juce::int64) (sr * 30.0));
        juce::AudioBuffer<float> audio ((int) juce::jmin (2u, reader->numChannels), length);
        reader->read (&audio, 0, length, 0, true, audio.getNumChannels() > 1);

        const auto pitch = analyse (audio, sr).pitchHz;
        if (pitch <= 0.0)
            return false;

        // Mono, then the loudest 10 ms block: the window starts just after it,
        // past the attack, where the note is steady.
        std::vector<float> mono ((size_t) length);
        for (int i = 0; i < length; ++i)
        {
            float s = 0.0f;
            for (int ch = 0; ch < audio.getNumChannels(); ++ch)
                s += audio.getSample (ch, i);
            mono[(size_t) i] = s / (float) audio.getNumChannels();
        }

        const auto block = (int) (sr * 0.01);
        int loudest = 0;
        double loudestEnergy = 0.0;
        for (int start = 0; start + block <= length; start += block)
        {
            double e = 0.0;
            for (int i = start; i < start + block; ++i)
                e += (double) mono[(size_t) i] * mono[(size_t) i];
            if (e > loudestEnergy) { loudestEnergy = e; loudest = start; }
        }

        const auto windowLength = juce::jmin (8192, fftSize);
        const auto start = juce::jmin (loudest + (int) (sr * 0.03), length - windowLength);
        if (start < 0)
            return false;

        std::vector<float> buffer ((size_t) fftSize * 2, 0.0f);
        for (int i = 0; i < windowLength; ++i)
        {
            const auto hann = 0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * (float) i / (float) (windowLength - 1));
            buffer[(size_t) i] = mono[(size_t) (start + i)] * hann;
        }

        juce::dsp::FFT fft (fftOrder);
        fft.performFrequencyOnlyForwardTransform (buffer.data());

        const auto binHz = sr / fftSize;
        auto peakNear = [&] (double hz, double halfWidthHz, double& foundHz)
        {
            const auto lo = juce::jmax (1, (int) ((hz - halfWidthHz) / binHz));
            const auto hi = juce::jmin (fftSize / 2 - 1, (int) ((hz + halfWidthHz) / binHz) + 1);
            int best = lo;
            for (int b = lo; b <= hi; ++b)
                if (buffer[(size_t) b] > buffer[(size_t) best])
                    best = b;
            foundHz = best * binHz;
            return buffer[(size_t) best];
        };

        // The analog pitch drifts: take this window's own fundamental.
        double f0 = pitch;
        peakNear (pitch, pitch * 0.05, f0);

        frame.name = file.getFileName();
        frame.f0 = f0;
        frame.amplitude.assign ((size_t) maxHarmonics, 0.0f);

        float top = 0.0f;
        for (int n = 1; n <= maxHarmonics && n * f0 < sr * 0.45; ++n)
        {
            double unused = 0.0;
            const auto a = peakNear (n * f0, f0 * 0.25, unused);
            frame.amplitude[(size_t) (n - 1)] = a;
            top = juce::jmax (top, a);
        }

        // Normalise; anything 70 dB down is the recording's noise, not the synth.
        double weighted = 0.0, total = 0.0;
        for (size_t i = 0; i < frame.amplitude.size(); ++i)
        {
            auto& a = frame.amplitude[i];
            a = top > 0.0f ? a / top : 0.0f;
            if (a < 0.0003f)
                a = 0.0f;
            weighted += (double) (i + 1) * a * a;
            total += (double) a * a;
        }
        frame.brightness = total > 0.0 ? weighted / total : 0.0;
        return true;
    }
}

int extractHarmonics (const juce::File& header, const juce::StringArray& files)
{
    if (files.size() != WavetableBank::numFrames)
    {
        std::printf ("Give exactly %d recordings (one per frame).\n", WavetableBank::numFrames);
        return 1;
    }

    std::vector<Frame> frames;
    for (const auto& path : files)
    {
        Frame f;
        if (! measure (juce::File (path), f))
        {
            std::printf ("Couldn't find a steady pitched note in %s\n", path.toRawUTF8());
            return 1;
        }
        frames.push_back (f);
    }

    // Frames run dark to bright, like the engine's other banks.
    std::sort (frames.begin(), frames.end(), [] (const Frame& a, const Frame& b) { return a.brightness < b.brightness; });

    juce::String s;
    s << "#pragma once\n\n"
      << "// Harmonic amplitudes measured from recordings of the owner's Roland\n"
      << "// Jupiter-8 by `ZeroAnalyze harmonics`. One frame per note, dark to bright;\n"
      << "// harmonic n at [frame][n - 1]. Written by that tool; don't edit by hand.\n\n"
      << "inline constexpr int jupiterHarmonicCount = " << maxHarmonics << ";\n\n"
      << "inline constexpr float jupiterHarmonics[" << (int) frames.size() << "][jupiterHarmonicCount] {\n";

    for (const auto& f : frames)
    {
        std::printf ("  %-24s f0 %6.1f Hz, brightness %5.1f\n", f.name.toRawUTF8(), f.f0, f.brightness);

        int last = maxHarmonics;
        while (last > 1 && f.amplitude[(size_t) (last - 1)] == 0.0f)
            --last;

        s << "    { // " << f.name << ", " << juce::String (f.f0, 1) << " Hz\n        ";
        for (int n = 0; n < last; ++n)
            s << juce::String (f.amplitude[(size_t) n], 5) << "f" << (n + 1 < last ? ", " : "") << ((n + 1) % 12 == 0 && n + 1 < last ? "\n        " : "");
        s << " },\n";
    }
    s << "};\n";

    header.replaceWithText (s);
    std::printf ("Wrote %s\n", header.getFullPathName().toRawUTF8());
    return 0;
}
