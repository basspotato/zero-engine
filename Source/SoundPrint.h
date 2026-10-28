#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <array>

// A compact fingerprint of a sound for Reference Match: its spectral shape in ten
// octave bands (31 Hz to 16 kHz, dB relative to the total), stereo width, density
// (peak-to-RMS), pitch, and how it moves: its attack time and its level at four
// times after the onset, and any wobble. Only the first 1.2 s after the onset counts, and the
// spectrum is taken from just after the loudest moment, so a reference and an
// engine render are measured alike.
struct SoundPrint
{
    static constexpr int numBands = 10, numEnvelopePoints = 4;
    static constexpr double span = 1.2; // seconds after the onset that are measured
    static constexpr float unknown = -1000.0f; // an envelope point after the next note in a loop

    std::array<float, numBands> bandsDb {};
    std::array<float, numEnvelopePoints> envelopeDb {}; // 100, 250, 500, 900 ms after the onset, dB below the loudest
    float sidePct = 0.0f, crestDb = 0.0f, levelDb = -100.0f, pitchHz = 0.0f, attackMs = 0.0f;
    float centroidHz = 0.0f; // brightness
    float moveRateHz = 0.0f, moveDepthDb = 0.0f; // a wobble (LFO on level or brightness): rate, 0 if none, and depth

    // How the sound evolves: log-mel spectrograms (40 bands, 40 Hz to 16 kHz, dB
    // under their loudest cell, floor -80) from the onset over the first note,
    // at two resolutions: 2048-sample frames every 512 (tone) and 512 every 128
    // (attack). noteSeconds: how long the first note lasts before the next one
    // (cut) or the 1.2 s measured; a match plays its candidates for as long.
    static constexpr int melBands = 40;
    std::vector<std::array<float, melBands>> melTone, melAttack;
    float noteSeconds = 0.0f;

    // A reference's notes in its first 2 s (seconds from its onset, pitch 0 when
    // unclear): candidates play the same pattern and are compared over all of it.
    struct Note { float start = 0.0f, length = 0.0f, pitchHz = 0.0f; };
    std::vector<Note> notes;
    bool cut = false;

    // findPitch is for references; engine renders already know their note.
    static SoundPrint of (const juce::AudioBuffer<float>& audio, double sampleRate, bool findPitch = true);

    // The composite loss's weights (mel spectrograms, MFCC timbre, brightness,
    // loudness envelope); set by tooling to tune, otherwise as tuned on matchbench.
    struct Weights { float mel = 1.0f, mfcc = 1.0f, centroid = 1.0f, loudness = 1.0f; };
    static inline Weights weights;

    // How far apart two prints are; about 1 means "clearly a different sound".
    // explain (tooling): set to the terms that make up the distance.
    static float distance (const SoundPrint& a, const SoundPrint& b, juce::String* explain = nullptr);
};
