#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "MacroMap.h"

struct ZeroSound : public juce::SynthesiserSound
{
    bool appliesToNote (int) override    { return true; }
    bool appliesToChannel (int) override { return true; }
};

// One note: up to 7 unison wavetable oscillators spread in pitch and stereo,
// a sine sub, an amp envelope, a mod envelope and a per-voice LFO.
class ZeroVoice : public juce::SynthesiserVoice
{
public:
    static constexpr int maxUnison = 7;

    ZeroVoice (juce::AudioProcessorValueTreeState& state, const MacroMap& macroMap);

    bool canPlaySound (juce::SynthesiserSound* sound) override;
    void startNote (int midiNote, float velocity, juce::SynthesiserSound*, int pitchWheel) override;
    void stopNote (float velocity, bool allowTailOff) override;
    void pitchWheelMoved (int newValue) override;
    void controllerMoved (int, int) override {}
    void setCurrentPlaybackSampleRate (double newRate) override;
    void renderNextBlock (juce::AudioBuffer<float>& output, int startSample, int numSamples) override;

    // Mono/legato: move this sounding voice to a new note, sliding over the
    // Glide time. retrigger restarts the envelopes (Mono); Legato doesn't.
    void glideTo (int midiNote, float velocity, bool retrigger);

private:
    struct Params
    {
        std::atomic<float>* weight;
        std::atomic<float>* punch;
        std::atomic<float>* width;
        std::atomic<float>* motion;
        std::atomic<float>* wavePos;
        std::atomic<float>* unison;
        std::atomic<float>* detune;
        std::atomic<float>* blend;
        std::atomic<float>* ampAttack;
        std::atomic<float>* ampDecay;
        std::atomic<float>* ampSustain;
        std::atomic<float>* ampRelease;
        std::atomic<float>* modAttack;
        std::atomic<float>* modDecay;
        std::atomic<float>* modSustain;
        std::atomic<float>* modRelease;
        std::atomic<float>* modToPos;
        std::atomic<float>* lfoRate;
        std::atomic<float>* lfoShape;
        std::atomic<float>* lfoToPos;
        std::atomic<float>* lfoToPitch;
        std::atomic<float>* modToPitch;
        std::atomic<float>* subOctave;
        std::atomic<float>* glide;
        std::atomic<float>* bendRange;
    } p;

    const MacroMap& macroMap;

    void updateEnvelopes();
    float nextLfo (float increment);

    juce::ADSR ampEnv, modEnv;
    juce::Random random;

    // Pitch in octaves (log2 Hz), so glides are even in pitch.
    double pitch = std::log2 (440.0), targetPitch = std::log2 (440.0);
    float bend = 0.0f; // pitch wheel, -1..1
    float level = 0.0f;
    double phases[maxUnison] {};
    double subPhase = 0.0;
    float lfoPhase = 0.0f;
    float lfoHeld = 0.0f;
};

// juce::Synthesiser plus Mono and Legato play modes: one voice and a stack of
// held notes, so releasing a note slides back to the one still held.
class ZeroSynth : public juce::Synthesiser
{
public:
    explicit ZeroSynth (juce::AudioProcessorValueTreeState& state);

    void noteOn (int midiChannel, int midiNoteNumber, float velocity) override;
    void noteOff (int midiChannel, int midiNoteNumber, float velocity, bool allowTailOff) override;
    void allNotesOff (int midiChannel, bool allowTailOff) override;

private:
    enum Mode { poly, mono, legato };

    void removeHeld (int note);

    std::atomic<float>& playMode;
    int lastMode = poly;

    static constexpr int maxHeld = 16;
    int heldNotes[maxHeld] {};
    float heldVelocities[maxHeld] {};
    int numHeld = 0;
};
