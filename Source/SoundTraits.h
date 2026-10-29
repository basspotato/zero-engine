#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <array>

// What makes a sound big and interesting, measured (Generate ranks its
// candidates by these; the targets per type come from measuring the owner's
// sounds, see GenerateTargets.h). From the first audible moment, up to 1 s:
//
//   level, width, brightness, bass   the four Generate always checked
//   body       % of energy 150-600 Hz (low mids: weight without sub)
//   spread     how many octaves the spectrum spans (energy-weighted spread of
//              log frequency): big sounds fill more of the range
//   density    crest factor in dB (peak over RMS): lower is denser, more
//              compressed, bigger
//   movement   mean change between successive spectra, dB per third-octave
//              band per 46 ms: how much the sound moves
//   richness   share of third-octave bands from 100 Hz to 10 kHz within 20 dB
//              of the strongest: many partials versus a few
struct SoundTraits
{
    float levelDb = -100.0f, sidePct = 0.0f, centroidHz = 0.0f, bassPct = 0.0f;
    float bodyPct = 0.0f, spreadOct = 0.0f, crestDb = 0.0f, movementDb = 0.0f, richness = 0.0f;

    static constexpr int count = 9;
    std::array<float, count> asArray() const
    {
        return { levelDb, sidePct, centroidHz, bassPct, bodyPct, spreadOct, crestDb, movementDb, richness };
    }

    // Allocates (FFT buffers): never on the audio thread.
    static SoundTraits of (const juce::AudioBuffer<float>& audio, double sampleRate);
};
