#include "Arpeggiator.h"

void Arpeggiator::prepare (double newSampleRate, int maxBlockSize)
{
    sampleRate = newSampleRate;
    out.ensureSize ((size_t) juce::jmax (256, maxBlockSize) * 6); // room for a busy block, so process never allocates
    reset();
}

void Arpeggiator::reset() noexcept
{
    numHeld = 0;
    sequenceLength = 0;
    step = -1;
    direction = 1;
    sounding = -1;
    samplesToNextStep = 0.0;
    samplesToNoteOff = -1.0;
}

void Arpeggiator::noteOn (int note, juce::uint8 velocity) noexcept
{
    noteOff (note); // a repeated key moves to the end, like a fresh press
    if (numHeld == maxHeld)
        noteOff (played[0]);
    played[(size_t) numHeld] = note;
    velocities[(size_t) numHeld] = velocity;
    ++numHeld;
}

void Arpeggiator::noteOff (int note) noexcept
{
    for (int i = 0; i < numHeld; ++i)
        if (played[(size_t) i] == note)
        {
            for (int j = i + 1; j < numHeld; ++j)
            {
                played[(size_t) (j - 1)] = played[(size_t) j];
                velocities[(size_t) (j - 1)] = velocities[(size_t) j];
            }
            --numHeld;
            return;
        }
}

int Arpeggiator::nextNote() noexcept
{
    if (sequenceLength == 0)
        return -1;

    if (sequenceLength == 1)
        step = 0;
    else if (direction == 0) // random: any note but the last one
    {
        const auto pick = rng.nextInt (sequenceLength - 1);
        step = pick >= step ? pick + 1 : pick;
    }
    else
    {
        step += direction;
        if (step >= sequenceLength || step < 0)
        {
            step = direction > 0 ? 0 : sequenceLength - 1;
        }
    }
    return sequence[(size_t) step];
}

const juce::MidiBuffer& Arpeggiator::process (const juce::MidiBuffer& midi, int numSamples, const Settings& s) noexcept
{
    out.clear();

    // Switched off: pass everything through, ending any arp note first.
    if (! s.on)
    {
        if (! wasOn)
            return midi;

        if (sounding >= 0)
            out.addEvent (juce::MidiMessage::noteOff (1, sounding), 0);
        out.addEvents (midi, 0, numSamples, 0);
        reset();
        wasOn = false;
        return out;
    }
    wasOn = true;

    const auto stepSamples = juce::jmax (1.0, s.beatsPerStep * 60.0 / juce::jmax (20.0, s.bpm) * sampleRate);
    const auto octaves = juce::jlimit (1, 4, s.octaves);

    auto rebuild = [&]
    {
        // Sorted (or as played) notes, repeated up the octaves.
        std::array<int, maxHeld> notes {};
        for (int i = 0; i < numHeld; ++i)
            notes[(size_t) i] = played[(size_t) i];
        if (s.mode != asPlayed)
            std::sort (notes.begin(), notes.begin() + numHeld);

        sequenceLength = 0;
        for (int o = 0; o < octaves; ++o)
            for (int i = 0; i < numHeld; ++i)
                if (const auto n = notes[(size_t) i] + 12 * o; n <= 127)
                    sequence[(size_t) sequenceLength++] = n;

        if (s.mode == down)
            std::reverse (sequence.begin(), sequence.begin() + sequenceLength);
        else if (s.mode == upDown && sequenceLength > 2)
        {
            // Up then back down, without repeating the top and bottom notes.
            const auto top = sequenceLength;
            for (int i = top - 2; i >= 1; --i)
                sequence[(size_t) sequenceLength++] = sequence[(size_t) i];
        }
        direction = s.mode == random ? 0 : 1;
        if (step >= sequenceLength)
            step = -1;
    };

    // Walk through the block: keys in, steps out, in time order.
    auto events = midi.begin();
    for (int pos = 0; pos <= numSamples; ++pos)
    {
        // Key events at this sample.
        bool changed = false;
        while (events != midi.end() && (*events).samplePosition <= pos)
        {
            const auto message = (*events).getMessage();
            if (message.isNoteOn())
            {
                const auto wasEmpty = numHeld == 0;
                noteOn (message.getNoteNumber(), message.getVelocity());
                changed = true;
                if (wasEmpty)
                {
                    step = -1;
                    samplesToNextStep = 0.0; // the first key starts the pattern at once
                }
            }
            else if (message.isNoteOff())
            {
                noteOff (message.getNoteNumber());
                changed = true;
            }
            else
                out.addEvent (message, juce::jmin (pos, numSamples - 1)); // bend, CCs, pedal
            ++events;
        }
        if (changed)
            rebuild();

        if (pos == numSamples)
            break;

        // The current note's gate ends.
        if (sounding >= 0 && samplesToNoteOff >= 0.0 && --samplesToNoteOff < 0.0)
        {
            out.addEvent (juce::MidiMessage::noteOff (1, sounding), pos);
            sounding = -1;
        }

        // All keys up: stop.
        if (numHeld == 0)
        {
            if (sounding >= 0)
            {
                out.addEvent (juce::MidiMessage::noteOff (1, sounding), pos);
                sounding = -1;
            }
            continue;
        }

        // Next step.
        if (samplesToNextStep <= 0.0)
        {
            if (sounding >= 0)
                out.addEvent (juce::MidiMessage::noteOff (1, sounding), pos);

            if (const auto note = nextNote(); note >= 0)
            {
                // Velocity of the key this note came from (any octave).
                soundingVelocity = 100;
                for (int i = 0; i < numHeld; ++i)
                    if ((note - played[(size_t) i]) % 12 == 0)
                        soundingVelocity = velocities[(size_t) i];
                out.addEvent (juce::MidiMessage::noteOn (1, note, soundingVelocity), pos);
                sounding = note;
                samplesToNoteOff = juce::jlimit (0.05, 1.0, (double) s.gate) * stepSamples;
            }
            samplesToNextStep += stepSamples;
        }
        samplesToNextStep -= 1.0;
    }

    return out;
}
