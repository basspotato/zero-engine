#pragma once

#include <juce_dsp/juce_dsp.h>
#include "SpectralCompressor.h"

// Per-instrument finishing chain, in order:
// saturation (Grit) > delay + reverb > 3-band upward+downward compression
// > tilt + air shelf > mono bass / stereo width > soft clipper > output gain.
struct FinishingSettings
{
    float grit      = 0.0f; // 0..1
    float delayMix      = 0.0f; // 0..1
    float delaySeconds  = 0.375f;
    float delayFeedback = 0.35f;
    float reverbMix     = 0.0f; // 0..1
    float reverbSize    = 0.6f;
    float ottDepth  = 0.0f; // 0..1
    float spectral  = 0.0f; // 0..1, the many-band stage just before the clipper
    float tiltDb    = 0.0f; // +/- dB around a 1 kHz pivot
    float air       = 0.0f; // 0..1
    float monoFreq  = 120.0f;
    float sideGain  = 1.0f; // side level above monoFreq
    float outputDb  = 0.0f;
    float midHighPassHz = 120.0f; // used when a separate sub is passed to process()
};

class FinishingChain
{
public:
    void prepare (double sampleRate, int maxBlockSize, int numChannels);
    void reset();
    // With `sub` (a mono clean sub, one sample per buffer sample), the buffer is
    // high-passed before saturation and the sub skips the chain, joining just
    // before the soft clipper: clean sub, dirty mids.
    void process (juce::AudioBuffer<float>& buffer, const FinishingSettings& settings, const float* sub = nullptr);

    int getLatencySamples() const;

    // What the chain is doing, for the display. Written per block on the
    // audio thread, read on the message thread.
    struct Meters
    {
        std::atomic<float> driveDb { 0 };
        std::atomic<float> bandGainDb[3] {};   // low, mid, high; + is upward
        std::atomic<float> tiltDb { 0 }, airDb { 0 };
        std::atomic<float> monoFreq { 120 }, width { 0 }; // side/mid ratio of the output
        std::atomic<float> clipDb { 0 };       // peak reduction this block
        std::atomic<float> rmsDb { -100 }, peakDb { -100 };
        std::atomic<float> momentaryLufs { -100 }, shortTermLufs { -100 }; // BS.1770: 0.4 s and 3 s windows
        std::atomic<float> channelPeakDb[2] { -100, -100 }, channelRmsDb[2] { -100, -100 }; // L, R (mono: both)
        std::atomic<float> correlation { 1 }; // L/R, -1..1, this block
    };

    const Meters& getMeters() const noexcept { return meters; }

private:
    // RBJ biquad (high shelf or high-pass), transposed direct form II.
    // Coefficients are plain floats, so updating them on the audio thread is
    // allocation-free.
    struct Shelf
    {
        void set (double fs, double freq, double q, double gainDb);
        void setHighPass (double fs, double freq, double q);
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
    void space (juce::AudioBuffer<float>& buffer, int numSamples, const FinishingSettings& s);
    void compress (juce::AudioBuffer<float>& buffer, int numSamples, float depth);
    void equalise (juce::AudioBuffer<float>& buffer, int numSamples, const FinishingSettings& s);
    void widen (juce::AudioBuffer<float>& buffer, int numSamples);
    void clipAndGain (juce::AudioBuffer<float>& buffer, int numSamples);

    double sampleRate = 44100.0;
    int maxBlock = 512;
    int channels = 2;

    std::unique_ptr<juce::dsp::Oversampling<float>> oversampling;
    SpectralCompressor spectral;
    juce::SmoothedValue<float> drive, bias, sideGain, outputGain;
    juce::SmoothedValue<float> ottDepth; // glides, so turning the knob doesn't crackle
    juce::SmoothedValue<float> tiltSmooth, airSmooth; // dB, likewise
    float dcState[2] {}, dcPrev[2] {}, dcCoeff = 0.999f;

    juce::dsp::LinkwitzRileyFilter<float> xoverLow, xoverHigh, lowAllpass, monoXover;
    Band bands[3];

    // Space: ping-pong delay lines and a reverb, both fed a high-passed send.
    juce::AudioBuffer<float> delayLines, send;
    int delayWrite = 0;
    juce::SmoothedValue<float> delaySamples, delayMix, reverbMix;
    float sendHpState[2] {}, sendHpCoeff = 0.0f, fbLpState[2] {}, fbLpCoeff = 0.0f;
    juce::Reverb reverb;

    Meters meters;
    float bandGainSum[3] {};
    float channelPeak[2] {}, channelMeanSquare[2] {}, sumLR = 0.0f, sumLL = 0.0f, sumRR = 0.0f;
    float meanSquare = 0.0f, rmsCoeff = 0.0f;
    float midEnergy = 0.0f, sideEnergy = 0.0f, clipPeakDb = 0.0f, outPeak = 0.0f;

    Shelf tilt, airShelf, midHighPass;
    float lastMidHighPass = -1.0f;

    // Loudness meter only: K-weighting, then 100 ms energy slots covering 3 s.
    Shelf kPre, kHighPass;
    static constexpr int loudnessSlots = 30;
    float slotEnergy[loudnessSlots] {};
    float slotSum = 0.0f;
    int slotPos = 0, slotFill = 0, slotLength = 4800;
    float lastTiltDb = -100.0f, lastAir = -1.0f, lastMonoFreq = -1.0f;
};
