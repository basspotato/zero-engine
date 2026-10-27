#pragma once

#include <juce_core/juce_core.h>

// Freesound CC0 sourcing for reference sounds. Credentials come from the
// FREESOUND_CLIENT_ID and FREESOUND_CLIENT_SECRET environment variables and
// never leave this machine except to freesound.org.

// Searches every genre x type pair for CC0 sounds and writes a shortlist CSV.
// Needs only the client secret (API key). Edit the CSV to drop sounds before fetching.
int screenFreesound (const juce::File& shortlist, int perPair);

// Downloads every sound in the shortlist at full quality into
// <outDir>/<Genre>/<Type>/ and writes <outDir>/credits.csv. Opens the browser
// once so the owner can approve access (OAuth2).
int fetchFreesound (const juce::File& shortlist, const juce::File& outDir, int maxDownloads);
