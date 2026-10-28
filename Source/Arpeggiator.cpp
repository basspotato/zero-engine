#include "Arpeggiator.h"

void Arpeggiator::prepare (double newSampleRate, int maxBlockSize)
{
    sampleRate = newSampleRate;
    out.ensureSize ((size_t) juce::jmax (256, maxBlockSize) * 8); // room for a busy block, so process never allocates
    reset();
}

void Arpeggiator::reset() noexcept
{
    numHeld = numDown = 0;
    keyDown.fill (false);
    sequenceLength = 0;
    step = seqStep = -1;
    direction = 1;
    numSounding = 0;
    samplesToNextStep = 0.0;
    samplesToNoteOff = -1.0;
    currentStep.store (-1, std::memory_order_relaxed);
}

void Arpeggiator::keyOn (int note, juce::uint8 velocity) noexcept
{
    keyOff (note); // a repeated key moves to the end, like a fresh press
    if (numHeld == maxHeld)
        keyOff (played[0]);
    played[(size_t) numHeld] = note;
    velocities[(size_t) numHeld] = velocity;
    ++numHeld;
}

void Arpeggiator::keyOff (int note) noexcept
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
    else if (++step >= sequenceLength)
        step = 0;

    return pattern[(size_t) step];
}

void Arpeggiator::play (int note, juce::uint8 velocity, int position) noexcept
{
    if (note < 0 || note > 127 || numSounding == (int) sounding.size())
        return;
    for (int i = 0; i < numSounding; ++i)
        if (sounding[(size_t) i] == note)
            return; // already on this step (e.g. two keys an octave apart in a sequence)
    out.addEvent (juce::MidiMessage::noteOn (1, note, velocity), position);
    sounding[(size_t) numSounding++] = note;
}

void Arpeggiator::stopAll (int position) noexcept
{
    for (int i = 0; i < numSounding; ++i)
        out.addEvent (juce::MidiMessage::noteOff (1, sounding[(size_t) i]), position);
    numSounding = 0;
}

const juce::MidiBuffer& Arpeggiator::process (const juce::MidiBuffer& midi, int numSamples, const Settings& s) noexcept
{
    out.clear();

    // Switched off: pass everything through, ending any pattern notes first.
    if (! s.on)
    {
        if (! wasOn)
            return midi;

        stopAll (0);
        out.addEvents (midi, 0, numSamples, 0);
        reset();
        wasOn = false;
        return out;
    }
    wasOn = true;

    const auto stepSamples = juce::jmax (1.0, s.beatsPerStep * 60.0 / juce::jmax (20.0, s.bpm) * sampleRate);
    const auto octaves = juce::jlimit (1, 4, s.octaves);
    const auto length = juce::jlimit (1, maxSteps, s.length);
    const auto isSequence = s.mode == sequence;

    // Latch switched off: drop the notes whose keys are no longer held.
    if (wasLatched && ! s.latch)
        for (int note = 0; note < 128; ++note)
            if (! keyDown[(size_t) note])
                keyOff (note);
    const auto latchReleased = wasLatched && ! s.latch;
    wasLatched = s.latch;

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
                    pattern[(size_t) sequenceLength++] = n;

        if (s.mode == down)
            std::reverse (pattern.begin(), pattern.begin() + sequenceLength);
        else if (s.mode == upDown && sequenceLength > 2)
        {
            // Up then back down, without repeating the top and bottom notes.
            const auto top = sequenceLength;
            for (int i = top - 2; i >= 1; --i)
                pattern[(size_t) sequenceLength++] = pattern[(size_t) i];
        }
        direction = s.mode == random ? 0 : 1;
        if (step >= sequenceLength)
            step = -1;
    };

    // Walk through the block: keys in, steps out, in time order.
    auto events = midi.begin();
    for (int pos = 0; pos <= numSamples; ++pos)
    {
        bool changed = pos == 0 && latchReleased; // rebuild the pattern without the dropped notes
        while (events != midi.end() && (*events).samplePosition <= pos)
        {
            const auto message = (*events).getMessage();
            if (message.isNoteOn())
            {
                const auto note = message.getNoteNumber();

                // Latched: the first key of a fresh chord (all keys were up) replaces the old one.
                if (s.latch && numDown == 0)
                    numHeld = 0;

                const auto wasEmpty = numHeld == 0;
                keyOn (note, message.getVelocity());
                if (! keyDown[(size_t) note])
                {
                    keyDown[(size_t) note] = true;
                    ++numDown;
                }
                changed = true;
                if (wasEmpty)
                {
                    step = seqStep = -1;
                    samplesToNextStep = 0.0; // the first key starts the pattern at once
                }
            }
            else if (message.isNoteOff())
            {
                const auto note = message.getNoteNumber();
                if (keyDown[(size_t) note])
                {
                    keyDown[(size_t) note] = false;
                    --numDown;
                }
                if (! s.latch) // latched notes stay in the pattern after the key lifts
                {
                    keyOff (note);
                    changed = true;
                }
            }
            else
                out.addEvent (message, juce::jmin (pos, numSamples - 1)); // bend, CCs, pedal
            ++events;
        }
        if (changed)
            rebuild();

        if (pos == numSamples)
            break;

        // The current notes' gate ends.
        if (numSounding > 0 && samplesToNoteOff >= 0.0 && --samplesToNoteOff < 0.0)
            stopAll (pos);

        // All keys up: stop.
        if (numHeld == 0)
        {
            stopAll (pos);
            currentStep.store (-1, std::memory_order_relaxed);
            continue;
        }

        // Next step.
        if (samplesToNextStep <= 0.0)
        {
            stopAll (pos);

            // The step pattern shapes every mode: a step that's off rests, its
            // pitch transposes, and an accent plays at full velocity.
            seqStep = (seqStep + 1) % length;
            currentStep.store (seqStep, std::memory_order_relaxed);
            const auto stepPlays = s.stepOn[(size_t) seqStep];
            const auto transpose = s.stepPitch[(size_t) seqStep];
            const auto accented = s.stepAccent[(size_t) seqStep];

            if (stepPlays && isSequence)
            {
                // Every held key (chords).
                for (int i = 0; i < numHeld; ++i)
                    play (played[(size_t) i] + transpose,
                          accented ? (juce::uint8) 127 : (juce::uint8) juce::jmax (1, velocities[(size_t) i] * 3 / 4), pos);
            }
            else if (stepPlays)
            {
                // The arp's next note; it only moves on when a step plays, so rests don't skip notes.
                if (const auto note = nextNote(); note >= 0)
                {
                    juce::uint8 velocity = 100; // the key this note came from (any octave)
                    for (int i = 0; i < numHeld; ++i)
                        if ((note - played[(size_t) i]) % 12 == 0)
                            velocity = velocities[(size_t) i];
                    play (note + transpose, accented ? (juce::uint8) 127 : velocity, pos);
                }
            }

            samplesToNoteOff = juce::jlimit (0.05, 1.0, (double) s.gate) * stepSamples;
            samplesToNextStep += stepSamples;
        }
        samplesToNextStep -= 1.0;
    }

    return out;
}
