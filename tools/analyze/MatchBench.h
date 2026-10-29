#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

// ZeroAnalyze matchbench [refFolder]: how well Reference Match does.
//
// Engine-made targets (Generate variations across genres and types, as single
// notes and as arp loops) are matched, then the matched sound plays the exact
// same notes as the target and both are compared as log-mel spectrograms over
// time: the mean difference in dB, a judge independent of the matcher's own
// fingerprint. Lower is better; the target's own designed preset is shown as a
// baseline. With refFolder, every WAV in it is matched too and the matcher's
// own closeness is printed (real sounds have no notes to replay).
int runMatchBench (const juce::StringArray& args);

struct Profile;

// The benchmark's renders and judge, shared with the library tools: a profile
// playing the type's test note or arp loop (2 s at 48 kHz), and the mean log-mel
// dB difference between two such renders.
juce::AudioBuffer<float> renderBenchSound (int genre, int type, const Profile& profile, bool loop);
float judgeBenchSound (const juce::AudioBuffer<float>& reference, const juce::AudioBuffer<float>& candidate);
