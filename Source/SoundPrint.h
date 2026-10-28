#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <array>

// A compact fingerprint of a sound for Reference Match: its spectral shape in ten
// octave bands (31 Hz to 16 kHz, dB relative to the total), stereo width, density
// (peak-to-RMS) and pitch. Taken from the same window of any sound: from just
// after its loudest moment, 0.4 s long, so a reference and an engine render are
// measured alike.
struct SoundPrint
{
    static constexpr int numBands = 10;
    std::array<float, numBands> bandsDb {};
    float sidePct = 0.0f, crestDb = 0.0f, levelDb = -100.0f, pitchHz = 0.0f;

    // findPitch is for references; engine renders already know their note.
    static SoundPrint of (const juce::AudioBuffer<float>& audio, double sampleRate, bool findPitch = true);

    // How far apart two prints are; about 1 means "clearly a different sound".
    static float distance (const SoundPrint& a, const SoundPrint& b);
};
