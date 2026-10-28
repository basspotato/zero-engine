#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <algorithm>
#include <array>
#include <atomic>

// Turns held notes into a tempo-synced pattern. Runs on the audio thread before
// the synth: note-ons and note-offs from the keys are taken in, the pattern's
// notes go out; everything else (pitch bend, CCs) passes through. No allocation
// once prepared. Starts on the first key and stops when all keys are released.
//
// Arp modes walk through the held notes one at a time. Sequence mode is a
// 16-step sequencer: each step that's on plays every held key (so chords work),
// shifted by that step's pitch, louder on accented steps.
class Arpeggiator
{
public:
    enum Mode { up, down, upDown, random, asPlayed, sequence };
    static constexpr int maxSteps = 16;

    struct Settings
    {
        bool on = false;
        int mode = up;
        double beatsPerStep = 0.25; // 1/16
        int octaves = 1;
        float gate = 0.6f;          // fraction of a step each note sounds
        double bpm = 120.0;

        int length = maxSteps;      // sequence mode
        std::array<bool, maxSteps> stepOn {};
        std::array<int, maxSteps> stepPitch {};
        std::array<bool, maxSteps> stepAccent {};
    };

    void prepare (double sampleRate, int maxBlockSize);
    void reset() noexcept;

    // The events the synth should play this block: the pattern's notes plus
    // everything else from the keys, or the input itself when the arp is off.
    const juce::MidiBuffer& process (const juce::MidiBuffer& midi, int numSamples, const Settings&) noexcept;

    // The sequence step now playing, or -1 (for the step grid's light).
    int getCurrentStep() const noexcept { return currentStep.load (std::memory_order_relaxed); }

private:
    void keyOn (int note, juce::uint8 velocity) noexcept;
    void keyOff (int note) noexcept;
    int nextNote() noexcept;
    void play (int note, juce::uint8 velocity, int position) noexcept;
    void stopAll (int position) noexcept;

    static constexpr int maxHeld = 16;
    std::array<int, maxHeld> played {};        // in the order pressed
    std::array<juce::uint8, maxHeld> velocities {};
    int numHeld = 0;

    std::array<int, maxHeld * 4> pattern {}; // the arp pattern, octaves included
    int sequenceLength = 0, step = -1, direction = 1, seqStep = -1;

    std::array<int, maxHeld> sounding {};     // pattern notes currently on
    int numSounding = 0;

    double sampleRate = 48000.0, samplesToNextStep = 0.0, samplesToNoteOff = -1.0;
    bool wasOn = false;
    std::atomic<int> currentStep { -1 };
    juce::Random rng;
    juce::MidiBuffer out;
};
