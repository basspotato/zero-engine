#pragma once

#include "SoundTraits.h"

// What Generate aims at, per type (Bass, Lead, Chord, Pad, Pluck, FX): the median
// and spread of each SoundTrait over the owner's sounds (1187 measured,
// `ZeroAnalyze library traits --write`; measurement only, nothing trained).
// Order: level, side %, centroid, bass %, body %, spread, crest, movement, richness.
namespace GenerateTargets
{
    struct Target { std::array<float, SoundTraits::count> median, spread; int measured; };
    inline const std::array<Target, 6> byType {{
        { { -12.132f, 1.062f, 125.158f, 84.888f, 10.010f, 0.935f, 9.365f, 4.392f, 0.500f },
          { 6.331f, 7.959f, 218.673f, 25.134f, 17.746f, 0.628f, 3.761f, 3.023f, 0.350f }, 580 }, // Bass
        { { -19.331f, 21.552f, 1135.678f, 0.052f, 35.299f, 1.267f, 13.127f, 4.125f, 0.700f },
          { 7.261f, 19.989f, 715.589f, 11.763f, 31.519f, 0.609f, 3.836f, 2.907f, 0.225f }, 88 }, // Lead
        { { -17.201f, 39.766f, 594.789f, 7.552f, 62.300f, 1.009f, 13.888f, 3.112f, 0.650f },
          { 6.282f, 17.993f, 190.895f, 12.919f, 19.943f, 0.225f, 3.154f, 1.168f, 0.200f }, 33 }, // Chord
        { { -17.645f, 31.141f, 594.023f, 0.211f, 55.136f, 0.913f, 12.678f, 3.430f, 0.500f },
          { 7.202f, 20.602f, 266.822f, 13.974f, 39.595f, 0.586f, 2.163f, 1.100f, 0.200f }, 104 }, // Pad
        { { -20.337f, 18.618f, 855.482f, 0.055f, 36.220f, 0.911f, 15.064f, 5.298f, 0.550f },
          { 5.624f, 23.792f, 928.722f, 9.159f, 38.058f, 0.469f, 3.786f, 3.145f, 0.250f }, 348 }, // Pluck
        { { -25.013f, 31.257f, 544.884f, 1.437f, 25.369f, 0.952f, 13.152f, 3.302f, 0.650f },
          { 14.200f, 21.541f, 933.888f, 43.364f, 38.865f, 0.394f, 4.448f, 1.519f, 0.325f }, 34 }, // FX
    }};
}
