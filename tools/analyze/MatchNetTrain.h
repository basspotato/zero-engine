#pragma once

#include <juce_core/juce_core.h>

// Training the Reference Match network (Source/MatchNet.h), on renders of this
// engine only (never the owner's packs or any other test sound):
//
//   ZeroAnalyze netdata <count> [out.bin] [seed]
//       render count random sounds (a preset, often with its settings and
//       wavetable moved, one note at a random pitch and length), read each as
//       a reference would be, and save features and settings
//       (default analysis/net/data.bin)
//   ZeroAnalyze nettrain [data.bin] [epochs] [--write]
//       train the network, print how well it guesses held-out sounds;
//       --write saves it into Source/MatchNetWeights.h
int runNetData (const juce::StringArray& args);
int runNetTrain (const juce::StringArray& args);
