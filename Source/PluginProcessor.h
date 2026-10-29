#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include "FinishingChain.h"
#include "MacroMap.h"
#include "SynthVoice.h"
#include "Profiles.h"
#include "Arpeggiator.h"
#include "ReferenceMatch.h"
#include "SoundTraits.h"
#include "TasteModel.h"
#include <optional>

class ZeroEngineProcessor : public juce::AudioProcessor,
                            private juce::Timer
{
public:
    ZeroEngineProcessor();

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    // Silence everything at once (voices, delay, reverb, compressors): offline
    // renders such as Reference Match start each candidate from scratch.
    void resetForRender();
    void setLightRender (bool on) { finishing.setLight (on); } // offline search renders: see FinishingChain::setLight
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override  { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 4.0; } // reverb and delay tails

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    const FinishingChain::Meters& getMeters() const noexcept { return finishing.getMeters(); }

    // Where a playing voice is in its wavetable (envelopes and LFOs included), or -1 when silent.
    float getLivePosition() const noexcept { return livePosition.load (std::memory_order_relaxed); }

    // MIDI learn for the six macros (message thread): learn takes the next CC.
    void learnMacro (int macro) noexcept { learningMacro = macro; }
    void clearMacroCC (int macro) noexcept { macroCC[(size_t) macro] = -1; }
    int getMacroCC (int macro) const noexcept { return macroCC[(size_t) macro].load(); }
    bool isLearning (int macro) const noexcept { return learningMacro.load() == macro; }

    // Voices currently sounding (diagnostics).
    int getActiveVoiceCount() const { int n = 0; for (int i = 0; i < synth.getNumVoices(); ++i) n += synth.getVoice (i)->isVoiceActive() ? 1 : 0; return n; }

    // The newest output samples, (L + R) / 2, for the display's waveform and
    // spectrum. The audio thread writes a ring buffer; a read may tear across a
    // block boundary, which is harmless for drawing.
    static constexpr int scopeSize = 8192;
    void copyScope (float* dest, int numSamples) const noexcept;

    // Message thread: loads the profile for the current Genre/Type if it changed.
    // Runs on a timer; offline tools call it directly after setting Genre/Type.
    void syncProfile();

    // Message thread: loads a given profile as if Genre/Type had been chosen
    // (the offline tuner passes candidate profiles through this).
    void loadProfile (int genre, int type, const Profile& profile);

    // Message thread: the Generate button. Loads a big, interesting variation on
    // the current Genre/Type's profile (see pickGeneratedProfile). anyGenreToo (the
    // mini tesseract): draws from every genre and blends of two instead, and
    // loads as the genre it mostly came from. Switching Genre or Type loads a
    // designed profile as usual.
    void generate (juce::int64 seed = juce::Time::currentTimeMillis(), bool anyGenreToo = false);

    // The heart (message thread): marks the sound playing now as liked, or
    // unmarks it. A heart is the strongest taste judgment (it beats the results
    // shown before it), and Generate breeds some candidates from hearted sounds
    // of the type. Saved in %APPDATA%/Zero Engine/Hearts.xml.
    void heartCurrent();
    bool isCurrentHearted() const noexcept { return currentHeart >= 0; }
    // The last 10 hearted sounds' names, newest first (the Favourites menu's LIKED
    // section), and recalling one exactly as it was hearted.
    juce::StringArray getRecentLikes();
    bool recallLike (const juce::String& name);
    int getTasteJudgments() const noexcept { return taste.judgments(); }
    TasteModel::Features getTasteWeights() const noexcept { return taste.meanWeights(); }

    // Tooling: keep favourites, taste and hearts under another folder (tests and
    // simulations must never touch the owner's), and pretend the last result
    // was played for some seconds before the next click.
    void setUserFolder (const juce::File& folder) { favouritesFolder = folder.getChildFile ("Favourites"); }
    void simulateListening (float seconds)
    {
        if (shown.empty()) return;
        shown.back().playedSeconds = seconds;
        shown.back().shownAt -= seconds > 0.0f ? seconds + 1.0 : 0.0;
    }
    std::optional<TasteModel::Features> getLastShownFeatures() const { return shown.empty() ? std::nullopt : std::optional { shown.back().features }; }

    // Message thread: favourites are whole sounds (every parameter) saved as
    // files, named "<Genre> <Type> <n>", so a generated sound is never lost.
    juce::String saveFavourite();            // the name it was saved under, or empty on failure
    // Quick save: the same, under a random two-word name ("Neon Viper"), never
    // one already used; the display shows the name for a few seconds.
    juce::String quickSave();
    juce::StringArray getFavourites() const; // newest first
    bool loadFavourite (const juce::String& name);
    bool deleteFavourite (const juce::String& name);
    juce::String renameFavourite (const juce::String& name, const juce::String& newName); // the new name, or empty if taken or invalid
    void setFavouritesFolder (const juce::File& folder) { favouritesFolder = folder; } // for tests

    // How many of the last Generate's candidates passed the check (out of 6), for tests and tuning.
    int getLastGenerateFits() const noexcept { return lastGenerateFits; }
    static inline bool debugGenerate = false; // tooling: print every candidate's scores
    static inline bool measureLight = true;    // Generate's check renders light (tooling can compare full quality)

    // Message thread: step back (-1) or forward (+1) through generated sounds.
    // Returns false at either end.
    bool stepHistory (int direction);
    bool canStepHistory (int direction) const { const auto t = historyPos + direction; return t >= 0 && t < (int) history.size(); }

    // Undo / redo (message thread): snapshots of the whole sound. The timer takes a
    // new step whenever the sound has changed and no mouse button is down, so a
    // drag, a menu choice, Generate or a Genre/Type change each make one step.
    bool undo();
    bool redo();
    bool canUndo() const noexcept { return ! undoStack.empty(); }
    bool canRedo() const noexcept { return ! redoStack.empty(); }

    // Reference Match (message thread): find the engine's closest version of a
    // dropped audio file, across every genre/type, in the background; the result
    // loads (genre and type included) as one undo step.
    void startMatch (const juce::File& reference);
    bool isMatching() const noexcept { return matcher != nullptr && matcher->getState() == ReferenceMatch::State::running; }
    juce::String getMatchStatus() const; // for the display; empty when there is nothing to say

    // The Sample wavetable bank (message or worker thread, never the audio
    // thread): built from measured harmonics, picked up by the voices on their
    // next block, saved with the project. Reference Match sets it.
    void setSampleHarmonics (const WavetableBank::Harmonics&);
    const std::optional<WavetableBank::Harmonics>& getSampleHarmonics() const noexcept { return sampleHarmonics; }
    const WavetableBank& bankFor (int table) const noexcept // the bank a table index plays from (for the display)
    {
        const auto* sample = sampleTable.load (std::memory_order_acquire);
        return table == WavetableBank::sampleBank && sample != nullptr ? *sample : WavetableBank::get();
    }

    // For engines used offline (Reference Match renders): no timer reloading profiles.
    void stopProfileSync() { stopTimer(); }
    // The tempo used when the host gives none (Reference Match renders at the reference's).
    void setFallbackTempo (float bpm) { fallbackBpm.store (juce::jlimit (40.0f, 300.0f, bpm)); }
    float getTempo() const noexcept { return hostBpm.load(); }

    juce::AudioProcessorValueTreeState state;
    juce::MidiKeyboardState keyboardState;

private:
    void timerCallback() override;
    bool restoreState (const juce::XmlElement&);
    void adoptCurrentGenreAndType();
    void checkpoint(); // record an undo step if the sound changed since the last one
    int generatePattern (juce::Random&, int genre); // a step pattern from the genre's grooves; returns its suggested rate
    void generateArp (int type, juce::Random&); // Generate's roll of the arp: on or off, its settings, a new pattern
    bool arpFromGenerate = false;                // the arp is on because Generate switched it on
    std::vector<juce::ValueTree> undoStack, redoStack;
    juce::ValueTree lastKnown;

    std::unique_ptr<MatchEngines> matchEngines;        // render for Reference Match (made on first use)
    std::unique_ptr<ReferenceMatch> matcher;           // after matchEngines, so it stops first
    juce::String matchSummary;
    double matchSummaryUntil = 0.0;
    void restoreTree (const juce::ValueTree&);

    std::vector<juce::ValueTree> history; // Generate history, oldest first (message thread)
    int historyPos = -1;

    // Generate's render-and-check (message thread): many variations measured on
    // private engines in parallel, see pickGeneratedProfile.
    using SoundCheck = SoundTraits;
    static SoundCheck measureProfile (ZeroEngineProcessor& engine, int genre, int type, const Profile&);
public:
    static SoundTraits measureForTools (ZeroEngineProcessor& engine, int genre, int type, const Profile& p) { return measureProfile (engine, genre, type, p); }
private:
    static constexpr int anyGenre = 12; // pickGeneratedProfile: draw from every genre
    std::pair<Profile, int> pickGeneratedProfile (int genre, int type, juce::Random&); // the profile and its leading genre
    std::vector<std::unique_ptr<ZeroEngineProcessor>> checkers; // private engines for the check renders, made on first use
    std::vector<SoundTraits> recentGenerated; // the last few results, so the next one differs

    // Taste: Generate results as shown, how long each was played, and what the
    // owner did next, becoming judgments for the TasteModel (see generate,
    // timerCallback, saveFavourite).
    struct Shown { TasteModel::Features features {}; double shownAt = 0.0; float playedSeconds = 0.0f; int label = 0; }; // label: 2 favourite, 1 kept, -1 skipped
    std::vector<Shown> shown;
    bool showingLatest = false; // the sound is still the last Generate result (not stepped back from, nor replaced)
    TasteModel taste;

    juce::File tasteFile() const { return favouritesFolder.getParentDirectory().getChildFile ("Taste.xml"); }
    void judgeLastShown();
    struct Heart { Profile profile {}; juce::ValueTree state; juce::String name; int genre = 0, type = 0; TasteModel::Features features {}; };
    juce::String randomName() const; // two random words not used by a favourite or a heart
    void saveHearts() const;
    std::vector<Heart> hearts;
    bool heartsLoaded = false;
    int currentHeart = -1; // which heart the sound playing is, -1 none
    std::optional<SoundTraits> lastPickTraits; // set by pickGeneratedProfile when its pick was measured
    juce::File heartsFile() const { return favouritesFolder.getParentDirectory().getChildFile ("Hearts.xml"); }
    void loadTasteAndHearts();
    int lastGenerateFits = 0;
    std::array<std::optional<SoundCheck>, 12 * 6> designedChecks; // genre * 6 + type

    juce::File favouritesFolder = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                                      .getChildFile ("Zero Engine").getChildFile ("Favourites");

    MacroMap macroMap;
    std::atomic<float> hostBpm { 120.0f }; // for tempo-synced LFOs; fallbackBpm when the host gives none
    std::atomic<float> fallbackBpm { 120.0f };
    std::atomic<float> livePosition { -1.0f }; // a playing voice's wavetable position, -1 when silent (display only)
    std::atomic<const WavetableBank*> sampleTable { nullptr }; // the voices' Sample bank, null until set
    std::vector<std::unique_ptr<WavetableBank>> sampleTables;   // the current one last; a few before it kept, as a voice may still read one
    std::optional<WavetableBank::Harmonics> sampleHarmonics;
    std::array<float, scopeSize> scope {};
    std::atomic<int> scopeWrite { 0 };
    ZeroSynth synth;
    Arpeggiator arp;
    std::array<std::atomic<juce::uint32>, 4> soundingNotes {}; // one bit per MIDI note
    std::atomic<float>* arpParams[5] {}; // on, mode, rate, octaves, gate
    std::atomic<float>* seqLengthParam = nullptr;
    std::atomic<float>* arpLatchParam = nullptr;
    std::array<std::atomic<int>, 6> macroCC { -1, -1, -1, -1, -1, -1 }; // learned CC per macro, -1 none
    std::atomic<int> learningMacro { -1 };
    juce::RangedAudioParameter* macroParams[6] {};
    std::atomic<float>* seqParams[3][Arpeggiator::maxSteps] {}; // on, pitch, accent per step

public:
    int getSequenceStep() const noexcept { return arp.getCurrentStep(); } // for the step grid
    // Notes the synth is being played (after the arp, so DAW MIDI and the
    // sequencer alike), for lighting the on-screen keys. Any thread.
    bool isNoteSounding (int note) const noexcept { return note >= 0 && note < 128 && (soundingNotes[(size_t) note >> 5].load (std::memory_order_relaxed) >> (note & 31) & 1u) != 0; }

private:
    std::atomic<int> appliedGenre { -1 }, appliedType { -1 };
    FinishingChain finishing;
    juce::AudioBuffer<float> synthBuffer; // L, R and the clean sub, sized in prepareToPlay

    std::atomic<float>& grit;
    std::atomic<float>& air;
    std::atomic<float>& width;
    std::atomic<float>& output;
    std::atomic<float>& levelTrim;
    std::atomic<float>& subSplit;
    std::atomic<float>& subOn;
    std::atomic<float>& midHighPass;
    float noteHighPassHz = 1000.0f; // high-pass ceiling from the lowest held note, smoothed
    std::atomic<float>& ottDepth;
    std::atomic<float>& spectral;
    std::atomic<float>& tilt;
    std::atomic<float>& monoFreq;
    std::atomic<float>& stereoWidth;
    std::atomic<float>& delayMix;
    std::atomic<float>& delayTime;
    std::atomic<float>& delayFeedback;
    std::atomic<float>& reverbMix;
    std::atomic<float>& reverbSize;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ZeroEngineProcessor)
};
