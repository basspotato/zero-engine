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
}

inline const juce::StringArray genreNames { "EDM", "Dubstep", "Techno", "Tech House", "Trap", "Synthwave" };
inline const juce::StringArray typeNames  { "Bass", "Lead", "Chord", "Pad", "Pluck", "FX" };

inline juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout layout;

    auto macro = [&] (const char* id, const char* name, float def)
    {
        layout.add (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name,
                                                           NormalisableRange<float> (0.0f, 1.0f), def));
    };

    macro (ParamIDs::weight, "Weight", 0.5f);
    macro (ParamIDs::grit,   "Grit",   0.2f);
    macro (ParamIDs::punch,  "Punch",  0.5f);
    macro (ParamIDs::width,  "Width",  0.4f);
    macro (ParamIDs::motion, "Motion", 0.0f);
    macro (ParamIDs::air,    "Air",    0.0f);

    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::genre, 1 }, "Genre", genreNames, 0));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::type, 1 },  "Type",  typeNames, 0));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { ParamIDs::output, 1 }, "Output",
                                                       NormalisableRange<float> (-24.0f, 6.0f), 0.0f));
    return layout;
}
