#pragma once

#include "SoundTraits.h"

// What Generate aims at, per type (Bass, Lead, Chord, Pad, Pluck, FX): the median
// and spread of each SoundTrait over the owner's sounds: their own projects for Bass,
// Lead, Pad and Pluck (476, packs left out: loose labels), all 1,187 for Chord and
// FX (too few of their own). `ZeroAnalyze library traits [exclude=...] --write`;
// measurement only, nothing trained.
// Order: level, side %, centroid, bass %, body %, spread, crest, movement, richness.
namespace GenerateTargets
{
    struct Target { std::array<float, SoundTraits::count> median, spread; int measured; };
    inline const std::array<Target, 6> byType {{
        { { -16.674f, 0.430f, 125.158f, 84.394f, 11.647f, 0.928f, 9.848f, 3.702f, 0.500f },
          { 7.713f, 8.996f, 212.409f, 28.656f, 18.890f, 0.638f, 4.158f, 1.784f, 0.325f }, 254 }, // Bass
        { { -26.965f, 11.794f, 1142.822f, 0.059f, 30.318f, 1.329f, 13.118f, 3.453f, 0.750f },
          { 7.051f, 12.499f, 521.834f, 14.363f, 29.597f, 0.591f, 3.923f, 1.286f, 0.225f }, 40 }, // Lead
        { { -17.201f, 39.766f, 594.789f, 7.552f, 62.300f, 1.009f, 13.888f, 3.112f, 0.650f },
          { 6.282f, 17.993f, 190.895f, 12.919f, 19.943f, 0.225f, 3.154f, 1.168f, 0.200f }, 33 }, // Chord
        { { -23.867f, 17.662f, 614.281f, 15.764f, 26.446f, 1.051f, 12.559f, 3.438f, 0.400f },
          { 6.389f, 18.520f, 133.476f, 14.453f, 36.416f, 0.815f, 2.207f, 1.075f, 0.175f }, 41 }, // Pad
        { { -21.796f, 19.074f, 874.728f, 0.087f, 35.200f, 1.047f, 14.782f, 4.420f, 0.650f },
          { 4.847f, 28.129f, 1254.779f, 10.683f, 32.446f, 0.445f, 3.772f, 1.937f, 0.225f }, 141 }, // Pluck
        { { -25.013f, 31.257f, 544.884f, 1.437f, 25.369f, 0.952f, 13.152f, 3.302f, 0.650f },
          { 14.200f, 21.541f, 933.888f, 43.364f, 38.865f, 0.394f, 4.448f, 1.519f, 0.325f }, 34 }, // FX
    }};
}
