#pragma once

#include <juce_dsp/juce_dsp.h>
#include <vector>

// A many-band OTT: the signal is split into 256 frequency bins (512-point STFT,
// 75 % overlap) and each bin is compressed upward and downward towards a pink
// reference that follows the sound's own level. Loud resonances come down, thin
// gaps fill in, and the result is smoother and denser. Bins below 150 Hz are
// left alone, so the sub and low end keep their weight. Stereo-linked, so the
// image doesn't move. Adds getLatencySamples() of delay at every setting.
// Our own implementation; real-time safe once prepared.
class SpectralCompressor
{
public:
    static constexpr int order = 9, size = 1 << order, hop = size / 4;

    void prepare (double sampleRate, int numChannels);
    void reset() noexcept;
    int getLatencySamples() const noexcept { return size; }

    // amount 0..1: 0 passes the signal through unchanged (only delayed).
    void process (juce::AudioBuffer<float>& buffer, int numSamples, float amount) noexcept;

private:
    void processFrame (float amount) noexcept;

    juce::dsp::FFT fft { order };
    std::vector<float> window, levelDb, gainDb, binHz;
    std::vector<float> input[2], output[2], frame[2]; // frame holds 2 * size for the FFT
    int channels = 2, pos = 0, hopCount = 0, firstBin = 1;
    float attack = 0.0f, release = 0.0f; // per frame
};
