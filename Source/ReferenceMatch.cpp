#include "ReferenceMatch.h"
#include "PluginProcessor.h"
#include "Parameters.h"
#include "Wavetable.h"

ReferenceMatch::ReferenceMatch (ZeroEngineProcessor& renderEngine)
    : juce::Thread ("Reference Match"), engine (renderEngine)
{
}

ReferenceMatch::~ReferenceMatch()
{
    stopThread (4000);
}

void ReferenceMatch::start (const juce::File& reference, int newGenre, int newType, const Profile& startFrom)
{
    stopThread (4000);
    file = reference;
    genre = newGenre;
    type = newType;
    from = startFrom;
    result = {};
    progress = 0.0f;
    state = State::running;
    startThread();
}

ReferenceMatch::Result ReferenceMatch::take()
{
    state = State::idle;
    return result;
}

SoundPrint ReferenceMatch::render (const Profile& profile, int note)
{
    // 0.6 s of the note (a triad for chords and pads), measured like the reference.
    constexpr double rate = 48000.0;
    constexpr int block = 512, length = 28800;
    engine.setPlayConfigDetails (0, 2, rate, block);
    engine.prepareToPlay (rate, block);
    engine.loadProfile (genre, type, profile);

    juce::AudioBuffer<float> out (2, length), chunk (2, block);
    juce::MidiBuffer midi;
    const auto chord = type == 2 || type == 3;
    constexpr int intervals[] { 0, 4, 7 };
    for (int pos = 0; pos < length; pos += block)
    {
        midi.clear();
        if (pos == 0)
            for (int i = 0; i < (chord ? 3 : 1); ++i)
                midi.addEvent (juce::MidiMessage::noteOn (1, juce::jlimit (0, 127, note + intervals[i]), (juce::uint8) 110), 0);
        chunk.clear();
        engine.processBlock (chunk, midi);
        for (int ch = 0; ch < 2; ++ch)
            out.copyFrom (ch, pos, chunk, ch, 0, juce::jmin (block, length - pos));
    }
    return SoundPrint::of (out, rate, false);
}

void ReferenceMatch::run()
{
    auto fail = [this] (const juce::String& why)
    {
        result.summary = why;
        state = State::failed;
    };

    // The reference: up to 10 s, any common format.
    juce::AudioFormatManager formats;
    formats.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> reader (formats.createReaderFor (file));
    if (reader == nullptr)
        return fail ("Can't read " + file.getFileName());

    const auto length = (int) juce::jmin<juce::int64> (reader->lengthInSamples, (juce::int64) (reader->sampleRate * 10.0));
    juce::AudioBuffer<float> audio (2, juce::jmax (1, length));
    reader->read (&audio, 0, length, 0, true, true);
    const auto target = SoundPrint::of (audio, reader->sampleRate, true);
    if (target.levelDb < -70.0f)
        return fail (file.getFileName() + " is silent");

    // Play the candidates at the reference's pitch, kept to the type's range.
    auto note = type == 0 ? 36 : type == 1 || type == 4 ? 72 : 60;
    if (target.pitchHz > 0.0f)
        note = juce::roundToInt (69.0 + 12.0 * std::log2 (target.pitchHz / 440.0));
    note = type == 0 ? juce::jlimit (24, 55, note) : juce::jlimit (36, 96, note);

    constexpr int budget = 130; // renders, about 4 s
    int renders = 0;
    auto best = from;
    auto bestDistance = SoundPrint::distance (render (best, note), target);

    // 1. The source: every wavetable at two positions.
    for (int table = 0; table < WavetableBank::numBanks && ! threadShouldExit(); ++table)
        for (const auto position : { 0.3f, 0.7f })
        {
            auto candidate = best;
            candidate.waveTable = (float) table;
            candidate.wavePos = position;
            if (const auto d = SoundPrint::distance (render (candidate, note), target); d < bestDistance)
            {
                best = candidate;
                bestDistance = d;
            }
            progress = 0.4f * (float) ++renders / (float) (WavetableBank::numBanks * 2);
        }

    // 2. Coordinate search on what shapes tone, width and density.
    struct Knob { float Profile::* member; float min, max, step; };
    const Knob knobs[] {
        { &Profile::wavePos, 0.0f, 1.0f, 0.1f },     { &Profile::tilt, -6.0f, 6.0f, 1.5f },
        { &Profile::air, 0.0f, 1.0f, 0.2f },          { &Profile::ottDepth, 0.0f, 1.0f, 0.2f },
        { &Profile::grit, 0.0f, 1.0f, 0.15f },        { &Profile::stereoWidth, 0.0f, 2.0f, 0.3f },
        { &Profile::weight, 0.0f, 1.0f, 0.2f },       { &Profile::detune, 0.0f, 1.0f, 0.15f },
    };
    float steps[std::size (knobs)];
    for (size_t k = 0; k < std::size (knobs); ++k)
        steps[k] = knobs[k].step;

    const auto searchStart = renders;
    while (renders < budget && ! threadShouldExit())
    {
        bool improved = false;
        for (size_t k = 0; k < std::size (knobs) && renders < budget && ! threadShouldExit(); ++k)
            for (const auto direction : { 1.0f, -1.0f })
            {
                auto candidate = best;
                auto& value = candidate.*knobs[k].member;
                const auto moved = juce::jlimit (knobs[k].min, knobs[k].max, value + direction * steps[k]);
                if (moved == value)
                    continue;
                value = moved;

                const auto d = SoundPrint::distance (render (candidate, note), target);
                ++renders;
                progress = 0.4f + 0.6f * (float) (renders - searchStart) / (float) (budget - searchStart);
                if (d < bestDistance)
                {
                    best = candidate;
                    bestDistance = d;
                    improved = true;
                    break;
                }
            }

        if (! improved)
        {
            bool anyLeft = false;
            for (auto& step : steps)
            {
                step *= 0.5f;
                anyLeft = anyLeft || step > 0.01f;
            }
            if (! anyLeft)
                break;
        }
    }

    if (threadShouldExit())
        return fail ("Match cancelled");

    result.ok = true;
    result.profile = best;
    result.genre = genre;
    result.type = type;
    result.summary = "MATCHED " + file.getFileNameWithoutExtension().toUpperCase()
                   + "  -  " + juce::String (juce::roundToInt (100.0f * std::exp (-bestDistance))) + "% CLOSE";
    progress = 1.0f;
    state = State::done;
}
