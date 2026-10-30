#include "MatchLibrary.h"
#include "MatchBench.h"
#include "MatchJudge.h"
#include <juce_audio_formats/juce_audio_formats.h>
#include <map>
#include <random>
#include "../../Source/PluginProcessor.h"
#include "../../Source/Parameters.h"
#include "../../Source/ReferenceMatch.h"
#include "../../Source/SoundPrint.h"
#include "../../Source/SoundTraits.h"

namespace
{
    constexpr double rate = 48000.0;

    juce::File libraryFolder() { return juce::File::getCurrentWorkingDirectory().getChildFile ("analysis/library"); }
    juce::File manifestFile()  { return libraryFolder().getChildFile ("library.csv"); }

    // One sound in the library. Engine renders also carry what remakes them.
    struct Entry
    {
        juce::String file, source, licence, type, use;
        int genre = -1, seed = 0;
        bool loop = false;
    };

    juce::String csvField (const juce::String& s) { return "\"" + s.replace ("\"", "\"\"") + "\""; }

    juce::StringArray parseCsvLine (const juce::String& line)
    {
        juce::StringArray fields;
        juce::String current;
        bool quoted = false;
        for (int i = 0; i < line.length(); ++i)
        {
            const auto c = line[i];
            if (quoted)
            {
                if (c == '"' && line[i + 1] == '"') { current << '"'; ++i; }
                else if (c == '"') quoted = false;
                else current << juce::String::charToString (c);
            }
            else if (c == '"') quoted = true;
            else if (c == ',') { fields.add (current); current.clear(); }
            else current << juce::String::charToString (c);
        }
        fields.add (current);
        return fields;
    }

    void writeManifest (const std::vector<Entry>& entries)
    {
        juce::String out ("file,source,licence,type,use,genre,seed,loop\n");
        for (const auto& e : entries)
            out << csvField (e.file) << "," << csvField (e.source) << "," << csvField (e.licence) << "," << csvField (e.type) << ","
                << csvField (e.use) << "," << e.genre << "," << e.seed << "," << (e.loop ? 1 : 0) << "\n";
        libraryFolder().createDirectory();
        manifestFile().replaceWithText (out);
    }

    std::vector<Entry> readManifest()
    {
        std::vector<Entry> entries;
        juce::StringArray lines;
        manifestFile().readLines (lines);
        for (int i = 1; i < lines.size(); ++i)
        {
            if (lines[i].trim().isEmpty())
                continue;
            const auto f = parseCsvLine (lines[i]);
            if (f.size() < 8)
                continue;
            entries.push_back ({ f[0], f[1], f[2], f[3], f[4], f[5].getIntValue(), f[6].getIntValue(), f[7].getIntValue() != 0 });
        }
        return entries;
    }

    juce::String typeFromFolder (const juce::String& name)
    {
        for (const auto& t : typeNames)
            if (t.equalsIgnoreCase (name))
                return t;
        return "Unknown";
    }

    // NSynth's family names, for the synth-made notes worth matching.
    juce::String typeFromNSynth (const juce::String& family)
    {
        if (family == "bass") return "Bass";
        if (family == "synth_lead") return "Lead";
        if (family == "keyboard" || family == "mallet") return "Pluck";
        if (family == "organ" || family == "string") return "Pad";
        return {};
    }

    std::vector<Entry> buildManifest()
    {
        std::vector<Entry> entries;

        // Freesound: <Genre>/<Type>/*.wav, the licence from credits.csv: the first
        // fetch (analysis/freesound/refs) and the Match fetch (analysis/library/freesound).
        for (const auto& freesound : { juce::File::getCurrentWorkingDirectory().getChildFile ("analysis/freesound/refs"),
                                        libraryFolder().getChildFile ("freesound") })
        {
            std::map<juce::String, juce::String> licences;
            juce::StringArray lines;
            freesound.getChildFile ("credits.csv").readLines (lines);
            for (int i = 1; i < lines.size(); ++i)
            {
                const auto f = parseCsvLine (lines[i]);
                if (f.size() >= 8)
                    licences[f[7].replace ("\\", "/")] = f[5].contains ("publicdomain") ? "CC0" : "CC-BY";
            }
            for (const auto& file : freesound.findChildFiles (juce::File::findFiles, true, "*.wav"))
            {
                const auto relative = file.getRelativePathFrom (freesound).replace ("\\", "/");
                const auto licence = licences.count (relative) > 0 ? licences[relative] : juce::String ("CC0");
                entries.push_back ({ file.getFullPathName(), "freesound", licence, typeFromFolder (file.getParentDirectory().getFileName()), "test" });
            }
        }

        // The owner's Jupiter-8 recordings: <genre>/<type>/*.wav.
        for (const auto& file : juce::File ("c:/zero training").findChildFiles (juce::File::findFiles, true, "*.wav"))
            entries.push_back ({ file.getFullPathName(), "jupiter", "owner", typeFromFolder (file.getParentDirectory().getFileName()), "test" });

        // The owner's test files: the references, not what an old Match returned.
        for (const auto& file : juce::File ("F:/tests").findChildFiles (juce::File::findFiles, false, "*.wav"))
            if (! file.getFileName().containsIgnoreCase ("returned"))
                entries.push_back ({ file.getFullPathName(), "owner", "owner", "Unknown", "test" });

        // NSynth (CC BY 4.0), when downloaded: synth-made notes only, named
        // <family>_<source>_<id>-<pitch>-<velocity>.wav.
        for (const auto& file : libraryFolder().getChildFile ("nsynth").findChildFiles (juce::File::findFiles, true, "*.wav"))
        {
            const auto parts = juce::StringArray::fromTokens (file.getFileNameWithoutExtension(), "_", "");
            if (parts.size() < 3 || parts[1] != "synthetic")
                continue;
            const auto family = parts.size() > 3 && parts[0] == "synth" ? juce::String ("synth_lead") : parts[0];
            if (const auto type = typeFromNSynth (family); type.isNotEmpty())
                entries.push_back ({ file.getFullPathName(), "nsynth", "CC-BY", type, "test" });
        }

        // Scanned folders (library scan), as their kept lists stand.
        for (const auto& list : libraryFolder().getChildFile ("scans").findChildFiles (juce::File::findFiles, false, "*.csv"))
        {
            juce::StringArray lines;
            list.readLines (lines);
            for (int i = 1; i < lines.size(); ++i)
                if (const auto f = parseCsvLine (lines[i]); f.size() >= 8)
                    entries.push_back ({ f[0], f[1], f[2], f[3], f[4], -1, 0, f[7].getIntValue() != 0 });
        }

        // Engine renders, from their index.
        juce::StringArray lines;
        libraryFolder().getChildFile ("engine/index.csv").readLines (lines);
        for (int i = 1; i < lines.size(); ++i)
        {
            const auto f = parseCsvLine (lines[i]);
            if (f.size() >= 5)
                entries.push_back ({ f[0], "engine", "own", typeNames[f[2].getIntValue()], "train", f[1].getIntValue(), f[3].getIntValue(), f[4].getIntValue() != 0 });
        }
        return entries;
    }

    bool writeWav (const juce::File& file, const juce::AudioBuffer<float>& audio)
    {
        file.deleteFile();
        juce::WavAudioFormat wav;
        std::unique_ptr<juce::AudioFormatWriter> writer (wav.createWriterFor (new juce::FileOutputStream (file), rate, 2, 24, {}, 0));
        return writer != nullptr && writer->writeFromAudioSampleBuffer (audio, 0, audio.getNumSamples());
    }
}

int runLibraryTraits (const juce::StringArray& args);

int runLibrary (const juce::StringArray& args)
{
    if (args.size() >= 2 && args[1] == "traits")
        return runLibraryTraits (args);
    if (args.size() >= 3 && args[1] == "engine")
    {
        // n Generate variations, spread over every genre and type, half notes,
        // half arp loops; the seed remakes each one's settings exactly.
        const auto count = args[2].getIntValue();
        const auto folder = libraryFolder().getChildFile ("engine");
        folder.createDirectory();
        juce::String index ("file,genre,type,seed,loop\n");
        for (int i = 0; i < count; ++i)
        {
            const auto genre = i % genreNames.size(), type = (i / genreNames.size()) % typeNames.size();
            const auto loop = (i / (genreNames.size() * typeNames.size())) % 2 == 1;
            const auto seed = 5000 + i;
            juce::Random random (seed);
            const auto profile = generateProfile (genre, type, random);
            const auto file = folder.getChildFile (juce::String::formatted ("%04d %s %s %s.wav", i, genreNames[genre].toRawUTF8(),
                                                                            typeNames[type].toRawUTF8(), loop ? "loop" : "note"));
            writeWav (file, renderBenchSound (genre, type, profile, loop));
            index << csvField (file.getFullPathName()) << "," << genre << "," << type << "," << seed << "," << (loop ? 1 : 0) << "\n";
            if ((i + 1) % 50 == 0)
                std::printf ("rendered %d of %d\n", i + 1, count);
        }
        folder.getChildFile ("index.csv").replaceWithText (index);
        std::printf ("rendered %d engine sounds into %s\n", count, folder.getFullPathName().toRawUTF8());
        return 0;
    }

    if (args.size() >= 2 && args[1] == "build")
    {
        const auto entries = buildManifest();
        writeManifest (entries);
        std::map<juce::String, std::map<juce::String, int>> counts;
        for (const auto& e : entries)
            ++counts[e.source][e.type];
        for (const auto& [source, types] : counts)
        {
            std::printf ("%-10s", source.toRawUTF8());
            for (const auto& [type, n] : types)
                std::printf ("  %s %d", type.toRawUTF8(), n);
            std::printf ("\n");
        }
        std::printf ("%d sounds in %s\n", (int) entries.size(), manifestFile().getFullPathName().toRawUTF8());
        return 0;
    }

    if (args.size() >= 3 && args[1] == "scan")
    {
        // A folder of mixed samples (the owner's production folder), read in
        // place and never changed: keeps pitched synth notes and one-instrument
        // loops, labels each with a likely type, and drops drums, voices,
        // noise, mixes and anything too short or long. Test-only: commercial
        // pack licences may forbid AI training, so these never train anything.
        const juce::File folder (args[2]);
        if (! folder.isDirectory())
        {
            std::printf ("not a folder: %s\n", folder.getFullPathName().toRawUTF8());
            return 1;
        }
        const juce::StringArray skipWords { "kick", "snare", "hat", "clap", "perc", "drum", "cymbal", "crash", "ride", "tom", "shaker",
                                            "rim", "snap", "vox", "vocal", "voice", "acapella", "speech", "master", "mixdown", "bounce", "stem", "hihat", "fill", "top",
                                            "guitar", "piano", "sax", "flute", "brass", "trumpet", "violin", "cello", "choir", "acoustic" };
        const std::pair<const char*, const char*> typeWords[] { { "808", "Bass" }, { "bass", "Bass" }, { "sub", "Bass" }, { "reese", "Bass" },
                                                                { "lead", "Lead" }, { "pad", "Pad" }, { "atmos", "Pad" }, { "string", "Pad" },
                                                                { "pluck", "Pluck" }, { "arp", "Pluck" }, { "bell", "Pluck" }, { "chord", "Chord" },
                                                                { "stab", "Chord" }, { "riser", "FX" }, { "sweep", "FX" }, { "uplifter", "FX" },
                                                                { "downlifter", "FX" } };

        juce::AudioFormatManager formats;
        formats.registerBasicFormats();
        std::vector<Entry> kept;
        std::map<juce::String, int> dropped;
        const auto verboseScan = args.contains ("-v"); // each file named as it is read
        const auto files = folder.findChildFiles (juce::File::findFiles, true, "*.wav;*.aif;*.aiff;*.flac");
        for (int i = 0; i < files.size(); ++i)
        {
            if ((i + 1) % 250 == 0)
                std::printf ("  %d of %d looked at, %d kept\n", i + 1, files.size(), (int) kept.size());
            const auto& file = files.getReference (i);
            if (verboseScan)
                std::printf ("    %s\n", file.getFullPathName().toRawUTF8());
            std::fflush (stdout); // so a log shows where a scan stopped
            // Only the file's own name and its folder speak for it (a pack named
            // "Drumsound & Bassline" holds synths too), as whole words ("stem"
            // is not "System", "tom" not "Atmosphere"); a plural s counts.
            const auto path = (file.getParentDirectory().getFileName() + " " + file.getFileNameWithoutExtension()).toLowerCase();
            const auto words = juce::StringArray::fromTokens (path, " _-.,()[]&+#'", "");
            const auto named = [&] (const juce::String& w) { return words.contains (w) || words.contains (w + "s"); };
            if (std::any_of (skipWords.begin(), skipWords.end(), named))
            {
                ++dropped["named as drums, voice, acoustic or a mix"];
                continue;
            }
            std::unique_ptr<juce::AudioFormatReader> reader (formats.createReaderFor (file));
            if (reader == nullptr)
            {
                ++dropped["unreadable"];
                continue;
            }
            const auto seconds = (double) reader->lengthInSamples / reader->sampleRate;
            if (seconds < 0.2 || seconds > 15.0)
            {
                ++dropped["shorter than 0.2 s or longer than 15 s"];
                continue;
            }
            juce::AudioBuffer<float> audio (2, (int) juce::jmin<juce::int64> (reader->lengthInSamples, (juce::int64) (reader->sampleRate * 4.0)));
            reader->read (&audio, 0, audio.getNumSamples(), 0, true, true);
            const auto print = SoundPrint::of (audio, reader->sampleRate, true);
            if (print.levelDb < -60.0f || print.notes.empty() || print.notes.front().pitchHz <= 0.0f)
            {
                ++dropped["no clear pitch (drums, noise, chords, mixes)"];
                continue;
            }
            if (print.notes.size() > 16)
            {
                ++dropped["too busy for one instrument"];
                continue;
            }

            juce::String type;
            for (const auto& [word, t] : typeWords)
                if (path.contains (word))
                {
                    type = t;
                    break;
                }
            if (type.isEmpty()) // from the sound: register, phrase, and how its loudness moves
            {
                // The tone spectrogram's frames (512 samples apart): falling 15 dB
                // within half a second of its loudest is a pluck; reaching its
                // loudest after 150 ms is a slow pad swell; otherwise a held lead.
                std::vector<float> loudness;
                for (const auto& frame : print.melTone)
                {
                    double power = 0.0;
                    for (auto v : frame)
                        power += std::pow (10.0, v / 10.0);
                    loudness.push_back ((float) (10.0 * std::log10 (power + 1.0e-12)));
                }
                const auto peakAt = (int) (std::max_element (loudness.begin(), loudness.end()) - loudness.begin());
                const auto framesPerSecond = reader->sampleRate / 512.0;
                const auto halfSecondOn = peakAt + (int) (0.5 * framesPerSecond);
                const auto decays = halfSecondOn >= (int) loudness.size() || loudness[(size_t) halfSecondOn] < loudness[(size_t) peakAt] - 15.0f;
                type = print.pitchHz < 130.0f ? "Bass" : (print.notes.size() > 1 || decays) ? "Pluck"
                     : peakAt > (int) (0.15 * framesPerSecond) ? "Pad" : "Lead";
            }
            kept.push_back ({ file.getFullPathName(), "owner-library", "owner's packs (test only)", type, "test", -1, 0, print.notes.size() > 1 });
        }

        const auto scans = libraryFolder().getChildFile ("scans");
        scans.createDirectory();
        juce::String out ("file,source,licence,type,use,genre,seed,loop\n");
        for (const auto& e : kept)
            out << csvField (e.file) << "," << csvField (e.source) << "," << csvField (e.licence) << "," << csvField (e.type) << ","
                << csvField (e.use) << ",-1,0," << (e.loop ? 1 : 0) << "\n";
        const auto list = scans.getChildFile (juce::File::createLegalFileName (folder.getFileName()) + ".csv");
        list.replaceWithText (out);

        std::map<juce::String, int> byType;
        int loops = 0;
        for (const auto& e : kept)
        {
            ++byType[e.type];
            loops += e.loop ? 1 : 0;
        }
        std::printf ("\n%d files looked at, %d kept (%d single notes, %d loops):\n", files.size(), (int) kept.size(), (int) kept.size() - loops, loops);
        for (const auto& [type, n] : byType)
            std::printf ("  %-6s %d\n", type.toRawUTF8(), n);
        std::printf ("dropped:\n");
        for (const auto& [why, n] : dropped)
            std::printf ("  %-45s %d\n", why.toRawUTF8(), n);
        std::printf ("kept list: %s (delete lines to leave sounds out), then: ZeroAnalyze library build\n", list.getFullPathName().toRawUTF8());
        return 0;
    }

    std::printf ("usage: library build | library engine <n> | library scan <folder>\n");
    return 1;
}

int runLibraryBench (const juce::StringArray& args)
{
    // Options: a sample size, a seed, source= / type= filters, and budget= / second=
    // (the search renders per worker in the first and second rounds).
    int count = 40;
    unsigned seed = 1;
    juce::String onlySource, onlyType;
    for (int i = 1; i < args.size(); ++i)
    {
        if (args[i].startsWith ("source=")) onlySource = args[i].fromFirstOccurrenceOf ("=", false, false);
        else if (args[i].startsWith ("type=")) onlyType = args[i].fromFirstOccurrenceOf ("=", false, false);
        else if (args[i].startsWith ("budget=")) ReferenceMatch::searchBudget = args[i].fromFirstOccurrenceOf ("=", false, false).getIntValue();
        else if (args[i].startsWith ("second=")) ReferenceMatch::secondBudget = args[i].fromFirstOccurrenceOf ("=", false, false).getIntValue();
        else if (i == 1) count = args[i].getIntValue();
        else if (i == 2) seed = (unsigned) args[i].getIntValue();
    }

    auto entries = readManifest();
    if (entries.empty())
    {
        std::printf ("no library yet: run 'library build' first\n");
        return 1;
    }

    // A sample spread evenly: shuffle within each source/type group, then take
    // one from each group in turn.
    std::map<juce::String, std::vector<Entry>> groups;
    for (const auto& e : entries)
        if ((onlySource.isEmpty() || juce::StringArray::fromTokens (onlySource, ",", "").contains (e.source))
            && (onlyType.isEmpty() || e.type.equalsIgnoreCase (onlyType)))
            groups[e.source + "/" + e.type].push_back (e);
    std::mt19937 rng (seed);
    for (auto& [name, group] : groups)
        std::shuffle (group.begin(), group.end(), rng);
    std::vector<Entry> sample;
    for (size_t round = 0; (int) sample.size() < count; ++round)
    {
        bool any = false;
        for (auto& [name, group] : groups)
            if (round < group.size() && (int) sample.size() < count)
            {
                sample.push_back (group[round]);
                any = true;
            }
        if (! any)
            break;
    }

    MatchEngines engines;
    ReferenceMatch match (engines.get());
    struct Row { Entry entry; juce::String word, chosen; float distance = 0.0f, seconds = 0.0f, judge = -1.0f; };
    std::vector<Row> rows;
    for (const auto& e : sample)
    {
        const auto began = juce::Time::getMillisecondCounter();
        match.start (juce::File (e.file), 0, 0, makeProfile (0, 0));
        while (match.getState() == ReferenceMatch::State::running)
            juce::Thread::sleep (20);
        const auto result = match.take();

        Row row { e };
        row.seconds = (float) (juce::Time::getMillisecondCounter() - began) / 1000.0f;
        if (result.ok)
        {
            // Scored by the frozen judge, so no round can pass by going easy on itself.
            row.distance = frozenJudge (result.target, result.matched);
            row.word = frozenJudgeWord (row.distance);
            row.chosen = genreNames[result.genre] + " " + typeNames[result.type];
            if (e.source == "engine") // known notes: replay them and judge as matchbench does
            {
                juce::Random random (e.seed);
                const auto type = typeNames.indexOf (e.type);
                const auto reference = renderBenchSound (e.genre, type, generateProfile (e.genre, type, random), e.loop);
                row.judge = judgeBenchSound (reference, renderBenchSound (result.genre, result.type, result.profile, e.loop));
            }
        }
        else
            row.word = "FAILED";
        std::printf ("%-9s %-7s %-11s %5.2f %-26s %4.1fs%s  %s\n", e.source.toRawUTF8(), e.type.toRawUTF8(), row.word.toRawUTF8(), row.distance,
                     row.chosen.toRawUTF8(), row.seconds, row.judge >= 0.0f ? juce::String::formatted ("  judge %.1f dB", row.judge).toRawUTF8() : "",
                     juce::File (e.file).getFileName().toRawUTF8());
        rows.push_back (row);
    }

    // Per category: how many, mean distance, and how the words fell.
    auto summarise = [&] (const char* title, auto key)
    {
        std::map<juce::String, std::vector<const Row*>> by;
        for (const auto& r : rows)
            by[key (r)].push_back (&r);
        std::printf ("\nby %s:\n", title);
        for (const auto& [name, group] : by)
        {
            double sum = 0.0;
            std::map<juce::String, int> words;
            for (const auto* r : group)
            {
                sum += r->distance;
                ++words[r->word];
            }
            std::printf ("  %-10s n %3d  mean %.2f ", name.toRawUTF8(), (int) group.size(), sum / (double) group.size());
            for (const auto* w : { "VERY CLOSE", "CLOSE", "ROUGH", "LOOSE", "FAILED" })
                std::printf (" %s %d", w, words[w]);
            std::printf ("\n");
        }
    };
    summarise ("source", [] (const Row& r) { return r.entry.source; });
    summarise ("type", [] (const Row& r) { return r.entry.type; });

    // The run, for comparing against later ones.
    const auto runs = libraryFolder().getChildFile ("runs");
    runs.createDirectory();
    juce::String csv ("file,source,type,word,distance,chosen,seconds,judge\n");
    for (const auto& r : rows)
        csv << csvField (r.entry.file) << "," << r.entry.source << "," << r.entry.type << "," << r.word << "," << r.distance << ","
            << csvField (r.chosen) << "," << r.seconds << "," << r.judge << "\n";
    const auto out = runs.getChildFile (juce::Time::getCurrentTime().formatted ("%Y-%m-%d %H-%M") + " n" + juce::String (count) + " seed" + juce::String ((int) seed) + ".csv");
    out.replaceWithText (csv);
    std::printf ("\nwrote %s\n", out.getFullPathName().toRawUTF8());
    return 0;
}

int runLibraryTraits (const juce::StringArray& args)
{
    // library traits [source=a,b] [under=<path>] [exclude=<path>] [--write]: SoundTraits
    // of every matching sound, per type: the targets Generate aims at. Measurement only.
    juce::StringArray sources { "owner-library" };
    juce::String under, exclude;
    for (const auto& a : args)
    {
        if (a.startsWith ("source=")) sources = juce::StringArray::fromTokens (a.fromFirstOccurrenceOf ("=", false, false), ",", "");
        if (a.startsWith ("under=")) under = a.fromFirstOccurrenceOf ("=", false, false);
        if (a.startsWith ("exclude=")) exclude = a.fromFirstOccurrenceOf ("=", false, false);
    }
    juce::AudioFormatManager formats;
    formats.registerBasicFormats();

    std::array<std::vector<SoundTraits>, 6> byType;
    juce::String csv ("file,type,level,side,centroid,bass,body,spread,crest,movement,richness\n");
    int read = 0;
    for (const auto& e : readManifest())
    {
        const auto type = typeNames.indexOf (e.type);
        if (type < 0 || ! sources.contains (e.source) || (under.isNotEmpty() && ! e.file.startsWithIgnoreCase (under))
            || (exclude.isNotEmpty() && e.file.startsWithIgnoreCase (exclude)))
            continue;
        std::unique_ptr<juce::AudioFormatReader> reader (formats.createReaderFor (juce::File (e.file)));
        if (reader == nullptr)
            continue;
        juce::AudioBuffer<float> audio (2, (int) juce::jmin<juce::int64> (reader->lengthInSamples, (juce::int64) (reader->sampleRate * 3.0)));
        reader->read (&audio, 0, audio.getNumSamples(), 0, true, true);
        // The notes (Reference Match's transcription) of the first moments, so
        // width and movement are measured on them as Generate measures its own.
        std::vector<float> fundamentals;
        for (const auto& n : SoundPrint::of (audio, reader->sampleRate).notes)
            if (n.start < 1.5f && n.pitchHz > 20.0f)
            {
                fundamentals.push_back (n.pitchHz);
                for (const auto hz : n.also)
                    if (hz > 20.0f)
                        fundamentals.push_back (hz);
            }
        const auto t = SoundTraits::of (audio, reader->sampleRate, fundamentals);
        if (t.levelDb < -60.0f)
            continue;
        byType[(size_t) type].push_back (t);
        csv << csvField (e.file) << "," << e.type;
        for (const auto v : t.asArray())
            csv << "," << juce::String (v, 3);
        csv << "\n";
        if (++read % 200 == 0)
            std::printf ("  %d measured\n", read);
    }
    libraryFolder().getChildFile ("traits.csv").replaceWithText (csv);

    // Per type: the median and the spread (half the 16th-84th percentile range,
    // a standard deviation that ignores outliers) of each trait.
    const char* names[] { "level", "side%", "centroid", "bass%", "body%", "spread", "crest", "movement", "richness" };
    std::array<std::array<float, SoundTraits::count>, 6> median {}, spread {};
    std::printf ("\n%-6s %5s", "type", "n");
    for (const auto* n : names)
        std::printf (" %16s", n);
    std::printf ("\n");
    for (int type = 0; type < 6; ++type)
    {
        const auto& list = byType[(size_t) type];
        std::printf ("%-6s %5d", typeNames[type].toRawUTF8(), (int) list.size());
        for (int k = 0; k < SoundTraits::count; ++k)
        {
            std::vector<float> values;
            for (const auto& t : list)
                values.push_back (t.asArray()[(size_t) k]);
            if (values.size() < 5)
            {
                std::printf (" %16s", "-");
                continue;
            }
            std::sort (values.begin(), values.end());
            auto at = [&] (double q) { return values[(size_t) juce::jlimit (0, (int) values.size() - 1, (int) (q * (double) (values.size() - 1)))]; };
            median[(size_t) type][(size_t) k] = at (0.5);
            spread[(size_t) type][(size_t) k] = juce::jmax (1.0e-3f, 0.5f * (at (0.84) - at (0.16)));
            std::printf (" %8.2f +- %5.2f", median[(size_t) type][(size_t) k], spread[(size_t) type][(size_t) k]);
        }
        std::printf ("\n");
    }

    if (args.contains ("--write"))
    {
        juce::String text;
        text << "#pragma once\n\n#include \"SoundTraits.h\"\n\n"
             << "// What Generate aims at, per type (Bass, Lead, Chord, Pad, Pluck, FX): the median\n"
             << "// and spread of each SoundTrait over the owner's sounds (" << read << " measured,\n"
             << "// `ZeroAnalyze library traits --write`; measurement only, nothing trained).\n"
             << "// Order: level, side %, centroid, bass %, body %, spread, crest, movement, richness.\n"
             << "namespace GenerateTargets\n{\n"
             << "    struct Target { std::array<float, SoundTraits::count> median, spread; int measured; };\n"
             << "    inline const std::array<Target, 6> byType {{\n";
        for (int type = 0; type < 6; ++type)
        {
            auto row = [] (const std::array<float, SoundTraits::count>& v)
            {
                juce::StringArray parts;
                for (const auto x : v)
                    parts.add (juce::String (x, 3) + "f");
                return "{ " + parts.joinIntoString (", ") + " }";
            };
            text << "        { " << row (median[(size_t) type]) << ",\n          " << row (spread[(size_t) type]) << ", "
                 << (int) byType[(size_t) type].size() << " }, // " << typeNames[type] << "\n";
        }
        text << "    }};\n}\n";
        juce::File::getCurrentWorkingDirectory().getChildFile ("Source/GenerateTargets.h").replaceWithText (text);
        std::printf ("wrote Source/GenerateTargets.h\n");
    }
    return 0;
}
