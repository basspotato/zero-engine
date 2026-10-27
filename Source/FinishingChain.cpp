#include "FinishingChain.h"

namespace
{
    constexpr float lowSplit  = 120.0f;  // OTT-style band edges
    constexpr float highSplit = 2500.0f;

    // Gain computer, shared by all three bands.
    constexpr float downThreshDb = -18.0f;
    constexpr float downSlope    = 0.75f;  // 4:1
    constexpr float upThreshDb   = -36.0f;
    constexpr float upSlope      = 0.6f;   // 2.5:1 upward
    constexpr float maxUpDb      = 15.0f;
    constexpr float floorDb      = -78.0f; // no upward gain below here, so tails and silence stay quiet
    constexpr float floorFadeDb  = 18.0f;

    constexpr float clipKnee = 0.8f;

    float coeffFor (double seconds, double fs) { return (float) (1.0 - std::exp (-1.0 / (seconds * fs))); }
}

//==============================================================================
void FinishingChain::Shelf::set (double fs, double freq, double q, double gainDb)
{
    freq = juce::jmin (freq, fs * 0.45);
    const auto A = std::pow (10.0, gainDb / 40.0);
    const auto w0 = juce::MathConstants<double>::twoPi * freq / fs;
    const auto cosw = std::cos (w0);
    const auto alpha = std::sin (w0) / (2.0 * q);
    const auto sqrtA2alpha = 2.0 * std::sqrt (A) * alpha;

    const auto a0 = (A + 1) - (A - 1) * cosw + sqrtA2alpha;
    b0 = (float) (A * ((A + 1) + (A - 1) * cosw + sqrtA2alpha) / a0);
    b1 = (float) (-2 * A * ((A - 1) + (A + 1) * cosw) / a0);
    b2 = (float) (A * ((A + 1) + (A - 1) * cosw - sqrtA2alpha) / a0);
    a1 = (float) (2 * ((A - 1) - (A + 1) * cosw) / a0);
    a2 = (float) (((A + 1) - (A - 1) * cosw - sqrtA2alpha) / a0);
}

float FinishingChain::Shelf::process (int ch, float x) noexcept
{
    const auto y = b0 * x + z1[ch];
    z1[ch] = b1 * x - a1 * y + z2[ch];
    z2[ch] = b2 * x - a2 * y;
    return y;
}

void FinishingChain::Shelf::reset() noexcept
{
    std::fill (std::begin (z1), std::end (z1), 0.0f);
    std::fill (std::begin (z2), std::end (z2), 0.0f);
}

//==============================================================================
void FinishingChain::prepare (double fs, int maxBlockSize, int numChannels)
{
    sampleRate = fs;
    maxBlock = juce::jmax (1, maxBlockSize);
    channels = juce::jlimit (1, 2, numChannels);

    oversampling = std::make_unique<juce::dsp::Oversampling<float>> (
        (size_t) channels, 1, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, false);
    oversampling->initProcessing ((size_t) maxBlock);

    drive.reset (fs * 2.0, 0.02);
    bias.reset (fs * 2.0, 0.02);
    sideGain.reset (fs, 0.02);
    outputGain.reset (fs, 0.02);
    dcCoeff = (float) std::exp (-juce::MathConstants<double>::twoPi * 10.0 / fs);

    const juce::dsp::ProcessSpec spec { fs, (juce::uint32) maxBlock, (juce::uint32) channels };
    for (auto* f : { &xoverLow, &xoverHigh, &lowAllpass, &monoXover })
        f->prepare (spec);

    xoverLow.setCutoffFrequency (lowSplit);
    xoverHigh.setCutoffFrequency (highSplit);
    lowAllpass.setType (juce::dsp::LinkwitzRileyFilterType::allpass);
    lowAllpass.setCutoffFrequency (highSplit);

    // Slower detection for the low band so it doesn't distort the waveform.
    const double times[3][2] { { 0.010, 0.150 }, { 0.005, 0.100 }, { 0.002, 0.060 } };
    for (int b = 0; b < 3; ++b)
    {
        bands[b].attack  = coeffFor (times[b][0], fs);
        bands[b].release = coeffFor (times[b][1], fs);
    }

    lastTiltDb = -100.0f;
    lastAir = lastMonoFreq = -1.0f;
    reset();
}

void FinishingChain::reset()
{
    if (oversampling != nullptr)
        oversampling->reset();

    for (auto* f : { &xoverLow, &xoverHigh, &lowAllpass, &monoXover })
        f->reset();

    for (auto& b : bands)
        b.env = 0.0f;

    tilt.reset();
    airShelf.reset();
    std::fill (std::begin (dcState), std::end (dcState), 0.0f);
    std::fill (std::begin (dcPrev), std::end (dcPrev), 0.0f);
}

int FinishingChain::getLatencySamples() const
{
    return oversampling != nullptr ? juce::roundToInt (oversampling->getLatencyInSamples()) : 0;
}

void FinishingChain::process (juce::AudioBuffer<float>& buffer, const FinishingSettings& s)
{
    jassert (oversampling != nullptr);

    drive.setTargetValue (1.0f + s.grit * 11.0f);
    bias.setTargetValue (s.grit * 0.2f); // a little asymmetry adds even harmonics
    sideGain.setTargetValue (s.sideGain);
    outputGain.setTargetValue (juce::Decibels::decibelsToGain (s.outputDb));

    if (s.monoFreq != lastMonoFreq)
    {
        monoXover.setCutoffFrequency (s.monoFreq);
        lastMonoFreq = s.monoFreq;
    }

    // Hosts may send blocks bigger than promised; the oversampler can't take them.
    for (int start = 0; start < buffer.getNumSamples(); start += maxBlock)
    {
        const auto n = juce::jmin (maxBlock, buffer.getNumSamples() - start);
        juce::AudioBuffer<float> chunk (buffer.getArrayOfWritePointers(), juce::jmin (channels, buffer.getNumChannels()), start, n);

        saturate (chunk, n);
        compress (chunk, n, s.ottDepth);
        equalise (chunk, n, s);
        widen (chunk, n);
        clipAndGain (chunk, n);
    }
}

//==============================================================================
void FinishingChain::saturate (juce::AudioBuffer<float>& buffer, int numSamples)
{
    juce::dsp::AudioBlock<float> block (buffer.getArrayOfWritePointers(), (size_t) buffer.getNumChannels(), (size_t) numSamples);
    auto up = oversampling->processSamplesUp (block);

    for (size_t i = 0; i < up.getNumSamples(); ++i)
    {
        const auto d = drive.getNextValue();
        const auto b = bias.getNextValue();
        const auto norm = 1.0f / std::tanh (d);
        const auto offset = std::tanh (b);

        for (size_t ch = 0; ch < up.getNumChannels(); ++ch)
        {
            auto& x = up.getChannelPointer (ch)[i];
            x = (std::tanh (d * x + b) - offset) * norm;
        }
    }

    oversampling->processSamplesDown (block);

    // DC blocker for the asymmetric curve.
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* data = buffer.getWritePointer (ch);
        for (int i = 0; i < numSamples; ++i)
        {
            const auto x = data[i];
            dcState[ch] = x - dcPrev[ch] + dcCoeff * dcState[ch];
            dcPrev[ch] = x;
            data[i] = dcState[ch];
        }
    }
}

void FinishingChain::compress (juce::AudioBuffer<float>& buffer, int numSamples, float depth)
{
    if (depth <= 0.0f)
    {
        // Keep the crossovers' state warm so turning depth up doesn't click.
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            for (int i = 0; i < numSamples; ++i)
            {
                float lo, hi, mid, top;
                const auto x = buffer.getSample (ch, i);
                xoverLow.processSample (ch, x, lo, hi);
                xoverHigh.processSample (ch, hi, mid, top);
                lowAllpass.processSample (ch, lo);
            }
        return;
    }

    const auto numCh = buffer.getNumChannels();
    float split[2][3];

    for (int i = 0; i < numSamples; ++i)
    {
        for (int ch = 0; ch < numCh; ++ch)
        {
            float lo, hi;
            xoverLow.processSample (ch, buffer.getSample (ch, i), lo, hi);
            xoverHigh.processSample (ch, hi, split[ch][1], split[ch][2]);
            split[ch][0] = lowAllpass.processSample (ch, lo); // phase-match the low band
        }

        float out[2] {};

        for (int b = 0; b < 3; ++b)
        {
            auto& band = bands[b];

            // Stereo-linked peak follower.
            auto peak = std::abs (split[0][b]);
            if (numCh > 1)
                peak = juce::jmax (peak, std::abs (split[1][b]));

            band.env += (peak > band.env ? band.attack : band.release) * (peak - band.env);

            const auto levelDb = 20.0f * std::log10 (band.env + 1.0e-9f);
            auto gainDb = 0.0f;

            if (levelDb > downThreshDb)
                gainDb = (downThreshDb - levelDb) * downSlope;
            else if (levelDb < upThreshDb)
                gainDb = juce::jmin ((upThreshDb - levelDb) * upSlope, maxUpDb)
                       * juce::jlimit (0.0f, 1.0f, (levelDb - floorDb) / floorFadeDb);

            const auto gain = juce::Decibels::decibelsToGain (gainDb * depth);

            for (int ch = 0; ch < numCh; ++ch)
                out[ch] += split[ch][b] * gain;
        }

        for (int ch = 0; ch < numCh; ++ch)
            buffer.setSample (ch, i, out[ch]);
    }
}

void FinishingChain::equalise (juce::AudioBuffer<float>& buffer, int numSamples, const FinishingSettings& s)
{
    if (s.tiltDb != lastTiltDb)
    {
        // Shelf of twice the tilt, with the opposite half applied as broadband gain, pivots at 1 kHz.
        tilt.set (sampleRate, 1000.0, 0.5, s.tiltDb * 2.0f);
        lastTiltDb = s.tiltDb;
    }

    if (s.air != lastAir)
    {
        airShelf.set (sampleRate, 8000.0, 0.7, s.air * 9.0f);
        lastAir = s.air;
    }

    const auto pivotGain = juce::Decibels::decibelsToGain (-s.tiltDb);

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* data = buffer.getWritePointer (ch);
        for (int i = 0; i < numSamples; ++i)
            data[i] = airShelf.process (ch, tilt.process (ch, data[i] * pivotGain));
    }
}

void FinishingChain::widen (juce::AudioBuffer<float>& buffer, int numSamples)
{
    if (buffer.getNumChannels() < 2)
        return;

    auto* left = buffer.getWritePointer (0);
    auto* right = buffer.getWritePointer (1);

    for (int i = 0; i < numSamples; ++i)
    {
        float lLow, lHigh, rLow, rHigh;
        monoXover.processSample (0, left[i], lLow, lHigh);
        monoXover.processSample (1, right[i], rLow, rHigh);

        const auto bass = 0.5f * (lLow + rLow);
        const auto mid = 0.5f * (lHigh + rHigh);
        const auto side = 0.5f * (lHigh - rHigh) * sideGain.getNextValue();

        left[i] = bass + mid + side;
        right[i] = bass + mid - side;
    }
}

void FinishingChain::clipAndGain (juce::AudioBuffer<float>& buffer, int numSamples)
{
    const auto numCh = buffer.getNumChannels();

    for (int i = 0; i < numSamples; ++i)
    {
        const auto gain = outputGain.getNextValue();

        for (int ch = 0; ch < numCh; ++ch)
        {
            auto x = buffer.getSample (ch, i);
            const auto ax = std::abs (x);

            // Linear up to the knee, then eases into a 0 dBFS ceiling.
            if (ax > clipKnee)
                x = std::copysign (clipKnee + (1.0f - clipKnee) * std::tanh ((ax - clipKnee) / (1.0f - clipKnee)), x);

            buffer.setSample (ch, i, x * gain);
        }
    }
}
