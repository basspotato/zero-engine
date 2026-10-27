// ZeroAnalyze: offline measurement of reference sounds and engine renders on
// loudness, brightness, bass and width, and a comparison between the two.
//
//   ZeroAnalyze analyze <folder> [out.csv]
//   ZeroAnalyze render  [outDir]            all 72 profiles: engine.csv + WAVs
//   ZeroAnalyze compare <refFolder> [outDir]
//   ZeroAnalyze test                                     engine checks, all profiles
//   ZeroAnalyze bench                                    CPU with 16 voices
//   ZeroAnalyze screen  [shortlist.csv] [perPair]        Freesound CC0 search
//   ZeroAnalyze fetch   [shortlist.csv] [outDir] [max]   Freesound downloads
//
// Reference layout: <refFolder>/<Genre>/<Type>/*.wav (any depth; the first
// folder names that match a genre and a type are used, e.g. "Dubstep/Bass").

#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <map>
#include "Descriptors.h"
#include "Freesound.h"
#include "Tests.h"
#include "../../Source/PluginProcessor.h"
#include "../../Source/Parameters.h"

namespace
{
    constexpr double renderRate = 48000.0;
    constexpr int renderBlock = 512;

    struct Item
    {
        juce::String name;
        int genre = -1, type = -1;
        Descriptors d;
    };

    juce::String normalise (const juce::String& s)
    {
        return s.toLowerCase().retainCharacters ("abcdefghijklmnopqrstuvwxyz0123456789");
    }

    int matchGenre (const juce::String& folder)
    {
        const auto n = normalise (folder);
        if (n == "dnb" || n == "drumandbass" || n == "drumnbass")
            return genreNames.indexOf ("Drum & Bass");

        for (int g = 0; g < genreNames.size(); ++g)
            if (n == normalise (genreNames[g]))
                return g;
        return -1;
    }

    int matchType (const juce::String& folder)
    {
        const auto n = normalise (folder);
        for (int t = 0; t < typeNames.size(); ++t)
        {
            const auto name = normalise (typeNames[t]);
            if (n == name || n == name + "s" || n == name + "es" || (name == "chord" && n == "keys") || (name == "fx" && n == "sfx"))
                return t;
        }
        return -1;
    }

    juce::String groupName (int genre, int type)
    {
        return (genre >= 0 ? genreNames[genre] : juce::String ("Any genre")) + " / "
             + (type >= 0 ? typeNames[type] : juce::String ("Any type"));
    }

    //==========================================================================
    std::vector<Item> analyseFolder (const juce::File& root)
    {
        juce::AudioFormatManager formats;
        formats.registerBasicFormats();

        std::vector<Item> items;
        for (const auto& entry : juce::RangedDirectoryIterator (root, true, formats.getWildcardForAllFormats()))
        {
            const auto file = entry.getFile();
            std::unique_ptr<juce::AudioFormatReader> reader (formats.createReaderFor (file));
            if (reader == nullptr || reader->lengthInSamples <= 0)
            {
                std::printf ("  skipped (unreadable): %s\n", file.getFileName().toRawUTF8());
                continue;
            }

            const auto length = (int) juce::jmin<juce::int64> (reader->lengthInSamples, (juce::int64) (reader->sampleRate * 120.0));
            juce::AudioBuffer<float> audio ((int) juce::jmin (2u, reader->numChannels), length);
            reader->read (&audio, 0, length, 0, true, audio.getNumChannels() > 1);

            Item item;
            item.name = file.getRelativePathFrom (root);
            for (auto dir = file.getParentDirectory(); dir != root && dir.isAChildOf (root); dir = dir.getParentDirectory())
            {
                // Walking upwards, so keep overwriting: the folder nearest the root wins.
                if (const auto g = matchGenre (dir.getFileName()); g >= 0) item.genre = g;
                if (const auto t = matchType (dir.getFileName()); t >= 0)  item.type = t;
            }

            item.d = analyse (audio, reader->sampleRate);
            items.push_back (item);
        }

        std::printf ("Analysed %d files in %s\n", (int) items.size(), root.getFullPathName().toRawUTF8());
        return items;
    }

    //==========================================================================
    struct Phrase
    {
        std::vector<int> notes;
        double hold, tail;
    };

    // A neutral test phrase per type, in the register each type usually plays.
    Phrase phraseFor (int type)
    {
        switch (type)
        {
            case 0:  return { { 36 },         1.0,  1.0 }; // bass: C2
            case 1:  return { { 72 },         1.0,  1.0 }; // lead: C5
            case 2:  return { { 60, 64, 67 }, 1.5,  1.5 }; // chord: C major
            case 3:  return { { 60, 64, 67 }, 3.0,  2.5 }; // pad
            case 4:  return { { 72 },         0.25, 1.0 }; // pluck
            default: return { { 60 },         2.0,  2.5 }; // fx
        }
    }

    juce::AudioBuffer<float> renderProfile (int genre, int type)
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

        const auto phrase = phraseFor (type);
        const auto offAt = (int) (phrase.hold * renderRate);
        const auto total = (int) ((phrase.hold + phrase.tail) * renderRate);

        juce::AudioBuffer<float> out (2, total);
        juce::AudioBuffer<float> block (2, renderBlock);
        juce::MidiBuffer midi;

        for (int pos = 0; pos < total; pos += renderBlock)
        {
            const auto n = juce::jmin (renderBlock, total - pos);
            block.setSize (2, n, false, false, true);
            midi.clear();

            for (auto note : phrase.notes)
            {
                if (pos == 0)
                    midi.addEvent (juce::MidiMessage::noteOn (1, note, (juce::uint8) 100), 0);
                if (offAt >= pos && offAt < pos + n)
                    midi.addEvent (juce::MidiMessage::noteOff (1, note), offAt - pos);
            }

            engine.processBlock (block, midi);
            for (int ch = 0; ch < 2; ++ch)
                out.copyFrom (ch, pos, block, ch, 0, n);
        }

        engine.releaseResources();
        return out;
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

    Item renderAndAnalyse (int genre, int type, const juce::File& wavDir)
    {
        const auto audio = renderProfile (genre, type);

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

            renders.push_back (renderAndAnalyse (genre, type, outDir.getChildFile ("renders")));
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
                     "  ZeroAnalyze test\n"
                     "  ZeroAnalyze bench\n"
                     "  ZeroAnalyze screen  [shortlist.csv] [perPair]\n"
                     "  ZeroAnalyze fetch   [shortlist.csv] [outDir] [maxDownloads]\n");
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

    return usage();
}
