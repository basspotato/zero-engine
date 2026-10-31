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

    // PolyBLEP: the step of a jump smoothed over a sample each side, so the
    // 303 oscillator's saw and square don't alias.
    float polyBlep (double t, double dt) noexcept
    {
        if (t < dt)
        {
            t /= dt;
            return (float) (t + t - t * t - 1.0);
        }
        if (t > 1.0 - dt)
        {
            t = (t - 1.0) / dt;
            return (float) (t * t + t + t + 1.0);
        }
        return 0.0f;
    }

    // 303 mode's oscillator: a falling saw, or a square with its corners a
    // little rounded (the 303 makes its square from the saw).
    float osc303 (double phase, double inc, bool square) noexcept
    {
        if (! square)
            return (float) (1.0 - 2.0 * phase) + polyBlep (phase, inc);

        auto second = phase + 0.5;
        if (second >= 1.0)
            second -= 1.0;
        const auto value = (phase < 0.5 ? 1.0f : -1.0f) + polyBlep (phase, inc) - polyBlep (second, inc);
        return 0.7f * value; // a square carries more power than a saw at the same peak
    }
}

ZeroVoice::ZeroVoice (juce::AudioProcessorValueTreeState& state, const MacroMap& map, const std::atomic<float>& bpm,
                      const std::atomic<const WavetableBank*>& sample,
                      std::atomic<float>& position, int index)
    : macroMap (map), hostBpm (bpm), livePosition (position), sampleTable (sample), seed (0x5eed + index), random (seed)
{
    auto get = [&] (const char* id) { return state.getRawParameterValue (id); };

    p = { get (ParamIDs::weight),    get (ParamIDs::punch),      get (ParamIDs::width),     get (ParamIDs::motion),
          get (ParamIDs::wavePos),   get (ParamIDs::unison),     get (ParamIDs::detune),    get (ParamIDs::blend),
          get (ParamIDs::ampAttack), get (ParamIDs::ampDecay),   get (ParamIDs::ampSustain), get (ParamIDs::ampRelease),
          get (ParamIDs::modAttack), get (ParamIDs::modDecay),   get (ParamIDs::modSustain), get (ParamIDs::modRelease),
          get (ParamIDs::modToPos),  get (ParamIDs::lfoRate),    get (ParamIDs::lfoShape),  get (ParamIDs::lfoToPos),
          get (ParamIDs::lfoToPitch), get (ParamIDs::modToPitch), get (ParamIDs::subOctave),
          get (ParamIDs::glide),     get (ParamIDs::bendRange),
          get (ParamIDs::filterMode), get (ParamIDs::filterCutoff), get (ParamIDs::filterRes), get (ParamIDs::filterEnv),
          get (ParamIDs::filterKeytrack), get (ParamIDs::lfoToCutoff), get (ParamIDs::velToCutoff), get (ParamIDs::velSens),
          get (ParamIDs::waveTable), get (ParamIDs::waveTableB), get (ParamIDs::tableMix), get (ParamIDs::subOn),
          get (ParamIDs::lfoSync),
          get (ParamIDs::acidCutoff), get (ParamIDs::acidRes), get (ParamIDs::acidEnvMod), get (ParamIDs::acidDecay), get (ParamIDs::acidAccent),
          get (ParamIDs::acidFilter), get (ParamIDs::acidWave) };
}

void ZeroVoice::startAcid() noexcept
{
    sweepTarget = 1.0f;
    acidAccented = noteVelocity >= 100.0f / 127.0f;
    if (acidAccented)
        pulseTarget = 1.0f;
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
    // Parameters are left alone once a note is released: the release then runs
    // as it began. (juce::ADSR, used before, recomputed its release speed from
    // the sustain level here, which once made released notes hang.)
    if (releasing)
        return;

    // Punch shortens the amp attack, up to 95%.
    const auto attackScale = 1.0f - 0.95f * p.punch->load();
    const auto decayScale = std::exp2 (macroMap.offset (MacroTarget::ampDecay));

    if (p.acidFilter->load() > 0.5f) // 303 mode: snaps on, sinks slowly while held, stops short
        ampEnv.setParameters ({ 0.003f, 1.6f, 0.0f, 0.012f });
    else
        ampEnv.setParameters ({ juce::jmax (0.001f, p.ampAttack->load() * attackScale), juce::jmax (0.001f, p.ampDecay->load() * decayScale),
                                p.ampSustain->load(), juce::jmax (0.005f, p.ampRelease->load()) });
    // A note restarting a voice that still sounds rises its mod envelope over
    // 4 ms at least: a big filter sweep snapping up in 1 ms on a loud voice
    // ticks (from silence the amp attack hides it; that stays the punch).
    modEnv.setParameters ({ juce::jmax (retriggered ? 0.004f : 0.001f, p.modAttack->load()), juce::jmax (0.001f, p.modDecay->load()),
                            p.modSustain->load(), juce::jmax (0.005f, p.modRelease->load()) });
}

void ZeroVoice::pitchWheelMoved (int newValue)
{
    bend = juce::jlimit (-1.0f, 1.0f, (float) (newValue - 8192) / 8192.0f);
}

void ZeroVoice::setVelocity (float velocity)
{
    noteVelocity = velocity;
    const auto sensitivity = p.acidFilter->load() > 0.5f ? 0.0f : p.velSens->load(); // a 303 has no velocity, only accent
    level = 0.3f * (1.0f - sensitivity + sensitivity * velocity);
}

void ZeroVoice::glideTo (int midiNote, float velocity, bool retrigger, bool jump)
{
    targetPitch = std::log2 (juce::MidiMessage::getMidiNoteInHertz (midiNote));
    if (jump) // no slide: on a voice still sounding, an unheard 2.5 ms move (an instant jump kinks the wave and ticks)
        quickSlide = true;

    if (retrigger)
    {
        releasing = false; // a retrigger restarts the envelopes
        retriggered = true; // of a voice still sounding (glideTo only comes to an active voice)
        setVelocity (velocity);
        updateEnvelopes();
        ampEnv.noteOn();
        modEnv.noteOn();
        startAcid(); // a slide (no retrigger) keeps the 303 sweep going, as on the 303
    }
}

void ZeroVoice::startNote (int midiNote, float velocity, juce::SynthesiserSound*, int pitchWheel)
{
    releasing = false;
    targetPitch = std::log2 (juce::MidiMessage::getMidiNoteInHertz (midiNote));
    pitchWheelMoved (pitchWheel);
    setVelocity (velocity);

    // A voice still sounding (a stolen voice, a mono retrigger, fast arp notes)
    // carries on: its filters, oscillator phases and LFO stay, the envelope
    // rises from where it is, and pitch, level and velocity's cutoff move there
    // over a few ms, so nothing jumps (the owner heard pops on arp patches and
    // fast lines). Only a silent voice starts fresh, on the note.
    const auto continuing = ampEnv.isActive();
    quickSlide = continuing;
    retriggered = continuing;
    if (! continuing)
    {
        pitch = targetPitch;
        levelNow = level;
        velocityNow = noteVelocity;
        ampRounded = 0.0f;
    }
    stolen = false;
    if (! continuing)
    {
        for (auto& stage : filters)
            for (auto& f : stage)
                f = {};
        for (auto& ladder : ladders)
            ladder = {};

        // Random start phases keep unison voices from starting phase-locked (a flat, phasey attack).
        for (auto& phase : phases)
            phase = random.nextDouble();
        phases[0] = 0.0; // the centre voice stays in phase with the sub, or a sine
                         // oscillator and the sub at the same pitch would randomly
                         // reinforce or cancel (808 level jumping note to note)
        subPhase = 0.0;
        lfoPhase = 0.0f;
        lfoHeld = random.nextFloat() * 2.0f - 1.0f;
        lfoSmoothing = 1.0f - std::exp (-1.0f / (0.003f * (float) getSampleRate()));
        lfoSmoothed = rawLfo (0.0f); // start where the shape starts, not from 0
    }

    if (! continuing)
        acidCharge = 0.0f; // a fresh voice has no accent charge left over
    updateEnvelopes();
    ampEnv.noteOn();
    modEnv.noteOn();
    startAcid();
}

void ZeroVoice::stopNote (float, bool allowTailOff)
{
    if (allowTailOff)
    {
        releasing = true;
        ampEnv.noteOff();
        modEnv.noteOff();
    }
    else
    {
        // Stopped at once, usually to be stolen for another note this instant:
        // the envelopes keep their level for that note to rise from (see
        // startNote); if none comes, the next render clears them.
        stolen = ampEnv.isActive();
        if (! stolen)
        {
            ampEnv.reset();
            modEnv.reset();
        }
        clearCurrentNote();
    }
}

float ZeroVoice::filterSample (int ch, float x, int mode, const Svf::Coeffs& resonant, const Svf::Coeffs& flat) noexcept
{
    float low, band, high;
    filters[0][ch].process (x, resonant, low, band, high);

    switch (mode)
    {
        case 0:  return low;
        case 1:  filters[1][ch].process (low, flat, low, band, high); return low; // second, unresonant stage: 24 dB/oct
        case 2:  return band;
        default: return high;
    }
}

float ZeroVoice::nextLfo (float increment)
{
    // Saw, square and sample & hold jump in one sample; through wavetable
    // position, cutoff or pitch that jump clicks (the owner heard pops on some
    // Generate results). About 3 ms of smoothing rounds the jumps off unheard.
    const auto raw = rawLfo (increment);
    lfoSmoothed += (raw - lfoSmoothed) * lfoSmoothing;
    return lfoSmoothed;
}

float ZeroVoice::rawLfo (float increment)
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
    {
        if (stolen) // stopped at once and no note took it over: silent now
        {
            ampEnv.reset();
            modEnv.reset();
            stolen = false;
        }
        return;
    }

    const auto& bank = WavetableBank::get();
    const auto sampleRate = getSampleRate();
    // 303 mode (the 303 FILTER switch): its own oscillator, filter range and
    // envelope in place of the sound's, as close to a TB-303 as we get.
    const auto filter303 = p.acidFilter->load() > 0.5f;

    updateEnvelopes();

    const auto voices = juce::jlimit (1, maxUnison, (int) p.unison->load());
    const auto detune = juce::jlimit (0.0f, 1.0f, p.detune->load() + macroMap.offset (MacroTarget::detune));
    const auto maxCents = 50.0f * detune * detune + 5.0f * detune;
    const auto spread = p.width->load();
    const auto blend = p.blend->load();
    const auto basePos = p.wavePos->load();
    const auto modToPos = juce::jlimit (-1.0f, 1.0f, p.modToPos->load() + macroMap.offset (MacroTarget::modToPos));
    const auto motion = macroMap.motion(); // the knob plus the mod wheel
    const auto lfoToPos = juce::jlimit (0.0f, 1.0f, p.lfoToPos->load() + motion * 0.5f + macroMap.offset (MacroTarget::lfoToPos));
    const auto lfoToPitch = filter303 ? 0.0f : p.lfoToPitch->load() * 2.0f; // semitones (a 303 has no vibrato)
    // Synced: one cycle per note division of the host tempo, so wobbles stay on
    // the beat at any BPM. Motion still doubles the rate per unit (the next division).
    const auto sync = juce::jlimit (0, (int) std::size (lfoSyncBeats) - 1, (int) p.lfoSync->load());
    const auto baseRate = sync > 0 ? hostBpm.load() / 60.0f / lfoSyncBeats[sync] : p.lfoRate->load();
    const auto lfoRate = juce::jlimit (0.05f, 40.0f, baseRate * std::exp2 (macroMap.offset (MacroTarget::lfoRate)));
    const auto lfoInc = (float) (lfoRate / sampleRate);
    const auto modToPitch = filter303 ? 0.0f : juce::jlimit (-36.0f, 36.0f, p.modToPitch->load() + macroMap.offset (MacroTarget::modToPitch)); // semitones at full mod envelope
    const auto subLevel = p.subOn->load() > 0.5f && ! filter303 ? p.weight->load() * 0.8f : 0.0f;
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
    const auto glideTime = filter303 ? juce::jmax (0.06f, p.glide->load()) : p.glide->load(); // the 303's slide, about 60 ms
    const auto glideCoeff = 1.0 - std::exp (-3.0 / (juce::jmax (0.0025f, glideTime) * sampleRate)); // ~95% there at glideTime (2.5 ms at least: no pitch steps)
    const auto quickCoeff = 1.0 - std::exp (-3.0 / (0.0025 * sampleRate));
    const auto ease = (float) (1.0 - std::exp (-1.0 / (0.003 * sampleRate))); // level and velocity's cutoff to a new note's
    const auto cornerEase = (float) (1.0 - std::exp (-1.0 / (0.0005 * sampleRate)));

    const auto maxFreq = std::exp2 (juce::jmax (pitch, targetPitch) + maxCents / 1200.0
                                    + (lfoToPitch + juce::jmax (0.0f, modToPitch) + juce::jmax (0.0f, bendSemis)) / 12.0);
    const auto mipLevel = WavetableBank::levelFor (maxFreq, sampleRate);
    const auto table = juce::jlimit (0, WavetableBank::numBanks - 1, (int) p.waveTable->load());
    const auto tableB = juce::jlimit (0, WavetableBank::numBanks - 1, (int) p.waveTableB->load());
    const auto* sample = sampleTable.load (std::memory_order_acquire);
    const auto& bankA = table == WavetableBank::sampleBank && sample != nullptr ? *sample : bank;
    const auto& bankB = tableB == WavetableBank::sampleBank && sample != nullptr ? *sample : bank;
    const auto tableMix = juce::jlimit (0.0f, 1.0f, p.tableMix->load() + macroMap.offset (MacroTarget::tableMix));
    const auto blendTables = tableMix > 1.0e-3f && tableB != table;

    // Filter. Fully open low-pass with no modulation is skipped, so profiles
    // that don't use it sound exactly as before.
    const auto filterMode = (int) p.filterMode->load();
    const auto velToCutoff = p.velToCutoff->load(); // softer notes are darker (eased per sample)
    const auto cutoff = p.filterCutoff->load() * std::exp2 (macroMap.offset (MacroTarget::filterCutoff));
    const auto filterEnv = p.filterEnv->load() + macroMap.offset (MacroTarget::filterEnv);
    const auto keytrack = p.filterKeytrack->load();
    const auto lfoToCutoff = p.lfoToCutoff->load();
    // The 303 row, on top of the sound. At neutral (Cutoff and Reso centred,
    // Env Mod and Accent at zero) none of it runs and the sound is untouched.
    const auto acidOctaves = p.acidCutoff->load();
    const auto acidResOffset = p.acidRes->load();
    const auto envMod = p.acidEnvMod->load();
    const auto accent = p.acidAccent->load();
    const auto acidOn = std::abs (acidOctaves) > 1.0e-4f || envMod > 1.0e-4f || accent > 1.0e-4f || filter303;
    // In 303 mode Reso covers the 303's whole range by itself (centre about
    // half, full right at the edge of ringing); otherwise it moves the sound's.
    const auto resonance = filter303 ? juce::jlimit (0.0f, 1.0f, 0.55f + 0.45f * acidResOffset)
                         : std::abs (acidResOffset) > 1.0e-4f ? juce::jlimit (0.0f, 0.95f, p.filterRes->load() + acidResOffset) // short of a scream
                                                              : p.filterRes->load();
    // Env Mod sweeps up to 4.5 octaves and, as on the 303, lowers the resting
    // cutoff as it rises; an accent's own sweep and its wow grow with resonance.
    const auto sweepFall = (float) std::exp (-3.0 / ((acidAccented ? 0.2 : (double) p.acidDecay->load()) * sampleRate)); // ~95 % down at Decay
    const auto pulseFall = (float) std::exp (-3.0 / (0.2 * sampleRate));
    // The charge fills slowly, so one accent only part fills it and accents in
    // a row climb (the 303's wow); it lets go over about a second.
    const auto chargeRise = (float) (1.0 - std::exp (-1.0 / (0.1 * sampleRate)));
    const auto chargeFall = (float) (1.0 - std::exp (-1.0 / (0.35 * sampleRate)));
    const auto accentGain = acidAccented ? 1.0f + (filter303 ? 1.0f : 0.8f) * accent : 1.0f; // about +5 dB at full Accent (+6 in 303 mode)
    // 303 mode: the cutoff is the 303's own (about 230 Hz to 2.8 kHz across the
    // Cutoff knob, 800 Hz at centre), the sound's filter settings left out.
    const auto cutoff303 = 800.0f * std::exp2 (0.45f * acidOctaves);
    const auto square303 = p.acidWave->load() > 0.5f;
    const auto accentEase = (float) (1.0 - std::exp (-1.0 / (0.003 * sampleRate)));
    const auto sweepRise = (float) (1.0 - std::exp (-1.0 / (0.0005 * sampleRate))); // ~1.5 ms to the top
    const auto wowDepth = accent * (1.5f + 7.0f * resonance);

    const auto velocityDarkens = velToCutoff > 1.0e-3f && juce::jmin (noteVelocity, velocityNow) < 0.999f;
    const auto filterOn = filterMode > 1 || cutoff < 19000.0f || velocityDarkens || std::abs (filterEnv) > 1.0e-3f
                       || lfoToCutoff > 1.0e-3f || keytrack > 1.0e-3f || acidOn || std::abs (acidResOffset) > 1.0e-4f || filter303;
    const auto k = 2.0f * (1.0f - 0.97f * resonance);
    const auto maxCutoff = (float) (0.45 * sampleRate);
    const auto middleC = std::log2 (261.63);
    Svf::Coeffs resonant, flat;
    Ladder303::Coeffs ladder;
    const auto ladderLow303 = (float) (1.0 - std::exp (-juce::MathConstants<double>::twoPi * 25.0 / (2.0 * sampleRate))); // only near-DC kept out of the feedback (oversampled)

    for (int i = 0; i < numSamples; ++i)
    {
        const auto lfo = nextLfo (lfoInc);
        const auto mod = modEnv.getNextSample();
        const auto pos = juce::jlimit (0.0f, 1.0f, basePos + mod * modToPos + lfo * lfoToPos * 0.5f);
        if (i == numSamples - 1)
            livePosition.store (pos, std::memory_order_relaxed); // for the display
        pitch += (quickSlide ? quickCoeff : glideCoeff) * (targetPitch - pitch);
        if (quickSlide && std::abs (targetPitch - pitch) < 1.0e-6)
            quickSlide = false;
        velocityNow += (noteVelocity - velocityNow) * ease;
        const auto inc = std::exp2 (pitch + (lfo * lfoToPitch + mod * modToPitch + bendSemis) / 12.0) / sampleRate;

        float l = 0.0f, r = 0.0f;
        if (filter303)
        {
            // One band-limited oscillator, saw or square, the same both sides.
            const auto s = 0.45f * osc303 (phases[0], inc, square303); // about a designed sound's level into the chain
            l = r = s;
            phases[0] += inc;
            if (phases[0] >= 1.0)
                phases[0] -= 1.0;
        }
        else
        for (int v = 0; v < voices; ++v)
        {
            auto s = bankA.lookup (table, mipLevel, pos, phases[v]);
            if (blendTables)
                s += tableMix * (bankB.lookup (tableB, mipLevel, pos, phases[v]) - s);
            l += s * gainL[v];
            r += s * gainR[v];

            phases[v] += inc * ratios[v];
            if (phases[v] >= 1.0)
                phases[v] -= 1.0;
        }

        // The 303 sweeps run even at neutral, so turning Env Mod up mid-note
        // joins the sweep where it is.
        const auto acidShift = filter303 ? envMod * (4.5f * acidSweep - 0.6f) + accent * 2.5f * acidPulse + wowDepth * acidCharge
                             : acidOn    ? acidOctaves + envMod * (4.5f * acidSweep - 1.2f) + accent * 2.0f * acidPulse + wowDepth * acidCharge
                                         : 0.0f;
        sweepTarget *= sweepFall;
        pulseTarget *= pulseFall;
        acidSweep += (sweepTarget - acidSweep) * sweepRise;
        acidPulse += (pulseTarget - acidPulse) * sweepRise;
        acidCharge += (acidPulse - acidCharge) * (acidPulse > acidCharge ? chargeRise : chargeFall);

        if (filterOn)
        {
            // Cutoff modulation at 1/8 rate is plenty and saves the tan(); 303 mode
            // (one voice, fast sweeps into high resonance) updates every other sample.
            if ((i & (filter303 ? 1 : 7)) == 0)
            {
                if (filter303)
                {
                    // At twice the rate (the ladder runs oversampled).
                    const auto fc = juce::jlimit (20.0f, (float) (0.9 * sampleRate), cutoff303 * std::exp2 (acidShift));
                    const auto g = std::tan (juce::MathConstants<float>::pi * fc / (float) (2.0 * sampleRate));
                    ladder = Ladder303::make (g, resonance, ladderLow303);
                }
                else
                {
                    const auto octaves = filterEnv * mod + lfoToCutoff * lfo + keytrack * (float) (pitch - middleC) + acidShift
                                       + velToCutoff * (velocityNow - 1.0f);
                    const auto fc = juce::jlimit (20.0f, maxCutoff, cutoff * std::exp2 (octaves));
                    const auto g = std::tan (juce::MathConstants<float>::pi * fc / (float) sampleRate);
                    resonant = Svf::make (g, k);
                    flat = Svf::make (g, juce::MathConstants<float>::sqrt2);
                }
            }

            if (filter303) // 303 mode: the ladder, oversampled and driven, in place of the sound's filter
            {
                l = ladders[0].processTwice (l, ladder, 2.0f);
                r = l; // mono, like the 303 (one ladder is enough)
            }
            else
            {
                l = filterSample (0, l, filterMode, resonant, flat);
                r = filterSample (1, r, filterMode, resonant, flat);
            }
        }

        const auto sub = std::sin (juce::MathConstants<double>::twoPi * subPhase) * subLevel;
        subPhase += inc * subRatio;
        if (subPhase >= 1.0)
            subPhase -= 1.0;

        // With a third channel the processor keeps the sub separate (Sub Split);
        // otherwise it is mixed into both sides as before.
        levelNow += (level - levelNow) * ease;
        // The amp envelope with its corners rounded over ~0.5 ms: a retrigger
        // turns a falling release straight into a rising attack, and on a
        // loud, slow bass wave that corner alone ticks.
        ampRounded += (ampEnv.getNextSample() - ampRounded) * cornerEase;
        auto gain = ampRounded * levelNow;
        if (acidOn)
        {
            // Eased over ~3 ms: a voice carried from an accented note to a plain
            // one (or back) would otherwise step in level and click.
            accentLevel += (accentGain - accentLevel) * accentEase;
            gain *= accentLevel;
        }
        if (output.getNumChannels() > 2)
        {
            output.addSample (0, startSample + i, l * gain);
            output.addSample (1, startSample + i, r * gain);
            output.addSample (2, startSample + i, (float) sub * gain);
        }
        else
        {
            output.addSample (0, startSample + i, (l + (float) sub) * gain);
            if (output.getNumChannels() > 1)
                output.addSample (1, startSample + i, (r + (float) sub) * gain);
        }
    }

    if (! ampEnv.isActive())
        clearCurrentNote();
}

//==============================================================================
ZeroSynth::ZeroSynth (juce::AudioProcessorValueTreeState& state)
    : playMode (*state.getRawParameterValue (ParamIDs::playMode)),
      filter303 (*state.getRawParameterValue (ParamIDs::acidFilter))
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
    const auto mode = currentMode();
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
    {
        // No other key held (the last note is only releasing): a fresh note,
        // started on its own pitch. Overlapping keys slide (and mono retriggers).
        const auto fresh = numHeld == 1;
        voice->glideTo (midiNoteNumber, velocity, mode == mono || fresh, fresh);
    }
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
    {
        if (! sustainDown)
            stopVoice (voice, velocity, allowTailOff);
    }
    else if (wasCurrent)
        voice->glideTo (heldNotes[numHeld - 1], heldVelocities[numHeld - 1], lastMode == mono);
}

void ZeroSynth::handleSustainPedal (int midiChannel, bool isDown)
{
    // The base class tracks keys per voice, which a gliding mono voice doesn't
    // follow, so mono/legato handles the pedal itself.
    if (lastMode == poly)
    {
        Synthesiser::handleSustainPedal (midiChannel, isDown);
        return;
    }

    sustainDown = isDown;

    if (! isDown && numHeld == 0)
        if (auto* voice = getVoice (0); voice != nullptr && voice->isVoiceActive())
            stopVoice (voice, 0.0f, true);
}

void ZeroSynth::allNotesOff (int midiChannel, bool allowTailOff)
{
    numHeld = 0;
    Synthesiser::allNotesOff (midiChannel, allowTailOff);
}
