#pragma once

#include <vector>

// Band-limited wavetable bank: a few morphable frames, each stored as an
// octave-spaced set of mip levels so no harmonic ever crosses Nyquist.
class WavetableBank
{
public:
    static constexpr int tableSize = 2048;
    static constexpr int numLevels = 11;
    static constexpr int numFrames = 5; // sine, triangle, saw, square, pulse 25%

    WavetableBank();

    // Shared, built once per process.
    static const WavetableBank& get();

    static int levelFor (double frequency, double sampleRate);

    // phase in [0, 1), position in [0, 1] morphs across frames.
    float lookup (int level, float position, double phase) const;

private:
    std::vector<float> data;

    float* table (int frame, int level) { return data.data() + (frame * numLevels + level) * (tableSize + 1); }
    const float* table (int frame, int level) const { return data.data() + (frame * numLevels + level) * (tableSize + 1); }
};
