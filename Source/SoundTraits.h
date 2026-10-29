#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <vector>

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
    // Clarity (not in asArray: measured separately, as it needs the notes): the
    // share of energy on the played notes' harmonics, 0..1. Detune smear, pitch
    // wobble, noise and inharmonic mush all lower it.
    float clarity = 1.0f;
    // Tuning (also outside asArray, from measureTuning): how far the pitch sits
    // from the note played (median, cents), how much it moves (cents), and how
    // noise-like the spectrum is (median flatness 150 Hz to 5 kHz, 0..1).
    float pitchOffsetCents = 0.0f, pitchSpreadCents = 0.0f, flatness = 0.0f;

    static constexpr int count = 9;
    std::array<float, count> asArray() const
    {
        return { levelDb, sidePct, centroidHz, bassPct, bodyPct, spreadOct, crestDb, movementDb, richness };
    }

    // Allocates (FFT buffers): never on the audio thread.
    // With the notes' fundamentals (Hz), width and movement count only the
    // spectrum near their harmonics (and the sub's octaves below, up to 300 Hz):
    // hiss and wash between them can't pass for width or movement.
    static SoundTraits of (const juce::AudioBuffer<float>& audio, double sampleRate, const std::vector<float>& fundamentals = {});
    static float clarityOf (const juce::AudioBuffer<float>& audio, double sampleRate, const std::vector<float>& fundamentals);
    static void measureTuning (const juce::AudioBuffer<float>& audio, double sampleRate, float expectedHz, SoundTraits& traits);
};
