#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

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

    explicit ZeroVoice (juce::AudioProcessorValueTreeState& state);

    bool canPlaySound (juce::SynthesiserSound* sound) override;
    void startNote (int midiNote, float velocity, juce::SynthesiserSound*, int pitchWheel) override;
    void stopNote (float velocity, bool allowTailOff) override;
    void pitchWheelMoved (int) override {}
    void controllerMoved (int, int) override {}
    void setCurrentPlaybackSampleRate (double newRate) override;
    void renderNextBlock (juce::AudioBuffer<float>& output, int startSample, int numSamples) override;

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
    } p;

    void updateEnvelopes();
    float nextLfo (float increment);

    juce::ADSR ampEnv, modEnv;
    juce::Random random;

    double frequency = 440.0;
    float level = 0.0f;
    double phases[maxUnison] {};
    double subPhase = 0.0;
    float lfoPhase = 0.0f;
    float lfoHeld = 0.0f;
};
