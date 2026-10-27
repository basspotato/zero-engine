#include "SynthVoice.h"
#include "Parameters.h"

namespace
{
    // PolyBLEP residual that removes the aliasing step from a naive saw.
    float polyBlep (float t, float dt)
    {
        if (t < dt)
        {
            t /= dt;
            return t + t - t * t - 1.0f;
        }
        if (t > 1.0f - dt)
        {
            t = (t - 1.0f) / dt;
            return t * t + t + t + 1.0f;
        }
        return 0.0f;
    }

    float advance (float& phase, float increment)
    {
        const auto current = phase;
        phase += increment;
        if (phase >= 1.0f)
            phase -= 1.0f;
        return current;
    }

    float saw (float phase, float increment)
    {
        return 2.0f * phase - 1.0f - polyBlep (phase, increment);
    }
}

ZeroVoice::ZeroVoice (juce::AudioProcessorValueTreeState& state)
    : weight (*state.getRawParameterValue (ParamIDs::weight)),
      punch  (*state.getRawParameterValue (ParamIDs::punch)),
      width  (*state.getRawParameterValue (ParamIDs::width))
{
}

bool ZeroVoice::canPlaySound (juce::SynthesiserSound* sound)
{
    return dynamic_cast<ZeroSound*> (sound) != nullptr;
}

void ZeroVoice::setCurrentPlaybackSampleRate (double newRate)
{
    SynthesiserVoice::setCurrentPlaybackSampleRate (newRate);
    adsr.setSampleRate (newRate);
}

void ZeroVoice::startNote (int midiNote, float velocity, juce::SynthesiserSound*, int)
{
    frequency = juce::MidiMessage::getMidiNoteInHertz (midiNote);
    level = velocity * 0.25f;

    // Spread starting phases so the unison voices don't start phase-locked.
    sawPhase[0] = 0.0f;
    sawPhase[1] = 0.33f;
    sawPhase[2] = 0.67f;
    subPhase = 0.0f;

    adsr.noteOn();
}

void ZeroVoice::stopNote (float, bool allowTailOff)
{
    if (allowTailOff)
    {
        adsr.noteOff();
    }
    else
    {
        adsr.reset();
        clearCurrentNote();
    }
}

void ZeroVoice::renderNextBlock (juce::AudioBuffer<float>& output, int startSample, int numSamples)
{
    if (! isVoiceActive())
        return;

    const auto sampleRate = (float) getSampleRate();
    const auto w = width.load();
    const auto subLevel = weight.load() * 0.8f;

    adsr.setParameters ({ juce::jmap (punch.load(), 0.03f, 0.001f), 0.3f, 0.8f, 0.25f });

    const auto detune = std::pow (2.0f, w * 20.0f / 1200.0f);
    const auto baseInc = (float) frequency / sampleRate;
    const float increments[3] { baseInc, baseInc / detune, baseInc * detune };
    const auto subInc = baseInc * 0.5f;
    const auto norm = 1.0f / (1.0f + w);

    for (int i = 0; i < numSamples; ++i)
    {
        const auto centre = saw (advance (sawPhase[0], increments[0]), increments[0]);
        const auto left   = saw (advance (sawPhase[1], increments[1]), increments[1]);
        const auto right  = saw (advance (sawPhase[2], increments[2]), increments[2]);
        const auto sub    = std::sin (juce::MathConstants<float>::twoPi * advance (subPhase, subInc)) * subLevel;

        const auto gain = adsr.getNextSample() * level;
        const auto l = ((centre + left * w) * norm + sub) * gain;
        const auto r = ((centre + right * w) * norm + sub) * gain;

        output.addSample (0, startSample + i, l);
        if (output.getNumChannels() > 1)
            output.addSample (1, startSample + i, r);
    }

    if (! adsr.isActive())
        clearCurrentNote();
}
