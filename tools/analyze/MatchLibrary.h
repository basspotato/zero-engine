#pragma once

#include <juce_core/juce_core.h>

// The Reference Match library: every sound we test Match on, in one manifest
// (analysis/library/library.csv: file, source, licence, type, use).
//
//   ZeroAnalyze library build          index the sources below into the manifest
//   ZeroAnalyze library engine <n>     render n engine sounds (Generate variations,
//                                      notes and arp loops) into analysis/library/engine,
//                                      with the seed that remakes their settings
//   ZeroAnalyze libbench [n] [seed] [source=...] [type=...]
//                                      match a sample of n files spread evenly over
//                                      sources and types; per-category results, and a
//                                      CSV in analysis/library/runs
//
// Sources: Freesound CC0 (analysis/freesound/refs, licence from credits.csv), the
// owner's Jupiter-8 recordings (c:/zero training), the owner's test files
// (F:/tests), engine renders, and NSynth (analysis/library/nsynth, CC BY 4.0) when
// downloaded. use = "test" for everything but engine renders ("train"): only
// licence-clean material ever trains anything.
int runLibrary (const juce::StringArray& args);
int runLibraryBench (const juce::StringArray& args);
