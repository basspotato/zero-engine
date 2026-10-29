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
#include "MatchLibrary.h"
#include "MatchNetTrain.h"
#include "GenerateNetTrain.h"
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

    // CPU check: every voice held on a profile, timed per block against the
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
        for (int v = 0; v < ZeroEngineProcessor::numVoices; ++v)
            midi.addEvent (juce::MidiMessage::noteOn (1, 36 + v * 2, (juce::uint8) 100), 0);

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
        std::printf ("%d voices held, 48 kHz, %d-sample blocks:\n", ZeroEngineProcessor::numVoices, renderBlock);
        for (const auto& [g, t] : { std::pair { "Trance", "Chord" }, { "EDM", "Pad" }, { "Dubstep", "Bass" },
                                    { "Drum & Bass", "Bass" }, { "Trap", "Bass" }, { "Hardstyle", "Lead" } })
            bench (genreNames.indexOf (g), typeNames.indexOf (t));
        return 0;
    }

    if (command == "screen-match")
        return screenFreesoundForMatch (argFile (args, 1, "analysis/library/freesound_shortlist.csv"),
                                        args.size() > 2 ? args[2].getIntValue() : 25);

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
            if (verbose && result.ok) // what holds the match back: the same with parts of the finishing chain off
            {
                ZeroEngineProcessor scorer;
                scorer.stopProfileSync();
                scorer.setFallbackTempo (result.bpm);
                auto variant = [&] (const char* name, auto change)
                {
                    auto q = p;
                    change (q);
                    std::printf ("    %-16s %.2f\n", name, ReferenceMatch::score (scorer, result.target, result.genre, result.type, q, false));
                };
                variant ("as found", [] (Profile&) {});
                variant ("no OTT", [] (Profile& q) { q.ottDepth = 0.0f; });
                variant ("no reverb/delay", [] (Profile& q) { q.reverbMix = 0.0f; q.delayMix = 0.0f; });
                variant ("no spectral", [] (Profile& q) { q.spectral = 0.0f; });
                variant ("all three off", [] (Profile& q) { q.ottDepth = 0.0f; q.reverbMix = 0.0f; q.delayMix = 0.0f; q.spectral = 0.0f; });
                variant ("+ grit off", [] (Profile& q) { q.ottDepth = 0.0f; q.reverbMix = 0.0f; q.delayMix = 0.0f; q.spectral = 0.0f; q.grit = 0.0f; });
            }
            if (verbose && result.ok)
            {
                const auto& t = result.target;
                std::printf ("    reference: pitch %.0f Hz, %d notes over %.2f s%s, side %.1f %%, wobble %.2f Hz (%.1f dB), rendered at %.0f BPM\n", t.pitchHz,
                             (int) t.notes.size(), t.noteSeconds, t.cut ? " (goes on)" : "", t.sidePct, t.moveRateHz, t.moveDepthDb, result.bpm);
                juce::String terms;
                SoundPrint::distance (result.matched, t, &terms);
                std::printf ("    match: distance %.2f = %s\n", result.distance, terms.toRawUTF8());
                // Loudness (dB under the loudest cell, summed over the mel bands) and
                // brightness (centroid band) every 8 tone frames, reference / match.
                auto frameOf = [] (const SoundPrint& s, size_t f, float& centroid)
                {
                    double power = 0.0, weighted = 0.0;
                    if (f < s.melTone.size())
                        for (int b = 0; b < SoundPrint::melBands; ++b)
                        {
                            const auto e = std::pow (10.0, s.melTone[f][(size_t) b] / 10.0);
                            power += e;
                            weighted += b * e;
                        }
                    centroid = power > 0.0 ? (float) (weighted / power) : 0.0f;
                    return (float) (10.0 * std::log10 (power + 1.0e-12));
                };
                std::printf ("    frame  ref dB  match dB  ref band  match band\n");
                for (size_t f = 0; f < juce::jmax (t.melTone.size(), result.matched.melTone.size()); f += 8)
                {
                    float rc, mc;
                    const auto r = frameOf (t, f, rc), m = frameOf (result.matched, f, mc);
                    std::printf ("    %5d  %6.1f  %8.1f  %8.1f  %10.1f\n", (int) f, r, m, rc, mc);
                }
            }
        }
        return 0;
    }

    if (command == "notes" && args.size() >= 2)
    {
        // Reference Match's transcription of a sound: the notes candidates will play.
        SoundPrint::debugNotes = true; // and why each detected note was kept or merged
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
        {
            std::printf ("  at %.2f s for %.2f s  %6.1f Hz", n.start, n.length, n.pitchHz);
            for (const auto hz : n.also)
                std::printf (" + %.1f", hz);
            std::printf ("\n");
        }
        return 0;
    }

    if (command == "library")
        return runLibrary (args);
    if (command == "gennetdata")
        return runGenerateNetData (args);
    if (command == "gennettrain")
        return runGenerateNetTrain (args);
    if (command == "gennetscreen")
        return runGenerateNetScreen (args);
    if (command == "netdata")
        return runNetData (args);
    if (command == "nettrain")
        return runNetTrain (args);
    if (command == "libbench")
        return runLibraryBench (args);

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

    if (command == "gendebug2" && args.size() >= 3)
    {
        // Generate's scoring for one profile, click by click: every keeper's terms.
        ZeroEngineProcessor engine;
        ZeroEngineProcessor::debugGenerate = true;
        auto* genre = engine.state.getParameter (ParamIDs::genre);
        auto* type = engine.state.getParameter (ParamIDs::type);
        genre->setValueNotifyingHost (genre->convertTo0to1 ((float) genreNames.indexOf (args[1])));
        type->setValueNotifyingHost (type->convertTo0to1 ((float) typeNames.indexOf (args[2])));
        engine.syncProfile();
        for (int s = 1; s <= 2; ++s)
        {
            std::printf ("click %d\n", s);
            engine.generate (s * 7919);
        }
        return 0;
    }

    if (command == "clarityaudit")
    {
        // Clarity (energy on the notes' harmonics) of each designed profile and of
        // 24 Generate variations of it: where generated sounds lose their tuning.
        ZeroEngineProcessor engine;
        engine.stopProfileSync();
        if (args.contains ("full")) // clarityaudit ... full: measure at full quality, as the plugin plays
            ZeroEngineProcessor::measureLight = false;
        if (args.size() >= 4) // clarityaudit <Genre> <Type>: each variation's settings that might smear it
        {
            const auto g = genreNames.indexOf (args[2]), t = typeNames.indexOf (args[3]);
            juce::Random random (g * 100 + t);
            for (int i = 0; i < 24; ++i)
            {
                const auto p = generateProfile (g, t, random);
                const auto m = ZeroEngineProcessor::measureForTools (engine, g, t, p);
                std::printf ("%.2f  off %+5.0f c  spread %4.0f c  flat %.3f  table %2d  unison %d detune %.2f  lfoPos %.2f  grit %.2f  cutoff %5.0f  filterEnv %.1f res %.2f\n",
                             m.clarity, m.pitchOffsetCents, m.pitchSpreadCents, m.flatness, (int) p.waveTable, (int) p.unison, p.detune, p.lfoToPos,
                             p.grit, p.filterCutoff, p.filterEnv, p.filterRes);
            }
            return 0;
        }
        std::printf ("%-24s %8s %8s %8s %8s\n", "profile", "designed", "var min", "var mean", "below-0.1");
        for (int g = 0; g < genreNames.size(); ++g)
            for (int t = 0; t < typeNames.size(); ++t)
            {
                const auto designed = ZeroEngineProcessor::measureForTools (engine, g, t, makeProfile (g, t)).clarity;
                juce::Random random (g * 100 + t);
                float lowest = 1.0f, sum = 0.0f;
                int worse = 0;
                for (int i = 0; i < 24; ++i)
                {
                    const auto c = ZeroEngineProcessor::measureForTools (engine, g, t, generateProfile (g, t, random)).clarity;
                    lowest = juce::jmin (lowest, c);
                    sum += c;
                    worse += c < designed - 0.1f;
                }
                std::printf ("%-24s %8.2f %8.2f %8.2f %8d\n", (genreNames[g] + " " + typeNames[t]).toRawUTF8(), designed, lowest, sum / 24.0f, worse);
            }
        return 0;
    }

    if (command == "mutatecheck")
    {
        // Mutate: how far each mutation lands from the sound it came from (in the
        // owner's spreads), against how far a fresh Generate click lands.
        ZeroEngineProcessor engine;
        auto* type = engine.state.getParameter (ParamIDs::type);
        type->setValueNotifyingHost (type->convertTo0to1 (1.0f)); // Lead
        engine.syncProfile();
        auto distance = [] (const TasteModel::Features& a, const TasteModel::Features& b)
        {
            double d = 0.0;
            for (int k = 0; k < TasteModel::numFeatures; ++k)
                d += juce::square ((double) a[(size_t) k] - b[(size_t) k]);
            return std::sqrt (d);
        };
        engine.generate (5);
        auto parent = *engine.getLastShownFeatures();
        for (int i = 0; i < 4; ++i)
        {
            engine.generate (100 + i, false, true);
            const auto child = *engine.getLastShownFeatures();
            engine.generate (200 + i);
            const auto fresh = *engine.getLastShownFeatures();
            std::printf ("mutation %d: %.2f from its parent; a fresh Generate %.2f from it\n", i + 1, distance (parent, child), distance (parent, fresh));
            engine.generate (5); // back to the same parent
            parent = *engine.getLastShownFeatures();
        }
        return 0;
    }

    if (command == "pluckcheck")
    {
        // The test's pluck loop (Techno Pluck, C4 Eb G F a quarter second apart),
        // rendered three times and read by SoundPrint: is it the same each time?
        for (int run = 0; run < 3; ++run)
        {
            ZeroEngineProcessor engine;
            engine.stopProfileSync();
            engine.setPlayConfigDetails (0, 2, 48000.0, 512);
            engine.prepareToPlay (48000.0, 512);
            auto p = makeProfile (genreNames.indexOf ("Techno"), typeNames.indexOf ("Pluck"));
            p.ampDecay = 0.12f; p.ampSustain = 0.0f; p.tilt -= 2.0f; p.waveTable = p.waveTable < 1.0f ? 2.0f : 0.0f; // as the test changes it
            engine.loadProfile (genreNames.indexOf ("Techno"), typeNames.indexOf ("Pluck"), p);
            juce::AudioBuffer<float> settle (2, 512), out (2, 72000), chunk (2, 512);
            juce::MidiBuffer midi;
            for (int i = 0; i < 10; ++i) { settle.clear(); engine.processBlock (settle, midi); }
            constexpr int steps[] { 0, 3, 7, 5 };
            for (int pos = 0; pos + 512 <= out.getNumSamples(); pos += 512)
            {
                midi.clear();
                for (int n = 0; n < 4; ++n)
                {
                    const auto at = n * 12000, off = at + 9600;
                    if (at >= pos && at < pos + 512) midi.addEvent (juce::MidiMessage::noteOn (1, 60 + steps[n], (juce::uint8) 110), at - pos);
                    if (off >= pos && off < pos + 512) midi.addEvent (juce::MidiMessage::noteOff (1, 60 + steps[n]), off - pos);
                }
                chunk.clear();
                engine.processBlock (chunk, midi);
                for (int ch = 0; ch < 2; ++ch)
                    out.copyFrom (ch, pos, chunk, ch, 0, 512);
            }
            const auto print = SoundPrint::of (out, 48000.0);
            std::printf ("run %d: pitch %.1f Hz, %d notes, rms %.5f, arp %s\n", run, print.pitchHz, (int) print.notes.size(), out.getRMSLevel (0, 0, out.getNumSamples()),
                         engine.state.getRawParameterValue (ParamIDs::arpOn)->load() > 0.5f ? "on" : "off");
            for (const auto& n : print.notes)
                std::printf ("   at %.2f %.1f Hz\n", n.start, n.pitchHz);
        }
        return 0;
    }

    if (command == "asynccheck")
    {
        // The tesseract's Generate in the background: how long the click itself
        // takes, and how long until the sound lands (message loop pumped).
        ZeroEngineProcessor engine;
        for (int i = 0; i < 4; ++i)
        {
            auto landed = false;
            const auto start = juce::Time::getMillisecondCounterHiRes();
            engine.generateAsync (i % 2 == 1, false, [&] { landed = true; });
            const auto clickMs = juce::Time::getMillisecondCounterHiRes() - start;
            while (! landed && juce::Time::getMillisecondCounterHiRes() - start < 5000.0)
                juce::MessageManager::getInstance()->runDispatchLoopUntil (5);
            std::printf ("click %.0f ms, sound landed after %.0f ms (%s), %s\n", clickMs, juce::Time::getMillisecondCounterHiRes() - start,
                         i % 2 == 1 ? "mood" : "genre", landed ? "ok" : "NEVER LANDED");
        }
        return 0;
    }

    if (command == "anycheck" && args.size() >= 2)
    {
        // The mini tesseract's genres: n any-genre clicks for a type, with the
        // judge on and off; which genre each loaded as.
        const auto t = typeNames.indexOf (args[1]);
        const auto n = args.size() > 2 ? args[2].getIntValue() : 12;
        for (int on = 1; on >= 0; --on)
        {
            ZeroEngineProcessor::useJudge = on == 1;
            ZeroEngineProcessor engine;
            auto* type = engine.state.getParameter (ParamIDs::type);
            type->setValueNotifyingHost (type->convertTo0to1 ((float) t));
            engine.syncProfile();
            juce::StringArray genres;
            for (int i = 0; i < n; ++i)
            {
                engine.generate (100 + i, true);
                genres.add (genreNames[(int) engine.state.getRawParameterValue (ParamIDs::genre)->load()]);
            }
            std::printf ("judge %s: %s\n", on ? "on " : "off", genres.joinIntoString (", ").toRawUTF8());
        }
        ZeroEngineProcessor::useJudge = true;
        return 0;
    }

    if (command == "genjudgeab")
        for (const auto& a : args)
            if (a.startsWith ("draw="))
                ZeroEngineProcessor::drawWithJudge = a.fromFirstOccurrenceOf ("=", false, false).getIntValue();
    if (command == "genjudgeab")
    {
        // Generate's final picks with the neural judge's screen and without, on
        // the same seeds: their size + interest against the designed sound
        // (Generate's own score, without novelty and taste), every profile, 3 clicks.
        ZeroEngineProcessor measurer;
        measurer.stopProfileSync();
        double sums[2] {}, variety[2] {};
        int wins = 0, losses = 0, count = 0;
        for (int g = 0; g < genreNames.size(); ++g)
            for (int t = 0; t < typeNames.size(); ++t)
            {
                const auto designed = ZeroEngineProcessor::measureForTools (measurer, g, t, makeProfile (g, t));
                double profileSums[2] {};
                std::vector<TasteModel::Features> pickedTraits[2];
                for (int on = 0; on < 2; ++on)
                {
                    ZeroEngineProcessor::useJudge = on == 1;
                    ZeroEngineProcessor engine;
                    auto* genre = engine.state.getParameter (ParamIDs::genre);
                    auto* type = engine.state.getParameter (ParamIDs::type);
                    genre->setValueNotifyingHost (genre->convertTo0to1 ((float) g));
                    type->setValueNotifyingHost (type->convertTo0to1 ((float) t));
                    engine.syncProfile();
                    for (int s = 1; s <= 3; ++s)
                    {
                        engine.generate (s * 7919 + g * 31 + t);
                        if (const auto picked = engine.getLastPickTraits())
                        {
                            profileSums[on] += ZeroEngineProcessor::sizeAndInterest (designed, *picked, t) / 3.0;
                            pickedTraits[on].push_back (TasteModel::featuresOf (*picked, t));
                        }
                    }
                }
                // Variety: the mean distance between the 3 picks, in the owner's spreads.
                for (int on = 0; on < 2; ++on)
                {
                    const auto& p = pickedTraits[on];
                    for (size_t i = 0; i < p.size(); ++i)
                        for (size_t j = i + 1; j < p.size(); ++j)
                        {
                            double d = 0.0;
                            for (int k = 0; k < TasteModel::numFeatures; ++k)
                                d += juce::square ((double) p[i][(size_t) k] - p[j][(size_t) k]);
                            variety[on] += std::sqrt (d) / 3.0;
                        }
                }
                sums[0] += profileSums[0];
                sums[1] += profileSums[1];
                wins += profileSums[1] > profileSums[0] + 0.1;
                losses += profileSums[1] < profileSums[0] - 0.1;
                ++count;
            }
        ZeroEngineProcessor::useJudge = true;
        std::printf ("variety (mean distance between a profile's picks): without the judge %.2f, with it %.2f\n", variety[0] / count, variety[1] / count);
        std::printf ("size + interest of Generate's picks: without the judge %+.2f, with it %+.2f (per profile: %d better, %d worse of %d)\n",
                     sums[0] / count, sums[1] / count, wins, losses, count);
        return 0;
    }

    if (command == "idlecheck")
    {
        // Silence with no keys: a fresh engine, after Generate clicks and after
        // genre/type changes, each followed by 3 s without MIDI; prints the
        // loudest sample of the last second (anything over -90 dB is a note
        // that shouldn't be there).
        constexpr double rate = 48000.0;
        constexpr int block = 512;
        ZeroEngineProcessor engine;
        engine.setUserFolder (juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("zero_idlecheck"));
        engine.setPlayConfigDetails (0, 2, rate, block);
        engine.prepareToPlay (rate, block);
        auto idle = [&] (const char* what)
        {
            juce::AudioBuffer<float> chunk (2, block);
            juce::MidiBuffer midi;
            float peak = 0.0f;
            for (int pos = 0, end = (int) (3 * rate); pos < end; pos += block)
            {
                chunk.clear();
                engine.processBlock (chunk, midi);
                if (pos >= end - (int) rate)
                    peak = juce::jmax (peak, chunk.getMagnitude (0, block));
            }
            std::printf ("%-36s last second's peak %6.1f dB%s\n", what, juce::Decibels::gainToDecibels (peak, -200.0f),
                         peak > 3.0e-5f ? "   <-- SOUNDING" : "");
        };
        idle ("fresh engine");
        auto* genre = engine.state.getParameter (ParamIDs::genre);
        auto* type = engine.state.getParameter (ParamIDs::type);
        // An on-screen key left down, then the preset changed: it must let go.
        engine.keyboardState.noteOn (1, 60, 0.8f);
        idle ("on-screen key held (should sound)");
        type->setValueNotifyingHost (type->convertTo0to1 (1.0f));
        engine.syncProfile();
        idle ("held, then the type changed (Lead)");
        for (int i = 0; i < 6; ++i)
        {
            engine.generate (900 + i, i % 2 == 1);
            idle (juce::String ("after Generate " + juce::String (i + 1) + (i % 2 ? " (mood)" : "")).toRawUTF8());
        }
        for (int g = 0; g < 12; ++g)
            for (int t = 0; t < 6; ++t)
            {
                genre->setValueNotifyingHost (genre->convertTo0to1 ((float) g));
                type->setValueNotifyingHost (type->convertTo0to1 ((float) t));
                engine.syncProfile();
                idle ((genreNames[g] + " " + typeNames[t]).toRawUTF8());
            }
        return 0;
    }

    if (command == "tastesim" && args.size() > 1 && args[1] == "settings")
    {
        // Does the settings taste learn what traits can't show? A simulated
        // listener who likes two wavetables (Analog, Jupiter) and nothing else
        // clicks Generate (EDM lead) 80 times: keeps what uses them, skips the
        // rest, hearts every third liked one. Run with the settings taste on
        // and off; prints how often the last 30 clicks gave a liked wavetable.
        const auto folder = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("zero_tastesim");
        const auto analog = (float) waveTableNames.indexOf ("Analog"), jupiter = (float) waveTableNames.indexOf ("Jupiter");
        for (const auto on : { false, true })
        {
            folder.deleteRecursively();
            ZeroEngineProcessor::useSettingsTaste = on;
            ZeroEngineProcessor engine;
            engine.setUserFolder (folder);
            auto* type = engine.state.getParameter (ParamIDs::type);
            type->setValueNotifyingHost (type->convertTo0to1 (1.0f)); // Lead
            engine.syncProfile();
            int liked = 0, early = 0, late = 0;
            for (int click = 0; click < 80; ++click)
            {
                engine.generate (4242 + click);
                const auto table = engine.state.getRawParameterValue (ParamIDs::waveTable)->load();
                const auto likes = table == analog || table == jupiter;
                if (likes && click < 30) ++early;
                if (likes && click >= 50) ++late;
                if (likes && ++liked % 3 == 0)
                    engine.heartCurrent();
                engine.simulateListening (likes ? 8.0f : 0.0f);
            }
            std::printf ("settings taste %-3s: liked wavetable in %2d of the first 30 clicks, %2d of the last 30 (judgments: %d, settings %d)\n",
                         on ? "on" : "off", early, late, engine.getTasteJudgments(), engine.getSettingsTasteJudgments());
        }
        ZeroEngineProcessor::useSettingsTaste = true;
        folder.deleteRecursively();
        return 0;
    }

    if (command == "tastesim")
    {
        // Does the taste model learn? A simulated listener who likes wide,
        // moving sounds (and dislikes dense ones) clicks the mini tesseract 80
        // times: plays what they like (kept), skips what they don't, hearts the
        // best. Prints what the model learned and whether results drift their
        // way. Everything under a temporary folder, never the owner's files.
        const auto folder = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("zero_tastesim");
        folder.deleteRecursively();
        ZeroEngineProcessor engine;
        ZeroEngineProcessor::useSettingsTaste = ! args.contains ("nosettings");
        engine.setUserFolder (folder);
        auto* type = engine.state.getParameter (ParamIDs::type);
        type->setValueNotifyingHost (type->convertTo0to1 (1.0f)); // Lead
        engine.syncProfile();

        TasteModel::Features liking {};
        liking[1] = 1.0f;   // width
        liking[7] = 1.0f;   // movement
        liking[6] = 0.6f;   // crest: likes less dense (higher crest)
        const char* names[] { "level", "width", "bright", "bass", "body", "spread", "crest", "move", "rich" };
        auto utility = [&] (const TasteModel::Features& f)
        {
            float u = 0.0f;
            for (int k = 0; k < TasteModel::numFeatures; ++k)
                u += liking[(size_t) k] * f[(size_t) k];
            return u;
        };
        double early = 0.0, late = 0.0;
        for (int click = 0; click < 80; ++click)
        {
            engine.generate (777 + click, true);
            const auto f = engine.getLastShownFeatures();
            if (! f.has_value())
                continue;
            const auto u = utility (*f);
            if (click < 20) early += u;
            if (click >= 60) late += u;
            if (u > 1.5f)
                engine.heartCurrent();
            engine.simulateListening (u > 0.0f ? 8.0f : 0.0f); // kept or skipped
        }
        std::printf ("judgments: %d\nlearned weights (truth in brackets):\n", engine.getTasteJudgments());
        const auto w = engine.getTasteWeights();
        for (int k = 0; k < TasteModel::numFeatures; ++k)
            std::printf ("  %-7s %+5.2f  (%+.1f)\n", names[k], w[(size_t) k], liking[(size_t) k]);
        std::printf ("listener's liking of results: first 20 clicks %.2f, last 20 %.2f\n", early / 20.0, late / 20.0);
        const auto likes = engine.getRecentLikes();
        std::printf ("recent likes (%d): %s\n", likes.size(), likes.joinIntoString (", ").toRawUTF8());
        if (! likes.isEmpty())
            std::printf ("recall \"%s\": %s, hearted %s\n", likes[0].toRawUTF8(), engine.recallLike (likes[0]) ? "ok" : "FAILED",
                         engine.isCurrentHearted() ? "yes" : "no");
        folder.deleteRecursively();
        return 0;
    }

    if (command == "genab")
    {
        // A blind listening test of Generate: per profile (each genre once, the
        // type cycling), pairs of a plain variation (generateProfile, what the old
        // Generate mostly gave) and the new Generate's pick, as a short phrase.
        // Each pair's two files are x and y in random order; key.csv says which.
        const auto folder = juce::File::getCurrentWorkingDirectory().getChildFile (args.size() > 1 ? args[1] : "analysis/generate_ab");
        const auto pairs = args.size() > 2 ? args[2].getIntValue() : 2;
        folder.deleteRecursively();
        folder.createDirectory();
        constexpr double rate = 48000.0;
        constexpr int block = 512;
        juce::Random shuffle (42);
        juce::String key ("pair,genre,type,x,y\n");

        auto phrase = [&] (ZeroEngineProcessor& engine, int type)
        {
            struct Note { double start, length; int note; };
            std::vector<Note> notes;
            constexpr int riff[] { 36, 36, 43, 39, 36, 36, 41, 39 };
            constexpr int line[] { 72, 75, 79, 77, 75, 72, 70, 72, 72, 75, 79, 82, 80, 79, 77, 75 };
            if (type == 0) // bass riff
                for (int i = 0; i < 8; ++i)
                    notes.push_back ({ i * 0.5, 0.42, riff[i] });
            else if (type == 1 || type == 4) // lead / pluck line
                for (int i = 0; i < 16; ++i)
                    notes.push_back ({ i * 0.25, type == 4 ? 0.18 : 0.23, line[i] });
            else if (type == 2 || type == 3) // chords: Cm Ab Eb Bb
            {
                const int chords[4][3] { { 60, 63, 67 }, { 56, 60, 63 }, { 55, 58, 63 }, { 58, 62, 65 } };
                for (int c = 0; c < 4; ++c)
                    for (const auto n : chords[c])
                        notes.push_back ({ c * 1.0, 0.95, n });
            }
            else
                notes.push_back ({ 0.0, 2.5, 60 }); // FX: one long hit
            juce::AudioBuffer<float> out (2, (int) (5.5 * rate)), chunk (2, block);
            juce::MidiBuffer midi;
            for (int i = 0; i < 10; ++i)
            {
                chunk.clear();
                engine.processBlock (chunk, midi);
            }
            for (int pos = 0; pos + block <= out.getNumSamples(); pos += block)
            {
                midi.clear();
                for (const auto& n : notes)
                {
                    const auto on = (int) (n.start * rate), off = (int) ((n.start + n.length) * rate);
                    if (on >= pos && on < pos + block)
                        midi.addEvent (juce::MidiMessage::noteOn (1, n.note, (juce::uint8) 105), on - pos);
                    if (off >= pos && off < pos + block)
                        midi.addEvent (juce::MidiMessage::noteOff (1, n.note), off - pos);
                }
                chunk.clear();
                engine.processBlock (chunk, midi);
                for (int ch = 0; ch < 2; ++ch)
                    out.copyFrom (ch, pos, chunk, ch, 0, block);
            }
            return out;
        };
        auto save = [&] (const juce::AudioBuffer<float>& audio, const juce::File& file)
        {
            juce::WavAudioFormat wav;
            std::unique_ptr<juce::AudioFormatWriter> writer (wav.createWriterFor (new juce::FileOutputStream (file), rate, 2, 24, {}, 0));
            if (writer != nullptr)
                writer->writeFromAudioSampleBuffer (audio, 0, audio.getNumSamples());
        };

        int pair = 0;
        for (int g = 0; g < genreNames.size(); ++g)
        {
            const auto t = g % typeNames.size();
            ZeroEngineProcessor engine;
            engine.setPlayConfigDetails (0, 2, rate, block);
            engine.prepareToPlay (rate, block);
            auto* genre = engine.state.getParameter (ParamIDs::genre);
            auto* type = engine.state.getParameter (ParamIDs::type);
            genre->setValueNotifyingHost (genre->convertTo0to1 ((float) g));
            type->setValueNotifyingHost (type->convertTo0to1 ((float) t));
            engine.syncProfile();
            for (int p = 0; p < pairs; ++p)
            {
                const auto seed = 1000 + 97 * g + p;
                juce::Random random (seed);
                engine.resetForRender();
                engine.loadProfile (g, t, generateProfile (g, t, random));
                const auto plain = phrase (engine, t);
                engine.resetForRender();
                engine.generate (seed);
                const auto picked = phrase (engine, t);

                const auto newIsX = shuffle.nextBool();
                const auto name = juce::String (++pair).paddedLeft ('0', 2) + "_" + genreNames[g].removeCharacters (" &") + "_" + typeNames[t];
                save (newIsX ? picked : plain, folder.getChildFile (name + "_x.wav"));
                save (newIsX ? plain : picked, folder.getChildFile (name + "_y.wav"));
                key << pair << "," << genreNames[g] << "," << typeNames[t] << "," << (newIsX ? "new,old" : "old,new") << "\n";
                std::printf ("%s\n", name.toRawUTF8());
            }
        }
        // And Mood mode: 18 clicks in a row (every mood about three times), named
        // by mood, each played as its type's phrase.
        {
            ZeroEngineProcessor engine;
            engine.setPlayConfigDetails (0, 2, rate, block);
            engine.prepareToPlay (rate, block);
            for (int p = 0; p < 18; ++p)
            {
                engine.resetForRender();
                engine.generate (5000 + 31 * p, true);
                const auto t = (int) engine.state.getRawParameterValue (ParamIDs::type)->load();
                const auto name = "mood_" + juce::String (p + 1).paddedLeft ('0', 2) + "_"
                                + engine.getMatchStatus().replaceCharacter (' ', '_').toLowerCase();
                save (phrase (engine, t), folder.getChildFile (name + ".wav"));
                std::printf ("%s\n", name.toRawUTF8());
            }
        }
        folder.getChildFile ("key.csv").replaceWithText (key);
        std::printf ("wrote %d pairs to %s (key.csv says which is new)\n", pair, folder.getFullPathName().toRawUTF8());
        return 0;
    }

    if (command == "gencheck")
    {
        // Generate's render-and-check: time per click and how many of the 24
        // candidates still fit the genre/type, 3 clicks per profile.
        std::printf ("%-24s %8s %s\n", "profile", "ms/click", "fits of 24");
        double totalMs = 0.0;
        int totalFits = 0, clicks = 0, totalOctave = 0;
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
                std::printf ("%-24s %8.0f %s  octave failures %d\n", (genreNames[g] + " " + typeNames[t]).toRawUTF8(), ms, fits.toRawUTF8(), engine.getOctaveFailures());
                totalOctave += engine.getOctaveFailures();
            }
        std::printf ("\nAverage %.0f ms per click; %.1f of 24 candidates fit on average; %d of %d picks failed the octave check.\n",
                     totalMs / clicks, (double) totalFits / clicks, totalOctave, clicks);
        return 0;
    }

    if (command == "wavestrip" && args.size() >= 2)
    {
        // The gravity waves after a click: the Generate screen at 6 moments.
        TesseractButton tesseract;
        tesseract.setSize (420, 220);
        const double moments[] { 0.6, 1.0, 1.5, 2.0, 2.7, 3.4 };
        juce::Image strip (juce::Image::ARGB, 420 * 3, 220 * 2, true);
        {
            juce::Graphics g (strip);
            for (size_t i = 0; i < std::size (moments); ++i)
            {
                tesseract.showWavesAt (moments[i]);
                g.drawImageAt (tesseract.createComponentSnapshot (tesseract.getLocalBounds()), 420 * (int) (i % 3), 220 * (int) (i / 3));
            }
        }
        juce::FileOutputStream out (argFile (args, 1, {}));
        out.setPosition (0);
        out.truncate();
        juce::PNGImageFormat().writeImageToStream (strip, out);
        std::printf ("Wrote %s\n", argFile (args, 1, {}).getFullPathName().toRawUTF8());
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
