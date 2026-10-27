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
juce::AudioBuffer<float> renderProfile (int genre, int type, const Profile* candidate = nullptr);
