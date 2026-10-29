#pragma once

#include "Profiles.h"
#include "SoundTraits.h"
#include <vector>

// Generate's neural judge (a surrogate): predicts what a candidate would
// measure (SoundTraits, clarity, wobble, noise) from its settings alone, so
// Generate can screen many more candidates than it can render and render only
// the promising ones. Trained on renders of this engine (`ZeroAnalyze
// gennetdata` / `gennettrain`); weights in GenerateNetWeights.h. A small MLP:
// inputs -> 128 -> 128 -> outputs, ReLU. Message thread; no allocation per call
// beyond the features.
namespace GenerateNet
{
    constexpr int numHidden = 128;
    constexpr int numOutputs = SoundTraits::count + 3; // traits in the owner's spreads, clarity, wobble / 20 c, flatness x 20

    std::vector<float> featuresOf (const Profile&, int genre, int type);
    int numInputs();

    // What a profile's measurements would be, as the targets are scaled
    // (outputsOf), and back to SoundTraits (traitsOf).
    std::array<float, numOutputs> outputsOf (const SoundTraits&, int type);
    SoundTraits traitsOf (const std::array<float, numOutputs>&, int type);

    struct Net
    {
        std::vector<float> weights;
        bool ready() const noexcept;
        std::array<float, numOutputs> run (const std::vector<float>& features) const;
    };
    size_t numWeights();
    const Net& builtIn(); // empty (not ready) until trained
}
