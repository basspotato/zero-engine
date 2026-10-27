#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "MacroMap.h"

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

    // Filter: mode 0 Low 12, 1 Low 24, 2 Band, 3 High; cutoff in Hz; env,
    // LFO amount in octaves.
    float filterMode, filterCutoff, filterRes, filterEnv, filterKeytrack, lfoToCutoff;
    float velToCutoff, velSens;

    // Playing: 0 poly, 1 mono, 2 legato; glide in seconds.
    float playMode, glide;

    // LFO.
    float lfoRate, lfoShape, lfoToPos, lfoToPitch;

    // Space: delay time is an index into delayTimeNames.
    float delayMix, delayTime, delayFeedback, reverbMix, reverbSize;

    // Finishing chain.
    float ottDepth, tilt, monoFreq, stereoWidth;

    // What each macro pushes besides its main job.
    MacroDirections directions;
};

Profile makeProfile (int genre, int type);

// Message thread only: writes every profile value through the host.
void applyProfile (juce::AudioProcessorValueTreeState& state, const Profile& profile);

// Any thread: points the macros at this profile's directions.
void loadMacroMap (MacroMap& map, const Profile& profile);
