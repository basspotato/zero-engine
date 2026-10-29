#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <optional>
#include "Wavetable.h"

// The fingerprint Reference Match compares: how a sound evolves over its whole
// phrase (log-mel spectrograms from the onset to where it dies away, up to 2 s),
// plus its stereo width and level. A reference is also transcribed into notes
// (onset, length, pitch), so candidates can play the same phrase; its pitch,
// octave-band shape and any wobble guide where the search starts.
struct SoundPrint
{
    static constexpr int numBands = 10;
    static constexpr double span = 1.2; // seconds after the onset in which the first note's wobble is measured

    std::array<float, numBands> bandsDb {}; // octave bands 31 Hz to 16 kHz, dB under the total (pitch and octave checks)
    float sidePct = 0.0f, levelDb = -100.0f, pitchHz = 0.0f;
    float maxHz = 21600.0f; // the highest frequency its sample rate holds (less a margin): bands above are not compared
    float moveRateHz = 0.0f, moveDepthDb = 0.0f; // a wobble (LFO on level or brightness): rate, 0 if none, and depth

    // How the sound evolves: log-mel spectrograms (40 bands, 40 Hz to 16 kHz, dB
    // under their loudest cell, floor -80) from the onset over the whole phrase
    // and its release, at two resolutions: 2048-sample frames every 512 (tone)
    // and 512 every 128 (attack). noteSeconds: how long that is.
    static constexpr int melBands = 40;
    std::vector<std::array<float, melBands>> melTone, melAttack;
    float noteSeconds = 0.0f;

    // A reference's notes in its first 2 s (seconds from its onset, pitch 0 when
    // unclear): candidates play the same pattern and are compared over all of it.
    // also: the other pitches it brings with it (a chord), loudest first.
    struct Note { float start = 0.0f, length = 0.0f, pitchHz = 0.0f; std::vector<float> also; };
    std::vector<Note> notes;
    bool cut = false; // still sounding where measuring stopped: compared only that far

    // A pitched reference's first note as harmonics (up to 64) at five moments
    // over its first half second, each scaled to its loudest: the Sample
    // wavetable Reference Match can give a candidate.
    std::optional<WavetableBank::Harmonics> harmonics;

    // findPitch: a reference (pitch and notes found); false for engine renders,
    // which are taken whole.
    static SoundPrint of (const juce::AudioBuffer<float>& audio, double sampleRate, bool findPitch = true);

    // The composite loss's weights (mel spectrograms, MFCC timbre, brightness,
    // loudness envelope); set by tooling to tune, otherwise as tuned on matchbench.
    struct Weights { float mel = 1.0f, mfcc = 1.0f, centroid = 1.0f, loudness = 1.0f; };
    static inline Weights weights;
    static inline bool debugNotes = false; // tooling: print each detected note and why it was merged or kept

    // How far apart two prints are; about 1 means "clearly a different sound".
    // explain (tooling): set to the terms that make up the distance.
    static float distance (const SoundPrint& a, const SoundPrint& b, juce::String* explain = nullptr);
};
