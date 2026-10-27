#pragma once

#include <juce_core/juce_core.h>

// Matches each profile to the mean of its references on loudness, dynamics,
// brightness, bass and width by adjusting five values (level trim, OTT depth,
// tilt, Weight, stereo width). With a header file given, writes the results
// into it (Source/ProfileTuning.h); otherwise only reports.
int tuneProfiles (const juce::File& refFolder, const juce::File& headerToWrite);

// Self-check without references: hides known changes in a few profiles,
// uses their renders as targets, and checks the tuner finds its way back.
int tuneTest();
