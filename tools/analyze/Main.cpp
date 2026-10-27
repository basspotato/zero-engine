// ZeroAnalyze: offline measurement of reference sounds and engine renders on
// loudness, brightness, bass and width, and a comparison between the two.
//
//   ZeroAnalyze analyze <folder> [out.csv]
//   ZeroAnalyze render  [outDir]            all 72 profiles: engine.csv + WAVs
//   ZeroAnalyze compare <refFolder> [outDir]
//   ZeroAnalyze tune    <refFolder> [--write]            match profiles to references
//   ZeroAnalyze tunetest                                 tuner self-check, no refs needed
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
#include "Library.h"
#include "Tuner.h"
#include "../../Source/PluginProcessor.h"
#include "../../Source/Parameters.h"

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
                     "  ZeroAnalyze tune    <refFolder> [--write]\n"
                     "  ZeroAnalyze tunetest\n"
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

    if (command == "tunetest")
        return tuneTest();

    if (command == "tune" && args.size() >= 2)
        return tuneProfiles (argFile (args, 1, {}),
                             args.contains ("--write") ? argFile (args, 99, "Source/ProfileTuning.h") : juce::File());

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
