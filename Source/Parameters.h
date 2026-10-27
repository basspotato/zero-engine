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
    inline constexpr auto waveTable = "waveTable";
    inline constexpr auto waveTableB = "waveTableB"; // blended in by tableMix, at the same position
    inline constexpr auto tableMix = "tableMix";
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
    inline constexpr auto subSplit   = "subSplit";    // clean sub skips the finishing chain until the clipper
    inline constexpr auto midHighPass = "midHighPass"; // high-pass on everything but the sub, when split

    inline constexpr auto lfoRate    = "lfoRate";
    inline constexpr auto lfoShape   = "lfoShape";
    inline constexpr auto lfoToPos   = "lfoToPos";
    inline constexpr auto lfoToPitch = "lfoToPitch";

    // Filter (per voice; the sub bypasses it).
    inline constexpr auto filterMode     = "filterMode";
    inline constexpr auto filterCutoff   = "filterCutoff";
    inline constexpr auto filterRes      = "filterRes";
    inline constexpr auto filterEnv      = "filterEnv";
    inline constexpr auto filterKeytrack = "filterKeytrack";
    inline constexpr auto lfoToCutoff    = "lfoToCutoff";
    inline constexpr auto velToCutoff    = "velToCutoff";
    inline constexpr auto velSens        = "velSens";

    // Playing.
    inline constexpr auto playMode  = "playMode";
    inline constexpr auto glide     = "glide";
    inline constexpr auto bendRange = "bendRange";

    // Space (in the finishing chain, before the OTT).
    inline constexpr auto delayMix      = "delayMix";
    inline constexpr auto delayTime     = "delayTime";
    inline constexpr auto delayFeedback = "delayFeedback";
    inline constexpr auto reverbMix     = "reverbMix";
    inline constexpr auto reverbSize    = "reverbSize";

    // Finishing chain.
    inline constexpr auto levelTrim   = "levelTrim"; // per-profile loudness match, before Output
    inline constexpr auto ottDepth    = "ottDepth";
    inline constexpr auto tilt        = "tilt";
    inline constexpr auto monoFreq    = "monoFreq";
    inline constexpr auto stereoWidth = "stereoWidth";
}

inline const juce::StringArray genreNames    { "EDM", "Dubstep", "Techno", "Tech House", "Trap", "Synthwave",
                                                "Drum & Bass", "Pop", "House", "Trance", "Future Bass", "Hardstyle" };
inline const juce::StringArray typeNames     { "Bass", "Lead", "Chord", "Pad", "Pluck", "FX" };
inline const juce::StringArray lfoShapeNames { "Sine", "Triangle", "Saw", "Square", "Sample & Hold" };
// Append only: saved sessions store the index.
inline const juce::StringArray waveTableNames  { "Basic", "Growl", "Digital", "Organ", "FM", "Wavefold",
                                                 "PWM", "Neuro", "Bell", "Choir", "Harsh", "Analog" };
inline const juce::StringArray filterModeNames { "Low 12", "Low 24", "Band", "High" };
inline const juce::StringArray delayTimeNames  { "1/16", "1/8", "1/8 Dotted", "1/4", "1/4 Dotted", "1/2" };
inline constexpr float delayTimeBeats[]        { 0.25f, 0.5f, 0.75f, 1.0f, 1.5f, 2.0f };
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

    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::waveTable, 1 }, "Wave Table", waveTableNames, 0));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::waveTableB, 1 }, "Wave Table B", waveTableNames, 1));
    unit (ParamIDs::tableMix, "Table Blend", 0.0f);
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
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { ParamIDs::subSplit, 1 }, "Sub Split", false));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { ParamIDs::midHighPass, 1 }, "Mid High-Pass",
                                                       NormalisableRange<float> (40.0f, 400.0f, 0.0f, 0.5f), 120.0f,
                                                       AudioParameterFloatAttributes().withLabel ("Hz")));

    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { ParamIDs::lfoRate, 1 }, "LFO Rate",
                                                       NormalisableRange<float> (0.05f, 20.0f, 0.0f, 0.3f), 2.0f,
                                                       AudioParameterFloatAttributes().withLabel ("Hz")));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::lfoShape, 1 }, "LFO Shape", lfoShapeNames, 0));
    unit (ParamIDs::lfoToPos,   "LFO > Position", 0.0f);
    unit (ParamIDs::lfoToPitch, "LFO > Pitch",    0.0f);

    auto octaves = [&] (const char* id, const char* name, float min, float max)
    {
        layout.add (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name, NormalisableRange<float> (min, max), 0.0f,
                                                           AudioParameterFloatAttributes().withLabel ("oct")));
    };

    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::filterMode, 1 }, "Filter Mode", filterModeNames, 1));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { ParamIDs::filterCutoff, 1 }, "Filter Cutoff",
                                                       NormalisableRange<float> (20.0f, 20000.0f, 0.0f, 0.25f), 20000.0f,
                                                       AudioParameterFloatAttributes().withLabel ("Hz")));
    unit (ParamIDs::filterRes, "Filter Resonance", 0.0f);
    octaves (ParamIDs::filterEnv, "Mod Env > Cutoff", -8.0f, 8.0f);
    unit (ParamIDs::filterKeytrack, "Filter Keytrack", 0.0f);
    octaves (ParamIDs::lfoToCutoff, "LFO > Cutoff", 0.0f, 6.0f);
    octaves (ParamIDs::velToCutoff, "Velocity > Cutoff", 0.0f, 4.0f); // full velocity = as set; softer = darker
    unit (ParamIDs::velSens, "Velocity > Level", 1.0f);

    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::playMode, 1 }, "Play Mode", playModeNames, 0));
    seconds (ParamIDs::glide, "Glide", 2.0f, 0.0f);
    layout.add (std::make_unique<AudioParameterInt> (ParameterID { ParamIDs::bendRange, 1 }, "Bend Range", 0, 24, 2,
                                                     AudioParameterIntAttributes().withLabel ("st")));

    unit (ParamIDs::delayMix, "Delay Mix", 0.0f);
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::delayTime, 1 }, "Delay Time", delayTimeNames, 2));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { ParamIDs::delayFeedback, 1 }, "Delay Feedback",
                                                       NormalisableRange<float> (0.0f, 0.9f), 0.35f));
    unit (ParamIDs::reverbMix,  "Reverb Mix",  0.0f);
    unit (ParamIDs::reverbSize, "Reverb Size", 0.6f);

    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { ParamIDs::levelTrim, 1 }, "Level Trim",
                                                       NormalisableRange<float> (-18.0f, 12.0f), 0.0f,
                                                       AudioParameterFloatAttributes().withLabel ("dB")));
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
