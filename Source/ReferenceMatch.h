#pragma once

#include <juce_audio_formats/juce_audio_formats.h>
#include "Profiles.h"
#include "SoundPrint.h"

class ZeroEngineProcessor;

// Reference Match: on a background thread, fingerprints a dropped sound (its
// first note's spectrogram over time), then finds the engine settings whose
// render is closest. Every genre/type preset is tried at the reference's pitch;
// the closest few are refined in parallel, one per render engine, each by a
// wavetable pass and a pattern search over tone, filter, envelopes, space and
// LFO. The best wins, genre and type included. Renders through its own engines,
// so the plugin keeps playing. No machine learning.
class ReferenceMatch : private juce::Thread
{
public:
    // Engines to render through, one per parallel search; made on the message thread.
    explicit ReferenceMatch (std::vector<ZeroEngineProcessor*> renderEngines);
    ~ReferenceMatch() override;

    // Message thread. Cancels any match already running. genre/type: where the
    // plugin is now, kept when nothing elsewhere is clearly closer.
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
        float distance = 0.0f;      // SoundPrint::distance of the best render
    };
    Result take();

    // Tuning, for tooling: search renders light (FinishingChain::setLight), the
    // renders each worker's search gets, and CMA-ES instead of the pattern search.
    static inline bool lightSearch = true;
    static inline int searchBudget = 80;  // the first round, per worker
    static inline int secondBudget = 60;  // the second round (from the two best), per worker
    static inline bool useCmaes = false;
    static inline float closeBelow = 2.0f; // a distance under this reads VERY CLOSE (x1.75 CLOSE, x2.5 ROUGH): on matchbench,
                                           // matches within about 4 dB of their target scored 2.8 to 3.3
    static inline bool debugPrint = false;  // tooling: print the preset ranking and finalists

    // How far one candidate is from a reference, rendered and compared exactly as
    // a match does (for tooling: does the true answer score near zero?).
    static float score (ZeroEngineProcessor& engine, const SoundPrint& target, int genre, int type, const Profile& profile, bool light);

private:
    void run() override;

    std::vector<ZeroEngineProcessor*> engines;
    juce::File file;
    int genre = 0, type = 0;
    Profile from {};
    Result result;
    std::atomic<State> state { State::idle };
    std::atomic<float> progress { 0.0f };
};

// The render engines a match searches with: one per spare CPU core, up to six
// (more did not render faster on a 16-thread CPU).
// Make on the message thread; keep alive longer than the ReferenceMatch using them.
class MatchEngines
{
public:
    MatchEngines();
    ~MatchEngines();
    std::vector<ZeroEngineProcessor*> get() const;

private:
    std::vector<std::unique_ptr<ZeroEngineProcessor>> owned;
};
