#pragma once

#include <array>
#include <vector>

// Band-limited wavetable banks: each bank has a few morphable frames, each
// stored as an octave-spaced set of mip levels so no harmonic ever crosses Nyquist.
class WavetableBank
{
public:
    static constexpr int tableSize = 2048;
    static constexpr int numLevels = 11;
    static constexpr int numBanks = 14; // see waveTableNames
    static constexpr int numFrames = 5; // per bank

    // The Sample bank: harmonics measured from a sound (Reference Match cuts
    // them from the reference), held per plugin instance; the shared bank
    // holds a plain saw in its place for when none is set.
    static constexpr int sampleBank = 13;
    static constexpr int numSampleHarmonics = 64;
    using Harmonics = std::array<std::array<float, numSampleHarmonics>, numFrames>; // amplitude of harmonic n + 1 per frame

    WavetableBank();
    explicit WavetableBank (const Harmonics& harmonics); // just the Sample bank, from these

    // Shared, built once per process.
    static const WavetableBank& get();

    static int levelFor (double frequency, double sampleRate);

    // phase in [0, 1), position in [0, 1] morphs across the bank's frames.
    // A Sample-bank-only object ignores bank.
    float lookup (int bank, int level, float position, double phase) const;

private:
    int banksHeld;
    std::vector<float> data;

    void build (int bank, const Harmonics* measured);

    size_t offset (int bank, int frame, int level) const
    {
        const auto slot = banksHeld == 1 ? 0 : bank;
        return (size_t) (((slot * numFrames + frame) * numLevels + level) * (tableSize + 1));
    }

    float* table (int bank, int frame, int level) { return data.data() + offset (bank, frame, level); }
    const float* table (int bank, int frame, int level) const { return data.data() + offset (bank, frame, level); }
};
