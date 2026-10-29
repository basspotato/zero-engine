#pragma once

#include "SoundTraits.h"

// What Generate aims at, per type (Bass, Lead, Chord, Pad, Pluck, FX): the median
// and spread of each SoundTrait over the owner's sounds (498 measured,
// `ZeroAnalyze library traits --write`; measurement only, nothing trained).
// Order: level, side %, centroid, bass %, body %, spread, crest, movement, richness.
namespace GenerateTargets
{
    struct Target { std::array<float, SoundTraits::count> median, spread; int measured; };
    inline const std::array<Target, 6> byType {{
        { { -16.674f, 0.422f, 133.675f, 84.176f, 11.835f, 0.924f, 10.109f, 3.685f, 0.500f },
          { 7.713f, 9.269f, 212.409f, 31.659f, 20.802f, 0.638f, 4.270f, 1.872f, 0.325f }, 256 }, // Bass
        { { -24.666f, 12.092f, 1142.822f, 0.055f, 32.236f, 1.309f, 13.118f, 3.094f, 0.750f },
          { 8.183f, 13.060f, 537.090f, 15.870f, 29.597f, 0.623f, 4.200f, 1.510f, 0.225f }, 38 }, // Lead
        { { -22.623f, 43.211f, 680.376f, 0.679f, 57.355f, 1.036f, 16.351f, 2.364f, 0.750f },
          { 6.804f, 19.006f, 256.034f, 13.231f, 27.908f, 0.225f, 4.503f, 0.520f, 0.225f }, 13 }, // Chord
        { { -24.737f, 18.449f, 619.969f, 6.425f, 5.765f, 1.102f, 12.580f, 3.848f, 0.350f },
          { 6.016f, 18.285f, 101.742f, 14.247f, 36.718f, 0.815f, 2.230f, 1.262f, 0.175f }, 39 }, // Pad
        { { -22.157f, 19.176f, 843.282f, 0.129f, 35.200f, 1.067f, 14.414f, 4.390f, 0.650f },
          { 5.026f, 28.401f, 1200.392f, 10.682f, 30.773f, 0.445f, 3.716f, 1.976f, 0.225f }, 142 }, // Pluck
        { { -30.815f, 22.135f, 249.959f, 16.132f, 11.625f, 0.934f, 13.023f, 3.291f, 0.700f },
          { 15.715f, 20.862f, 367.566f, 43.364f, 32.602f, 0.359f, 4.448f, 1.867f, 0.250f }, 10 }, // FX
    }};
}
