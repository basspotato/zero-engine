#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include "Descriptors.h"

struct Profile;

// Shared by the ZeroAnalyze commands: reference folders and engine renders.

inline constexpr double renderRate = 48000.0;
inline constexpr int renderBlock = 512;

struct Item
{
    juce::String name;
    int genre = -1, type = -1; // -1: no matching folder
    Descriptors d;
};

juce::String normalise (const juce::String& s);
juce::String groupName (int genre, int type);

// Measures every audio file under root; genre and type come from the folder
// names (<Genre>/<Type>/..., any depth).
std::vector<Item> analyseFolder (const juce::File& root);

// Plays the type's test phrase through the engine with a profile loaded:
// the genre/type's own profile, or `candidate` when given (the tuner's).
// rootNote transposes the phrase so its lowest note is that MIDI note
// (-1: the type's default register).
juce::AudioBuffer<float> renderProfile (int genre, int type, const Profile* candidate = nullptr, int rootNote = -1);

// The note a group of references mostly plays (median detected pitch), or -1
// when fewer than half of them have a clear pitch. Kept to the type's range.
int groupNote (const std::vector<const Item*>& items, int type);

// The phrase root that makes this profile's render show the same detected
// pitch as the references (detected against detected, so a sub an octave
// down reads the same on both sides), or -1 to keep the default register.
int matchedRoot (int genre, int type, int referenceNote);
