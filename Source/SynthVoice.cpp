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

ZeroVoice::ZeroVoice (juce::AudioProcessorValueTreeState& state, const MacroMap& map)
    : macroMap (map)
{
    auto get = [&] (const char* id) { return state.getRawParameterValue (id); };

    p = { get (ParamIDs::weight),    get (ParamIDs::punch),      get (ParamIDs::width),     get (ParamIDs::motion),
          get (ParamIDs::wavePos),   get (ParamIDs::unison),     get (ParamIDs::detune),    get (ParamIDs::blend),
          get (ParamIDs::ampAttack), get (ParamIDs::ampDecay),   get (ParamIDs::ampSustain), get (ParamIDs::ampRelease),
          get (ParamIDs::modAttack), get (ParamIDs::modDecay),   get (ParamIDs::modSustain), get (ParamIDs::modRelease),
          get (ParamIDs::modToPos),  get (ParamIDs::lfoRate),    get (ParamIDs::lfoShape),  get (ParamIDs::lfoToPos),
          get (ParamIDs::lfoToPitch), get (ParamIDs::modToPitch), get (ParamIDs::subOctave),
          get (ParamIDs::glide),     get (ParamIDs::bendRange) };
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
    const auto decayScale = std::exp2 (macroMap.offset (MacroTarget::ampDecay));

    ampEnv.setParameters ({ juce::jmax (0.001f, p.ampAttack->load() * attackScale), juce::jmax (0.001f, p.ampDecay->load() * decayScale),
                            p.ampSustain->load(), juce::jmax (0.005f, p.ampRelease->load()) });
    modEnv.setParameters ({ juce::jmax (0.001f, p.modAttack->load()), juce::jmax (0.001f, p.modDecay->load()),
                            p.modSustain->load(), juce::jmax (0.005f, p.modRelease->load()) });
}

void ZeroVoice::pitchWheelMoved (int newValue)
{
    bend = juce::jlimit (-1.0f, 1.0f, (float) (newValue - 8192) / 8192.0f);
}

void ZeroVoice::glideTo (int midiNote, float velocity, bool retrigger)
{
    targetPitch = std::log2 (juce::MidiMessage::getMidiNoteInHertz (midiNote));

    if (retrigger)
    {
        level = velocity * 0.3f;
        updateEnvelopes();
        ampEnv.noteOn();
        modEnv.noteOn();
    }
}

void ZeroVoice::startNote (int midiNote, float velocity, juce::SynthesiserSound*, int pitchWheel)
{
    pitch = targetPitch = std::log2 (juce::MidiMessage::getMidiNoteInHertz (midiNote));
    pitchWheelMoved (pitchWheel);
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
    const auto detune = juce::jlimit (0.0f, 1.0f, p.detune->load() + macroMap.offset (MacroTarget::detune));
    const auto maxCents = 50.0f * detune * detune + 5.0f * detune;
    const auto spread = p.width->load();
    const auto blend = p.blend->load();
    const auto basePos = p.wavePos->load();
    const auto modToPos = juce::jlimit (-1.0f, 1.0f, p.modToPos->load() + macroMap.offset (MacroTarget::modToPos));
    const auto motion = p.motion->load();
    const auto lfoToPos = juce::jlimit (0.0f, 1.0f, p.lfoToPos->load() + motion * 0.5f + macroMap.offset (MacroTarget::lfoToPos));
    const auto lfoToPitch = p.lfoToPitch->load() * 2.0f; // semitones
    const auto lfoRate = juce::jlimit (0.05f, 40.0f, p.lfoRate->load() * std::exp2 (macroMap.offset (MacroTarget::lfoRate)));
    const auto lfoInc = (float) (lfoRate / sampleRate);
    const auto modToPitch = juce::jlimit (-36.0f, 36.0f, p.modToPitch->load() + macroMap.offset (MacroTarget::modToPitch)); // semitones at full mod envelope
    const auto subLevel = p.weight->load() * 0.8f;
    const auto subRatio = p.subOctave->load() < -0.5f ? 0.5 : 1.0;

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
    const auto bendSemis = bend * p.bendRange->load();
    const auto glideTime = p.glide->load();
    const auto glideCoeff = glideTime > 0.0005f ? 1.0 - std::exp (-3.0 / (glideTime * sampleRate)) : 1.0; // ~95% there at glideTime

    const auto maxFreq = std::exp2 (juce::jmax (pitch, targetPitch) + maxCents / 1200.0
                                    + (lfoToPitch + juce::jmax (0.0f, modToPitch) + juce::jmax (0.0f, bendSemis)) / 12.0);
    const auto mipLevel = WavetableBank::levelFor (maxFreq, sampleRate);

    for (int i = 0; i < numSamples; ++i)
    {
        const auto lfo = nextLfo (lfoInc);
        const auto mod = modEnv.getNextSample();
        const auto pos = juce::jlimit (0.0f, 1.0f, basePos + mod * modToPos + lfo * lfoToPos * 0.5f);
        pitch += glideCoeff * (targetPitch - pitch);
        const auto inc = std::exp2 (pitch + (lfo * lfoToPitch + mod * modToPitch + bendSemis) / 12.0) / sampleRate;

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
        subPhase += inc * subRatio;
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

//==============================================================================
ZeroSynth::ZeroSynth (juce::AudioProcessorValueTreeState& state)
    : playMode (*state.getRawParameterValue (ParamIDs::playMode))
{
}

void ZeroSynth::removeHeld (int note)
{
    for (int i = 0; i < numHeld; ++i)
    {
        if (heldNotes[i] == note)
        {
            for (int j = i + 1; j < numHeld; ++j)
            {
                heldNotes[j - 1] = heldNotes[j];
                heldVelocities[j - 1] = heldVelocities[j];
            }
            --numHeld;
            return;
        }
    }
}

void ZeroSynth::noteOn (int midiChannel, int midiNoteNumber, float velocity)
{
    const auto mode = (int) playMode.load();
    if (mode != lastMode)
    {
        // Switching modes mid-note would strand voices the other mode can't see.
        allNotesOff (0, true);
        lastMode = mode;
    }

    if (mode == poly)
    {
        Synthesiser::noteOn (midiChannel, midiNoteNumber, velocity);
        return;
    }

    removeHeld (midiNoteNumber);
    if (numHeld == maxHeld)
        removeHeld (heldNotes[0]); // drop the oldest
    heldNotes[numHeld] = midiNoteNumber;
    heldVelocities[numHeld] = velocity;
    ++numHeld;

    auto* voice = dynamic_cast<ZeroVoice*> (getVoice (0));
    if (voice == nullptr)
        return;

    if (voice->isVoiceActive())
        voice->glideTo (midiNoteNumber, velocity, mode == mono || numHeld == 1); // legato only slides while a key is held
    else
        startVoice (voice, getSound (0).get(), midiChannel, midiNoteNumber, velocity);
}

void ZeroSynth::noteOff (int midiChannel, int midiNoteNumber, float velocity, bool allowTailOff)
{
    if (lastMode == poly)
    {
        Synthesiser::noteOff (midiChannel, midiNoteNumber, velocity, allowTailOff);
        return;
    }

    const auto wasCurrent = numHeld > 0 && heldNotes[numHeld - 1] == midiNoteNumber;
    removeHeld (midiNoteNumber);

    auto* voice = dynamic_cast<ZeroVoice*> (getVoice (0));
    if (voice == nullptr || ! voice->isVoiceActive())
        return;

    if (numHeld == 0)
        stopVoice (voice, velocity, allowTailOff);
    else if (wasCurrent)
        voice->glideTo (heldNotes[numHeld - 1], heldVelocities[numHeld - 1], lastMode == mono);
}

void ZeroSynth::allNotesOff (int midiChannel, bool allowTailOff)
{
    numHeld = 0;
    Synthesiser::allNotesOff (midiChannel, allowTailOff);
}
