#pragma once

#include <vector>

// Band-limited wavetable banks: each bank has a few morphable frames, each
// stored as an octave-spaced set of mip levels so no harmonic ever crosses Nyquist.
class WavetableBank
{
public:
    static constexpr int tableSize = 2048;
    static constexpr int numLevels = 11;
    static constexpr int numBanks = 4;  // see waveTableNames
    static constexpr int numFrames = 5; // per bank

    WavetableBank();

    // Shared, built once per process.
    static const WavetableBank& get();

    static int levelFor (double frequency, double sampleRate);

    // phase in [0, 1), position in [0, 1] morphs across the bank's frames.
    float lookup (int bank, int level, float position, double phase) const;

private:
    std::vector<float> data;

    size_t offset (int bank, int frame, int level) const
    {
        return (size_t) (((bank * numFrames + frame) * numLevels + level) * (tableSize + 1));
    }

    float* table (int bank, int frame, int level) { return data.data() + offset (bank, frame, level); }
    const float* table (int bank, int frame, int level) const { return data.data() + offset (bank, frame, level); }
};
