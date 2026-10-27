#pragma once

#include "PluginProcessor.h"

// "Why it's finished": one column per finishing stage, in chain order, each
// with a live meter and a plain-words caption.
class DisplayPanel : public juce::Component,
                     private juce::Timer
{
public:
    explicit DisplayPanel (const FinishingChain::Meters& meters);
    void paint (juce::Graphics& g) override;

private:
    void timerCallback() override;

    const FinishingChain::Meters& meters;

    // Smoothed copies of the meters, for drawing.
    float drive = 0, bands[3] {}, tilt = 0, air = 0, monoFreq = 120, width = 0, clip = 0, rms = -100, peak = -100;
    float momentary = -100, shortTerm = -100;
};

class ZeroEngineEditor : public juce::AudioProcessorEditor
{
public:
    explicit ZeroEngineEditor (ZeroEngineProcessor&);
    ~ZeroEngineEditor() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboAttachment  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    struct Macro
    {
        juce::Slider knob { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox };
        juce::Label label;
        std::unique_ptr<SliderAttachment> attachment;
    };

    ZeroEngineProcessor& processor;

    std::array<Macro, 6> macros;
    Macro outputKnob;
    juce::ComboBox genreBox, typeBox;
    std::unique_ptr<ComboAttachment> genreAttachment, typeAttachment;
    DisplayPanel display;

    // Temporary: all engine parameters as plain sliders, shown in place of the display.
    juce::GenericAudioProcessorEditor devPanel;
    juce::TextButton devButton { "Dev" };
    juce::MidiKeyboardComponent keyboard;

    void setUpMacro (Macro&, const char* paramID, const juce::String& name);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ZeroEngineEditor)
};
