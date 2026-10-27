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
    constexpr float floorDb      = -60.0f; // no upward gain below here, so reverb tails and silence still die away
    constexpr float floorFadeDb  = 12.0f;  // full upward gain from -48 dB

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
    rmsCoeff = coeffFor (0.3, fs);

    delayLines.setSize (2, (int) std::ceil (2.1 * fs)); // up to a half note at 60 BPM
    send.setSize (2, maxBlock);
    delaySamples.reset (fs, 0.1);
    delayMix.reset (fs, 0.05);
    reverbMix.reset (fs, 0.05);
    sendHpCoeff = (float) std::exp (-juce::MathConstants<double>::twoPi * 250.0 / fs);
    fbLpCoeff = (float) std::exp (-juce::MathConstants<double>::twoPi * 6000.0 / fs);
    reverb.setSampleRate (fs);
    slotLength = juce::jmax (1, juce::roundToInt (0.1 * fs));

    // BS.1770 K-weighting at this sample rate (same prototypes as tools/analyze).
    {
        const auto k = std::tan (juce::MathConstants<double>::pi * 1681.974450955533 / fs);
        const auto q = 0.7071752369554196;
        const auto vh = std::pow (10.0, 3.999843853973347 / 20.0);
        const auto vb = std::pow (vh, 0.4996667741545416);
        const auto a0 = 1.0 + k / q + k * k;
        kPre.b0 = (float) ((vh + vb * k / q + k * k) / a0);
        kPre.b1 = (float) (2.0 * (k * k - vh) / a0);
        kPre.b2 = (float) ((vh - vb * k / q + k * k) / a0);
        kPre.a1 = (float) (2.0 * (k * k - 1.0) / a0);
        kPre.a2 = (float) ((1.0 - k / q + k * k) / a0);
    }
    {
        const auto k = std::tan (juce::MathConstants<double>::pi * 38.13547087602444 / fs);
        const auto q = 0.5003270373238773;
        const auto a0 = 1.0 + k / q + k * k;
        kHighPass.b0 = 1.0f; kHighPass.b1 = -2.0f; kHighPass.b2 = 1.0f;
        kHighPass.a1 = (float) (2.0 * (k * k - 1.0) / a0);
        kHighPass.a2 = (float) ((1.0 - k / q + k * k) / a0);
    }

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
    meanSquare = 0.0f;
    delayLines.clear();
    delayWrite = 0;
    reverb.reset();
    std::fill (std::begin (sendHpState), std::end (sendHpState), 0.0f);
    std::fill (std::begin (fbLpState), std::end (fbLpState), 0.0f);
    kPre.reset();
    kHighPass.reset();
    std::fill (std::begin (slotEnergy), std::end (slotEnergy), 0.0f);
    slotSum = 0.0f;
    slotPos = slotFill = 0;
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
    delaySamples.setTargetValue (juce::jlimit (1.0f, (float) delayLines.getNumSamples() - 2.0f, s.delaySeconds * (float) sampleRate));
    delayMix.setTargetValue (s.delayMix);
    reverbMix.setTargetValue (s.reverbMix);

    juce::Reverb::Parameters reverbParams;
    reverbParams.roomSize = s.reverbSize;
    reverbParams.damping = 0.5f;
    reverbParams.wetLevel = 1.0f;
    reverbParams.dryLevel = 0.0f;
    reverbParams.width = 1.0f;
    reverb.setParameters (reverbParams);
    outputGain.setTargetValue (juce::Decibels::decibelsToGain (s.outputDb));

    if (s.monoFreq != lastMonoFreq)
    {
        monoXover.setCutoffFrequency (s.monoFreq);
        lastMonoFreq = s.monoFreq;
    }

    std::fill (std::begin (bandGainSum), std::end (bandGainSum), 0.0f);
    midEnergy = sideEnergy = clipPeakDb = outPeak = 0.0f;

    // Hosts may send blocks bigger than promised; the oversampler can't take them.
    for (int start = 0; start < buffer.getNumSamples(); start += maxBlock)
    {
        const auto n = juce::jmin (maxBlock, buffer.getNumSamples() - start);
        juce::AudioBuffer<float> chunk (buffer.getArrayOfWritePointers(), juce::jmin (channels, buffer.getNumChannels()), start, n);

        saturate (chunk, n);
        space (chunk, n, s);
        compress (chunk, n, s.ottDepth);
        equalise (chunk, n, s);
        widen (chunk, n);
        clipAndGain (chunk, n);
    }

    const auto total = (float) juce::jmax (1, buffer.getNumSamples());
    meters.driveDb = juce::Decibels::gainToDecibels (drive.getCurrentValue());
    for (int b = 0; b < 3; ++b)
        meters.bandGainDb[b] = bandGainSum[b] / total;
    meters.tiltDb = s.tiltDb;
    meters.airDb = s.air * 9.0f;
    meters.monoFreq = s.monoFreq;
    meters.width = midEnergy > 1.0e-9f ? std::sqrt (sideEnergy / midEnergy) : 0.0f;
    meters.clipDb = clipPeakDb;
    meters.rmsDb = juce::Decibels::gainToDecibels (std::sqrt (meanSquare), -100.0f);
    meters.peakDb = juce::Decibels::gainToDecibels (outPeak, -100.0f);

    auto lufs = [this] (int slots)
    {
        auto sum = 0.0f;
        for (int i = 1; i <= slots; ++i)
            sum += slotEnergy[(slotPos - i + loudnessSlots) % loudnessSlots];
        const auto z = sum / (float) slots;
        return z > 1.0e-10f ? -0.691f + 10.0f * std::log10 (z) : -100.0f;
    };
    meters.momentaryLufs = lufs (4);
    meters.shortTermLufs = lufs (loudnessSlots);
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

void FinishingChain::space (juce::AudioBuffer<float>& buffer, int numSamples, const FinishingSettings& s)
{
    const auto numCh = buffer.getNumChannels();
    const auto stereo = numCh > 1;
    const auto lineLength = delayLines.getNumSamples();
    auto* lineL = delayLines.getWritePointer (0);
    auto* lineR = delayLines.getWritePointer (1);
    auto* sendL = send.getWritePointer (0);
    auto* sendR = send.getWritePointer (1);
    const auto feedback = s.delayFeedback;

    // High-passed send, so delay and reverb keep the low end dry.
    for (int i = 0; i < numSamples; ++i)
    {
        for (int ch = 0; ch < 2; ++ch)
        {
            const auto x = buffer.getSample (stereo ? ch : 0, i);
            sendHpState[ch] = x + sendHpCoeff * (sendHpState[ch] - x); // one-pole low-pass...
            (ch == 0 ? sendL : sendR)[i] = x - sendHpState[ch];        // ...subtracted: high-pass
        }
    }

    // Ping-pong: the input enters the left line, each line feeds the other.
    // Reverb runs on the same send afterwards, so it isn't fed the echoes twice.
    for (int i = 0; i < numSamples; ++i)
    {
        const auto d = delaySamples.getNextValue();
        auto readPos = (float) delayWrite - d;
        if (readPos < 0.0f)
            readPos += (float) lineLength;

        const auto i0 = (int) readPos;
        const auto i1 = (i0 + 1) % lineLength;
        const auto frac = readPos - (float) i0;
        const auto outL = lineL[i0] + frac * (lineL[i1] - lineL[i0]);
        const auto outR = lineR[i0] + frac * (lineR[i1] - lineR[i0]);

        // Darken the repeats a little each time round.
        fbLpState[0] = outR + fbLpCoeff * (fbLpState[0] - outR);
        fbLpState[1] = outL + fbLpCoeff * (fbLpState[1] - outL);

        lineL[delayWrite] = 0.5f * (sendL[i] + sendR[i]) + feedback * fbLpState[0];
        lineR[delayWrite] = feedback * fbLpState[1];
        delayWrite = (delayWrite + 1) % lineLength;

        const auto mix = delayMix.getNextValue();
        if (stereo)
        {
            buffer.addSample (0, i, outL * mix);
            buffer.addSample (1, i, outR * mix);
        }
        else
        {
            buffer.addSample (0, i, 0.5f * (outL + outR) * mix);
        }
    }

    if (s.reverbMix <= 0.0f && reverbMix.getCurrentValue() <= 0.0f)
        return; // skip the reverb's CPU while it's off

    reverb.processStereo (sendL, sendR, numSamples);

    for (int i = 0; i < numSamples; ++i)
    {
        const auto mix = reverbMix.getNextValue();
        if (stereo)
        {
            buffer.addSample (0, i, sendL[i] * mix);
            buffer.addSample (1, i, sendR[i] * mix);
        }
        else
        {
            buffer.addSample (0, i, 0.5f * (sendL[i] + sendR[i]) * mix);
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
            bandGainSum[b] += gainDb * depth;

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

        const auto fullMid = bass + mid;
        midEnergy += fullMid * fullMid;
        sideEnergy += side * side;
    }
}

void FinishingChain::clipAndGain (juce::AudioBuffer<float>& buffer, int numSamples)
{
    const auto numCh = buffer.getNumChannels();

    for (int i = 0; i < numSamples; ++i)
    {
        const auto gain = outputGain.getNextValue();
        auto square = 0.0f;

        for (int ch = 0; ch < numCh; ++ch)
        {
            auto x = buffer.getSample (ch, i);
            const auto ax = std::abs (x);

            // Linear up to the knee, then eases into a 0 dBFS ceiling.
            if (ax > clipKnee)
            {
                x = std::copysign (clipKnee + (1.0f - clipKnee) * std::tanh ((ax - clipKnee) / (1.0f - clipKnee)), x);
                clipPeakDb = juce::jmax (clipPeakDb, juce::Decibels::gainToDecibels (ax / std::abs (x)));
            }

            const auto y = x * gain;
            buffer.setSample (ch, i, y);
            square += y * y;
            outPeak = juce::jmax (outPeak, std::abs (y));
        }

        meanSquare += rmsCoeff * (square / (float) numCh - meanSquare);

        // K-weighted energy, summed over L and R (a mono output counts for both).
        auto weighted = 0.0f;
        for (int ch = 0; ch < numCh; ++ch)
        {
            const auto k = kHighPass.process (ch, kPre.process (ch, buffer.getSample (ch, i)));
            weighted += k * k;
        }
        slotSum += numCh == 1 ? weighted * 2.0f : weighted;

        if (++slotFill == slotLength)
        {
            slotEnergy[slotPos] = slotSum / (float) slotLength;
            slotPos = (slotPos + 1) % loudnessSlots;
            slotSum = 0.0f;
            slotFill = 0;
        }
    }
}
