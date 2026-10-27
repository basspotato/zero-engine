#include "SynthVoice.h"
#include "Parameters.h"
#include "Wavetable.h"

namespace
{
    // Symmetric unison offsets in [-1, 1]; the centre voice (offset 0) is first.
    float unisonOffset (int index, int count)
    {
        if (count <= 1 || index == 0)
            return 0.0f;

        const auto pair = (index + 1) / 2;   // 1, 1, 2, 2, 3, 3
        const auto sign = (index % 2 == 1) ? -1.0f : 1.0f;
        const auto pairs = count / 2;
        return sign * (float) pair / (float) pairs;
    }
}

ZeroVoice::ZeroVoice (juce::AudioProcessorValueTreeState& state)
{
    auto get = [&] (const char* id) { return state.getRawParameterValue (id); };

    p = { get (ParamIDs::weight),    get (ParamIDs::punch),      get (ParamIDs::width),     get (ParamIDs::motion),
          get (ParamIDs::wavePos),   get (ParamIDs::unison),     get (ParamIDs::detune),    get (ParamIDs::blend),
          get (ParamIDs::ampAttack), get (ParamIDs::ampDecay),   get (ParamIDs::ampSustain), get (ParamIDs::ampRelease),
          get (ParamIDs::modAttack), get (ParamIDs::modDecay),   get (ParamIDs::modSustain), get (ParamIDs::modRelease),
          get (ParamIDs::modToPos),  get (ParamIDs::lfoRate),    get (ParamIDs::lfoShape),  get (ParamIDs::lfoToPos),
          get (ParamIDs::lfoToPitch) };
}

bool ZeroVoice::canPlaySound (juce::SynthesiserSound* sound)
{
    return dynamic_cast<ZeroSound*> (sound) != nullptr;
}

void ZeroVoice::setCurrentPlaybackSampleRate (double newRate)
{
    SynthesiserVoice::setCurrentPlaybackSampleRate (newRate);
    if (newRate > 0.0)
    {
        ampEnv.setSampleRate (newRate);
        modEnv.setSampleRate (newRate);
    }
}

void ZeroVoice::updateEnvelopes()
{
    // Punch shortens the amp attack, up to 95%.
    const auto attackScale = 1.0f - 0.95f * p.punch->load();

    ampEnv.setParameters ({ juce::jmax (0.001f, p.ampAttack->load() * attackScale), juce::jmax (0.001f, p.ampDecay->load()),
                            p.ampSustain->load(), juce::jmax (0.005f, p.ampRelease->load()) });
    modEnv.setParameters ({ juce::jmax (0.001f, p.modAttack->load()), juce::jmax (0.001f, p.modDecay->load()),
                            p.modSustain->load(), juce::jmax (0.005f, p.modRelease->load()) });
}

void ZeroVoice::startNote (int midiNote, float velocity, juce::SynthesiserSound*, int)
{
    frequency = juce::MidiMessage::getMidiNoteInHertz (midiNote);
    level = velocity * 0.3f;

    // Random start phases keep unison voices from starting phase-locked (a flat, phasey attack).
    for (auto& phase : phases)
        phase = random.nextDouble();
    subPhase = 0.0;
    lfoPhase = 0.0f;
    lfoHeld = random.nextFloat() * 2.0f - 1.0f;

    updateEnvelopes();
    ampEnv.noteOn();
    modEnv.noteOn();
}

void ZeroVoice::stopNote (float, bool allowTailOff)
{
    if (allowTailOff)
    {
        ampEnv.noteOff();
        modEnv.noteOff();
    }
    else
    {
        ampEnv.reset();
        modEnv.reset();
        clearCurrentNote();
    }
}

float ZeroVoice::nextLfo (float increment)
{
    const auto phase = lfoPhase;
    lfoPhase += increment;
    if (lfoPhase >= 1.0f)
    {
        lfoPhase -= 1.0f;
        lfoHeld = random.nextFloat() * 2.0f - 1.0f;
    }

    switch ((int) p.lfoShape->load())
    {
        case 0:  return std::sin (juce::MathConstants<float>::twoPi * phase);
        case 1:  return 1.0f - 4.0f * std::abs (phase - 0.5f);
        case 2:  return 2.0f * phase - 1.0f;
        case 3:  return phase < 0.5f ? 1.0f : -1.0f;
        default: return lfoHeld;
    }
}

void ZeroVoice::renderNextBlock (juce::AudioBuffer<float>& output, int startSample, int numSamples)
{
    if (! isVoiceActive())
        return;

    const auto& bank = WavetableBank::get();
    const auto sampleRate = getSampleRate();

    updateEnvelopes();

    const auto voices = juce::jlimit (1, maxUnison, (int) p.unison->load());
    const auto detune = p.detune->load();
    const auto maxCents = 50.0f * detune * detune + 5.0f * detune;
    const auto spread = p.width->load();
    const auto blend = p.blend->load();
    const auto basePos = p.wavePos->load();
    const auto modToPos = p.modToPos->load();
    const auto motion = p.motion->load();
    const auto lfoToPos = juce::jmin (1.0f, p.lfoToPos->load() + motion * 0.5f);
    const auto lfoToPitch = p.lfoToPitch->load() * 2.0f; // semitones
    const auto lfoInc = (float) (p.lfoRate->load() / sampleRate);
    const auto subLevel = p.weight->load() * 0.8f;

    // Per unison voice: pitch ratio and equal-power stereo gains.
    double ratios[maxUnison];
    float gainL[maxUnison], gainR[maxUnison];
    const auto sideGain = blend;
    const auto centreGain = 1.0f - 0.5f * blend;
    const auto norm = 1.0f / std::sqrt ((float) voices);

    for (int v = 0; v < voices; ++v)
    {
        const auto offset = unisonOffset (v, voices);
        ratios[v] = std::pow (2.0, offset * maxCents / 1200.0);
        const auto pan = (offset * spread + 1.0f) * 0.25f * juce::MathConstants<float>::pi;
        const auto g = (v == 0 ? centreGain : sideGain) * norm;
        gainL[v] = std::cos (pan) * g;
        gainR[v] = std::sin (pan) * g;
    }

    // Choose the mip level for the highest pitch this block can reach.
    const auto maxFreq = frequency * std::pow (2.0, maxCents / 1200.0 + lfoToPitch / 12.0);
    const auto mipLevel = WavetableBank::levelFor (maxFreq, sampleRate);

    for (int i = 0; i < numSamples; ++i)
    {
        const auto lfo = nextLfo (lfoInc);
        const auto mod = modEnv.getNextSample();
        const auto pos = juce::jlimit (0.0f, 1.0f, basePos + mod * modToPos + lfo * lfoToPos * 0.5f);
        const auto inc = frequency * std::exp2 (lfo * lfoToPitch / 12.0f) / sampleRate;

        float l = 0.0f, r = 0.0f;
        for (int v = 0; v < voices; ++v)
        {
            const auto s = bank.lookup (mipLevel, pos, phases[v]);
            l += s * gainL[v];
            r += s * gainR[v];

            phases[v] += inc * ratios[v];
            if (phases[v] >= 1.0)
                phases[v] -= 1.0;
        }

        const auto sub = std::sin (juce::MathConstants<double>::twoPi * subPhase) * subLevel;
        subPhase += inc * 0.5;
        if (subPhase >= 1.0)
            subPhase -= 1.0;

        const auto gain = ampEnv.getNextSample() * level;
        output.addSample (0, startSample + i, (l + (float) sub) * gain);
        if (output.getNumChannels() > 1)
            output.addSample (1, startSample + i, (r + (float) sub) * gain);
    }

    if (! ampEnv.isActive())
        clearCurrentNote();
}
