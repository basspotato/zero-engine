#pragma once

#include <juce_core/juce_core.h>

// Measures the harmonic spectrum of one steady stretch of each recording (one
// recording per wavetable frame) and writes them, dark to bright, as a header
// the engine builds a wavetable bank from (Source/JupiterHarmonics.h).
int extractHarmonics (const juce::File& header, const juce::StringArray& files);
