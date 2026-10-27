#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

// A starting point for one genre x type pair: every engine and finishing
// parameter plus the six macro positions. Built from a per-type template and
// per-genre adjustments, so any single pair can be overridden later.
struct Profile
{
    // Macros.
    float weight, grit, punch, width, motion, air;

    // Oscillator.
    float wavePos, unison, detune, blend;

    // Envelopes (seconds, sustain 0..1).
    float ampAttack, ampDecay, ampSustain, ampRelease;
    float modAttack, modDecay, modSustain, modRelease, modToPos, modToPitch;

    // Sine sub: -1 = octave below the note, 0 = at the note.
    float subOctave;

    // LFO.
    float lfoRate, lfoShape, lfoToPos, lfoToPitch;

    // Finishing chain.
    float ottDepth, tilt, monoFreq, stereoWidth;
};

Profile makeProfile (int genre, int type);

// Message thread only: writes every profile value through the host.
void applyProfile (juce::AudioProcessorValueTreeState& state, const Profile& profile);
