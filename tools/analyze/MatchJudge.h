#pragma once

#include "../../Source/SoundPrint.h"

// The frozen judge for the Reference Match rounds (agreed with the owner,
// 2026-10-28): a copy of the comparison as it stood before round 1, kept here,
// outside the plugin, so no improvement round can make the verdict more lenient.
// Do not change it while the rounds run; if a round changes how prints are
// measured, say so in analysis/match_log.md.
//
// Same scale as SoundPrint::distance then: under 2 VERY CLOSE, under 3.5 CLOSE,
// under 5 ROUGH, else LOOSE.
float frozenJudge (const SoundPrint& reference, const SoundPrint& match);
juce::String frozenJudgeWord (float distance);
