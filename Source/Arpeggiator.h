#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <algorithm>
#include <array>

// Turns held notes into a tempo-synced pattern. Runs on the audio thread before
// the synth: note-ons and note-offs from the keys are taken in, the pattern's
// notes go out; everything else (pitch bend, CCs) passes through. No allocation
// once prepared. Starts on the first key and stops when all keys are released.
class Arpeggiator
{
public:
    enum Mode { up, down, upDown, random, asPlayed };

    struct Settings
    {
        bool on = false;
        int mode = up;
        double beatsPerStep = 0.25; // 1/16
        int octaves = 1;
        float gate = 0.6f;          // fraction of a step each note sounds
        double bpm = 120.0;
    };

    void prepare (double sampleRate, int maxBlockSize);
    void reset() noexcept;

    // The events the synth should play this block: the arpeggiated notes plus
    // everything else from the keys, or the input itself when the arp is off.
    const juce::MidiBuffer& process (const juce::MidiBuffer& midi, int numSamples, const Settings&) noexcept;

private:
    void noteOn (int note, juce::uint8 velocity) noexcept;
    void noteOff (int note) noexcept;
    int nextNote() noexcept;

    static constexpr int maxHeld = 16;
    std::array<int, maxHeld> played {};        // in the order pressed
    std::array<juce::uint8, maxHeld> velocities {};
    int numHeld = 0;

    std::array<int, maxHeld * 4> sequence {}; // the pattern, octaves included
    int sequenceLength = 0, step = -1, direction = 1;
    int sounding = -1;                        // the arp note currently on, or -1
    juce::uint8 soundingVelocity = 100;
    double sampleRate = 48000.0, samplesToNextStep = 0.0, samplesToNoteOff = -1.0;
    bool wasOn = false;
    juce::Random rng;
    juce::MidiBuffer out;
};
