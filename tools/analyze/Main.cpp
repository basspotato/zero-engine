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
#include "../../Source/FinishingChain.h"
#include "../../Source/PluginEditor.h"
#include "../../Source/Parameters.h"
#include "../../Source/SoundPrint.h"

namespace
{
    // The editor's AcidStrip and Wavetable display, for the UI checks.
    void findEditorParts (juce::Component& root, AcidStrip*& strip, ScopeView*& scope)
    {
        if (auto* s = dynamic_cast<AcidStrip*> (&root)) strip = s;
        if (auto* s = dynamic_cast<ScopeView*> (&root)) scope = s;
        for (auto* child : root.getChildren())
            findEditorParts (*child, strip, scope);
    }

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

    if (command == "acidcheck")
        return runAcidCheck();

    if (command == "arpsync")
        return runArpSyncCheck();

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
        type->setValueNotifyingHost (type->convertTo0to1 (args.size() > 2 ? (float) typeNames.indexOf (args[2]) : 1.0f)); // Lead unless given
        if (args.size() > 2)
        {
            auto* genre = engine.state.getParameter (ParamIDs::genre);
            genre->setValueNotifyingHost (genre->convertTo0to1 ((float) genreNames.indexOf (args[1].replace ("_", " "))));
        }
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

    if (command == "traits" && args.size() >= 2)
    {
        // SoundTraits (as Generate measures them, no notes) of every WAV in a folder, as CSV.
        juce::AudioFormatManager formats;
        formats.registerBasicFormats();
        std::printf ("file,level,side,centroid,bass,body,spread,crest,movement,richness\n");
        for (const auto& file : juce::File::getCurrentWorkingDirectory().getChildFile (args[1]).findChildFiles (juce::File::findFiles, false, "*.wav"))
        {
            std::unique_ptr<juce::AudioFormatReader> reader (formats.createReaderFor (file));
            if (reader == nullptr)
                continue;
            juce::AudioBuffer<float> audio (2, (int) reader->lengthInSamples);
            reader->read (&audio, 0, audio.getNumSamples(), 0, true, true);
            const auto t = SoundTraits::of (audio, reader->sampleRate);
            std::printf ("%s", file.getFileName().toRawUTF8());
            for (const auto v : t.asArray())
                std::printf (",%.3f", v);
            std::printf ("\n");
        }
        return 0;
    }

    if (command == "crushcheck")
    {
        // Generate picks squashed into the clipper: per profile, two clicks; a pick
        // is crushed when its crest sits under the type's floor (the owner's median
        // less 1.5 spreads, as Generate's score has it). The designed sounds too.
        int picks = 0, crushed = 0, designedCrushed = 0;
        for (int g = 0; g < genreNames.size(); ++g)
            for (int t = 0; t < typeNames.size(); ++t)
            {
                auto owned = std::make_unique<ZeroEngineProcessor>();
                auto& engine = *owned;
                auto* genre = engine.state.getParameter (ParamIDs::genre);
                auto* type = engine.state.getParameter (ParamIDs::type);
                genre->setValueNotifyingHost (genre->convertTo0to1 ((float) g));
                type->setValueNotifyingHost (type->convertTo0to1 ((float) t));
                engine.syncProfile();
                const auto designed = ZeroEngineProcessor::measureForTools (engine, g, t, makeProfile (g, t));
                designedCrushed += ZeroEngineProcessor::crushedOf (designed, t) > 0.0f;
                juce::String line;
                for (int s = 0; s < 2; ++s)
                {
                    engine.generate (300 + 7 * s + 11 * g + t);
                    if (const auto m = engine.getLastPickTraits())
                    {
                        ++picks;
                        const auto c = ZeroEngineProcessor::crushedOf (*m, t) > 0.0f;
                        crushed += c;
                        line << juce::String (m->crestDb, 1) << (c ? "* " : "  ");
                    }
                }
                std::printf ("%-24s designed crest %5.1f%s  picks %s\n", (genreNames[g] + " " + typeNames[t]).toRawUTF8(), designed.crestDb,
                             ZeroEngineProcessor::crushedOf (designed, t) > 0.0f ? "*" : " ", line.toRawUTF8());
            }
        std::printf ("crushed (*): %d of %d picks; %d of 72 designed sounds\n", crushed, picks, designedCrushed);
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
        // A latched arp plays on through a new sound until it's switched off.
        {
            auto set = [&] (const char* id, float v) { auto* p = engine.state.getParameter (id); p->setValueNotifyingHost (p->convertTo0to1 (v)); };
            set (ParamIDs::arpOn, 1.0f);
            set (ParamIDs::arpLatch, 1.0f);
            engine.keyboardState.noteOn (1, 60, 0.8f);
            idle ("latched arp, key held (should sound)");
            engine.keyboardState.noteOff (1, 60, 0.0f);
            engine.generate (4321);
            idle ("latched arp after Generate (should sound)");
            set (ParamIDs::arpOn, 0.0f);
            set (ParamIDs::arpLatch, 0.0f);
            engine.syncProfile();
            idle ("arp switched off");
        }
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
        // And Mood mode: 24 clicks in a row (every mood about three times), named
        // by mood, each played as its type's phrase.
        {
            ZeroEngineProcessor engine;
            engine.setPlayConfigDetails (0, 2, rate, block);
            engine.prepareToPlay (rate, block);
            for (int p = 0; p < 24; ++p)
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
        // And Mutate: 8 sounds (types in turn), each as a (the Generate result) and b
        // (one MUTATE click on it).
        for (int m = 0; m < 8; ++m)
        {
            const auto g = (3 * m + 1) % genreNames.size(), t = m % typeNames.size();
            auto owned = std::make_unique<ZeroEngineProcessor>(); // on the heap: main's stack is full of engines already
            auto& engine = *owned;
            engine.setPlayConfigDetails (0, 2, rate, block);
            engine.prepareToPlay (rate, block);
            auto* genre = engine.state.getParameter (ParamIDs::genre);
            auto* type = engine.state.getParameter (ParamIDs::type);
            genre->setValueNotifyingHost (genre->convertTo0to1 ((float) g));
            type->setValueNotifyingHost (type->convertTo0to1 ((float) t));
            engine.syncProfile();
            engine.resetForRender();
            engine.generate (7000 + 13 * m);
            const auto name = "mutate_" + juce::String (m + 1).paddedLeft ('0', 2) + "_" + genreNames[g].removeCharacters (" &") + "_" + typeNames[t];
            save (phrase (engine, t), folder.getChildFile (name + "_a.wav"));
            engine.resetForRender();
            engine.generate (8000 + 13 * m, false, true);
            save (phrase (engine, t), folder.getChildFile (name + "_b.wav"));
            std::printf ("%s\n", name.toRawUTF8());
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
        // Full size, one under another, with the time each frame takes to draw.
        TesseractButton tesseract;
        constexpr int w = 912, h = 226;
        tesseract.setSize (w, h);
        const double moments[] { 0.7, 1.0, 1.4, 2.0, 2.8, 3.7, 6.0 }; // the last after the waves, for the time
        juce::Image strip (juce::Image::ARGB, w, h * (int) std::size (moments), true);
        {
            juce::Graphics g (strip);
            for (size_t i = 0; i < std::size (moments); ++i)
            {
                tesseract.showWavesAt (moments[i]);
                const auto start = juce::Time::getMillisecondCounterHiRes();
                const auto frame = tesseract.createComponentSnapshot (tesseract.getLocalBounds(), true, 2.0f);
                std::printf ("%.1f s: %.1f ms at 2x\n", moments[i], juce::Time::getMillisecondCounterHiRes() - start);
                g.drawImage (frame, juce::Rectangle<float> (0.0f, (float) (h * (int) i), (float) w, (float) h));
            }
        }
        juce::FileOutputStream out (argFile (args, 1, {}));
        out.setPosition (0);
        out.truncate();
        juce::PNGImageFormat().writeImageToStream (strip, out);
        std::printf ("Wrote %s\n", argFile (args, 1, {}).getFullPathName().toRawUTF8());
        return 0;
    }

    if (command == "uicheck")
    {
        // The 303 knobs' health: is anything in front of them (a hit test over
        // each knob's face), and how long the pieces take to paint, in ms, against
        // the 16.7 ms a 60 Hz frame allows.
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
        for (int i = 0; i < 10; ++i)
        {
            engine.processBlock (block, midi);
            juce::MessageManager::getInstance()->runDispatchLoopUntil (30);
        }

        std::function<void (juce::Component&, std::vector<juce::Component*>&)> collect = [&] (juce::Component& c, std::vector<juce::Component*>& all)
        {
            all.push_back (&c);
            for (auto* child : c.getChildren())
                collect (*child, all);
        };
        std::vector<juce::Component*> all;
        collect (*editor, all);
        AcidStrip* strip = nullptr;
        ScopeView* scopeView = nullptr;
        findEditorParts (*editor, strip, scopeView);
        if (strip == nullptr || scopeView == nullptr)
            return 1;
        editor->setVisible (true); // hit tests only see a shown component

        // The 303 knobs: the sliders whose bottom edge sits just above the strip.
        int knobs = 0, blocked = 0;
        for (auto* c : all)
            if (auto* knob = dynamic_cast<juce::Slider*> (c); knob != nullptr && knob->getParentComponent() == editor.get()
                && knob->getBottom() <= strip->getY() && knob->getBottom() >= strip->getY() - 3 && knob->getX() >= strip->getX() - 2)
            {
                ++knobs;
                const auto centre = knob->getBounds().getCentre().toFloat();
                const auto radius = (float) knob->getWidth() * 0.5f;
                for (int step = 0; step < 17; ++step)
                {
                    // The centre, then rings at 0.5, 0.8 and 0.95 of the radius, every 45 degrees.
                    const auto ring = step == 0 ? 0.0f : step <= 8 ? 0.5f : step <= 16 ? 0.8f : 0.95f;
                    const auto angle = (float) (step % 8) * juce::MathConstants<float>::pi * 0.25f;
                    const auto at = (centre + juce::Point<float> (std::cos (angle), std::sin (angle)) * (ring * radius)).toInt();
                    auto* hit = editor->getComponentAt (at);
                    if (hit != knob)
                    {
                        ++blocked;
                        std::printf ("  knob at x=%d: a point %.0f%% out is %s\n", knob->getX(), ring * 100.0f,
                                     hit == nullptr ? "the editor" : typeid (*hit).name());
                    }
                }
            }
        std::printf ("303 knobs found: %d; points where something else is in front: %d\n", knobs, blocked);

        auto timeIt = [] (const std::function<void()>& work, int n)
        {
            work(); // warm up
            const auto start = juce::Time::getMillisecondCounterHiRes();
            for (int i = 0; i < n; ++i)
                work();
            return (juce::Time::getMillisecondCounterHiRes() - start) / n;
        };
        // Painted into plain software images, so the time is the drawing's own (not a graphics card's).
        auto paintMs = [&] (juce::Component& c, int n)
        {
            juce::Image image (juce::Image::ARGB, juce::jmax (1, c.getWidth()), juce::jmax (1, c.getHeight()), true, juce::SoftwareImageType());
            return timeIt ([&] { juce::Graphics g (image); c.paintEntireComponent (g, false); }, n);
        };
        const auto stripMs = paintMs (*strip, 100);
        // The display: Spectrum (the default, 30 per second) and the still Wavetable picture (only when the settings change).
        scopeView->setMode (ScopeView::spectrum);
        const auto spectrumMs = paintMs (*scopeView, 100);
        scopeView->setMode (ScopeView::wavetable);
        const auto tableMs = paintMs (*scopeView, 100);
        const auto editorMs = paintMs (*editor, 20);
        TesseractButton* tesseract = nullptr;
        for (auto* c : all)
            if (auto* t = dynamic_cast<TesseractButton*> (c); t != nullptr && (tesseract == nullptr || t->getWidth() > tesseract->getWidth()))
                tesseract = t;
        const auto screenMs = tesseract != nullptr ? paintMs (*tesseract, 20) : 0.0;
        std::printf ("paint: 303 strip %.2f ms (30 per second while 303 mode is on), Spectrum display %.2f ms (30 per second), "
                     "Wavetable picture %.2f ms (only when the settings change), space screen with the tesseract %.1f ms (60 per second), "
                     "whole editor %.1f ms\n", stripMs, spectrumMs, tableMs, screenMs, editorMs);
        std::printf ("as a share of one core (software drawing, which reads about 1.4x the real app's):  strip %.0f%%,  spectrum %.0f%%,  space screen %.0f%%\n",
                     stripMs * 3.0, spectrumMs * 3.0, screenMs * 6.0);
        return 0;
    }

    if (command == "acidline")
    {
        // 303 mode on a classic 16-step acid line (130 BPM, accents, slides),
        // saw and square, plus a slow sweep of Cutoff and Env Mod over 8 bars;
        // WAVs for listening, each checked for clicks as clickcheck does.
        const auto dir = argFile (args, 1, "analysis/acid");
        dir.createDirectory();
        constexpr double rate = 48000.0, bpm = 130.0;
        constexpr int block = 256;
        const auto stepSamples = (int) (rate * 60.0 / bpm / 4.0);
        struct Step { int note; bool accent, slide, rest; };
        const Step line[16] { { 36, true, false, false }, { 36, false, false, false }, { 48, false, true, false },  { 36, false, false, false },
                              { 39, true, false, false }, { 36, false, false, false }, { 0, false, false, true },   { 43, false, true, false },
                              { 36, true, false, false }, { 34, false, false, false }, { 36, false, false, false }, { 48, true, true, false },
                              { 36, false, false, false }, { 39, false, false, false }, { 41, true, false, false },  { 36, false, false, false } };

        auto render = [&] (const juce::String& name, bool square, int bars, std::function<void (ZeroEngineProcessor&, double)> automate)
        {
            auto owned = std::make_unique<ZeroEngineProcessor>();
            auto& engine = *owned;
            engine.setUserFolder (juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("zero_acidline"));
            auto set = [&] (const char* id, float v) { auto* p = engine.state.getParameter (id); p->setValueNotifyingHost (p->convertTo0to1 (v)); };
            set (ParamIDs::genre, (float) genreNames.indexOf ("Techno"));
            set (ParamIDs::type, (float) typeNames.indexOf ("Bass"));
            engine.syncProfile();
            set (ParamIDs::acidFilter, 1.0f);
            set (ParamIDs::acidWave, square ? 1.0f : 0.0f);
            set (ParamIDs::acidCutoff, -0.5f);
            set (ParamIDs::acidRes, 0.6f);
            auto option = [&] (const char* key, float fallback)
            {
                for (const auto& a : args)
                    if (a.startsWith (juce::String (key) + "="))
                        return a.fromFirstOccurrenceOf ("=", false, false).getFloatValue();
                return fallback;
            };
            set (ParamIDs::acidEnvMod, option ("env", 0.7f));
            set (ParamIDs::acidCutoff, option ("cut", -0.5f));
            set (ParamIDs::acidDecay, 0.35f);
            set (ParamIDs::acidAccent, 0.8f);
            if (args.contains ("dry")) // the 303 voice alone, without the loaded sound's finishing
            {
                set (ParamIDs::grit, 0.0f);
                set (ParamIDs::tilt, 0.0f);
                set (ParamIDs::air, 0.0f);
                set (ParamIDs::reverbMix, 0.0f);
                set (ParamIDs::delayMix, 0.0f);
            }
            engine.setPlayConfigDetails (0, 2, rate, block);
            engine.prepareToPlay (rate, block);

            const auto total = bars * 16 * stepSamples + (int) rate;
            juce::AudioBuffer<float> out (2, (total / block) * block), chunk (2, block);
            juce::MidiBuffer midi;
            for (int pos = 0; pos + block <= out.getNumSamples(); pos += block)
            {
                if (automate)
                    automate (engine, (double) pos / (bars * 16 * stepSamples));
                midi.clear();
                for (int s = 0; s < bars * 16; ++s)
                {
                    const auto& step = line[s % 16];
                    if (step.rest)
                        continue;
                    const auto on = s * stepSamples;
                    const auto off = on + (step.slide ? stepSamples + stepSamples / 8 : stepSamples * 6 / 10); // a slide overlaps the next note
                    if (on >= pos && on < pos + block)
                        midi.addEvent (juce::MidiMessage::noteOn (1, step.note, (juce::uint8) (step.accent ? 120 : 80)), on - pos);
                    if (off >= pos && off < pos + block)
                        midi.addEvent (juce::MidiMessage::noteOff (1, step.note), off - pos);
                }
                chunk.clear();
                engine.processBlock (chunk, midi);
                for (int ch = 0; ch < 2; ++ch)
                    out.copyFrom (ch, pos, chunk, ch, 0, block);
            }

            // Clicks, as clickcheck: a sample-to-sample jump against the loudest within 20 ms.
            const auto* l = out.getReadPointer (0);
            const auto n = out.getNumSamples();
            std::vector<float> d ((size_t) n, 0.0f);
            for (int i = 1; i < n; ++i)
                d[(size_t) i] = std::abs (l[i] - l[i - 1]);
            const auto reach = (int) (0.02 * rate), self = 5;
            float worst = 0.0f;
            double worstAt = 0.0;
            for (int i = reach; i < n - reach; ++i)
            {
                if (d[(size_t) i] < 1.0e-3f)
                    continue;
                float around = 0.0f;
                for (int j = i - reach; j <= i + reach; ++j)
                    if (std::abs (j - i) > self)
                        around = juce::jmax (around, d[(size_t) j]);
                if (const auto r = d[(size_t) i] / juce::jmax (1.0e-4f, around); r > worst) { worst = r; worstAt = i / rate; }
            }

            const auto file = dir.getChildFile (name + ".wav");
            file.deleteFile();
            if (auto stream = std::unique_ptr<juce::OutputStream> (file.createOutputStream()))
                if (auto writer = std::unique_ptr<juce::AudioFormatWriter> (juce::WavAudioFormat().createWriterFor (stream.get(), rate, 2, 24, {}, 0)))
                {
                    stream.release();
                    writer->writeFromAudioSampleBuffer (out, 0, out.getNumSamples());
                }
            std::printf ("%-16s peak %.2f  worst jump %.1fx at %.3f s%s\n", name.toRawUTF8(), out.getMagnitude (0, out.getNumSamples()),
                         worst, worstAt, worst > 4.0f ? "  << click" : "");
        };

        render ("acid_saw", false, 4, {});
        render ("acid_square", true, 4, {});
        render ("acid_sweep", false, 8, [] (ZeroEngineProcessor& engine, double t)
        {
            auto set = [&] (const char* id, float v) { auto* p = engine.state.getParameter (id); p->setValueNotifyingHost (p->convertTo0to1 (v)); };
            set (ParamIDs::acidCutoff, (float) (-4.0 + 8.0 * juce::jmin (1.0, t)));            // closed to open over 8 bars
            set (ParamIDs::acidEnvMod, (float) (0.2 + 0.8 * std::sin (juce::MathConstants<double>::pi * juce::jmin (1.0, t))));
        });
        std::printf ("Wrote %s\n", dir.getFullPathName().toRawUTF8());
        return 0;
    }

    if (command == "clickcheck")
    {
        // Clicks in Generate's picks: per profile, 3 picks each play two notes (on,
        // off, on again while the first tail rings); a click is a jump between
        // neighbouring samples far above the sound's own (the 99.9th percentile
        // of sample-to-sample change over the steady part). Prints the worst with
        // their envelope settings.
        const auto withArp = args.contains ("arp");
        constexpr double rate = 48000.0;
        constexpr int block = 512;
        struct Found { float ratio; juce::String what; };
        std::vector<Found> found;
        int picks = 0;
        for (int g = 0; g < genreNames.size(); ++g)
            for (int t = 0; t < typeNames.size(); ++t)
            {
                auto owned = std::make_unique<ZeroEngineProcessor>();
                auto& engine = *owned;
                engine.setUserFolder (juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("zero_clickcheck"));
                engine.setPlayConfigDetails (0, 2, rate, block);
                engine.prepareToPlay (rate, block);
                auto set = [&] (const char* id, float v) { auto* p = engine.state.getParameter (id); p->setValueNotifyingHost (p->convertTo0to1 (v)); };
                set (ParamIDs::genre, (float) g);
                set (ParamIDs::type, (float) t);
                engine.syncProfile();
                for (int s = 0; s < 3; ++s)
                {
                    engine.generate (900 + 31 * g + 7 * t + s);
                    set (ParamIDs::arpOn, withArp ? 1.0f : 0.0f); // "arp": a held chord through the arp instead of two notes
                    engine.resetForRender();
                    const auto note = t == 0 ? 36 : 60;
                    juce::AudioBuffer<float> out (2, (int) (2.5 * rate)), chunk (2, block);
                    juce::MidiBuffer midi;
                    for (int pos = 0; pos + block <= out.getNumSamples(); pos += block)
                    {
                        midi.clear();
                        auto at = [&] (double sec) { const auto i = (int) (sec * rate); return i >= pos && i < pos + block ? i - pos : -1; };
                        if (withArp)
                        {
                            for (const auto n : { note, note + 3, note + 7 })
                            {
                                if (const auto i = at (0.1); i >= 0) midi.addEvent (juce::MidiMessage::noteOn (1, n, (juce::uint8) 110), i);
                                if (const auto i = at (2.0); i >= 0) midi.addEvent (juce::MidiMessage::noteOff (1, n), i);
                            }
                        }
                        else
                        {
                            if (const auto i = at (0.1); i >= 0) midi.addEvent (juce::MidiMessage::noteOn (1, note, (juce::uint8) 110), i);
                            if (const auto i = at (0.9); i >= 0) midi.addEvent (juce::MidiMessage::noteOff (1, note), i);
                            if (const auto i = at (1.0); i >= 0) midi.addEvent (juce::MidiMessage::noteOn (1, note + 7, (juce::uint8) 110), i);
                            if (const auto i = at (1.6); i >= 0) midi.addEvent (juce::MidiMessage::noteOff (1, note + 7), i);
                        }
                        chunk.clear();
                        engine.processBlock (chunk, midi);
                        for (int ch = 0; ch < 2; ++ch)
                            out.copyFrom (ch, pos, chunk, ch, 0, block);
                    }
                    ++picks;
                    // A click stands out from the sound right around it: each sample-to-
                    // sample change against the loudest change within 20 ms either side
                    // (leaving out its own 0.1 ms), over the rendered part only.
                    const auto* l = out.getReadPointer (0);
                    const auto filled = (out.getNumSamples() / block) * block;
                    std::vector<float> d ((size_t) filled, 0.0f);
                    for (int i = 1; i < filled; ++i)
                        d[(size_t) i] = std::abs (l[i] - l[i - 1]);
                    const auto reach = (int) (0.02 * rate), self = 5; // longer than a low bass cycle, whose own edges aren't clicks
                    float ratio = 0.0f;
                    double worstAt = 0.0;
                    for (int i = reach; i < filled - reach; ++i)
                    {
                        if (d[(size_t) i] < 1.0e-3f)
                            continue;
                        float around = 0.0f;
                        for (int j = i - reach; j <= i + reach; ++j)
                            if (std::abs (j - i) > self)
                                around = juce::jmax (around, d[(size_t) j]);
                        if (const auto r = d[(size_t) i] / juce::jmax (1.0e-4f, around); r > ratio) { ratio = r; worstAt = i / rate; }
                    }
                    if (ratio > 4.0f)
                    {
                        auto value = [&] (const char* id) { return engine.state.getRawParameterValue (id)->load(); };
                        found.push_back ({ ratio, (genreNames[g] + " " + typeNames[t] + " pick " + juce::String (s + 1)).paddedRight (' ', 26)
                                                  + " click " + juce::String (ratio, 1) + "x at " + juce::String (worstAt, 3) + " s"
                                                  + "  attack " + juce::String (value (ParamIDs::ampAttack) * 1000.0f, 1) + " ms"
                                                  + "  release " + juce::String (value (ParamIDs::ampRelease) * 1000.0f, 0) + " ms"
                                                  + "  play " + playModeNames[(int) value (ParamIDs::playMode)]
                                                  + "  table " + waveTableNames[(int) value (ParamIDs::waveTable)]
                                                  + "  lfo " + lfoShapeNames[(int) value (ParamIDs::lfoShape)] + " sync " + juce::String (value (ParamIDs::lfoSync), 0)
                                                  + " >cut " + juce::String (value (ParamIDs::lfoToCutoff), 2) + " >pos " + juce::String (value (ParamIDs::lfoToPos), 2)
                                                  + "  modDecay " + juce::String (value (ParamIDs::modDecay) * 1000.0f, 0) + " ms env " + juce::String (value (ParamIDs::filterEnv), 1)
                                                  + "  grit " + juce::String (value (ParamIDs::grit), 2) + " sub " + juce::String (value (ParamIDs::subOn), 0) });
                    }
                }
            }
        std::sort (found.begin(), found.end(), [] (const Found& a, const Found& b) { return a.ratio > b.ratio; });
        std::printf ("%d of %d picks have a click (a jump over 4x anything within 20 ms):\n", (int) found.size(), picks);
        for (size_t i = 0; i < juce::jmin<size_t> (25, found.size()); ++i)
            std::printf ("  %s\n", found[i].what.toRawUTF8());
        return 0;
    }

    if (command == "peek" && args.size() >= 3)
    {
        // The samples of a WAV around a moment (left channel), with their second
        // difference: what a click actually looks like.
        juce::AudioFormatManager formats;
        formats.registerBasicFormats();
        std::unique_ptr<juce::AudioFormatReader> reader (formats.createReaderFor (argFile (args, 1, {})));
        if (reader == nullptr)
            return 1;
        const auto centre = (juce::int64) (args[2].getDoubleValue() * reader->sampleRate);
        const auto half = args.size() > 3 ? args[3].getIntValue() : 24;
        juce::AudioBuffer<float> buffer (1, 2 * half + 2);
        reader->read (&buffer, 0, buffer.getNumSamples(), centre - half - 2, true, false);
        for (int i = 2; i < buffer.getNumSamples(); ++i)
        {
            const auto x = buffer.getSample (0, i);
            const auto d2 = x - 2.0f * buffer.getSample (0, i - 1) + buffer.getSample (0, i - 2);
            std::printf ("%+6d  %+.5f  d2 %+.5f %s\n", i - half - 2, x, d2, std::abs (d2) > 0.01f ? "<" : "");
        }
        return 0;
    }

    if (command == "clickhunt")
    {
        // A stricter click search than clickcheck, played like a person: per
        // profile, two Generate picks play a staccato 1/8 line, a legato line
        // (overlapping notes), chord changes and fast repeated 1/16s. A click is a
        // spike in the second difference (a break in the waveform) against the
        // largest within 20 ms around it, on either side. Lists every pick over
        // the threshold (default 3x) with its settings; the worst 12 go to
        // analysis/clicks as WAVs named with the spike's time.
        const auto threshold = args.size() > 1 ? juce::jmax (1.5f, args[1].getFloatValue()) : 3.0f;
        const auto dir = juce::File::getCurrentWorkingDirectory().getChildFile ("analysis/clicks");
        dir.deleteRecursively();
        dir.createDirectory();
        constexpr double rate = 48000.0;
        constexpr int block = 512;
        struct Found { float ratio; double at; juce::String what; juce::AudioBuffer<float> audio; };
        std::vector<Found> found;
        int picks = 0;
        for (int g = 0; g < genreNames.size(); ++g)
            for (int t = 0; t < typeNames.size(); ++t)
            {
                auto owned = std::make_unique<ZeroEngineProcessor>();
                auto& engine = *owned;
                engine.setUserFolder (juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("zero_clickhunt"));
                engine.setPlayConfigDetails (0, 2, rate, block);
                engine.prepareToPlay (rate, block);
                auto set = [&] (const char* id, float v) { auto* p = engine.state.getParameter (id); p->setValueNotifyingHost (p->convertTo0to1 (v)); };
                set (ParamIDs::genre, (float) g);
                set (ParamIDs::type, (float) t);
                engine.syncProfile();
                for (int s = 0; s < 2; ++s)
                {
                    engine.generate (4400 + 37 * g + 11 * t + s);
                    set (ParamIDs::arpOn, 0.0f);
                    if (args.contains ("dry")) // the voices alone: is a click theirs or the finishing chain's?
                        for (auto* id : { ParamIDs::grit, ParamIDs::ottDepth, ParamIDs::spectral, ParamIDs::reverbMix, ParamIDs::delayMix })
                            set (id, 0.0f);
                    if (args.contains ("only") && ! (typeNames[t] == "Bass" && (genreNames[g] == "Pop" || genreNames[g] == "Drum & Bass")))
                        continue;
                    engine.resetForRender();
                    const auto base = t == 0 ? 36 : 57;

                    // (on, off, note, velocity) in seconds.
                    struct Note { double on, off; int note, velocity; };
                    std::vector<Note> notes;
                    const int melody[] { 0, 3, 7, 10, 12, 10, 7, 3 };
                    for (int i = 0; i < 8; ++i) // staccato 1/8s
                        notes.push_back ({ 0.25 * i, 0.25 * i + 0.18, base + melody[i], 80 + 5 * i });
                    for (int i = 0; i < 8; ++i) // legato: each overlaps the next by 50 ms
                        notes.push_back ({ 2.0 + 0.25 * i, 2.0 + 0.25 * i + 0.3, base + melody[7 - i], 120 - 5 * i });
                    const int roots[] { 0, 5, 3, 7 };
                    for (int c = 0; c < 4; ++c) // chord changes, 20 ms overlap
                        for (const auto interval : { 0, 4, 7 })
                            notes.push_back ({ 4.0 + 0.5 * c, 4.0 + 0.5 * c + 0.52, base + 12 * (t == 0 ? 0 : 0) + roots[c] + interval, 100 });
                    for (int i = 0; i < 12; ++i) // fast repeated 1/16s
                        notes.push_back ({ 6.0 + 0.125 * i, 6.0 + 0.125 * i + 0.06, base + 12, i % 4 == 0 ? 120 : 85 });

                    juce::AudioBuffer<float> out (2, (int) (8.5 * rate / block) * block), chunk (2, block);
                    juce::MidiBuffer midi;
                    for (int pos = 0; pos + block <= out.getNumSamples(); pos += block)
                    {
                        midi.clear();
                        for (const auto& n : notes)
                        {
                            const auto on = (int) (n.on * rate), off = (int) (n.off * rate);
                            if (on >= pos && on < pos + block)
                                midi.addEvent (juce::MidiMessage::noteOn (1, n.note, (juce::uint8) n.velocity), on - pos);
                            if (off >= pos && off < pos + block)
                                midi.addEvent (juce::MidiMessage::noteOff (1, n.note), off - pos);
                        }
                        chunk.clear();
                        engine.processBlock (chunk, midi);
                        for (int ch = 0; ch < 2; ++ch)
                            out.copyFrom (ch, pos, chunk, ch, 0, block);
                    }
                    ++picks;

                    float worst = 0.0f;
                    double worstAt = 0.0;
                    const auto n = out.getNumSamples();
                    const auto reach = (int) (0.02 * rate), self = 4;
                    std::vector<float> d ((size_t) n, 0.0f);
                    for (int ch = 0; ch < 2; ++ch)
                    {
                        const auto* x = out.getReadPointer (ch);
                        for (int i = 2; i < n; ++i)
                            d[(size_t) i] = std::abs (x[i] - 2.0f * x[i - 1] + x[i - 2]);
                        // Running max of the neighbourhood, by blocks of `self` (fast enough).
                        for (int i = reach; i < n - reach; ++i)
                        {
                            if (d[(size_t) i] < 2.0e-4f)
                                continue;
                            float around = 0.0f;
                            for (int j = i - reach; j <= i + reach; ++j)
                                if (std::abs (j - i) > self && d[(size_t) j] > around)
                                    around = d[(size_t) j];
                            if (const auto r = d[(size_t) i] / juce::jmax (5.0e-5f, around); r > worst) { worst = r; worstAt = i / rate; }
                        }
                    }
                    if (worst > threshold)
                    {
                        auto value = [&] (const char* id) { return engine.state.getRawParameterValue (id)->load(); };
                        const auto section = worstAt < 2.0 ? "staccato" : worstAt < 4.0 ? "legato" : worstAt < 6.0 ? "chords" : worstAt < 7.5 ? "16ths" : "tail";
                        found.push_back ({ worst, worstAt, (genreNames[g] + " " + typeNames[t] + " pick " + juce::String (s + 1)).paddedRight (' ', 24)
                                                  + juce::String (worst, 1) + "x at " + juce::String (worstAt, 3) + " s (" + section + ")"
                                                  + "  play " + playModeNames[(int) value (ParamIDs::playMode)]
                                                  + "  atk " + juce::String (value (ParamIDs::ampAttack) * 1000.0f, 1) + " rel " + juce::String (value (ParamIDs::ampRelease) * 1000.0f, 0)
                                                  + "  res " + juce::String (value (ParamIDs::filterRes), 2) + " env " + juce::String (value (ParamIDs::filterEnv), 1)
                                                  + " modDec " + juce::String (value (ParamIDs::modDecay) * 1000.0f, 0)
                                                  + "  glide " + juce::String (value (ParamIDs::glide) * 1000.0f, 0)
                                                  + "  grit " + juce::String (value (ParamIDs::grit), 2) + " ott " + juce::String (value (ParamIDs::ottDepth), 2)
                                                  + " spec " + juce::String (value (ParamIDs::spectral), 2)
                                                  + "  lfo>cut " + juce::String (value (ParamIDs::lfoToCutoff), 2) + " >pos " + juce::String (value (ParamIDs::lfoToPos), 2),
                                          out });
                    }
                }
                std::fprintf (stderr, "\r%s %s   ", genreNames[g].toRawUTF8(), typeNames[t].toRawUTF8());
            }
        std::sort (found.begin(), found.end(), [] (const Found& a, const Found& b) { return a.ratio > b.ratio; });
        std::printf ("\n%d of %d picks over %.1fx:\n", (int) found.size(), picks, threshold);
        for (size_t i = 0; i < found.size(); ++i)
        {
            std::printf ("  %s\n", found[i].what.toRawUTF8());
            if (i < 12)
            {
                const auto file = dir.getChildFile (juce::String (i + 1).paddedLeft ('0', 2) + " at " + juce::String (found[i].at, 3) + "s.wav");
                if (auto stream = std::unique_ptr<juce::OutputStream> (file.createOutputStream()))
                    if (auto writer = std::unique_ptr<juce::AudioFormatWriter> (juce::WavAudioFormat().createWriterFor (stream.get(), rate, 2, 24, {}, 0)))
                    {
                        stream.release();
                        writer->writeFromAudioSampleBuffer (found[i].audio, 0, found[i].audio.getNumSamples());
                    }
            }
        }
        return 0;
    }

    if (command == "tastereport")
    {
        // What the owner's taste models have learned (read only): the trait
        // weights, and the settings the settings taste leans towards and away from.
        const auto folder = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory).getChildFile ("Zero Engine");
        TasteModel traits;
        traits.load (folder.getChildFile ("Taste.xml"));
        std::printf ("trait taste, %d judgments (+ = likes more of it):\n", traits.judgments());
        const char* traitNames[] { "level", "width", "brightness", "bass", "body", "spread", "density(crest)", "movement", "richness" };
        const auto w = traits.meanWeights();
        for (int k = 1; k < TasteModel::numFeatures; ++k)
            std::printf ("  %-15s %+5.2f\n", traitNames[k], w[(size_t) k]);

        SettingsTaste settings;
        settings.load (folder.getChildFile ("TasteSettings.xml"));
        settings.refit();
        juce::StringArray names { "wave position", "tilt", "air", "OTT", "grit", "stereo width", "weight", "detune", "attack", "decay", "sustain",
                                  "release", "filter cutoff", "resonance", "filter env", "mod decay", "reverb", "delay", "LFO rate", "LFO>cutoff",
                                  "LFO>position", "delay feedback" };
        for (const auto& t : waveTableNames) names.add ("table " + t);
        for (const auto& t : waveTableNames) names.add ("2nd table " + t);
        for (const auto& m : filterModeNames) names.add ("filter " + m);
        for (int i = 0; i < 5; ++i) names.add ("LFO shape " + juce::String (i));
        for (const auto& t : typeNames) names.add ("type " + t);
        for (const auto& g : genreNames) names.add ("genre " + g);
        for (const auto* n : { "Weight macro", "Grit macro", "Punch macro", "Width macro", "Motion macro", "Air macro", "unison blend", "table mix",
                               "keytrack", "sub on", "reverb size", "spectral", "unison voices", "mod>position", "mod>pitch", "LFO>pitch", "LFO sync",
                               "delay time", "level trim", "mono below", "mod attack", "mod sustain" })
            names.add (n);
        const auto& sw = settings.weights();
        std::vector<std::pair<float, int>> ranked;
        for (size_t i = 0; i < sw.size(); ++i)
            ranked.push_back ({ sw[i], (int) i });
        std::sort (ranked.begin(), ranked.end());
        std::printf ("\nsettings taste, %d judgments (%zu settings):\n  leans towards:", settings.judgments(), sw.size());
        for (size_t i = ranked.size(); i-- > ranked.size() - 10 && i < ranked.size();)
            std::printf ("\n    %+5.2f  %s", ranked[i].first, names[ranked[i].second].toRawUTF8());
        std::printf ("\n  leans away from:");
        for (size_t i = 0; i < 10 && i < ranked.size(); ++i)
            std::printf ("\n    %+5.2f  %s", ranked[i].first, names[ranked[i].second].toRawUTF8());
        std::printf ("\n");
        return 0;
    }

    if (command == "sweep")
    {
        // Off-style picks across every genre/type: 8 Generate picks each against
        // the designed sound; flags types whose picks drift brighter (median over
        // an octave), wider, louder or squashed, with the tables they used.
        struct Row { float drift; juce::String text; };
        std::vector<Row> rows;
        for (int g = 0; g < genreNames.size(); ++g)
            for (int t = 0; t < typeNames.size(); ++t)
            {
                auto owned = std::make_unique<ZeroEngineProcessor>();
                auto& engine = *owned;
                engine.setUserFolder (juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("zero_sweep"));
                auto set = [&] (const char* id, float v) { auto* p = engine.state.getParameter (id); p->setValueNotifyingHost (p->convertTo0to1 (v)); };
                set (ParamIDs::genre, (float) g);
                set (ParamIDs::type, (float) t);
                engine.syncProfile();
                const auto designed = ZeroEngineProcessor::measureForTools (engine, g, t, makeProfile (g, t));
                std::vector<float> octaves, width, crest;
                std::map<juce::String, int> tables;
                for (int i = 0; i < 8; ++i)
                {
                    engine.generate (700 + 13 * i + g);
                    if (const auto m = engine.getLastPickTraits())
                    {
                        octaves.push_back (std::log2 (juce::jmax (20.0f, m->centroidHz) / juce::jmax (20.0f, designed.centroidHz)));
                        width.push_back (m->sidePct - designed.sidePct);
                        crest.push_back (m->crestDb);
                        ++tables[waveTableNames[(int) engine.state.getRawParameterValue (ParamIDs::waveTable)->load()]];
                    }
                }
                if (octaves.empty())
                    continue;
                auto median = [] (std::vector<float> v) { std::sort (v.begin(), v.end()); return v[v.size() / 2]; };
                const auto bright = median (octaves), wider = median (width);
                juce::String used;
                for (const auto& [name, n] : tables)
                    used << name << " " << n << "  ";
                const auto drift = juce::jmax (bright, wider / 20.0f);
                rows.push_back ({ drift, (genreNames[g] + " " + typeNames[t]).paddedRight (' ', 20) + " brighter " + juce::String (bright, 2) + " oct"
                                         + "  wider " + juce::String (wider, 1) + " pts  crest " + juce::String (median (crest), 1) + " dB  tables: " + used });
            }
        std::sort (rows.begin(), rows.end(), [] (const Row& a, const Row& b) { return a.drift > b.drift; });
        std::printf ("Generate picks against the designed sound (median of 8), most drifted first:\n");
        for (const auto& r : rows)
            std::printf ("  %s\n", r.text.toRawUTF8());
        return 0;
    }

    if (command == "picks" && args.size() >= 3)
    {
        // One genre/type: the designed sound and 8 Generate picks, with the
        // wavetable, filter and the measured brightness of each.
        auto owned = std::make_unique<ZeroEngineProcessor>();
        auto& engine = *owned;
        const auto g = genreNames.indexOf (args[1].replace ("_", " ")), t = typeNames.indexOf (args[2]);
        if (g < 0 || t < 0)
            return 1;
        auto set = [&] (const char* id, float v) { auto* p = engine.state.getParameter (id); p->setValueNotifyingHost (p->convertTo0to1 (v)); };
        set (ParamIDs::genre, (float) g);
        set (ParamIDs::type, (float) t);
        engine.syncProfile();
        auto show = [&] (const char* what, const SoundTraits& m)
        {
            const auto table = (int) engine.state.getRawParameterValue (ParamIDs::waveTable)->load();
            std::printf ("%-10s %-9s pos %.2f  cutoff %6.0f Hz  unison %.0f  centroid %6.0f Hz  side %4.1f%%\n", what, waveTableNames[table].toRawUTF8(),
                         engine.state.getRawParameterValue (ParamIDs::wavePos)->load(), engine.state.getRawParameterValue (ParamIDs::filterCutoff)->load(),
                         engine.state.getRawParameterValue (ParamIDs::unison)->load(), m.centroidHz, m.sidePct);
        };
        show ("designed", ZeroEngineProcessor::measureForTools (engine, g, t, makeProfile (g, t)));
        for (int i = 0; i < 8; ++i)
        {
            engine.generate (600 + 17 * i);
            if (const auto m = engine.getLastPickTraits())
                show (("pick " + juce::String (i + 1)).toRawUTF8(), *m);
        }
        return 0;
    }

    if (command == "easeab")
    {
        // The pads and chords that were squashed into the saturation, as designed
        // now (with their Input Trim) and before (none): a held chord pair each, to
        // analysis/ease_ab, with numbers.
        constexpr double rate = 48000.0;
        constexpr int block = 512;
        const auto folder = juce::File::getCurrentWorkingDirectory().getChildFile ("analysis/ease_ab");
        folder.createDirectory();
        const std::pair<const char*, const char*> presets[] { { "Hardstyle", "Pad" }, { "Dubstep", "Chord" }, { "Drum & Bass", "Pad" },
                                                              { "Hardstyle", "Chord" }, { "Techno", "Pad" }, { "Dubstep", "Pad" } };
        for (const auto& [genreName, typeName] : presets)
            for (const auto before : { true, false })
            {
                auto owned = std::make_unique<ZeroEngineProcessor>();
                auto& engine = *owned;
                engine.setPlayConfigDetails (0, 2, rate, block);
                engine.prepareToPlay (rate, block);
                const auto g = genreNames.indexOf (genreName), t = typeNames.indexOf (typeName);
                auto p = makeProfile (g, t);
                if (before)
                    p.inputTrim = 0.0f;
                engine.loadProfile (g, t, p);
                juce::AudioBuffer<float> out (2, (int) (5.0 * rate)), chunk (2, block);
                juce::MidiBuffer midi;
                const int chords[2][3] { { 60, 63, 67 }, { 56, 60, 63 } };
                for (int pos = 0; pos + block <= out.getNumSamples(); pos += block)
                {
                    midi.clear();
                    for (int c = 0; c < 2; ++c)
                        for (const auto n : chords[c])
                        {
                            const auto on = (int) (c * 2.0 * rate), off = on + (int) (1.8 * rate);
                            if (on >= pos && on < pos + block) midi.addEvent (juce::MidiMessage::noteOn (1, n, (juce::uint8) 105), on - pos);
                            if (off >= pos && off < pos + block) midi.addEvent (juce::MidiMessage::noteOff (1, n), off - pos);
                        }
                    chunk.clear();
                    engine.processBlock (chunk, midi);
                    for (int ch = 0; ch < 2; ++ch)
                        out.copyFrom (ch, pos, chunk, ch, 0, block);
                }
                const auto d = analyse (out, rate);
                const auto file = folder.getChildFile (juce::String (genreName).removeCharacters (" &") + "_" + typeName
                                                       + (before ? "_before.wav" : "_now.wav"));
                juce::WavAudioFormat wav;
                std::unique_ptr<juce::AudioFormatWriter> writer (wav.createWriterFor (new juce::FileOutputStream (file), rate, 2, 24, {}, 0));
                if (writer != nullptr)
                    writer->writeFromAudioSampleBuffer (out, 0, out.getNumSamples());
                std::printf ("%-36s LUFS %6.1f  PLR %5.1f  centroid %5.0f\n", file.getFileName().toRawUTF8(), d.lufs, d.plr, d.centroidHz);
            }
        return 0;
    }

    if (command == "delayab")
    {
        // The delay before and after it stopped leaning left: delay-heavy presets
        // playing a short line, once with the old ping-pong (left line first) and
        // once with the cross-fed stereo delay, to analysis/delay_ab.
        constexpr double rate = 48000.0;
        constexpr int block = 512;
        const auto folder = juce::File::getCurrentWorkingDirectory().getChildFile ("analysis/delay_ab");
        folder.deleteRecursively();
        folder.createDirectory();
        const std::pair<const char*, const char*> presets[] { { "Trance", "Lead" }, { "Trance", "Pluck" }, { "Synthwave", "Lead" }, { "Future Bass", "Chord" } };
        constexpr int line[] { 72, 75, 79, 77, 75, 72, 70, 72 };
        for (const auto& [genreName, typeName] : presets)
            for (const auto old : { true, false })
            {
                FinishingChain::legacyPingPong = old;
                auto owned = std::make_unique<ZeroEngineProcessor>();
                auto& engine = *owned;
                engine.setPlayConfigDetails (0, 2, rate, block);
                engine.prepareToPlay (rate, block);
                auto set = [&] (const char* id, float v) { auto* p = engine.state.getParameter (id); p->setValueNotifyingHost (p->convertTo0to1 (v)); };
                set (ParamIDs::genre, (float) genreNames.indexOf (genreName));
                set (ParamIDs::type, (float) typeNames.indexOf (typeName));
                engine.syncProfile();
                juce::AudioBuffer<float> out (2, (int) (6.0 * rate)), chunk (2, block);
                juce::MidiBuffer midi;
                const auto down = juce::String (typeName) == "Chord" ? 12 : 0;
                for (int pos = 0; pos + block <= out.getNumSamples(); pos += block)
                {
                    midi.clear();
                    for (int n = 0; n < 8; ++n)
                    {
                        const auto on = (int) (n * 0.375 * rate), off = on + (int) (0.3 * rate);
                        if (on >= pos && on < pos + block) midi.addEvent (juce::MidiMessage::noteOn (1, line[n] - down, (juce::uint8) 105), on - pos);
                        if (off >= pos && off < pos + block) midi.addEvent (juce::MidiMessage::noteOff (1, line[n] - down), off - pos);
                    }
                    chunk.clear();
                    engine.processBlock (chunk, midi);
                    for (int ch = 0; ch < 2; ++ch)
                        out.copyFrom (ch, pos, chunk, ch, 0, block);
                }
                const auto file = folder.getChildFile (juce::String (genreName).removeCharacters (" ") + "_" + typeName + (old ? "_old_pingpong.wav" : "_new_centred.wav"));
                juce::WavAudioFormat wav;
                std::unique_ptr<juce::AudioFormatWriter> writer (wav.createWriterFor (new juce::FileOutputStream (file), rate, 2, 24, {}, 0));
                if (writer != nullptr)
                    writer->writeFromAudioSampleBuffer (out, 0, out.getNumSamples());
                std::printf ("%s\n", file.getFileName().toRawUTF8());
            }
        FinishingChain::legacyPingPong = false;
        return 0;
    }

    if (command == "arpstrip" && args.size() >= 2)
    {
        // The arp page's pattern display while an arp runs (Trance Pluck, a held
        // C minor chord, some steps transposed), at four moments, stacked.
        constexpr double rate = 48000.0;
        constexpr int block = 256;
        auto owned = std::make_unique<ZeroEngineProcessor>();
        auto& engine = *owned;
        engine.setPlayConfigDetails (0, 2, rate, block);
        engine.prepareToPlay (rate, block);
        auto set = [&] (const juce::String& id, float v) { auto* p = engine.state.getParameter (id); p->setValueNotifyingHost (p->convertTo0to1 (v)); };
        set (ParamIDs::genre, (float) genreNames.indexOf ("Trance"));
        set (ParamIDs::type, (float) typeNames.indexOf ("Pluck"));
        engine.syncProfile();
        set (ParamIDs::arpOn, 1.0f);
        const int transposed[][2] { { 2, 7 }, { 5, -5 }, { 9, 12 }, { 13, 3 }, { 14, -12 } };
        for (const auto& t : transposed)
            set (SeqIDs::pitch (t[0]), (float) t[1]);
        ArpScope scope (engine.state, [&engine] { return engine.getSequenceStep(); },
                        [&engine] (float* dest, int n) { engine.copyScope (dest, n); });
        scope.setSize (900, 260);
        const double moments[] { 1.2, 1.5, 1.8, 2.4 };
        juce::Image strip (juce::Image::ARGB, 900, 260 * (int) std::size (moments), true);
        juce::AudioBuffer<float> audio (2, block);
        juce::MidiBuffer midi;
        double t = 0.0, nextFrame = 0.0;
        size_t shot = 0;
        while (shot < std::size (moments))
        {
            midi.clear();
            if (t == 0.0)
                for (auto note : { 60, 63, 67 })
                    midi.addEvent (juce::MidiMessage::noteOn (1, note, (juce::uint8) 110), 0);
            audio.clear();
            engine.processBlock (audio, midi);
            t += block / rate;
            if (t >= nextFrame)
            {
                scope.previewFrame (t);
                nextFrame += 1.0 / 30.0;
            }
            if (t >= moments[shot])
            {
                juce::Graphics g (strip);
                g.drawImageAt (scope.createComponentSnapshot (scope.getLocalBounds()), 0, 260 * (int) shot);
                ++shot;
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
            if (args[a].startsWith ("display=")) // the display's view: display=wavetable, waveform or spectrum
            {
                const auto id = args[a].endsWith ("wavetable") ? (int) ScopeView::wavetable : args[a].endsWith ("waveform") ? (int) ScopeView::waveform : (int) ScopeView::spectrum;
                std::function<void (juce::Component&)> pick = [&] (juce::Component& c)
                {
                    if (auto* box = dynamic_cast<juce::ComboBox*> (&c); box != nullptr && box->getNumItems() == 3 && box->getItemText (0) == "Wavetable") // the display's menu, not genre or type
                        box->setSelectedId (id, juce::sendNotificationSync);
                    for (auto* child : c.getChildren())
                        pick (*child);
                };
                pick (*editor);
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
