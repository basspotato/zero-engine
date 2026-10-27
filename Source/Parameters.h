#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace ParamIDs
{
    // The six macro knobs.
    inline constexpr auto weight = "weight";
    inline constexpr auto grit   = "grit";
    inline constexpr auto punch  = "punch";
    inline constexpr auto width  = "width";
    inline constexpr auto motion = "motion";
    inline constexpr auto air    = "air";

    inline constexpr auto genre  = "genre";
    inline constexpr auto type   = "type";
    inline constexpr auto output = "output";

    // Engine parameters (set by profiles later; exposed in the dev panel for now).
    inline constexpr auto wavePos = "wavePos";
    inline constexpr auto unison  = "unison";
    inline constexpr auto detune  = "detune";
    inline constexpr auto blend   = "blend";

    inline constexpr auto ampAttack  = "ampAttack";
    inline constexpr auto ampDecay   = "ampDecay";
    inline constexpr auto ampSustain = "ampSustain";
    inline constexpr auto ampRelease = "ampRelease";

    inline constexpr auto modAttack  = "modAttack";
    inline constexpr auto modDecay   = "modDecay";
    inline constexpr auto modSustain = "modSustain";
    inline constexpr auto modRelease = "modRelease";
    inline constexpr auto modToPos   = "modToPos";
    inline constexpr auto modToPitch = "modToPitch";
    inline constexpr auto subOctave  = "subOctave";

    inline constexpr auto lfoRate    = "lfoRate";
    inline constexpr auto lfoShape   = "lfoShape";
    inline constexpr auto lfoToPos   = "lfoToPos";
    inline constexpr auto lfoToPitch = "lfoToPitch";

    // Playing.
    inline constexpr auto playMode  = "playMode";
    inline constexpr auto glide     = "glide";
    inline constexpr auto bendRange = "bendRange";

    // Finishing chain.
    inline constexpr auto ottDepth    = "ottDepth";
    inline constexpr auto tilt        = "tilt";
    inline constexpr auto monoFreq    = "monoFreq";
    inline constexpr auto stereoWidth = "stereoWidth";
}

inline const juce::StringArray genreNames    { "EDM", "Dubstep", "Techno", "Tech House", "Trap", "Synthwave",
                                                "Drum & Bass", "Pop", "House", "Trance", "Future Bass", "Hardstyle" };
inline const juce::StringArray typeNames     { "Bass", "Lead", "Chord", "Pad", "Pluck", "FX" };
inline const juce::StringArray lfoShapeNames { "Sine", "Triangle", "Saw", "Square", "Sample & Hold" };
inline const juce::StringArray playModeNames { "Poly", "Mono", "Legato" }; // Legato: overlapping notes slide without retriggering

inline juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout layout;

    auto unit = [&] (const char* id, const char* name, float def)
    {
        layout.add (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name,
                                                           NormalisableRange<float> (0.0f, 1.0f), def));
    };

    auto seconds = [&] (const char* id, const char* name, float max, float def)
    {
        layout.add (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name,
                                                           NormalisableRange<float> (0.0f, max, 0.0f, 0.3f), def,
                                                           AudioParameterFloatAttributes().withLabel ("s")));
    };

    unit (ParamIDs::weight, "Weight", 0.5f);
    unit (ParamIDs::grit,   "Grit",   0.2f);
    unit (ParamIDs::punch,  "Punch",  0.5f);
    unit (ParamIDs::width,  "Width",  0.6f);
    unit (ParamIDs::motion, "Motion", 0.0f);
    unit (ParamIDs::air,    "Air",    0.0f);

    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::genre, 1 }, "Genre", genreNames, 0));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::type, 1 },  "Type",  typeNames, 0));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { ParamIDs::output, 1 }, "Output",
                                                       NormalisableRange<float> (-24.0f, 6.0f), 0.0f));

    unit (ParamIDs::wavePos, "Wave Position", 0.5f);
    layout.add (std::make_unique<AudioParameterInt> (ParameterID { ParamIDs::unison, 1 }, "Unison Voices", 1, 7, 5));
    unit (ParamIDs::detune, "Unison Detune", 0.35f);
    unit (ParamIDs::blend,  "Unison Blend",  0.7f);

    seconds (ParamIDs::ampAttack,  "Amp Attack",  5.0f, 0.005f);
    seconds (ParamIDs::ampDecay,   "Amp Decay",   5.0f, 0.3f);
    unit    (ParamIDs::ampSustain, "Amp Sustain", 0.8f);
    seconds (ParamIDs::ampRelease, "Amp Release", 5.0f, 0.25f);

    seconds (ParamIDs::modAttack,  "Mod Attack",  5.0f, 0.0f);
    seconds (ParamIDs::modDecay,   "Mod Decay",   5.0f, 0.4f);
    unit    (ParamIDs::modSustain, "Mod Sustain", 0.0f);
    seconds (ParamIDs::modRelease, "Mod Release", 5.0f, 0.3f);
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { ParamIDs::modToPos, 1 }, "Mod Env > Position",
                                                       NormalisableRange<float> (-1.0f, 1.0f), 0.0f));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { ParamIDs::modToPitch, 1 }, "Mod Env > Pitch",
                                                       NormalisableRange<float> (-24.0f, 24.0f), 0.0f,
                                                       AudioParameterFloatAttributes().withLabel ("st")));
    layout.add (std::make_unique<AudioParameterInt> (ParameterID { ParamIDs::subOctave, 1 }, "Sub Octave", -1, 0, -1));

    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { ParamIDs::lfoRate, 1 }, "LFO Rate",
                                                       NormalisableRange<float> (0.05f, 20.0f, 0.0f, 0.3f), 2.0f,
                                                       AudioParameterFloatAttributes().withLabel ("Hz")));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::lfoShape, 1 }, "LFO Shape", lfoShapeNames, 0));
    unit (ParamIDs::lfoToPos,   "LFO > Position", 0.0f);
    unit (ParamIDs::lfoToPitch, "LFO > Pitch",    0.0f);

    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::playMode, 1 }, "Play Mode", playModeNames, 0));
    seconds (ParamIDs::glide, "Glide", 2.0f, 0.0f);
    layout.add (std::make_unique<AudioParameterInt> (ParameterID { ParamIDs::bendRange, 1 }, "Bend Range", 0, 24, 2,
                                                     AudioParameterIntAttributes().withLabel ("st")));

    unit (ParamIDs::ottDepth, "OTT Depth", 0.35f);
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { ParamIDs::tilt, 1 }, "Tilt",
                                                       NormalisableRange<float> (-6.0f, 6.0f), 0.0f,
                                                       AudioParameterFloatAttributes().withLabel ("dB")));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { ParamIDs::monoFreq, 1 }, "Mono Below",
                                                       NormalisableRange<float> (40.0f, 300.0f, 0.0f, 0.5f), 120.0f,
                                                       AudioParameterFloatAttributes().withLabel ("Hz")));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { ParamIDs::stereoWidth, 1 }, "Stereo Width",
                                                       NormalisableRange<float> (0.0f, 2.0f), 1.0f));

    return layout;
}
