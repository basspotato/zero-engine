#include <juce_audio_formats/juce_audio_formats.h>
#include "Library.h"
#include "../../Source/PluginProcessor.h"
#include "../../Source/Parameters.h"

namespace
{
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
}

juce::String normalise (const juce::String& s)
{
    return s.toLowerCase().retainCharacters ("abcdefghijklmnopqrstuvwxyz0123456789");
}

juce::String groupName (int genre, int type)
{
    return (genre >= 0 ? genreNames[genre] : juce::String ("Any genre")) + " / "
         + (type >= 0 ? typeNames[type] : juce::String ("Any type"));
}

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

juce::AudioBuffer<float> renderProfile (int genre, int type, const Profile* candidate)
{
    ZeroEngineProcessor engine;
    engine.loadProfile (genre, type, candidate != nullptr ? *candidate : makeProfile (genre, type));

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
