#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "Parameters.h"

namespace MacroIndex  { enum { weight, grit, punch, width, motion, air, count }; }

// Secondary targets a macro can push. Units: ottDepth, detune, modToPos and
// lfoToPos in their own 0..1 ranges; tilt in dB; ampDecay and lfoRate in
// octaves (x2 per unit); modToPitch in semitones; filterCutoff and
// filterEnv and midHighPass in octaves; delayMix, reverbMix and tableMix 0..1.
namespace MacroTarget { enum { ottDepth, tilt, detune, ampDecay, modToPos, lfoToPos, lfoRate, modToPitch, filterCutoff, filterEnv,
                               delayMix, reverbMix, tableMix, midHighPass, count }; }

struct MacroDirections
{
    float amount[MacroIndex::count][MacroTarget::count] {};
};

// The profile-specific part of each macro. Macros keep their fixed main job
// (Weight = sub, Grit = drive, ...); this adds what the current profile wants
// them to push as well. Offsets are relative to the profile's own macro
// positions, so a freshly loaded profile sounds exactly as designed.
// Written on the message thread, read lock-free on the audio thread.
class MacroMap
{
public:
    explicit MacroMap (juce::AudioProcessorValueTreeState& state)
        : macros { state.getRawParameterValue (ParamIDs::weight), state.getRawParameterValue (ParamIDs::grit),
                   state.getRawParameterValue (ParamIDs::punch),  state.getRawParameterValue (ParamIDs::width),
                   state.getRawParameterValue (ParamIDs::motion), state.getRawParameterValue (ParamIDs::air) }
    {
    }

    void set (const float (&centres)[MacroIndex::count], const MacroDirections& directions) noexcept
    {
        for (int m = 0; m < MacroIndex::count; ++m)
        {
            centre[m] = centres[m];
            for (int t = 0; t < MacroTarget::count; ++t)
                amount[m][t] = directions.amount[m][t];
        }
    }

    float offset (int target) const noexcept
    {
        auto sum = 0.0f;
        for (int m = 0; m < MacroIndex::count; ++m)
        {
            const auto value = m == MacroIndex::motion ? motion() : macros[m]->load();
            sum += (value - centre[m].load()) * amount[m][target].load();
        }
        return sum;
    }

    // The mod wheel (0..1) adds on top of the Motion knob, up to full Motion.
    // Audio thread writes, voices read.
    void setModWheel (float amount01) noexcept { wheel.store (juce::jlimit (0.0f, 1.0f, amount01)); }
    float motion() const noexcept { return juce::jlimit (0.0f, 1.0f, macros[MacroIndex::motion]->load() + wheel.load()); }

private:
    std::atomic<float>* macros[MacroIndex::count];
    std::atomic<float> centre[MacroIndex::count] {};
    std::atomic<float> wheel { 0.0f };
    std::atomic<float> amount[MacroIndex::count][MacroTarget::count] {};
};
