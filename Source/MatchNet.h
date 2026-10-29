#pragma once

#include "SoundPrint.h"
#include "ReferenceMatch.h"
#include <vector>

// Neural starting points for Reference Match (the approach Synplant 2's
// Genopatch takes): a small network, trained only on renders of this engine,
// reads a reference's first note and guesses which preset, wavetable and
// settings made it. The search then starts from its guesses as well as from
// the closest presets. Runs on the match thread, never the audio thread.
namespace MatchNet
{
    // Input: the first note's log-mel spectrum averaged over four stretches
    // (0-50 ms, 50-150 ms, 150-400 ms, 400 ms-1 s), 0..1 from -80..0 dB, plus
    // its stereo width and pitch.
    constexpr int numBands = SoundPrint::melBands;
    constexpr int numInputs = 4 * numBands + 2;
    constexpr int numHidden = 256;
    constexpr int numPresets = 72;   // genre * 6 + type
    constexpr int numTables = 13;    // the built-in wavetables (the Sample one is the reference's own)
    constexpr int numUnits = ReferenceMatch::numSearched;
    constexpr int numOutputs = numPresets + numTables + numUnits;

    using Features = std::array<float, numInputs>;
    Features featuresOf (const SoundPrint&);

    // Weights, layer by layer: W1 [hidden x inputs], b1, W2 [hidden x hidden],
    // b2, W3 [outputs x hidden], b3.
    constexpr size_t numWeights = (size_t) numHidden * numInputs + numHidden + (size_t) numHidden * numHidden + numHidden
                                + (size_t) numOutputs * numHidden + numOutputs;

    struct Net
    {
        std::vector<float> weights; // numWeights, or empty: no net
        bool ready() const noexcept { return weights.size() == numWeights; }

        // Raw outputs: preset logits, table logits, unit logits (sigmoid gives 0..1).
        std::array<float, numOutputs> run (const Features&) const;
    };

    // The net built into the plugin (MatchNetWeights.h), or empty if none was trained.
    const Net& builtIn();

    struct Guess { int genre = 0, type = 0, table = 0; std::array<float, numUnits> units {}; float probability = 0.0f; };
    // The count most likely presets, each with the net's table and settings.
    std::vector<Guess> guess (const Net&, const SoundPrint&, int count);
}
