#pragma once

#include <juce_core/juce_core.h>

// Generate's neural judge (Source/GenerateNet.h), trained on renders of this
// engine only:
//
//   ZeroAnalyze gennetdata [count] [out.bin] [seed]   candidates as Generate makes them,
//                                                     measured (default analysis/net/generate.bin)
//   ZeroAnalyze gennettrain [data.bin] [epochs] [--write]   train; held-out R2 per output;
//                                                     --write saves Source/GenerateNetWeights.h
//   ZeroAnalyze gennetscreen                          does screening pick better candidates
//                                                     than chance? (24 profiles x 200 candidates)
int runGenerateNetData (const juce::StringArray& args);
int runGenerateNetTrain (const juce::StringArray& args);
int runGenerateNetScreen (const juce::StringArray& args);
