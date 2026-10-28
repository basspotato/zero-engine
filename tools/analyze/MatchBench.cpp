#include "MatchBench.h"
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_dsp/juce_dsp.h>
#include "../../Source/PluginProcessor.h"
#include "../../Source/Parameters.h"
#include "../../Source/ReferenceMatch.h"

namespace
{
    constexpr double rate = 48000.0;
    constexpr int block = 512;

    struct Note { int start, note, length; };

    // One held note, or an arp loop: eight short notes a fifth of a second apart.
    std::vector<Note> patternFor (int type, bool loop)
    {
        const auto root = type == 0 ? 36 : 60;
        if (! loop)
            return { { 0, root, (int) (1.2 * rate) } };

        std::vector<Note> notes;
        constexpr int steps[] { 0, 7, 12, 7, 3, 7, 12, 15 };
        for (int i = 0; i < 8; ++i)
            notes.push_back ({ (int) (i * 0.2 * rate), root + steps[i], (int) (0.15 * rate) });
        return notes;
    }

    juce::AudioBuffer<float> render (int genre, int type, const Profile& profile, const std::vector<Note>& notes)
    {
        ZeroEngineProcessor engine;
        engine.stopProfileSync();
        engine.setPlayConfigDetails (0, 2, rate, block);
        engine.prepareToPlay (rate, block);
        engine.loadProfile (genre, type, profile);

        juce::AudioBuffer<float> settle (2, block), out (2, (int) (2.0 * rate)), chunk (2, block);
        juce::MidiBuffer midi;
        for (int i = 0; i < 10; ++i)
        {
            settle.clear();
            engine.processBlock (settle, midi);
        }
        for (int pos = 0; pos + block <= out.getNumSamples(); pos += block)
        {
            midi.clear();
            for (const auto& n : notes)
            {
                if (n.start >= pos && n.start < pos + block)
                    midi.addEvent (juce::MidiMessage::noteOn (1, n.note, (juce::uint8) 100), n.start - pos);
                if (n.start + n.length >= pos && n.start + n.length < pos + block)
                    midi.addEvent (juce::MidiMessage::noteOff (1, n.note), n.start + n.length - pos);
            }
            chunk.clear();
            engine.processBlock (chunk, midi);
            for (int ch = 0; ch < 2; ++ch)
                out.copyFrom (ch, pos, chunk, ch, 0, block);
        }
        return out;
    }

    // Log-mel spectrogram: 40 bands 40 Hz to 16 kHz, 2048-sample frames every
    // 512, in dB under the loudest cell (floor -80), from the first audible frame.
    std::vector<std::array<float, 40>> melOf (const juce::AudioBuffer<float>& audio)
    {
        constexpr int order = 11, size = 1 << order, hop = 512, bands = 40;
        static const auto weights = []
        {
            auto mel = [] (double hz) { return 2595.0 * std::log10 (1.0 + hz / 700.0); };
            auto hzOf = [] (double m) { return 700.0 * (std::pow (10.0, m / 2595.0) - 1.0); };
            std::vector<std::array<float, bands>> w ((size_t) size / 2);
            const auto lo = mel (40.0), hi = mel (16000.0);
            for (int b = 0; b < bands; ++b)
            {
                const auto left = hzOf (lo + (hi - lo) * b / (bands + 1)), centre = hzOf (lo + (hi - lo) * (b + 1) / (bands + 1)),
                           right = hzOf (lo + (hi - lo) * (b + 2) / (bands + 1));
                for (int bin = 1; bin < size / 2; ++bin)
                {
                    const auto hz = bin * rate / size;
                    w[(size_t) bin][(size_t) b] = (float) juce::jmax (0.0, hz < centre ? (hz - left) / (centre - left) : (right - hz) / (right - centre));
                }
            }
            return w;
        }();

        juce::dsp::FFT fft (order);
        std::vector<float> frame ((size_t) size * 2);
        std::vector<std::array<float, bands>> out;
        float loudest = -200.0f;
        for (int start = 0; start + size <= audio.getNumSamples(); start += hop)
        {
            for (int i = 0; i < size; ++i)
                frame[(size_t) i] = 0.5f * (audio.getSample (0, start + i) + audio.getSample (1, start + i))
                                  * (0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * (float) i / (float) size));
            std::fill (frame.begin() + size, frame.end(), 0.0f);
            fft.performFrequencyOnlyForwardTransform (frame.data());

            std::array<float, bands> cell {};
            for (int b = 0; b < bands; ++b)
            {
                double power = 0.0;
                for (int bin = 1; bin < size / 2; ++bin)
                    power += weights[(size_t) bin][(size_t) b] * frame[(size_t) bin] * frame[(size_t) bin];
                cell[(size_t) b] = (float) (10.0 * std::log10 (power + 1.0e-12));
                loudest = juce::jmax (loudest, cell[(size_t) b]);
            }
            out.push_back (cell);
        }
        for (auto& cell : out)
            for (auto& v : cell)
                v = juce::jmax (-80.0f, v - loudest);

        // From the first frame with anything in it.
        size_t first = 0;
        while (first < out.size() && *std::max_element (out[first].begin(), out[first].end()) < -60.0f)
            ++first;
        out.erase (out.begin(), out.begin() + (long) first);
        return out;
    }

    // Mean dB difference between two log-mel spectrograms (cells where both are
    // under -70 dB skipped).
    float judge (const juce::AudioBuffer<float>& a, const juce::AudioBuffer<float>& b)
    {
        const auto x = melOf (a), y = melOf (b);
        double sum = 0.0;
        int n = 0;
        for (size_t f = 0; f < juce::jmin (x.size(), y.size()); ++f)
            for (size_t band = 0; band < 40; ++band)
                if (x[f][band] > -70.0f || y[f][band] > -70.0f)
                {
                    sum += std::abs (x[f][band] - y[f][band]);
                    ++n;
                }
        return n > 0 ? (float) (sum / n) : 0.0f;
    }

    // A second judge on the same spectrograms: timbre (mean difference of MFCCs
    // 1-13, dB) and brightness (mean centroid difference, mel bands), over frames
    // where both sound.
    std::pair<float, float> timbreJudge (const juce::AudioBuffer<float>& a, const juce::AudioBuffer<float>& b)
    {
        const auto x = melOf (a), y = melOf (b);
        auto features = [] (const std::array<float, 40>& f, std::array<float, 13>& mfcc, float& centroid)
        {
            for (int k = 1; k <= 13; ++k)
            {
                double sum = 0.0;
                for (int band = 0; band < 40; ++band)
                    sum += f[(size_t) band] * std::cos (juce::MathConstants<double>::pi * k * (band + 0.5) / 40.0);
                mfcc[(size_t) k - 1] = (float) (sum * std::sqrt (2.0 / 40.0));
            }
            double power = 0.0, weighted = 0.0;
            for (int band = 0; band < 40; ++band)
            {
                const auto p = std::pow (10.0, f[(size_t) band] / 10.0);
                power += p;
                weighted += band * p;
            }
            centroid = (float) (weighted / juce::jmax (1.0e-12, power));
            return 10.0 * std::log10 (power + 1.0e-12);
        };
        double timbre = 0.0, bright = 0.0;
        int n = 0;
        for (size_t f = 0; f < juce::jmin (x.size(), y.size()); ++f)
        {
            std::array<float, 13> mx, my;
            float cx, cy;
            if (features (x[f], mx, cx) < -50.0 || features (y[f], my, cy) < -50.0)
                continue;
            double m = 0.0;
            for (size_t k = 0; k < 13; ++k)
                m += std::abs (mx[k] - my[k]);
            timbre += m / 13.0;
            bright += std::abs (cx - cy);
            ++n;
        }
        return n > 0 ? std::pair { (float) (timbre / n), (float) (bright / n) } : std::pair { 0.0f, 0.0f };
    }

    ReferenceMatch::Result runMatch (const juce::File& file, int genre, int type)
    {
        MatchEngines engines;
        ReferenceMatch match (engines.get());
        match.start (file, genre, type, makeProfile (genre, type));
        while (match.getState() == ReferenceMatch::State::running)
            juce::Thread::sleep (20);
        return match.take();
    }
}

int runMatchBench (const juce::StringArray& arguments)
{
    // w=mel,mfcc,centroid,loudness: try other composite-loss weights.
    auto args = arguments;
    for (int i = args.size(); --i >= 0;)
        if (args[i].startsWith ("light=") || args[i].startsWith ("budget=") || args[i].startsWith ("cmaes="))
        {
            const auto value = args[i].fromFirstOccurrenceOf ("=", false, false).getIntValue();
            if (args[i].startsWith ("light=")) ReferenceMatch::lightSearch = value != 0;
            if (args[i].startsWith ("budget=")) ReferenceMatch::searchBudget = value;
            if (args[i].startsWith ("cmaes=")) ReferenceMatch::useCmaes = value != 0;
            args.remove (i);
        }
        else if (args[i].startsWith ("w="))
        {
            const auto w = juce::StringArray::fromTokens (args[i].substring (2), ",", "");
            SoundPrint::weights = { w[0].getFloatValue(), w[1].getFloatValue(), w[2].getFloatValue(), w[3].getFloatValue() };
            args.remove (i);
        }
    std::printf ("search: %s, %d renders, %s\n", ReferenceMatch::lightSearch ? "light" : "full", ReferenceMatch::searchBudget,
                 ReferenceMatch::useCmaes ? "CMA-ES" : "pattern");
    std::printf ("weights: mel %.2f, mfcc %.2f, centroid %.2f, loudness %.2f\n", SoundPrint::weights.mel, SoundPrint::weights.mfcc,
                 SoundPrint::weights.centroid, SoundPrint::weights.loudness);
    struct Target { const char* genre; const char* type; int seed; bool loop; };
    const Target targets[] {
        { "Synthwave", "Pluck", 11, false }, { "Techno", "Bass", 12, false },      { "Trance", "Lead", 13, false },
        { "Future Bass", "Chord", 14, false }, { "Dubstep", "Bass", 15, false },  { "House", "Pluck", 16, false },
        { "Synthwave", "Pluck", 21, true },  { "Techno", "Pluck", 22, true },      { "Trance", "Lead", 23, true },
        { "EDM", "Pluck", 24, true },        { "Tech House", "Bass", 25, true },   { "Hardstyle", "Lead", 26, true },
    };

    const auto temp = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("zero_matchbench");
    temp.createDirectory();
    juce::WavAudioFormat wav;

    std::printf ("%-12s %-6s %-5s  preset dB  matched dB  time   matcher says\n", "genre", "type", "kind");
    double presetSum = 0.0, matchedSum = 0.0, timbreSum = 0.0, brightSum = 0.0, presetTimbreSum = 0.0;
    for (const auto& t : targets)
    {
        const auto genre = genreNames.indexOf (t.genre), type = typeNames.indexOf (t.type);
        juce::Random random (t.seed);
        const auto truth = generateProfile (genre, type, random);
        const auto notes = patternFor (type, t.loop);
        const auto reference = render (genre, type, truth, notes);

        const auto file = temp.getChildFile (juce::String (t.genre) + " " + t.type + " " + juce::String (t.seed) + ".wav");
        file.deleteFile();
        {
            std::unique_ptr<juce::AudioFormatWriter> writer (wav.createWriterFor (new juce::FileOutputStream (file), rate, 2, 24, {}, 0));
            writer->writeFromAudioSampleBuffer (reference, 0, reference.getNumSamples());
        }

        const auto preset = judge (reference, render (genre, type, makeProfile (genre, type), notes));
        {
            // Sanity: the true settings, through the matcher's own render and compare.
            ZeroEngineProcessor engine;
            engine.stopProfileSync();
            const auto print = SoundPrint::of (reference, rate, true);
            auto knownNotes = print; // the same, with the notes as actually played instead of as heard
            knownNotes.notes.clear();
            for (const auto& n : notes)
                knownNotes.notes.push_back ({ (float) (n.start / rate), (float) ((t.loop ? 0.2 * rate : n.length) / rate), (float) juce::MidiMessage::getMidiNoteInHertz (n.note) });
            std::printf ("    truth with the real notes scores %.2f\n", ReferenceMatch::score (engine, knownNotes, genre, type, truth, false));
            std::printf ("    truth scores %.2f (light %.2f), preset %.2f; %d notes heard\n", ReferenceMatch::score (engine, print, genre, type, truth, false),
                         ReferenceMatch::score (engine, print, genre, type, truth, true), ReferenceMatch::score (engine, print, genre, type, makeProfile (genre, type), false),
                         (int) print.notes.size());
        }
        const auto began = juce::Time::getMillisecondCounter();
        const auto result = runMatch (file, genre, type);
        const auto seconds = (juce::Time::getMillisecondCounter() - began) / 1000.0;
        const auto matchedAudio = result.ok ? render (result.genre, result.type, result.profile, notes) : juce::AudioBuffer<float> (2, 1024);
        const auto matched = result.ok ? judge (reference, matchedAudio) : 99.0f;
        const auto [timbre, bright] = timbreJudge (reference, matchedAudio);
        timbreSum += timbre;
        brightSum += bright;
        presetTimbreSum += timbreJudge (reference, render (genre, type, makeProfile (genre, type), notes)).first;
        if (result.ok) // beside the target, to listen to
        {
            const auto out = file.getSiblingFile (file.getFileNameWithoutExtension() + " matched.wav");
            out.deleteFile();
            std::unique_ptr<juce::AudioFormatWriter> writer (wav.createWriterFor (new juce::FileOutputStream (out), rate, 2, 24, {}, 0));
            writer->writeFromAudioSampleBuffer (matchedAudio, 0, matchedAudio.getNumSamples());
        }
        presetSum += preset;
        matchedSum += matched;
        std::printf ("%-12s %-6s %-5s  %9.1f  %10.1f  %4.1fs  pitch %4.0f/%4.0f  d %4.2f  %s\n", t.genre, t.type, t.loop ? "loop" : "note", preset, matched, seconds,
                     result.target.pitchHz, juce::MidiMessage::getMidiNoteInHertz (notes[0].note), result.distance,
                     result.summary.fromFirstOccurrenceOf ("-  ", false, false).toRawUTF8());
    }
    const auto count = (double) std::size (targets);
    std::printf ("mean: preset %.2f dB, matched %.2f dB\n", presetSum / count, matchedSum / count);
    std::printf ("mean timbre (MFCC): preset %.2f, matched %.2f dB; matched brightness off by %.2f mel bands\n", presetTimbreSum / count, timbreSum / count, brightSum / count);

    if (args.size() >= 2)
        for (const auto& file : juce::File (args[1]).findChildFiles (juce::File::findFiles, false, "*.wav"))
        {
            if (! file.getFileName().containsIgnoreCase ("match") || file.getFileName().containsIgnoreCase ("returned"))
                continue; // owner's references only, not what the old matcher gave back
            const auto result = runMatch (file, genreNames.indexOf ("Synthwave"), typeNames.indexOf ("Pluck"));
            std::printf ("%-28s %s\n", file.getFileName().toRawUTF8(), result.summary.fromFirstOccurrenceOf ("-  ", false, false).toRawUTF8());
            if (! result.ok)
                continue;

            // What it found, as one note at the reference's pitch, to listen to.
            const auto note = result.target.pitchHz > 0.0f ? juce::roundToInt (69.0 + 12.0 * std::log2 (result.target.pitchHz / 440.0)) : 60;
            const auto audio = render (result.genre, result.type, result.profile, { { 0, juce::jlimit (24, 96, note), (int) (0.8 * rate) } });
            const auto out = juce::File::getCurrentWorkingDirectory().getChildFile ("analysis/match_v2").getChildFile (file.getFileNameWithoutExtension() + " - v2 match.wav");
            out.getParentDirectory().createDirectory();
            out.deleteFile();
            std::unique_ptr<juce::AudioFormatWriter> writer (wav.createWriterFor (new juce::FileOutputStream (out), rate, 2, 24, {}, 0));
            writer->writeFromAudioSampleBuffer (audio, 0, audio.getNumSamples());
        }
    return 0;
}
