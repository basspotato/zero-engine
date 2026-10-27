#pragma once

// Per-profile corrections found by `ZeroAnalyze tune` against reference
// sounds. Written by that tool; edit by hand only to clear an entry.
// Untuned entries leave the hand-designed profile unchanged.

struct ProfileTrim
{
    bool tuned;
    float levelDb, ottDepth, tilt, weight, stereoWidth;
};

// [genre][type], in genreNames / typeNames order.
inline constexpr ProfileTrim profileTrims[12][6] {
    { {}, {}, {}, {}, {}, {} }, // EDM
    { {}, {}, {}, {}, {}, {} }, // Dubstep
    { {}, {}, { true, 0.000f, 0.300f, -2.750f, 0.150f, 1.200f }, {}, {}, {} }, // Techno
    { {}, {}, {}, {}, { true, 0.000f, 0.500f, 2.500f, 0.100f, 1.100f }, {} }, // Tech House
    { {}, {}, {}, {}, {}, {} }, // Trap
    { {}, {}, {}, {}, { true, 0.000f, 0.350f, 1.500f, 0.100f, 1.100f }, {} }, // Synthwave
    { {}, {}, {}, {}, {}, {} }, // Drum & Bass
    { {}, {}, {}, {}, {}, {} }, // Pop
    { {}, {}, {}, {}, {}, {} }, // House
    { {}, {}, {}, {}, {}, {} }, // Trance
    { {}, {}, {}, {}, {}, {} }, // Future Bass
    { {}, {}, {}, {}, {}, {} }, // Hardstyle
};
