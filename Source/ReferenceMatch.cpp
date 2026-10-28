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
    // 1.05 s of the note (a triad for chords and pads), measured like the reference.
    constexpr double rate = 48000.0;
    constexpr int block = 512;
    const auto length = 50176;
    engine.setPlayConfigDetails (0, 2, rate, block);
    engine.prepareToPlay (rate, block);
    engine.resetForRender(); // nothing of the last candidate may ring into this one
    engine.loadProfile (genre, type, profile);

    // A tenth of a second of silence first, so knob smoothing settles.
    juce::AudioBuffer<float> out (2, length), chunk (2, block);
    juce::MidiBuffer midi;
    for (int pos = 0; pos < 5120; pos += block)
    {
        chunk.clear();
        engine.processBlock (chunk, midi);
    }

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
    result.target = target;
    if (target.levelDb < -70.0f)
        return fail (file.getFileName() + " is silent");

    // Play the candidates at the reference's pitch, kept to the type's range.
    auto note = type == 0 ? 36 : type == 1 || type == 4 ? 72 : 60;
    if (target.pitchHz > 0.0f)
        note = juce::roundToInt (69.0 + 12.0 * std::log2 (target.pitchHz / 440.0));
    note = type == 0 ? juce::jlimit (24, 55, note) : juce::jlimit (36, 96, note);

    constexpr int budget = 150; // renders, about 7 s
    int renders = 0;
    auto best = from;
    auto bestPrint = render (best, note);
    auto bestDistance = SoundPrint::distance (bestPrint, target);

    // 0. The octave, for chords and pads (whose period can sit an octave or two
    //    under the root) and sounds with no clear pitch: try the neighbours and
    //    keep the closest. A single note's pitch is read reliably.
    if (type == 2 || type == 3 || target.pitchHz <= 0.0f)
    {
        const auto read = note;
        for (const auto shift : { -12, 12, 24 })
        {
            const auto candidate = read + shift;
            if (candidate < (type == 0 ? 24 : 36) || candidate > (type == 0 ? 55 : 96))
                continue;
            const auto print = render (best, candidate);
            if (const auto d = SoundPrint::distance (print, target); d < bestDistance)
            {
                note = candidate;
                bestPrint = print;
                bestDistance = d;
            }
        }
    }

    // 0b. The envelope, roughly, so the wavetable stage compares tones on a sound
    //     that already moves like the reference.
    for (const auto sustain : { 0.0f, 0.5f, 1.0f })
        for (const auto decay : { 0.15f, 0.6f, 2.0f })
        {
            auto candidate = best;
            candidate.ampSustain = sustain;
            candidate.ampDecay = decay;
            const auto print = render (candidate, note);
            if (const auto d = SoundPrint::distance (print, target); d < bestDistance)
            {
                best = candidate;
                bestDistance = d;
                bestPrint = print;
            }
        }

    // 1. The source: every wavetable at two positions.
    for (int table = 0; table < WavetableBank::numBanks && ! threadShouldExit(); ++table)
        for (const auto position : { 0.3f, 0.7f })
        {
            auto candidate = best;
            candidate.waveTable = (float) table;
            candidate.wavePos = position;
            const auto print = render (candidate, note);
            if (const auto d = SoundPrint::distance (print, target); d < bestDistance)
            {
                best = candidate;
                bestDistance = d;
                bestPrint = print;
            }
            progress = 0.4f * (float) ++renders / (float) (WavetableBank::numBanks * 2);
        }

    // 1a. Brightness, coarsely: the filter and tilt together, since either
    //     alone can stall halfway (a darker filter with a brighter tilt).
    {
        const auto base = best;
        for (const auto cutoff : { 250.0f, 600.0f, 1500.0f, 4000.0f, 20000.0f })
            for (const auto tilt : { -4.0f, 0.0f, 3.0f })
            {
                auto candidate = base;
                candidate.filterCutoff = cutoff;
                candidate.filterEnv = juce::jmin (base.filterEnv, 2.0f); // an envelope sweep would hide the cutoff
                candidate.tilt = tilt;
                const auto print = render (candidate, note);
                if (const auto d = SoundPrint::distance (print, target); d < bestDistance)
                {
                    best = candidate;
                    bestDistance = d;
                    bestPrint = print;
                }
            }
    }

    // 1b. A wobble in the reference: a free-running LFO at its rate, on the
    //     filter or the wavetable position.
    if (target.moveRateHz > 0.0f)
        for (const auto [toCutoff, toPos] : { std::pair { 1.5f, 0.0f }, { 3.0f, 0.0f }, { 0.0f, 0.5f } })
        {
            auto candidate = best;
            candidate.lfoSync = 0.0f;
            candidate.lfoRate = target.moveRateHz;
            candidate.lfoToCutoff = toCutoff;
            candidate.lfoToPos = toPos;
            const auto print = render (candidate, note);
            if (const auto d = SoundPrint::distance (print, target); d < bestDistance)
            {
                best = candidate;
                bestDistance = d;
                bestPrint = print;
            }
        }

    // 2. Coordinate search on what shapes tone, width, density, envelope and wobble.
    //    Attack and decay move by doublings (a step of 1 = twice as long).
    struct Knob { float Profile::* member; float min, max, step; bool doubling = false; };
    const Knob knobs[] {
        { &Profile::wavePos, 0.0f, 1.0f, 0.1f },     { &Profile::tilt, -6.0f, 6.0f, 1.5f },
        { &Profile::air, 0.0f, 1.0f, 0.2f },          { &Profile::ottDepth, 0.0f, 1.0f, 0.2f },
        { &Profile::grit, 0.0f, 1.0f, 0.15f },        { &Profile::stereoWidth, 0.0f, 2.0f, 0.3f },
        { &Profile::weight, 0.0f, 1.0f, 0.2f },       { &Profile::detune, 0.0f, 1.0f, 0.15f },
        { &Profile::ampAttack, 0.001f, 2.0f, 1.0f, true }, { &Profile::ampDecay, 0.02f, 4.0f, 1.0f, true },
        { &Profile::ampSustain, 0.0f, 1.0f, 0.2f },          { &Profile::filterCutoff, 60.0f, 20000.0f, 1.0f, true },
        { &Profile::reverbMix, 0.0f, 1.0f, 0.15f },          { &Profile::delayMix, 0.0f, 1.0f, 0.15f },
        { &Profile::lfoRate, 0.1f, 20.0f, 0.5f, true },     { &Profile::lfoToCutoff, 0.0f, 6.0f, 1.0f },
        { &Profile::lfoToPos, 0.0f, 1.0f, 0.2f },          { &Profile::filterEnv, -2.0f, 6.0f, 1.0f },
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
                if (knobs[k].member == &Profile::lfoRate && candidate.lfoSync > 0.5f)
                    continue; // tempo-synced: the rate knob does nothing
                auto& value = candidate.*knobs[k].member;
                const auto moved = juce::jlimit (knobs[k].min, knobs[k].max, knobs[k].doubling
                                                     ? juce::jmax (value, knobs[k].min) * std::exp2 (direction * steps[k])
                                                     : value + direction * steps[k]);
                if (moved == value)
                    continue;
                value = moved;

                const auto print = render (candidate, note);
                const auto d = SoundPrint::distance (print, target);
                ++renders;
                progress = 0.4f + 0.6f * (float) (renders - searchStart) / (float) (budget - searchStart);
                if (d < bestDistance)
                {
                    best = candidate;
                    bestDistance = d;
                    bestPrint = print;
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
    result.matched = bestPrint;
    result.genre = genre;
    result.type = type;
    result.summary = "MATCHED " + file.getFileNameWithoutExtension().toUpperCase()
                   + "  -  " + juce::String (juce::roundToInt (100.0f * std::exp (-bestDistance))) + "% CLOSE";
    // A reference far outside the type's range was played where the type lives, so
    // say so: a pluck matched as a Bass comes out as a bass.
    if (type == 0 && target.pitchHz > 260.0f)
        result.summary << "  -  HIGH FOR A BASS, TRY LEAD OR PLUCK";
    else if (type != 0 && target.pitchHz > 0.0f && target.pitchHz < 60.0f)
        result.summary << "  -  LOW, TRY BASS";
    progress = 1.0f;
    state = State::done;
}
