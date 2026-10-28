#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <array>

// A compact fingerprint of a sound for Reference Match: its spectral shape in ten
// octave bands (31 Hz to 16 kHz, dB relative to the total), stereo width, density
// (peak-to-RMS), pitch, and how it moves: its attack time and its level at four
// times after the onset. Only the first 1.2 s after the onset counts, and the
// spectrum is taken from just after the loudest moment, so a reference and an
// engine render are measured alike.
struct SoundPrint
{
    static constexpr int numBands = 10, numEnvelopePoints = 4;
    static constexpr double span = 1.2; // seconds after the onset that are measured

    std::array<float, numBands> bandsDb {};
    std::array<float, numEnvelopePoints> envelopeDb {}; // 100, 250, 500, 900 ms after the onset, dB below the loudest
    float sidePct = 0.0f, crestDb = 0.0f, levelDb = -100.0f, pitchHz = 0.0f, attackMs = 0.0f;

    // findPitch is for references; engine renders already know their note.
    static SoundPrint of (const juce::AudioBuffer<float>& audio, double sampleRate, bool findPitch = true);

    // How far apart two prints are; about 1 means "clearly a different sound".
    static float distance (const SoundPrint& a, const SoundPrint& b);
};
