#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include "FinishingChain.h"
#include "MacroMap.h"

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
    double getTailLengthSeconds() const override { return 0.5; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    const FinishingChain::Meters& getMeters() const noexcept { return finishing.getMeters(); }

    // Message thread: loads the profile for the current Genre/Type if it changed.
    // Runs on a timer; offline tools call it directly after setting Genre/Type.
    void syncProfile();

    juce::AudioProcessorValueTreeState state;
    juce::MidiKeyboardState keyboardState;

private:
    void timerCallback() override;

    MacroMap macroMap;
    juce::Synthesiser synth;
    std::atomic<int> appliedGenre { -1 }, appliedType { -1 };
    FinishingChain finishing;

    std::atomic<float>& grit;
    std::atomic<float>& air;
    std::atomic<float>& width;
    std::atomic<float>& output;
    std::atomic<float>& ottDepth;
    std::atomic<float>& tilt;
    std::atomic<float>& monoFreq;
    std::atomic<float>& stereoWidth;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ZeroEngineProcessor)
};
