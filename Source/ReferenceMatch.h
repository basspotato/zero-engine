#pragma once

#include <juce_audio_formats/juce_audio_formats.h>
#include "Profiles.h"
#include "SoundPrint.h"

class ZeroEngineProcessor;

// Reference Match: on a background thread, fingerprints a dropped sound, then
// searches the current genre/type's sound (wavetable and position, tilt, Air,
// OTT, Grit, Weight, width, detune, amp envelope) for the settings whose render is closest,
// played at the reference's pitch. Renders through its own engine, so the
// plugin keeps playing. A few seconds; no machine learning.
class ReferenceMatch : private juce::Thread
{
public:
    explicit ReferenceMatch (ZeroEngineProcessor& renderEngine);
    ~ReferenceMatch() override;

    // Message thread. Cancels any match already running.
    void start (const juce::File& reference, int genre, int type, const Profile& startFrom);

    enum class State { idle, running, done, failed };
    State getState() const noexcept { return state.load(); }
    float getProgress() const noexcept { return progress.load(); }

    // Once done or failed (message thread): the result and a line for the display.
    // take() returns the state to idle.
    struct Result
    {
        bool ok = false;
        Profile profile {};
        int genre = 0, type = 0;
        juce::String summary;
        SoundPrint target, matched; // the reference and the best render, for tooling
    };
    Result take();

private:
    void run() override;
    SoundPrint render (const Profile&, int note);

    ZeroEngineProcessor& engine;
    juce::File file;
    int genre = 0, type = 0;
    Profile from {};
    Result result;
    std::atomic<State> state { State::idle };
    std::atomic<float> progress { 0.0f };
};
