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
    { {}, {}, {}, {}, {}, {} }, // Techno
    { {}, {}, {}, {}, {}, {} }, // Tech House
    { {}, {}, {}, {}, {}, {} }, // Trap
    { {}, {}, {}, {}, {}, {} }, // Synthwave
    { {}, {}, {}, {}, {}, {} }, // Drum & Bass
    { {}, {}, {}, {}, {}, {} }, // Pop
    { {}, {}, {}, {}, {}, {} }, // House
    { {}, {}, {}, {}, {}, {} }, // Trance
    { {}, {}, {}, {}, {}, {} }, // Future Bass
    { {}, {}, {}, {}, {}, {} }, // Hardstyle
};
