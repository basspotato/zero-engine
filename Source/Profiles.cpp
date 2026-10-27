#include "Profiles.h"
#include "Parameters.h"

namespace
{
    enum Genre { edm, dubstep, techno, techHouse, trap, synthwave, drumAndBass, pop, house, trance, futureBass, hardstyle };
    enum Type  { bass, lead, chord, pad, pluck, fx };
    enum Lfo   { sine, triangle, saw, square, sampleHold };

    // Wave positions across the frames: sine, triangle, saw, square, pulse.
    constexpr float sineWave = 0.0f, sawWave = 0.5f, squareWave = 0.75f;

    Profile typeTemplate (int type)
    {
        Profile p {};
        p.lfoRate = 2.0f;
        p.lfoShape = sine;
        p.monoFreq = 120.0f;
        p.stereoWidth = 1.0f;
        p.subOctave = -1.0f;

        switch (type)
        {
            case bass:
                p.weight = 0.7f; p.grit = 0.35f; p.punch = 0.6f; p.width = 0.3f; p.motion = 0.0f; p.air = 0.0f;
                p.wavePos = sawWave; p.unison = 3; p.detune = 0.15f; p.blend = 0.4f;
                p.ampAttack = 0.002f; p.ampDecay = 0.3f; p.ampSustain = 0.9f; p.ampRelease = 0.12f;
                p.modAttack = 0.0f; p.modDecay = 0.25f; p.modSustain = 0.2f; p.modRelease = 0.2f; p.modToPos = -0.2f;
                p.ottDepth = 0.45f; p.tilt = -1.0f; p.monoFreq = 150.0f; p.stereoWidth = 0.8f;
                p.subOctave = 0.0f; // an octave below a bass note is inaudible and just eats headroom
                break;

            case lead:
                p.weight = 0.2f; p.grit = 0.25f; p.punch = 0.5f; p.width = 0.6f; p.motion = 0.0f; p.air = 0.3f;
                p.wavePos = sawWave; p.unison = 5; p.detune = 0.3f; p.blend = 0.6f;
                p.ampAttack = 0.005f; p.ampDecay = 0.4f; p.ampSustain = 0.8f; p.ampRelease = 0.25f;
                p.modAttack = 0.0f; p.modDecay = 0.5f; p.modSustain = 0.3f; p.modRelease = 0.3f; p.modToPos = 0.1f;
                p.lfoRate = 5.5f; p.lfoToPitch = 0.05f; // light vibrato
                p.ottDepth = 0.4f; p.tilt = 0.5f;
                break;

            case chord:
                p.weight = 0.15f; p.grit = 0.15f; p.punch = 0.4f; p.width = 0.75f; p.motion = 0.1f; p.air = 0.35f;
                p.wavePos = sawWave; p.unison = 7; p.detune = 0.35f; p.blend = 0.7f;
                p.ampAttack = 0.01f; p.ampDecay = 0.6f; p.ampSustain = 0.7f; p.ampRelease = 0.4f;
                p.modAttack = 0.0f; p.modDecay = 0.8f; p.modSustain = 0.4f; p.modRelease = 0.4f; p.modToPos = 0.15f;
                p.lfoRate = 0.3f; p.lfoShape = triangle; p.lfoToPos = 0.1f;
                p.ottDepth = 0.3f; p.monoFreq = 150.0f; p.stereoWidth = 1.2f;
                break;

            case pad:
                p.weight = 0.1f; p.grit = 0.1f; p.punch = 0.0f; p.width = 0.9f; p.motion = 0.35f; p.air = 0.4f;
                p.wavePos = 0.4f; p.unison = 7; p.detune = 0.4f; p.blend = 0.8f;
                p.ampAttack = 0.8f; p.ampDecay = 1.5f; p.ampSustain = 0.8f; p.ampRelease = 2.0f;
                p.modAttack = 1.5f; p.modDecay = 2.0f; p.modSustain = 0.6f; p.modRelease = 2.0f; p.modToPos = 0.2f;
                p.lfoRate = 0.2f; p.lfoShape = triangle; p.lfoToPos = 0.25f;
                p.ottDepth = 0.25f; p.tilt = -0.5f; p.monoFreq = 200.0f; p.stereoWidth = 1.4f;
                break;

            case pluck:
                p.weight = 0.1f; p.grit = 0.2f; p.punch = 0.8f; p.width = 0.6f; p.motion = 0.0f; p.air = 0.4f;
                p.wavePos = 0.6f; p.unison = 3; p.detune = 0.2f; p.blend = 0.5f;
                p.ampAttack = 0.001f; p.ampDecay = 0.35f; p.ampSustain = 0.0f; p.ampRelease = 0.3f;
                p.modAttack = 0.0f; p.modDecay = 0.15f; p.modSustain = 0.0f; p.modRelease = 0.2f; p.modToPos = -0.45f;
                p.lfoRate = 1.0f;
                p.ottDepth = 0.5f; p.tilt = 0.5f; p.monoFreq = 150.0f; p.stereoWidth = 1.1f;
                break;

            default: // fx
                p.weight = 0.0f; p.grit = 0.4f; p.punch = 0.2f; p.width = 0.9f; p.motion = 0.6f; p.air = 0.5f;
                p.wavePos = 0.8f; p.unison = 5; p.detune = 0.6f; p.blend = 0.7f;
                p.ampAttack = 0.3f; p.ampDecay = 2.0f; p.ampSustain = 0.5f; p.ampRelease = 2.5f;
                p.modAttack = 2.0f; p.modDecay = 2.0f; p.modSustain = 0.5f; p.modRelease = 2.0f; p.modToPos = 0.5f;
                p.lfoRate = 4.0f; p.lfoShape = sampleHold; p.lfoToPos = 0.6f; p.lfoToPitch = 0.2f;
                p.ottDepth = 0.5f; p.tilt = 1.0f; p.stereoWidth = 1.5f;
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
                    break;

                case dubstep: // wobble
                    p.lfoRate = 3.0f; p.lfoShape = sine; p.lfoToPos = 0.7f;
                    break;

                case drumAndBass: // reese: detuned saws over a solid sub at the note
                    p.unison = 5; p.detune = 0.4f; p.blend = 0.6f;
                    p.lfoRate = 0.3f; p.lfoShape = triangle; p.lfoToPos = 0.2f;
                    p.weight = 0.9f; p.tilt = 0.0f;
                    break;

                case techno: // rolling, short
                    p.ampDecay = 0.18f; p.ampSustain = 0.3f; p.modToPos = -0.35f;
                    break;

                case synthwave: // square-ish analog bass
                    p.wavePos = squareWave; p.unison = 1; p.grit = 0.15f;
                    break;

                default:
                    break;
            }
        }

        if ((genre == trance || genre == futureBass || genre == edm) && (type == lead || type == chord))
            p.unison = 7; // supersaw

        if (genre == hardstyle && type == lead)
        {
            p.lfoRate = 6.0f; p.lfoToPitch = 0.1f; // screech
        }
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

    applySignature (p, genre, type);
    return p;
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

    set (ParamIDs::lfoRate,    p.lfoRate);
    set (ParamIDs::lfoShape,   p.lfoShape);
    set (ParamIDs::lfoToPos,   p.lfoToPos);
    set (ParamIDs::lfoToPitch, p.lfoToPitch);

    set (ParamIDs::ottDepth,    p.ottDepth);
    set (ParamIDs::tilt,        p.tilt);
    set (ParamIDs::monoFreq,    p.monoFreq);
    set (ParamIDs::stereoWidth, p.stereoWidth);
}
