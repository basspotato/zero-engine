#pragma once

#include "Profiles.h"

// Generate's Mood mode: sounds free of the genre lanes, made from recipes
// rather than from a genre's designed profile. Each mood picks its own type
// (a lush pad loads as a Pad) and draws its settings from wide ranges that
// suit it: warm wavetables, gentle detune, slow attacks, long tails, space,
// slow movement. A genre's profile only fills what a recipe leaves alone
// (the finishing chain, the clean-sub rules), from genres that suit the mood.
namespace Moods
{
    enum { lushPad, airyDrone, warmKeys, glassyBell, darkTexture, softPluck, soaringLead, deepBass, pluckBass, anthemLead, stabChords, count };

    const char* nameOf (int mood);  // "LUSH PAD"...
    int typeOf (int mood);          // the type it loads as (Bass, Lead, Chord, Pad, Pluck, FX = 0..5)

    // A new sound in the mood; baseGenre: the genre whose profile filled the rest.
    Profile make (int mood, juce::Random&, int& baseGenre);
}
