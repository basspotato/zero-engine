#pragma once

#include "PluginProcessor.h"

// Placeholder for the main display (profile map / target-vs-current meters later).
class DisplayPanel : public juce::Component
{
public:
    void paint (juce::Graphics& g) override;
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
