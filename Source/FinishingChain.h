#pragma once

#include <juce_dsp/juce_dsp.h>

// Per-instrument finishing chain, in order:
// saturation (Grit) > 3-band upward+downward compression > tilt + air shelf
// > mono bass / stereo width > soft clipper > output gain.
struct FinishingSettings
{
    float grit      = 0.0f; // 0..1
    float ottDepth  = 0.0f; // 0..1
    float tiltDb    = 0.0f; // +/- dB around a 1 kHz pivot
    float air       = 0.0f; // 0..1
    float monoFreq  = 120.0f;
    float sideGain  = 1.0f; // side level above monoFreq
    float outputDb  = 0.0f;
};

class FinishingChain
{
public:
    void prepare (double sampleRate, int maxBlockSize, int numChannels);
    void reset();
    void process (juce::AudioBuffer<float>& buffer, const FinishingSettings& settings);

    int getLatencySamples() const;

private:
    // RBJ high shelf, transposed direct form II. Coefficients are plain floats,
    // so updating them on the audio thread is allocation-free.
    struct Shelf
    {
        void set (double fs, double freq, double q, double gainDb);
        float process (int ch, float x) noexcept;
        void reset() noexcept;

        float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
        float z1[2] {}, z2[2] {};
    };

    struct Band
    {
        float env = 0.0f;
        float attack = 0.0f, release = 0.0f;
    };

    void saturate (juce::AudioBuffer<float>& buffer, int numSamples);
    void compress (juce::AudioBuffer<float>& buffer, int numSamples, float depth);
    void equalise (juce::AudioBuffer<float>& buffer, int numSamples, const FinishingSettings& s);
    void widen (juce::AudioBuffer<float>& buffer, int numSamples);
    void clipAndGain (juce::AudioBuffer<float>& buffer, int numSamples);

    double sampleRate = 44100.0;
    int maxBlock = 512;
    int channels = 2;

    std::unique_ptr<juce::dsp::Oversampling<float>> oversampling;
    juce::SmoothedValue<float> drive, bias, sideGain, outputGain;
    float dcState[2] {}, dcPrev[2] {}, dcCoeff = 0.999f;

    juce::dsp::LinkwitzRileyFilter<float> xoverLow, xoverHigh, lowAllpass, monoXover;
    Band bands[3];

    Shelf tilt, airShelf;
    float lastTiltDb = -100.0f, lastAir = -1.0f, lastMonoFreq = -1.0f;
};
