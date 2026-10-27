#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include "FinishingChain.h"
#include "MacroMap.h"
#include "SynthVoice.h"
#include "Profiles.h"

class ZeroEngineProcessor : public juce::AudioProcessor,
                            private juce::Timer
{
public:
    ZeroEngineProcessor();

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
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

    // Message thread: the Generate button. Loads a random variation on the
    // current Genre/Type's profile. Switching Genre or Type loads a designed profile as usual.
    void generate (juce::int64 seed = juce::Time::currentTimeMillis());

    juce::AudioProcessorValueTreeState state;
    juce::MidiKeyboardState keyboardState;

private:
    void timerCallback() override;

    MacroMap macroMap;
    std::atomic<float> hostBpm { 120.0f }; // for tempo-synced LFOs; 120 when the host gives none
    std::atomic<float> livePosition { -1.0f }; // a playing voice's wavetable position, -1 when silent (display only)
    std::array<float, scopeSize> scope {};
    std::atomic<int> scopeWrite { 0 };
    ZeroSynth synth;
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
