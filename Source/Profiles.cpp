#include "Profiles.h"
#include "Parameters.h"

namespace
{
    enum Genre { edm, dubstep, techno, techHouse, trap, synthwave, drumAndBass, pop, house, trance, futureBass, hardstyle };
    enum Type  { bass, lead, chord, pad, pluck, fx };
    enum Lfo   { sine, triangle, saw, square, sampleHold };
    // lfoSyncNames indices: one LFO cycle per note division of the host tempo.
    constexpr float syncQuarterTriplet = 4.0f, syncEighth = 5.0f, syncSixteenth = 7.0f;

    // Wave positions across the Basic frames: sine, triangle, saw, square, pulse.
    constexpr float sineWave = 0.0f, sawWave = 0.5f, squareWave = 0.75f;
    // waveTableNames indices.
    constexpr float growlTable = 1.0f, digitalTable = 2.0f, organTable = 3.0f, fmTable = 4.0f,
                    pwmTable = 6.0f, bellTable = 8.0f, choirTable = 9.0f, harshTable = 10.0f, analogTable = 11.0f,
                    jupiterTable = 12.0f;

    Profile typeTemplate (int type)
    {
        Profile p {};
        p.lfoRate = 2.0f;
        p.lfoShape = sine;
        p.monoFreq = 120.0f;
        p.stereoWidth = 1.0f;
        p.subOctave = -1.0f;
        p.midHighPass = 120.0f;     // only used when subSplit is on
        p.subOn = 1.0f;

        // The other half of clean sub, dirty mids: everything that isn't a bass
        // or FX leaves the low end to the bass (bass_clarity.md).
        if (type == lead || type == chord || type == pad || type == pluck)
        {
            p.subOn = 0.0f;
            p.subSplit = 1.0f;      // the split path is what applies the high-pass
            p.midHighPass = (type == chord || type == pad) ? 150.0f : 200.0f;
        }

        p.filterMode = 1.0f;        // Low 24, fully open: bypassed
        p.filterCutoff = 20000.0f;
        p.velSens = 1.0f;
        p.waveTableB = growlTable;  // unused while tableMix is 0
        p.delayTime = 2.0f;         // 1/8 dotted
        p.delayFeedback = 0.35f;
        p.reverbSize = 0.6f;

        switch (type)
        {
            case bass:
                p.weight = 0.7f; p.grit = 0.35f; p.punch = 0.6f; p.width = 0.3f; p.motion = 0.0f; p.air = 0.0f;
                p.wavePos = sawWave; p.unison = 3; p.detune = 0.15f; p.blend = 0.4f;
                p.ampAttack = 0.002f; p.ampDecay = 0.3f; p.ampSustain = 0.9f; p.ampRelease = 0.12f;
                p.modAttack = 0.0f; p.modDecay = 0.25f; p.modSustain = 0.2f; p.modRelease = 0.2f; p.modToPos = -0.2f;
                p.ottDepth = 0.45f; p.tilt = -1.0f; p.monoFreq = 150.0f; p.stereoWidth = 0.8f;
                p.subOctave = 0.0f; // an octave below a bass note is inaudible and just eats headroom
                p.playMode = 2.0f;  // legato: basses play one note at a time
                p.subSplit = 1.0f;  // clean sub, dirty mids (research_notes/bass_clarity.md)
                break;

            case lead:
                p.weight = 0.2f; p.grit = 0.25f; p.punch = 0.5f; p.width = 0.6f; p.motion = 0.0f; p.air = 0.3f;
                p.wavePos = sawWave; p.unison = 5; p.detune = 0.3f; p.blend = 0.6f;
                p.ampAttack = 0.005f; p.ampDecay = 0.4f; p.ampSustain = 0.8f; p.ampRelease = 0.25f;
                p.modAttack = 0.0f; p.modDecay = 0.5f; p.modSustain = 0.3f; p.modRelease = 0.3f; p.modToPos = 0.1f;
                p.lfoRate = 5.5f; p.lfoToPitch = 0.05f; // light vibrato
                p.ottDepth = 0.4f; p.tilt = 0.5f;
                p.delayMix = 0.15f; p.reverbMix = 0.15f;
                break;

            case chord:
                p.weight = 0.15f; p.grit = 0.15f; p.punch = 0.4f; p.width = 0.75f; p.motion = 0.1f; p.air = 0.35f;
                p.wavePos = sawWave; p.unison = 7; p.detune = 0.35f; p.blend = 0.7f;
                p.ampAttack = 0.01f; p.ampDecay = 0.6f; p.ampSustain = 0.7f; p.ampRelease = 0.4f;
                p.modAttack = 0.0f; p.modDecay = 0.8f; p.modSustain = 0.4f; p.modRelease = 0.4f; p.modToPos = 0.15f;
                p.lfoRate = 0.3f; p.lfoShape = triangle; p.lfoToPos = 0.1f;
                p.ottDepth = 0.3f; p.monoFreq = 150.0f; p.stereoWidth = 1.2f;
                p.reverbMix = 0.2f; p.reverbSize = 0.7f;
                break;

            case pad:
                p.weight = 0.1f; p.grit = 0.1f; p.punch = 0.0f; p.width = 0.9f; p.motion = 0.35f; p.air = 0.4f;
                p.wavePos = 0.4f; p.unison = 7; p.detune = 0.4f; p.blend = 0.8f;
                p.ampAttack = 0.8f; p.ampDecay = 1.5f; p.ampSustain = 0.8f; p.ampRelease = 2.0f;
                p.modAttack = 1.5f; p.modDecay = 2.0f; p.modSustain = 0.6f; p.modRelease = 2.0f; p.modToPos = 0.2f;
                p.lfoRate = 0.2f; p.lfoShape = triangle; p.lfoToPos = 0.25f;
                p.ottDepth = 0.25f; p.tilt = -0.5f; p.monoFreq = 200.0f; p.stereoWidth = 0.9f; // ~37% side: wide, still holds in mono
                p.reverbMix = 0.35f; p.reverbSize = 0.85f;
                break;

            case pluck:
                p.weight = 0.1f; p.grit = 0.2f; p.punch = 0.8f; p.width = 0.6f; p.motion = 0.0f; p.air = 0.4f;
                p.wavePos = 0.6f; p.unison = 3; p.detune = 0.2f; p.blend = 0.5f;
                p.ampAttack = 0.001f; p.ampDecay = 0.35f; p.ampSustain = 0.0f; p.ampRelease = 0.3f;
                p.modAttack = 0.0f; p.modDecay = 0.15f; p.modSustain = 0.0f; p.modRelease = 0.2f; p.modToPos = -0.2f;
                p.filterCutoff = 500.0f; p.filterRes = 0.25f; p.filterEnv = 5.5f; p.filterKeytrack = 0.5f; // classic filter pluck
                p.velToCutoff = 1.5f;
                p.lfoRate = 1.0f;
                p.ottDepth = 0.5f; p.tilt = 0.5f; p.monoFreq = 150.0f; p.stereoWidth = 1.1f;
                p.delayMix = 0.2f; p.delayTime = 1.0f; p.delayFeedback = 0.4f; p.reverbMix = 0.25f; // 1/8 echoes
                break;

            default: // fx
                p.weight = 0.0f; p.grit = 0.4f; p.punch = 0.2f; p.width = 0.9f; p.motion = 0.6f; p.air = 0.5f;
                p.wavePos = 0.8f; p.unison = 5; p.detune = 0.6f; p.blend = 0.7f;
                p.ampAttack = 0.3f; p.ampDecay = 2.0f; p.ampSustain = 0.5f; p.ampRelease = 2.5f;
                p.modAttack = 2.0f; p.modDecay = 2.0f; p.modSustain = 0.5f; p.modRelease = 2.0f; p.modToPos = 0.5f;
                p.lfoRate = 4.0f; p.lfoSync = syncEighth; p.lfoShape = sampleHold; p.lfoToPos = 0.6f; p.lfoToPitch = 0.2f; // steps on 1/8s
                p.ottDepth = 0.5f; p.tilt = 1.0f; p.stereoWidth = 0.7f; // references sit at 4-43% side; 1.5 went anti-phase
                p.delayMix = 0.2f; p.reverbMix = 0.4f; p.reverbSize = 0.8f;
                break;
        }

        return p;
    }

    // Broad genre character, added on top of the type template.
    struct GenreShift
    {
        float grit = 0, ott = 0, air = 0, tilt = 0, detune = 0, width = 0, motion = 0, punch = 0, weight = 0;
    };

    GenreShift genreShift (int genre)
    {
        switch (genre)
        {
            //                   grit    ott    air   tilt  detune width motion punch weight
            case edm:         return { 0.05f, 0.10f, 0.10f, 0.0f, 0.05f, 0.0f,  0.0f,  0.0f,  0.0f };
            case dubstep:     return { 0.35f, 0.30f, 0.0f,  0.0f, 0.0f,  0.0f,  0.3f,  0.0f,  0.1f };
            case techno:      return { 0.15f, 0.0f, -0.10f,-1.0f,-0.10f,-0.15f, 0.15f, 0.0f,  0.0f };
            case techHouse:   return { 0.10f, 0.0f,  0.0f,  0.0f,-0.10f,-0.10f, 0.0f,  0.15f, 0.0f };
            case trap:        return { 0.10f, 0.0f,  0.05f, 0.0f, 0.0f,  0.0f,  0.0f,  0.0f,  0.25f };
            case synthwave:   return {-0.10f,-0.15f,-0.10f,-1.0f, 0.10f, 0.10f, 0.10f, 0.0f,  0.0f };
            case drumAndBass: return { 0.30f, 0.25f, 0.0f,  0.0f, 0.15f, 0.0f,  0.2f,  0.0f,  0.1f };
            case pop:         return {-0.15f,-0.10f, 0.20f, 0.5f,-0.05f, 0.0f,  0.0f,  0.05f, 0.0f };
            case house:       return { 0.0f,  0.05f, 0.05f, 0.0f, 0.0f,  0.0f,  0.0f,  0.10f, 0.0f };
            case trance:      return { 0.0f,  0.10f, 0.15f, 0.0f, 0.20f, 0.15f, 0.0f,  0.0f,  0.0f };
            case futureBass:  return { 0.0f,  0.15f, 0.20f, 0.0f, 0.20f, 0.15f, 0.2f,  0.0f,  0.0f };
            case hardstyle:   return { 0.45f, 0.20f, 0.0f,  0.5f, 0.0f,  0.0f,  0.0f,  0.20f, 0.1f };
            default:          return {};
        }
    }

    // How wet each genre runs, added to the type's delay and reverb (basses stay dry).
    void applySpace (Profile& p, int genre, int type)
    {
        if (type == bass)
            return;

        auto delay = 0.0f, verb = 0.0f;
        switch (genre)
        {
            case synthwave:            verb = 0.10f; break;
            case trance:               verb = 0.10f; delay = 0.05f; break;
            case pop:                  verb = -0.05f; break;
            case techno: case techHouse: verb = -0.05f; delay = 0.05f; break;
            case hardstyle:            verb = 0.05f; break;
            case trap:                 delay = type == lead ? 0.05f : 0.0f; break;
            default:                   break;
        }

        p.delayMix = juce::jlimit (0.0f, 1.0f, p.delayMix + delay);
        p.reverbMix = juce::jlimit (0.0f, 1.0f, p.reverbMix + verb);
    }

    // Profiles with too few references to tune (fewer than 3), designed from
    // reports/Artist sound design targets.md instead. Runs last, so it wins.
    void applyResearchedGaps (Profile& p, int genre, int type)
    {
        if (genre == edm && type == bass)
        {
            // Foghorn (Chase & Status "Baddadan"): bright stacked partials over the
            // clean sub; organ drawbars give the strong 3rd harmonic (+19 st), blended
            // with a saw, brightened by the mod envelope at each note.
            p.waveTable = organTable; p.wavePos = 0.5f; p.waveTableB = 0.0f; p.tableMix = 0.5f;
            p.unison = 3; p.detune = 0.15f;
            p.ampAttack = 0.002f; p.ampSustain = 1.0f; p.ampRelease = 0.024f;
            p.modAttack = 0.005f; p.modDecay = 0.15f; p.modSustain = 0.3f; p.modToPos = 0.3f;
            p.filterCutoff = 6000.0f; p.filterKeytrack = 0.5f;
            p.weight = 0.7f; p.grit = 0.7f; p.punch = 0.6f; p.width = 0.2f; p.air = 0.3f;
            p.ottDepth = 0.75f; p.tilt = 2.0f;
        }

        if (genre == dubstep && type == chord)
        {
            // Melodic-dubstep drop chord (Seven Lions, Illenium): two saw stacks of
            // 7 voices with low detune, a quick pitch blip, driven after the high-pass.
            p.wavePos = sawWave; p.unison = 7; p.detune = 0.25f; p.blend = 0.8f;
            p.ampAttack = 0.01f; p.ampDecay = 0.5f; p.ampSustain = 1.0f; p.ampRelease = 0.35f;
            p.modAttack = 0.0f; p.modDecay = 0.05f; p.modSustain = 0.0f; p.modToPos = 0.0f; p.modToPitch = 3.0f;
            p.lfoRate = 0.3f; p.lfoShape = triangle; p.lfoToPos = 0.0f;
            p.grit = 0.35f; p.ottDepth = 0.4f; p.air = 0.55f; p.tilt = 1.5f; p.width = 0.8f;
            p.reverbMix = 0.2f;
        }

        if (genre == techno && type == pluck)
        {
            // Melodic-techno pluck: saw through a 24 dB low-pass with a strong, short
            // envelope; dotted-1/8 echoes.
            p.wavePos = sawWave; p.unison = 2; p.detune = 0.15f;
            p.ampDecay = 0.22f; p.ampRelease = 0.15f;
            p.modDecay = 0.18f; p.modToPos = 0.0f;
            p.filterCutoff = 800.0f; p.filterRes = 0.3f; p.filterEnv = 4.0f;
            p.grit = 0.35f; p.width = 0.5f; p.ottDepth = 0.4f; p.tilt = 2.0f;
            p.delayMix = 0.2f; p.delayTime = 2.0f;
        }

        if (genre == techHouse && type == bass)
        {
            // Fisher / Chris Lake / Stussy roller: near-sine, mono, a short plucky
            // low-pass envelope at 150 Hz, light saturation on the mids only.
            p.wavePos = 0.15f; p.unison = 1; p.detune = 0.0f;
            p.ampAttack = 0.002f; p.ampDecay = 0.2f; p.ampSustain = 0.7f; p.ampRelease = 0.04f;
            p.modAttack = 0.0f; p.modDecay = 0.12f; p.modSustain = 0.1f; p.modToPos = 0.0f;
            p.filterCutoff = 450.0f; p.filterRes = 0.0f; p.filterEnv = 2.0f; p.velToCutoff = 1.0f;
            p.weight = 0.85f; p.grit = 0.3f; p.punch = 0.6f; p.width = 0.05f; p.air = 0.0f;
            p.ottDepth = 0.2f;
        }

        if (genre == techHouse && type == chord)
        {
            // Minor-7th stab: two saws, a closed low-pass that snaps open, dotted echoes.
            p.wavePos = sawWave; p.unison = 2; p.detune = 0.15f;
            p.ampAttack = 0.001f; p.ampDecay = 0.5f; p.ampSustain = 0.5f; p.ampRelease = 0.25f;
            p.modAttack = 0.0f; p.modDecay = 0.25f; p.modSustain = 0.0f; p.modToPos = 0.0f;
            p.filterCutoff = 2500.0f; p.filterEnv = 2.0f; p.filterKeytrack = 0.5f;
            p.lfoToPos = 0.0f;
            p.grit = 0.4f; p.punch = 0.6f; p.width = 0.5f; p.ottDepth = 0.7f; p.stereoWidth = 0.9f; p.tilt = 1.0f;
            p.delayMix = 0.15f; p.reverbMix = 0.15f;
        }

        if (genre == techHouse && type == pad)
        {
            // Filtered chord-pad rather than an ambient wash; slow cutoff drift.
            p.wavePos = sawWave; p.unison = 3; p.detune = 0.15f;
            p.ampAttack = 0.5f; p.ampSustain = 1.0f; p.ampRelease = 2.0f;
            p.modToPos = 0.0f;
            p.filterMode = 0.0f; p.filterCutoff = 4000.0f; p.lfoToCutoff = 0.15f;
            p.lfoRate = 0.12f; p.lfoToPos = 0.0f;
            p.grit = 0.05f; p.ottDepth = 0.3f; p.air = 0.5f; p.tilt = 1.5f; p.levelTrim = 5.0f;
        }

        if (genre == trap && type == chord)
        {
            // Festival brass (RL Grime): a saw stack whose low-pass swells open, distorted.
            p.wavePos = sawWave; p.unison = 5; p.detune = 0.3f;
            p.ampAttack = 0.015f; p.ampDecay = 0.5f; p.ampSustain = 1.0f; p.ampRelease = 0.3f;
            p.modAttack = 0.05f; p.modDecay = 0.4f; p.modSustain = 0.5f; p.modToPos = 0.0f;
            p.filterMode = 0.0f; p.filterCutoff = 1400.0f; p.filterEnv = 3.0f; p.filterKeytrack = 0.5f;
            p.lfoToPos = 0.0f;
            p.grit = 0.55f; p.ottDepth = 0.5f; p.width = 0.7f; p.air = 0.45f; p.tilt = 2.5f;
        }

        if (genre == trap && type == pad)
        {
            // Dark choir pad (Metro Boomin palette): low unison, static low-pass, big reverb.
            p.waveTable = choirTable; p.wavePos = 0.3f; p.waveTableB = analogTable; p.tableMix = 0.3f;
            p.unison = 3; p.detune = 0.2f;
            p.ampAttack = 0.5f; p.ampSustain = 1.0f; p.ampRelease = 2.0f;
            p.modToPos = 0.0f;
            p.filterMode = 0.0f; p.filterCutoff = 4500.0f;
            p.lfoRate = 0.2f; p.lfoToPos = 0.15f;
            p.grit = 0.05f; p.ottDepth = 0.2f; p.air = 0.35f; p.tilt = 1.5f; p.levelTrim = 5.0f;
            p.reverbMix = 0.4f;
        }

        if (genre == synthwave && type == chord)
        {
            // Jupiter / Juno chord: the owner's Jupiter bank over an analog saw, slow
            // brassy swell, PWM-like movement at chorus rate.
            p.waveTable = jupiterTable; p.wavePos = 0.85f; p.waveTableB = analogTable; p.tableMix = 0.5f;
            p.unison = 3; p.detune = 0.2f;
            p.ampAttack = 0.2f; p.ampDecay = 1.5f; p.ampSustain = 0.7f; p.ampRelease = 0.6f;
            p.modToPos = 0.0f;
            p.filterMode = 0.0f; p.filterCutoff = 6000.0f; p.filterRes = 0.1f;
            p.lfoRate = 0.5f; p.lfoShape = triangle; p.lfoToPos = 0.15f;
            p.grit = 0.1f; p.ottDepth = 0.25f; p.width = 0.8f; p.tilt = 2.0f;
            p.reverbMix = 0.3f;
        }

        if (genre == synthwave && type == pad)
        {
            // Jupiter-8 pad: the owner's Jupiter bank with an analog saw, long envelope,
            // gentle low-pass, Juno-chorus-rate movement.
            p.waveTable = jupiterTable; p.wavePos = 0.8f; p.waveTableB = analogTable; p.tableMix = 0.5f;
            p.unison = 3; p.detune = 0.2f;
            p.ampAttack = 0.8f; p.ampSustain = 0.9f; p.ampRelease = 3.0f;
            p.modToPos = 0.0f;
            p.filterMode = 0.0f; p.filterCutoff = 4500.0f; p.filterRes = 0.1f;
            p.lfoRate = 0.5f; p.lfoShape = triangle; p.lfoToPos = 0.2f;
            p.grit = 0.05f; p.ottDepth = 0.2f; p.tilt = 1.5f; p.stereoWidth = 0.7f; p.levelTrim = 3.5f;
            p.reverbMix = 0.4f;
        }

        if (genre == synthwave && type == fx)
        {
            // Laser zap: a saw diving two octaves into the note, into dotted-1/8 echoes and a hall.
            p.waveTable = 0.0f; p.wavePos = sawWave; p.unison = 3; p.detune = 0.3f;
            p.ampAttack = 0.001f; p.ampDecay = 0.6f; p.ampSustain = 0.2f; p.ampRelease = 1.0f;
            p.modAttack = 0.0f; p.modDecay = 0.25f; p.modSustain = 0.0f; p.modToPos = 0.0f; p.modToPitch = 24.0f;
            p.lfoToPos = 0.0f; p.lfoToPitch = 0.0f;
            p.grit = 0.25f; p.tilt = 4.0f; p.air = 0.7f; p.ottDepth = 0.8f;
            p.delayMix = 0.35f; p.delayTime = 2.0f; p.delayFeedback = 0.45f; p.reverbMix = 0.4f; p.reverbSize = 0.85f;
        }
    }

    // Signature sounds that a shift alone can't reach.
    void applySignature (Profile& p, int genre, int type)
    {
        if (type == bass)
        {
            switch (genre)
            {
                case trap: // 808: held sine with a fast pitch drop, driven for harmonics
                    p.wavePos = sineWave; p.unison = 1; p.detune = 0.0f;
                    p.ampAttack = 0.001f; p.ampSustain = 1.0f; p.ampRelease = 0.5f;
                    p.modAttack = 0.0f; p.modDecay = 0.07f; p.modSustain = 0.0f; p.modRelease = 0.05f;
                    p.modToPos = 0.0f; p.modToPitch = 12.0f;
                    p.grit = 0.6f; p.punch = 0.7f; p.ottDepth = 0.3f; p.tilt = 0.0f;
                    p.monoFreq = 300.0f;
                    p.glide = 0.08f; // overlapping notes slide
                    break;

                case dubstep: // classic wobble: the LFO sweeps the Basic shapes
                    p.lfoRate = 3.0f; p.lfoSync = syncQuarterTriplet; p.lfoShape = sine; p.lfoToPos = 0.7f; // 3 Hz at 120 BPM
                    break;

                case drumAndBass: // deep rolling reese: darker detuned wave over a heavy sub
                    p.wavePos = 0.35f; p.unison = 5; p.detune = 0.3f; p.blend = 0.5f;
                    p.lfoRate = 0.25f; p.lfoShape = triangle; p.lfoToPos = 0.15f;
                    p.weight = 1.0f; p.grit = 0.4f; p.motion = 0.1f; p.air = 0.0f;
                    p.ottDepth = 0.4f; p.tilt = -2.5f; p.monoFreq = 150.0f;
                    p.glide = 0.03f;
                    break;

                case techno: // rolling, short, filter-plucked
                    p.ampDecay = 0.18f; p.ampSustain = 0.3f; p.modToPos = -0.35f;
                    p.filterCutoff = 250.0f; p.filterRes = 0.35f; p.filterEnv = 4.0f; p.velToCutoff = 1.0f;
                    break;

                case synthwave: // square-ish analog filter bass
                    p.wavePos = squareWave; p.unison = 1; p.grit = 0.15f;
                    p.filterCutoff = 700.0f; p.filterRes = 0.3f; p.filterEnv = 3.0f; p.velToCutoff = 1.0f;
                    break;

                default:
                    break;
            }

            // Where the mid layer starts (bass_clarity.md table): lighter genres keep
            // more body in the mids; the 808, synthwave and techno basses are one
            // saturated layer (no clean sub).
            switch (genre)
            {
                case edm: case futureBass: case house: case pop: p.midHighPass = 90.0f; break;
                case trap:                                      p.subSplit = 0.0f; break; // the 808 is saturated whole: its distorted fundamental is the sound
                case synthwave:                                 p.subSplit = 0.0f; break;
                case techno:                                    p.subSplit = 0.0f; break; // its loudness comes from the driven sub; references will decide
                default:                                        break; // 120 Hz
            }
        }

        if ((genre == trance || genre == futureBass || genre == edm) && (type == lead || type == chord))
            p.unison = 7; // supersaw

        // The owner's favourite, the trance lead ("big and deep, a bit bright"),
        // lent to other genres' leads: its width, air, squash and space as a
        // floor, with each genre's own grit and tilt kept on top. Genres with a
        // lead of their own (below) stay out of it so they don't all sound alike.
        const auto bigLead = genre == edm;
        if (type == lead && bigLead)
        {
            p.unison = 7;
            p.detune = juce::jmax (p.detune, 0.5f);
            p.width = juce::jmax (p.width, 0.75f);
            p.air = juce::jmax (p.air, 0.45f);
            p.ottDepth = juce::jmax (p.ottDepth, 0.5f);
            p.delayMix = juce::jmax (p.delayMix, 0.2f);
            p.reverbMix = juce::jmax (p.reverbMix, 0.25f);
        }

        // Each genre's own lead (reports/Artist sound design targets.md).
        if (type == lead)
        {
            switch (genre)
            {
                case dubstep: // screech: harsh table moved at 1/16 (140 BPM), driven and squashed, mono
                    p.waveTable = harshTable; p.wavePos = 0.8f; p.unison = 3; p.detune = 0.25f;
                    p.modToPos = 0.2f;
                    p.lfoRate = 9.33f; p.lfoSync = syncSixteenth; p.lfoShape = square; p.lfoToPos = 0.2f; p.lfoToPitch = 0.0f;
                    p.grit = 0.75f; p.motion = 0.5f; p.air = 0.5f; p.width = 0.5f;
                    p.ottDepth = 0.8f; p.tilt = 2.5f; p.stereoWidth = 1.0f;
                    p.delayMix = 0.1f; p.reverbMix = 0.1f;
                    p.playMode = 1.0f; p.glide = 0.03f;
                    break;

                case techno: // hoover / dark poly: slow PWM, pitch dip into the note, darker and drier
                    p.waveTable = pwmTable; p.wavePos = 0.4f; p.unison = 3; p.detune = 0.35f;
                    p.modAttack = 0.0f; p.modDecay = 0.15f; p.modSustain = 0.0f; p.modToPos = 0.0f; p.modToPitch = -2.0f;
                    p.lfoRate = 0.4f; p.lfoShape = triangle; p.lfoToPos = 0.3f; p.lfoToPitch = 0.0f;
                    p.filterCutoff = 5000.0f; p.filterRes = 0.2f; p.filterKeytrack = 0.5f;
                    p.grit = 0.45f; p.air = 0.2f; p.width = 0.35f;
                    p.ottDepth = 0.45f; p.tilt = -0.5f; p.stereoWidth = 0.8f;
                    p.delayMix = 0.15f; p.reverbMix = 0.12f;
                    p.playMode = 2.0f; p.glide = 0.04f;
                    break;

                case techHouse: // short, dry, near-mono sine/organ hook (Mau P style)
                    p.waveTable = organTable; p.wavePos = 0.55f; p.unison = 1; p.detune = 0.0f;
                    p.ampAttack = 0.002f; p.ampDecay = 0.25f; p.ampSustain = 0.45f; p.ampRelease = 0.12f;
                    p.modToPos = 0.0f;
                    p.filterCutoff = 6000.0f; p.filterEnv = 1.5f; p.velToCutoff = 1.0f;
                    p.lfoToPitch = 0.0f;
                    p.grit = 0.4f; p.punch = 0.7f; p.air = 0.3f; p.width = 0.2f;
                    p.ottDepth = 0.5f; p.tilt = 1.0f; p.stereoWidth = 0.6f; p.levelTrim = 5.0f;
                    p.delayMix = 0.12f; p.delayTime = 1.0f; p.reverbMix = 0.08f; // 1/8 echoes
                    p.playMode = 1.0f;
                    break;

                case trap: // brass horn stab (RL Grime style): saw with a filter swell, distorted
                    p.unison = 3; p.detune = 0.2f;
                    p.ampAttack = 0.015f; p.ampDecay = 0.5f; p.ampSustain = 0.6f; p.ampRelease = 0.15f;
                    p.modAttack = 0.03f; p.modDecay = 0.35f; p.modSustain = 0.4f; p.modToPos = 0.0f;
                    p.filterMode = 0.0f; p.filterCutoff = 1000.0f; p.filterEnv = 4.0f; p.filterKeytrack = 0.5f;
                    p.lfoRate = 5.0f; p.lfoToPitch = 0.03f;
                    p.grit = 0.6f; p.air = 0.35f; p.width = 0.45f;
                    p.ottDepth = 0.6f; p.tilt = 1.0f;
                    p.reverbMix = 0.15f; p.delayMix = 0.1f;
                    break;

                case synthwave: // big Jupiter lead: the owner's Jupiter-8 (measured) layered with a bright
                                // analog saw, 7 voices wide, vibrato and glide
                    p.waveTable = jupiterTable; p.wavePos = 0.85f; p.waveTableB = analogTable; p.tableMix = 0.5f;
                    p.unison = 7; p.detune = 0.5f;
                    p.ampAttack = 0.01f; p.ampSustain = 0.9f; p.ampRelease = 0.35f;
                    p.modToPos = 0.0f;
                    p.lfoRate = 5.2f; p.lfoShape = triangle; p.lfoToPitch = 0.05f; p.lfoToPos = 0.05f;
                    p.grit = 0.2f; p.air = 0.5f; p.width = 0.8f;
                    p.ottDepth = 0.55f; p.tilt = 0.75f; p.stereoWidth = 0.8f;
                    p.delayMix = 0.25f; p.reverbMix = 0.3f; p.reverbSize = 0.8f; // dotted 1/8 from the template
                    p.playMode = 2.0f; p.glide = 0.06f;
                    break;

                case futureBass: // vocal-chop-like lead (Illenium, San Holo, Flume): choir formants over a saw,
                                 // a scoop up into each note, formants moving at 1/8
                    p.waveTable = choirTable; p.wavePos = 0.6f; p.waveTableB = 0.0f; p.tableMix = 0.5f; // B: Basic at saw-ish
                    p.unison = 5; p.detune = 0.4f;
                    p.ampAttack = 0.005f; p.ampSustain = 0.85f; p.ampRelease = 0.25f;
                    p.modAttack = 0.0f; p.modDecay = 0.08f; p.modSustain = 0.0f; p.modToPos = 0.0f; p.modToPitch = -2.0f;
                    p.lfoSync = syncEighth; p.lfoShape = triangle; p.lfoToPos = 0.15f; p.lfoToPitch = 0.0f;
                    p.grit = 0.2f; p.air = 0.5f; p.width = 0.75f;
                    p.ottDepth = 0.7f; p.tilt = 1.0f;
                    p.delayMix = 0.2f; p.reverbMix = 0.25f;
                    break;

                case drumAndBass: // sharp mono stab-lead (Sub Focus, Wilkinson): sync-peak table, quick bright
                                  // envelope sweep, glide, driven
                    p.waveTable = digitalTable; p.wavePos = 0.5f; p.unison = 5; p.detune = 0.35f;
                    p.ampAttack = 0.002f; p.ampDecay = 0.4f; p.ampSustain = 0.7f; p.ampRelease = 0.15f;
                    p.modAttack = 0.0f; p.modDecay = 0.2f; p.modSustain = 0.2f; p.modToPos = 0.3f;
                    p.lfoToPitch = 0.0f;
                    p.grit = 0.45f; p.air = 0.4f; p.width = 0.5f;
                    p.ottDepth = 0.6f; p.tilt = 1.0f;
                    p.delayMix = 0.15f; p.delayTime = 1.0f; p.reverbMix = 0.15f; // 1/8 echoes
                    p.playMode = 1.0f; p.glide = 0.04f;
                    break;

                case pop: // clean, bright lead between saw and square: light unison, vibrato, polished
                    p.wavePos = 0.65f; p.unison = 3; p.detune = 0.15f;
                    p.ampAttack = 0.005f; p.ampSustain = 0.85f; p.ampRelease = 0.25f;
                    p.modToPos = 0.0f;
                    p.lfoRate = 5.5f; p.lfoToPitch = 0.04f;
                    p.grit = 0.15f; p.air = 0.6f; p.width = 0.6f;
                    p.ottDepth = 0.5f; p.tilt = 2.0f;
                    p.delayMix = 0.15f; p.reverbMix = 0.2f;
                    break;

                case house: // electric-keys lead: soft FM bell-piano tone that decays, dotted echoes
                    p.waveTable = fmTable; p.wavePos = 0.7f; p.unison = 2; p.detune = 0.1f;
                    p.ampAttack = 0.002f; p.ampDecay = 0.6f; p.ampSustain = 0.65f; p.ampRelease = 0.3f;
                    p.modAttack = 0.0f; p.modDecay = 0.4f; p.modSustain = 0.2f; p.modToPos = 0.25f; // brighter on the strike
                    p.lfoToPitch = 0.0f;
                    p.grit = 0.3f; p.air = 0.45f; p.width = 0.5f;
                    p.ottDepth = 0.55f; p.tilt = 2.0f;
                    p.delayMix = 0.2f; p.reverbMix = 0.2f;
                    break;

                default:
                    break;
            }
        }

        if (genre == hardstyle && type == lead)
        {
            p.waveTable = digitalTable; p.wavePos = 0.9f; // high sync peak: bright
            p.lfoRate = 6.0f; p.lfoToPitch = 0.1f; // screech
        }

        if (genre == house && type == chord)
        {
            p.waveTable = organTable; p.wavePos = 0.9f; // organ stab, most drawbars out
        }

        if (genre == trap && type == pluck)
        {
            p.waveTable = bellTable; p.wavePos = 0.6f; // bell pluck
            p.filterCutoff = 2500.0f; p.filterEnv = 3.0f; // keep the bell's high partials
        }

        if (genre == synthwave && type == pad)
        {
            p.waveTable = analogTable; p.wavePos = 0.6f; // warm analog pad
        }

        if ((genre == pop || genre == trance) && type == pad)
        {
            p.waveTableB = choirTable; p.tableMix = 0.35f; // a little vocal shimmer
        }

        if (genre == hardstyle && type == bass)
        {
            p.waveTable = harshTable; p.wavePos = 0.6f; // distorted kick-bass grit
        }

        if ((genre == dubstep || genre == drumAndBass) && type == fx)
        {
            p.waveTable = fmTable; // metallic sweeps; the FX template's high position and S&H LFO move it
        }

        applyResearchedGaps (p, genre, type);
    }

    MacroDirections macroDirections (int genre, int type)
    {
        namespace M = MacroIndex;
        namespace T = MacroTarget;

        MacroDirections d;
        auto& a = d.amount;

        // Shared defaults.
        a[M::weight][T::tilt]     = -2.0f;  // heavier is darker
        a[M::grit][T::ottDepth]   = 0.15f;
        a[M::punch][T::ottDepth]  = 0.3f;
        a[M::punch][T::ampDecay]  = -0.5f;  // tighter
        a[M::width][T::detune]    = 0.2f;
        a[M::motion][T::lfoRate]  = 1.0f;
        a[M::air][T::tilt]        = 1.0f;
        a[M::air][T::filterCutoff] = 1.0f; // opens the filter where one is in use
        a[M::air][T::reverbMix]   = type == bass ? 0.0f : 0.15f;

        // With no sub on these types, Weight adds body by lowering the high-pass.
        if (type == lead || type == chord || type == pad || type == pluck)
            a[M::weight][T::midHighPass] = -1.5f;

        switch (type)
        {
            case bass:  a[M::punch][T::modToPos] = -0.2f; break; // stronger wave pluck
            case pluck: a[M::punch][T::modToPos] = -0.3f; a[M::punch][T::ampDecay] = -1.0f; a[M::punch][T::filterEnv] = 2.0f; break;
            case chord: a[M::width][T::detune] = 0.3f; break;
            case pad:   a[M::motion][T::lfoToPos] = 0.3f; a[M::motion][T::detune] = 0.15f; a[M::punch][T::ottDepth] = 0.15f; break;
            case fx:    a[M::motion][T::lfoRate] = 2.0f; a[M::motion][T::modToPitch] = 12.0f; break;
            default:    break;
        }

        if (type == bass)
        {
            switch (genre)
            {
                case trap: // Punch deepens the 808 knock, Weight stays clean
                    a[M::punch][T::modToPitch] = 12.0f; a[M::punch][T::modToPos] = 0.0f;
                    a[M::punch][T::ampDecay] = 0.0f; a[M::weight][T::tilt] = -1.0f;
                    break;

                case dubstep: // Motion speeds and deepens the wobble
                    a[M::motion][T::lfoRate] = 2.0f; a[M::motion][T::lfoToPos] = 0.3f;
                    a[M::punch][T::ottDepth] = 0.5f;
                    break;

                case drumAndBass: // Motion opens the reese
                    a[M::motion][T::lfoToPos] = 0.3f; a[M::motion][T::detune] = 0.2f;
                    break;

                default:
                    break;
            }
        }

        if (genre == dubstep || genre == hardstyle || genre == drumAndBass)
            a[M::grit][T::ottDepth] = 0.35f; // grit comes with squash

        if (genre == pop || genre == trance || genre == futureBass)
            a[M::air][T::tilt] = 2.0f;

        return d;
    }
}

Profile makeProfile (int genre, int type)
{
    auto p = typeTemplate (type);
    const auto s = genreShift (genre);

    auto unitAdd = [] (float& v, float d) { v = juce::jlimit (0.0f, 1.0f, v + d); };
    unitAdd (p.grit,   s.grit);
    unitAdd (p.ottDepth, s.ott);
    unitAdd (p.air,    s.air);
    unitAdd (p.detune, s.detune);
    unitAdd (p.width,  s.width);
    unitAdd (p.motion, s.motion);
    unitAdd (p.punch,  s.punch);
    unitAdd (p.weight, s.weight);
    p.tilt = juce::jlimit (-6.0f, 6.0f, p.tilt + s.tilt);

    applySpace (p, genre, type);
    applySignature (p, genre, type);
    p.directions = macroDirections (genre, type);

    if (genre >= 0 && genre < 12 && type >= 0 && type < 6 && profileTrims[genre][type].tuned)
        applyTrim (p, profileTrims[genre][type]);

    return p;
}

Profile generateProfile (int genre, int type, juce::Random& random)
{
    // A variation on the genre/type's own profile, so it still sounds finished:
    // the sound source, envelopes, filter, movement, macros and space change;
    // the finishing chain, clean-sub split, high-pass and play mode stay.
    auto p = makeProfile (genre, type);

    auto between = [&] (float lo, float hi) { return lo + random.nextFloat() * (hi - lo); };
    auto nudge = [&] (float& v, float amount, float lo = 0.0f, float hi = 1.0f) { v = juce::jlimit (lo, hi, v + between (-amount, amount)); };
    auto scale = [&] (float& v, float octaves, float lo, float hi) { v = juce::jlimit (lo, hi, v * std::exp2 (between (-octaves, octaves))); };
    auto chance = [&] (float probability) { return random.nextFloat() < probability; };

    // Source: sometimes a different table that suits the type, always a new position.
    if (chance (0.4f))
    {
        static constexpr float bassTables[] { 0.0f, growlTable, fmTable, 5.0f /* Wavefold */, 7.0f /* Neuro */, harshTable, digitalTable };
        static constexpr float toneTables[] { 0.0f, analogTable, digitalTable, organTable, fmTable, pwmTable, choirTable, jupiterTable };
        p.waveTable = type == bass ? bassTables[random.nextInt ((int) std::size (bassTables))]
                                   : toneTables[random.nextInt ((int) std::size (toneTables))];
    }
    nudge (p.wavePos, 0.25f);
    if (chance (0.4f))
    {
        static constexpr float layers[] { 0.0f, analogTable, choirTable, jupiterTable, pwmTable };
        p.waveTableB = layers[random.nextInt ((int) std::size (layers))];
        p.tableMix = between (0.2f, 0.6f);
    }

    p.unison = (float) juce::jlimit (1, type == bass ? 5 : 7, (int) p.unison + random.nextInt (5) - 2);
    scale (p.detune, 0.6f, 0.0f, 0.8f);

    // Envelopes: same shape, different timing.
    scale (p.ampAttack, 0.8f, 0.001f, 2.0f);
    scale (p.ampDecay, 0.7f, 0.05f, 4.0f);
    // Tails must still die away; FX tails are already long, so they get shorter.
    scale (p.ampRelease, 0.7f, 0.02f, type == fx ? p.ampRelease * 0.8f : juce::jmin (3.0f, p.ampRelease * 1.25f));
    nudge (p.ampSustain, 0.15f);
    scale (p.modDecay, 0.8f, 0.03f, 4.0f);
    nudge (p.modToPos, 0.25f, -1.0f, 1.0f);

    // Filter, where one is in use.
    if (p.filterCutoff < 19000.0f)
    {
        scale (p.filterCutoff, 0.8f, 80.0f, 16000.0f);
        nudge (p.filterRes, 0.15f, 0.0f, 0.6f);
    }

    // Movement: often a tempo-synced sweep through the table.
    if (chance (type == bass || type == lead || type == fx ? 0.5f : 0.3f))
    {
        p.lfoSync = (float) (3 + random.nextInt (6)); // 1/4 to 1/16T
        p.lfoShape = (float) random.nextInt (5);
        p.lfoToPos = between (0.1f, type == bass ? 0.6f : 0.35f);
    }

    // Macros and space.
    for (auto* macro : { &p.weight, &p.grit, &p.punch, &p.width, &p.motion, &p.air })
        nudge (*macro, 0.15f);
    nudge (p.delayMix, 0.1f, 0.0f, 0.5f);
    nudge (p.reverbMix, 0.1f, 0.0f, type == fx ? p.reverbMix : 0.6f);
    nudge (p.tilt, 1.0f, -6.0f, 6.0f);

    if (type == bass)
        p.delayMix = p.reverbMix = 0.0f; // basses stay dry
    p.delayFeedback = juce::jmin (p.delayFeedback, 0.35f);

    // Level trim is gain after the clipper: a brighter or denser variation plus
    // a positive trim would go over the ceiling.
    p.levelTrim = juce::jmin (p.levelTrim, 0.0f);

    return p;
}

void applyTrim (Profile& p, const ProfileTrim& trim)
{
    p.levelTrim = trim.levelDb;
    p.ottDepth = trim.ottDepth;
    p.tilt = trim.tilt;
    p.weight = trim.weight;
    p.stereoWidth = trim.stereoWidth;
}

void loadMacroMap (MacroMap& map, const Profile& p)
{
    const float centres[MacroIndex::count] { p.weight, p.grit, p.punch, p.width, p.motion, p.air };
    map.set (centres, p.directions);
}

void applyProfile (juce::AudioProcessorValueTreeState& state, const Profile& p)
{
    auto set = [&] (const char* id, float value)
    {
        if (auto* param = state.getParameter (id))
        {
            param->beginChangeGesture();
            param->setValueNotifyingHost (param->convertTo0to1 (value));
            param->endChangeGesture();
        }
    };

    set (ParamIDs::weight, p.weight);
    set (ParamIDs::grit,   p.grit);
    set (ParamIDs::punch,  p.punch);
    set (ParamIDs::width,  p.width);
    set (ParamIDs::motion, p.motion);
    set (ParamIDs::air,    p.air);

    set (ParamIDs::waveTable, p.waveTable);
    set (ParamIDs::waveTableB, p.waveTableB);
    set (ParamIDs::tableMix, p.tableMix);
    set (ParamIDs::wavePos, p.wavePos);
    set (ParamIDs::unison,  p.unison);
    set (ParamIDs::detune,  p.detune);
    set (ParamIDs::blend,   p.blend);

    set (ParamIDs::ampAttack,  p.ampAttack);
    set (ParamIDs::ampDecay,   p.ampDecay);
    set (ParamIDs::ampSustain, p.ampSustain);
    set (ParamIDs::ampRelease, p.ampRelease);
    set (ParamIDs::modAttack,  p.modAttack);
    set (ParamIDs::modDecay,   p.modDecay);
    set (ParamIDs::modSustain, p.modSustain);
    set (ParamIDs::modRelease, p.modRelease);
    set (ParamIDs::modToPos,   p.modToPos);
    set (ParamIDs::modToPitch, p.modToPitch);
    set (ParamIDs::subOctave,  p.subOctave);
    set (ParamIDs::subOn,      p.subOn);
    set (ParamIDs::subSplit,   p.subSplit);
    set (ParamIDs::midHighPass, p.midHighPass);

    set (ParamIDs::filterMode,     p.filterMode);
    set (ParamIDs::filterCutoff,   p.filterCutoff);
    set (ParamIDs::filterRes,      p.filterRes);
    set (ParamIDs::filterEnv,      p.filterEnv);
    set (ParamIDs::filterKeytrack, p.filterKeytrack);
    set (ParamIDs::lfoToCutoff,    p.lfoToCutoff);
    set (ParamIDs::velToCutoff,    p.velToCutoff);
    set (ParamIDs::velSens,        p.velSens);

    set (ParamIDs::playMode,   p.playMode);
    set (ParamIDs::glide,      p.glide);

    set (ParamIDs::lfoRate,    p.lfoRate);
    set (ParamIDs::lfoShape,   p.lfoShape);
    set (ParamIDs::lfoSync,    p.lfoSync);
    set (ParamIDs::lfoToPos,   p.lfoToPos);
    set (ParamIDs::lfoToPitch, p.lfoToPitch);

    set (ParamIDs::delayMix,      p.delayMix);
    set (ParamIDs::delayTime,     p.delayTime);
    set (ParamIDs::delayFeedback, p.delayFeedback);
    set (ParamIDs::reverbMix,     p.reverbMix);
    set (ParamIDs::reverbSize,    p.reverbSize);

    set (ParamIDs::levelTrim,   p.levelTrim);
    set (ParamIDs::ottDepth,    p.ottDepth);
    set (ParamIDs::tilt,        p.tilt);
    set (ParamIDs::monoFreq,    p.monoFreq);
    set (ParamIDs::stereoWidth, p.stereoWidth);
}
