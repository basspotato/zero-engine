// ZeroAnalyze: offline measurement of reference sounds and engine renders on
// loudness, brightness, bass and width, and a comparison between the two.
//
//   ZeroAnalyze analyze <folder> [out.csv]
//   ZeroAnalyze render  [outDir]            all 72 profiles: engine.csv + WAVs
//   ZeroAnalyze compare <refFolder> [outDir]
//   ZeroAnalyze tune    <refFolder> [--write] [--tone]   match profiles to references (--tone: brightness only)
//   ZeroAnalyze tunetest                                 tuner self-check, no refs needed
//   ZeroAnalyze test                                     engine checks, all profiles
//   ZeroAnalyze bench                                    CPU with 16 voices
//   ZeroAnalyze screen  [shortlist.csv] [perPair]        Freesound CC0 search
//   ZeroAnalyze fetch   [shortlist.csv] [outDir] [max]   Freesound downloads
//   ZeroAnalyze harmonics <out.h> <5 wavs>              wavetable frames from recordings
//
// Reference layout: <refFolder>/<Genre>/<Type>/*.wav (any depth; the first
// folder names that match a genre and a type are used, e.g. "Dubstep/Bass").

#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <map>
#include "Descriptors.h"
#include "Freesound.h"
#include "Tests.h"
#include "MatchBench.h"
#include "Library.h"
#include "Tuner.h"
#include "Harmonics.h"
#include "../../Source/PluginProcessor.h"
#include "../../Source/PluginEditor.h"
#include "../../Source/Parameters.h"
#include "../../Source/SoundPrint.h"

namespace
{
    void writeWav (const juce::File& file, const juce::AudioBuffer<float>& audio)
    {
        file.getParentDirectory().createDirectory();
        file.deleteFile();

        auto stream = file.createOutputStream();
        if (stream == nullptr)
            return;

        juce::WavAudioFormat wav;
        std::unique_ptr<juce::AudioFormatWriter> writer (wav.createWriterFor (stream.get(), renderRate, 2, 24, {}, 0));
        if (writer != nullptr)
        {
            stream.release(); // the writer owns it now
            writer->writeFromAudioSampleBuffer (audio, 0, audio.getNumSamples());
        }
    }

    Item renderAndAnalyse (int genre, int type, const juce::File& wavDir, int rootNote = -1)
    {
        const auto audio = renderProfile (genre, type, nullptr, rootNote);

        Item item;
        item.genre = genre;
        item.type = type;
        item.name = genreNames[genre] + " " + typeNames[type];
        item.d = analyse (audio, renderRate);

        if (wavDir != juce::File())
            writeWav (wavDir.getChildFile (normalise (genreNames[genre]) + "_" + normalise (typeNames[type]) + ".wav"), audio);

        return item;
    }

    //==========================================================================
    void writeCsv (const juce::File& file, const std::vector<Item>& items)
    {
        juce::String csv ("name,genre,type");
        for (const auto& f : descriptorFields)
            csv << "," << f.name;
        csv << "\n";

        for (const auto& item : items)
        {
            csv << "\"" << item.name.replace ("\"", "\"\"") << "\","
                << (item.genre >= 0 ? genreNames[item.genre] : juce::String()) << ","
                << (item.type >= 0 ? typeNames[item.type] : juce::String());
            for (const auto& f : descriptorFields)
                csv << "," << juce::String (item.d.*f.member, 3);
            csv << "\n";
        }

        file.getParentDirectory().createDirectory();
        file.replaceWithText (csv);
        std::printf ("Wrote %s\n", file.getFullPathName().toRawUTF8());
    }

    struct Stats
    {
        double mean = 0, spread = 0;
    };

    Stats statsOf (const std::vector<const Item*>& items, double Descriptors::* member)
    {
        Stats s;
        for (auto* i : items)
            s.mean += i->d.*member;
        s.mean /= (double) items.size();

        for (auto* i : items)
            s.spread += std::pow (i->d.*member - s.mean, 2.0);
        s.spread = items.size() > 1 ? std::sqrt (s.spread / (double) (items.size() - 1)) : 0.0;
        return s;
    }

    // Plain-words verdict for the four headline dimensions. Differences inside
    // the references' own spread (or a small floor) count as a match.
    juce::String verdict (const char* field, double diff, double spread)
    {
        const juce::String f (field);
        auto outside = [&] (double floor) { return std::abs (diff) > juce::jmax (spread, floor); };

        if (f == "lufs" && outside (1.5))        return diff > 0 ? "engine louder" : "engine quieter";
        if (f == "plr_db" && outside (1.5))      return diff > 0 ? "engine more dynamic" : "engine more squashed";
        if (f == "centroid_hz" && outside (300)) return diff > 0 ? "engine brighter" : "engine darker";
        if (f == "high_pct" && outside (3))      return diff > 0 ? "engine more top end" : "engine less top end";
        if (f == "bass_pct" && outside (5))      return diff > 0 ? "engine more bass" : "engine less bass";
        if (f == "sub_pct" && outside (5))       return diff > 0 ? "engine more sub" : "engine less sub";
        if (f == "side_pct" && outside (4))      return diff > 0 ? "engine wider" : "engine narrower";
        return {};
    }

    void compare (const std::vector<Item>& refs, const juce::File& outDir)
    {
        // One comparison per genre/type group present in the references.
        std::map<std::pair<int, int>, std::vector<const Item*>> groups;
        for (const auto& r : refs)
            groups[{ r.genre, r.type }].push_back (&r);

        std::vector<Item> renders;
        juce::String csv ("genre,type,refs,field,ref_mean,ref_spread,engine,diff,verdict\n");

        for (const auto& [key, members] : groups)
        {
            const auto [genre, type] = key;
            std::printf ("\n%s  (%d refs)\n", groupName (genre, type).toRawUTF8(), (int) members.size());

            if (genre < 0 || type < 0)
            {
                std::printf ("  not compared: put these under <Genre>/<Type> folders to match a profile\n");
                continue;
            }

            renders.push_back (renderAndAnalyse (genre, type, outDir.getChildFile ("renders"), matchedRoot (genre, type, groupNote (members, type))));
            const auto& engine = renders.back().d;

            std::printf ("  %-12s %10s %8s %10s %8s\n", "", "ref", "+/-", "engine", "diff");
            for (const auto& f : descriptorFields)
            {
                const auto s = statsOf (members, f.member);
                const auto value = engine.*f.member;
                const auto diff = value - s.mean;
                const auto v = verdict (f.name, diff, s.spread);

                std::printf ("  %-12s %10.1f %8.1f %10.1f %+8.1f  %s\n", f.name, s.mean, s.spread, value, diff, v.toRawUTF8());
                csv << genreNames[genre] << "," << typeNames[type] << "," << (int) members.size() << "," << f.name << ","
                    << juce::String (s.mean, 3) << "," << juce::String (s.spread, 3) << "," << juce::String (value, 3) << ","
                    << juce::String (diff, 3) << "," << v << "\n";
            }
        }

        writeCsv (outDir.getChildFile ("refs.csv"), refs);
        writeCsv (outDir.getChildFile ("engine.csv"), renders);
        outDir.getChildFile ("compare.csv").replaceWithText (csv);
        std::printf ("Wrote %s\n", outDir.getChildFile ("compare.csv").getFullPathName().toRawUTF8());
    }

    // CPU check: all 16 voices held on a profile, timed per block against the
    // block's real-time budget. Single core, Release build.
    void bench (int genre, int type)
    {
        ZeroEngineProcessor engine;
        auto setChoice = [&] (const char* id, int index)
        {
            auto* param = engine.state.getParameter (id);
            param->setValueNotifyingHost (param->convertTo0to1 ((float) index));
        };
        setChoice (ParamIDs::genre, genre);
        setChoice (ParamIDs::type, type);
        engine.syncProfile();

        engine.setPlayConfigDetails (0, 2, renderRate, renderBlock);
        engine.prepareToPlay (renderRate, renderBlock);

        juce::AudioBuffer<float> block (2, renderBlock);
        juce::MidiBuffer midi;
        for (int v = 0; v < 16; ++v)
            midi.addEvent (juce::MidiMessage::noteOn (1, 40 + v * 2, (juce::uint8) 100), 0);

        const auto blocks = (int) (10.0 * renderRate / renderBlock);
        const auto budget = renderBlock / renderRate;
        double total = 0.0, worst = 0.0;

        for (int b = 0; b < blocks; ++b)
        {
            const auto start = juce::Time::getHighResolutionTicks();
            engine.processBlock (block, midi);
            const auto seconds = juce::Time::highResolutionTicksToSeconds (juce::Time::getHighResolutionTicks() - start);
            midi.clear();

            if (b > 10) // skip warm-up
            {
                total += seconds;
                worst = juce::jmax (worst, seconds);
            }
        }

        const auto average = total / (blocks - 11);
        std::printf ("  %-26s avg %5.1f%%  worst %5.1f%% of one core\n", (genreNames[genre] + " / " + typeNames[type]).toRawUTF8(),
                     100.0 * average / budget, 100.0 * worst / budget);
    }

    juce::File argFile (const juce::StringArray& args, int index, const juce::String& fallback)
    {
        return juce::File::getCurrentWorkingDirectory().getChildFile (args.size() > index ? args[index] : fallback);
    }

    int usage()
    {
        std::printf ("Usage:\n"
                     "  ZeroAnalyze analyze <folder> [out.csv]\n"
                     "  ZeroAnalyze render  [outDir]\n"
                     "  ZeroAnalyze compare <refFolder> [outDir]\n"
                     "  ZeroAnalyze tune    <refFolder> [--write] [--tone]\n"
                     "  ZeroAnalyze tunetest\n"
                     "  ZeroAnalyze test\n"
                     "  ZeroAnalyze bench\n"
                     "  ZeroAnalyze screen  [shortlist.csv] [perPair]\n"
                     "  ZeroAnalyze fetch   [shortlist.csv] [outDir] [maxDownloads]\n"
                     "  ZeroAnalyze harmonics <out.h> <wav1> ... <wav5>\n");
        return 1;
    }
}

int main (int argc, char* argv[])
{
    juce::ScopedJuceInitialiser_GUI juce; // the engine's parameters and timer need a message manager

    juce::StringArray args;
    for (int i = 1; i < argc; ++i)
        args.add (juce::String::fromUTF8 (argv[i]));

    if (args.isEmpty())
        return usage();

    const auto command = args[0];

    if (command == "analyze" && args.size() >= 2)
    {
        const auto items = analyseFolder (argFile (args, 1, {}));
        writeCsv (argFile (args, 2, "analysis/refs.csv"), items);
        return 0;
    }

    if (command == "render")
    {
        const auto outDir = argFile (args, 1, "analysis");
        std::vector<Item> items;
        for (int g = 0; g < genreNames.size(); ++g)
            for (int t = 0; t < typeNames.size(); ++t)
                items.push_back (renderAndAnalyse (g, t, outDir.getChildFile ("renders")));
        writeCsv (outDir.getChildFile ("engine.csv"), items);
        return 0;
    }

    if (command == "compare" && args.size() >= 2)
    {
        compare (analyseFolder (argFile (args, 1, {})), argFile (args, 2, "analysis"));
        return 0;
    }

    if (command == "test")
        return runTests();

    if (command == "tunetest")
        return tuneTest();

    if (command == "tune" && args.size() >= 2)
        return tuneProfiles (argFile (args, 1, {}),
                             args.contains ("--write") ? argFile (args, 99, "Source/ProfileTuning.h") : juce::File(),
                             args.contains ("--tone"));

    if (command == "bench")
    {
        std::printf ("16 voices held, 48 kHz, %d-sample blocks:\n", renderBlock);
        for (const auto& [g, t] : { std::pair { "Trance", "Chord" }, { "EDM", "Pad" }, { "Dubstep", "Bass" },
                                    { "Drum & Bass", "Bass" }, { "Trap", "Bass" }, { "Hardstyle", "Lead" } })
            bench (genreNames.indexOf (g), typeNames.indexOf (t));
        return 0;
    }

    if (command == "screen")
        return screenFreesound (argFile (args, 1, "analysis/freesound/shortlist.csv"),
                                args.size() > 2 ? args[2].getIntValue() : 20);

    if (command == "fetch")
        return fetchFreesound (argFile (args, 1, "analysis/freesound/shortlist.csv"),
                               argFile (args, 2, "analysis/freesound/refs"),
                               args.size() > 3 ? args[3].getIntValue() : 450);

    if (command == "match" && args.size() >= 2)
    {
        // Reference Match from the command line: one file with a genre and type, or a
        // folder laid out as <Genre>/<Type>/*.wav. Prints how close each gets and
        // what it chose.
        const auto verbose = args.contains ("-v");
        ReferenceMatch::debugPrint = verbose;
        args.removeString ("-v");
        const juce::File root (juce::File::getCurrentWorkingDirectory().getChildFile (args[1]));
        struct Job { juce::File file; int genre, type; };
        std::vector<Job> jobs;
        auto indexOf = [] (const juce::StringArray& names, const juce::String& name) { return names.indexOf (name, true); };
        if (root.existsAsFile() && args.size() >= 4)
            jobs.push_back ({ root, indexOf (genreNames, args[2]), indexOf (typeNames, args[3]) });
        else
            for (const auto& file : root.findChildFiles (juce::File::findFiles, true, "*.wav;*.aif;*.aiff;*.flac"))
                jobs.push_back ({ file, indexOf (genreNames, file.getParentDirectory().getParentDirectory().getFileName()),
                                  indexOf (typeNames, file.getParentDirectory().getFileName()) });

        MatchEngines engines;
        ReferenceMatch match (engines.get());
        for (const auto& job : jobs)
        {
            if (job.genre < 0 || job.type < 0)
            {
                std::printf ("%s: unknown genre/type folder\n", job.file.getFullPathName().toRawUTF8());
                continue;
            }
            const auto began = juce::Time::getMillisecondCounter();
            match.start (job.file, job.genre, job.type, makeProfile (job.genre, job.type));
            while (match.getState() == ReferenceMatch::State::running)
                juce::Thread::sleep (20);
            const auto result = match.take();
            const auto& p = result.profile;
            std::printf ("%-12s %-6s %-22s %5.1f s  %s", genreNames[job.genre].toRawUTF8(), typeNames[job.type].toRawUTF8(),
                         job.file.getFileName().toRawUTF8(), (juce::Time::getMillisecondCounter() - began) / 1000.0,
                         result.summary.fromFirstOccurrenceOf ("-  ", false, false).toRawUTF8());
            if (result.ok)
                std::printf ("  table %d pos %.2f tilt %+.1f air %.2f ott %.2f grit %.2f width %.2f A %.3f D %.2f S %.2f",
                             (int) p.waveTable, p.wavePos, p.tilt, p.air, p.ottDepth, p.grit, p.stereoWidth, p.ampAttack, p.ampDecay, p.ampSustain);
            std::printf ("\n");
            if (verbose && result.ok)
                for (const auto* s : { &result.target, &result.matched })
                {
                    std::printf ("    %s pitch %4.0f att %4.0f env", s == &result.target ? "ref" : "got", s->pitchHz, s->attackMs);
                    for (auto e : s->envelopeDb) std::printf (" %5.1f", e);
                    std::printf ("  bands");
                    for (auto b : s->bandsDb) std::printf (" %3.0f", b);
                    std::printf ("  side %4.1f crest %4.1f\n", s->sidePct, s->crestDb);
                }
        }
        return 0;
    }

    if (command == "notes" && args.size() >= 2)
    {
        // Reference Match's transcription of a sound: the notes candidates will play.
        juce::AudioFormatManager formats;
        formats.registerBasicFormats();
        std::unique_ptr<juce::AudioFormatReader> reader (formats.createReaderFor (juce::File::getCurrentWorkingDirectory().getChildFile (args[1])));
        if (reader == nullptr)
            return 1;
        juce::AudioBuffer<float> audio (2, (int) juce::jmin<juce::int64> (reader->lengthInSamples, (juce::int64) (reader->sampleRate * 10.0)));
        reader->read (&audio, 0, audio.getNumSamples(), 0, true, true);
        const auto print = SoundPrint::of (audio, reader->sampleRate, true);
        std::printf ("%d notes, %.2f s measured%s\n", (int) print.notes.size(), print.noteSeconds, print.cut ? " (goes on past it)" : "");
        for (const auto& n : print.notes)
            std::printf ("  at %.2f s for %.2f s  %6.1f Hz\n", n.start, n.length, n.pitchHz);
        return 0;
    }

    if (command == "matchbench")
        return runMatchBench (args);

    if (command == "noise" && args.size() >= 2)
    {
        // Noise between the harmonics of a held C4 (dB under the harmonics), per
        // profile: a clean synth is quiet there; hiss and aliasing raised by the
        // compressors are not. Genre/Type, or "all"; then id=value overrides
        // (e.g. spectral=0 ottDepth=0) to find which stage adds it.
        std::vector<std::pair<int, int>> pairs;
        auto overrides = args;
        overrides.removeRange (0, args[1] == "all" ? 2 : 3);
        if (args[1] == "all")
            for (int g = 0; g < genreNames.size(); ++g)
                for (int t = 0; t < typeNames.size(); ++t)
                    pairs.push_back ({ g, t });
        else
            pairs.push_back ({ genreNames.indexOf (args[1], true), typeNames.indexOf (args[2], true) });

        constexpr int fftOrder = 14, fftSize = 1 << fftOrder;
        juce::dsp::FFT fft (fftOrder);
        for (const auto& [g, t] : pairs)
        {
            ZeroEngineProcessor engine;
            engine.stopProfileSync();
            engine.loadProfile (g, t, makeProfile (g, t));
            for (const auto& o : overrides)
                if (auto* param = engine.state.getParameter (o.upToFirstOccurrenceOf ("=", false, false)))
                    param->setValueNotifyingHost (param->convertTo0to1 (o.fromFirstOccurrenceOf ("=", false, false).getFloatValue()));
            engine.setPlayConfigDetails (0, 2, 48000.0, 512);
            engine.prepareToPlay (48000.0, 512);

            juce::AudioBuffer<float> out (2, 48000 * 2), block (2, 512);
            juce::MidiBuffer midi;
            for (int pos = 0; pos + 512 <= out.getNumSamples(); pos += 512)
            {
                midi.clear();
                if (pos == 0)
                    midi.addEvent (juce::MidiMessage::noteOn (1, 60, (juce::uint8) 100), 0);
                block.clear();
                engine.processBlock (block, midi);
                for (int ch = 0; ch < 2; ++ch)
                    out.copyFrom (ch, pos, block, ch, 0, 512);
            }

            std::vector<float> frame ((size_t) fftSize * 2, 0.0f);
            const auto from = 48000 / 2;
            for (int i = 0; i < fftSize; ++i)
                frame[(size_t) i] = 0.5f * (out.getSample (0, from + i) + out.getSample (1, from + i))
                                  * (0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * (float) i / (float) fftSize));
            fft.performFrequencyOnlyForwardTransform (frame.data());

            // Harmonics of C4 from 1 to 10 kHz, and the midpoints between them.
            const auto f0 = 261.63, binHz = 48000.0 / fftSize;
            double harmonics = 0.0, between = 0.0;
            int count = 0;
            for (int h = 4; h * f0 < 10000.0; ++h)
            {
                auto peakNear = [&] (double hz, int radius)
                {
                    double best = 0.0;
                    for (int b = (int) (hz / binHz) - radius; b <= (int) (hz / binHz) + radius; ++b)
                        best = juce::jmax (best, (double) frame[(size_t) b] * frame[(size_t) b]);
                    return best;
                };
                harmonics += peakNear (h * f0, 3);
                between += peakNear ((h + 0.5) * f0, 1);
                ++count;
            }
            std::printf ("%-12s %-6s floor %6.1f dB under the harmonics\n", genreNames[g].toRawUTF8(), typeNames[t].toRawUTF8(),
                         10.0 * std::log10 (harmonics / juce::jmax (1.0e-20, between)));
        }
        return 0;
    }

    if (command == "gendebug" && args.size() >= 4)
    {
        // One Generate result (genre, type, seed): the parameters that shape its tail,
        // and its level each second through a 1 s note and 8 s of silence.
        ZeroEngineProcessor engine;
        auto* genre = engine.state.getParameter (ParamIDs::genre);
        auto* type = engine.state.getParameter (ParamIDs::type);
        genre->setValueNotifyingHost (genre->convertTo0to1 ((float) genreNames.indexOf (args[1])));
        type->setValueNotifyingHost (type->convertTo0to1 ((float) typeNames.indexOf (args[2])));
        engine.syncProfile();
        engine.generate (args[3].getLargeIntValue());
        for (int a = 4; a < args.size(); ++a) // id=value overrides, to find what holds a tail up
            if (auto* param = engine.state.getParameter (args[a].upToFirstOccurrenceOf ("=", false, false)))
                param->setValueNotifyingHost (param->convertTo0to1 (args[a].fromFirstOccurrenceOf ("=", false, false).getFloatValue()));

        for (const auto* id : { ParamIDs::ampSustain, ParamIDs::ampRelease, ParamIDs::waveTable, ParamIDs::filterRes,
                                ParamIDs::delayMix, ParamIDs::delayFeedback, ParamIDs::reverbMix, ParamIDs::lfoToPos,
                                ParamIDs::lfoSync, ParamIDs::weight, ParamIDs::ottDepth, ParamIDs::playMode, ParamIDs::glide })
            std::printf ("%-14s %.3f\n", id, engine.state.getRawParameterValue (id)->load());

        engine.setPlayConfigDetails (0, 2, 48000.0, 512);
        engine.prepareToPlay (48000.0, 512);
        juce::AudioBuffer<float> block (2, 512);
        juce::MidiBuffer midi;
        float peak = 0.0f;
        for (int b = 0, second = 0; b < 48000 * 9 / 512; ++b)
        {
            midi.clear();
            if (b == 0)  midi.addEvent (juce::MidiMessage::noteOn (1, 48, (juce::uint8) 110), 0);
            if (b == 93) midi.addEvent (juce::MidiMessage::noteOff (1, 48), 0);
            block.clear();
            engine.processBlock (block, midi);
            peak = juce::jmax (peak, block.getMagnitude (0, 512));
            if ((b + 1) * 512 / 48000 > second)
            {
                std::printf ("second %d peak %.1f dBFS, %d voices\n", second, juce::Decibels::gainToDecibels (peak), engine.getActiveVoiceCount());
                peak = 0.0f;
                ++second;
            }
        }
        return 0;
    }

    if (command == "segments" && args.size() >= 2)
    {
        // Loudness and brightness of a WAV in slices, to check how a sound moves
        // over time (a riser should brighten, an impact should fall away).
        juce::AudioFormatManager formats;
        formats.registerBasicFormats();
        std::unique_ptr<juce::AudioFormatReader> reader (formats.createReaderFor (argFile (args, 1, {})));
        if (reader == nullptr)
            return usage();
        const auto slice = args.size() > 2 ? args[2].getDoubleValue() : 0.5;
        const auto sliceSamples = (int) (slice * reader->sampleRate);
        juce::AudioBuffer<float> audio (2, sliceSamples);
        std::printf ("%8s %8s %10s\n", "from s", "LUFS", "centroid");
        for (juce::int64 start = 0; start + sliceSamples <= reader->lengthInSamples; start += sliceSamples)
        {
            reader->read (&audio, 0, sliceSamples, start, true, true);
            const auto d = analyse (audio, reader->sampleRate);
            std::printf ("%8.2f %8.1f %10.0f\n", (double) start / reader->sampleRate, d.lufs, d.centroidHz);
        }
        return 0;
    }

    if (command == "gencheck")
    {
        // Generate's render-and-check: time per click and how many of the 6
        // candidates still fit the genre/type, 3 clicks per profile.
        std::printf ("%-24s %8s %s\n", "profile", "ms/click", "fits of 6");
        double totalMs = 0.0;
        int totalFits = 0, clicks = 0;
        for (int g = 0; g < genreNames.size(); ++g)
            for (int t = 0; t < typeNames.size(); ++t)
            {
                ZeroEngineProcessor engine;
                auto* genre = engine.state.getParameter (ParamIDs::genre);
                auto* type = engine.state.getParameter (ParamIDs::type);
                genre->setValueNotifyingHost (genre->convertTo0to1 ((float) g));
                type->setValueNotifyingHost (type->convertTo0to1 ((float) t));
                engine.syncProfile();

                juce::String fits;
                const auto start = juce::Time::getMillisecondCounterHiRes();
                for (int s = 1; s <= 3; ++s)
                {
                    engine.generate (s * 7919 + g * 31 + t);
                    fits << engine.getLastGenerateFits() << " ";
                    totalFits += engine.getLastGenerateFits();
                }
                const auto ms = (juce::Time::getMillisecondCounterHiRes() - start) / 3.0;
                totalMs += ms * 3.0;
                clicks += 3;
                std::printf ("%-24s %8.0f %s\n", (genreNames[g] + " " + typeNames[t]).toRawUTF8(), ms, fits.toRawUTF8());
            }
        std::printf ("\nAverage %.0f ms per click; %.1f of 6 candidates fit on average.\n", totalMs / clicks, (double) totalFits / clicks);
        return 0;
    }

    if (command == "foldstrip" && args.size() >= 2)
    {
        // The Generate tesseract at 11 points through its fold, side by side.
        TesseractButton tesseract;
        tesseract.setSize (120, 130);
        const float points[] { 0.0f, 0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f, 0.9f, 1.0f };
        juce::Image strip (juce::Image::ARGB, 120 * (int) std::size (points), 130, true);
        {
            juce::Graphics g (strip);
            for (size_t i = 0; i < std::size (points); ++i)
            {
                tesseract.showFoldAt (points[i]);
                g.drawImageAt (tesseract.createComponentSnapshot (tesseract.getLocalBounds()), 120 * (int) i, 0);
            }
        }
        juce::FileOutputStream out (argFile (args, 1, {}));
        out.setPosition (0);
        out.truncate();
        juce::PNGImageFormat().writeImageToStream (strip, out);
        std::printf ("Wrote %s\n", argFile (args, 1, {}).getFullPathName().toRawUTF8());
        return 0;
    }

    if (command == "snapshot" && args.size() >= 2)
    {
        // The editor as a PNG, after playing a chord through the engine so the
        // meters and scope have something to show. For checking layout by eye.
        ZeroEngineProcessor engine;
        engine.setPlayConfigDetails (0, 2, renderRate, renderBlock);
        engine.prepareToPlay (renderRate, renderBlock);
        std::unique_ptr<juce::AudioProcessorEditor> editor (engine.createEditor());

        juce::AudioBuffer<float> block (2, renderBlock);
        juce::MidiBuffer midi;
        for (int i = 0; i < 60; ++i)
        {
            midi.clear();
            if (i == 0)
                for (auto note : { 48, 55, 60 })
                    midi.addEvent (juce::MidiMessage::noteOn (1, note, (juce::uint8) 110), 0);
            engine.processBlock (block, midi);
        }

        // Let the editor's timers pick up the meters a few times.
        for (int i = 0; i < 20; ++i)
        {
            engine.processBlock (block, midi);
            juce::MessageManager::getInstance()->runDispatchLoopUntil (40);
        }

        std::printf ("Live wavetable position %.3f (knob %.3f)\n", engine.getLivePosition(),
                     engine.state.getRawParameterValue (ParamIDs::wavePos)->load());

        // Extra arguments click buttons by their text, in order (e.g. ADVANCED FILTER);
        // "size=WxH" resizes the window first (e.g. size=1350x900).
        for (int a = 2; a < args.size(); ++a)
        {
            if (args[a].startsWith ("clickstep=")) // press a step pad, as a mouse would
            {
                std::function<SequencerGrid* (juce::Component&)> findGrid = [&] (juce::Component& c) -> SequencerGrid*
                {
                    if (auto* g = dynamic_cast<SequencerGrid*> (&c))
                        return g;
                    for (auto* child : c.getChildren())
                        if (auto* found = findGrid (*child))
                            return found;
                    return nullptr;
                };
                if (auto* grid = findGrid (*editor))
                {
                    const auto step = args[a].fromFirstOccurrenceOf ("=", false, false).getIntValue();
                    const juce::Point<float> at ((float) grid->getWidth() * ((float) step + 0.5f) / 16.0f, 12.0f);
                    const auto now = juce::Time::getCurrentTime();
                    const juce::MouseEvent e (juce::Desktop::getInstance().getMainMouseSource(), at, {}, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                                              grid, grid, now, at, now, 1, false);
                    grid->mouseDown (e);
                    grid->mouseUp (e);
                    juce::MessageManager::getInstance()->runDispatchLoopUntil (60);
                    std::printf ("Step %d on: %.0f, arp mode: %s\n", step + 1,
                                 engine.state.getRawParameterValue (SeqIDs::on (step))->load(),
                                 arpModeNames[(int) engine.state.getRawParameterValue (ParamIDs::arpMode)->load()].toRawUTF8());
                }
                continue;
            }
            if (args[a] == "dialog") // the rename dialog, written next to the snapshot
            {
                juce::AlertWindow dialog ("Rename favourite", {}, juce::MessageBoxIconType::NoIcon, editor.get());
                dialog.setLookAndFeel (&editor->getLookAndFeel());
                dialog.addTextEditor ("name", "Trap Pad 1");
                dialog.addButton ("Rename", 1);
                dialog.addButton ("Cancel", 0);
                dialog.setSize (dialog.getWidth(), dialog.getHeight());
                juce::FileOutputStream dialogOut (argFile (args, 1, {}).withFileExtension ("dialog.png"));
                dialogOut.setPosition (0);
                dialogOut.truncate();
                juce::PNGImageFormat().writeImageToStream (dialog.createComponentSnapshot (dialog.getLocalBounds()), dialogOut);
                dialog.setLookAndFeel (nullptr);
                continue;
            }
            if (args[a].startsWith ("size="))
            {
                const auto size = args[a].fromFirstOccurrenceOf ("=", false, false);
                editor->setSize (size.upToFirstOccurrenceOf ("x", false, false).getIntValue(), size.fromFirstOccurrenceOf ("x", false, false).getIntValue());
                continue;
            }
            std::function<juce::Button* (juce::Component&)> find = [&] (juce::Component& c) -> juce::Button*
            {
                if (auto* b = dynamic_cast<juce::Button*> (&c); b != nullptr && b->getButtonText() == args[a])
                    return b;
                for (auto* child : c.getChildren())
                    if (auto* found = find (*child))
                        return found;
                return nullptr;
            };
            if (auto* button = find (*editor))
            {
                button->triggerClick();
                juce::MessageManager::getInstance()->runDispatchLoopUntil (60);
            }
            else
                std::printf ("No button labelled %s\n", args[a].toRawUTF8());
        }
        const auto image = editor->createComponentSnapshot (editor->getLocalBounds());
        juce::FileOutputStream out (argFile (args, 1, {}));
        out.setPosition (0);
        out.truncate();
        juce::PNGImageFormat().writeImageToStream (image, out);
        std::printf ("Wrote %s\n", argFile (args, 1, {}).getFullPathName().toRawUTF8());
        return 0;
    }

    if (command == "harmonics" && args.size() >= 3)
    {
        juce::StringArray files;
        for (int i = 2; i < args.size(); ++i)
            files.add (args[i]);
        return extractHarmonics (argFile (args, 1, {}), files);
    }

    return usage();
}
