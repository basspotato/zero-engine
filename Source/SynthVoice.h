#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

struct ZeroSound : public juce::SynthesiserSound
{
    bool appliesToNote (int) override    { return true; }
    bool appliesToChannel (int) override { return true; }
};

// Placeholder voice: three detuned anti-aliased saws (unison spread by Width)
// plus a sine sub (Weight), shaped by an ADSR whose attack follows Punch.
class ZeroVoice : public juce::SynthesiserVoice
{
public:
    explicit ZeroVoice (juce::AudioProcessorValueTreeState& state);

    bool canPlaySound (juce::SynthesiserSound* sound) override;
    void startNote (int midiNote, float velocity, juce::SynthesiserSound*, int pitchWheel) override;
    void stopNote (float velocity, bool allowTailOff) override;
    void pitchWheelMoved (int) override {}
    void controllerMoved (int, int) override {}
    void setCurrentPlaybackSampleRate (double newRate) override;
    void renderNextBlock (juce::AudioBuffer<float>& output, int startSample, int numSamples) override;

private:
    std::atomic<float>& weight;
    std::atomic<float>& punch;
    std::atomic<float>& width;

    juce::ADSR adsr;
    double frequency = 440.0;
    float level = 0.0f;
    float sawPhase[3] {};
    float subPhase = 0.0f;
};
