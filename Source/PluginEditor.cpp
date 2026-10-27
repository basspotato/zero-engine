#include "PluginEditor.h"
#include "Parameters.h"

namespace Palette
{
    const juce::Colour background { 0xff121418 };
    const juce::Colour panel      { 0xff1c1f26 };
    const juce::Colour accent     { 0xff36d6c3 };
    const juce::Colour text       { 0xffd8dde6 };
}

void DisplayPanel::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    g.setColour (Palette::panel);
    g.fillRoundedRectangle (bounds, 8.0f);
    g.setColour (Palette::text.withAlpha (0.4f));
    g.setFont (juce::FontOptions (14.0f));
    g.drawFittedText ("Display", getLocalBounds(), juce::Justification::centred, 1);
}

ZeroEngineEditor::ZeroEngineEditor (ZeroEngineProcessor& p)
    : AudioProcessorEditor (p),
      processor (p),
      devPanel (p),
      keyboard (p.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    const std::array<std::pair<const char*, const char*>, 6> macroParams { {
        { ParamIDs::weight, "Weight" }, { ParamIDs::grit,   "Grit" },   { ParamIDs::punch, "Punch" },
        { ParamIDs::width,  "Width" },  { ParamIDs::motion, "Motion" }, { ParamIDs::air,   "Air" } } };

    for (size_t i = 0; i < macros.size(); ++i)
        setUpMacro (macros[i], macroParams[i].first, macroParams[i].second);

    setUpMacro (outputKnob, ParamIDs::output, "Output");

    genreBox.addItemList (genreNames, 1);
    typeBox.addItemList (typeNames, 1);
    addAndMakeVisible (genreBox);
    addAndMakeVisible (typeBox);
    genreAttachment = std::make_unique<ComboAttachment> (processor.state, ParamIDs::genre, genreBox);
    typeAttachment  = std::make_unique<ComboAttachment> (processor.state, ParamIDs::type, typeBox);

    addAndMakeVisible (display);
    addChildComponent (devPanel);
    addAndMakeVisible (keyboard);

    devButton.setClickingTogglesState (true);
    devButton.onClick = [this]
    {
        const auto showDev = devButton.getToggleState();
        devPanel.setVisible (showDev);
        display.setVisible (! showDev);
    };
    addAndMakeVisible (devButton);

    setSize (760, 520);
}

void ZeroEngineEditor::setUpMacro (Macro& macro, const char* paramID, const juce::String& name)
{
    macro.knob.setColour (juce::Slider::rotarySliderFillColourId, Palette::accent);
    macro.label.setText (name, juce::dontSendNotification);
    macro.label.setJustificationType (juce::Justification::centred);
    macro.label.setColour (juce::Label::textColourId, Palette::text);
    addAndMakeVisible (macro.knob);
    addAndMakeVisible (macro.label);
    macro.attachment = std::make_unique<SliderAttachment> (processor.state, paramID, macro.knob);
}

void ZeroEngineEditor::paint (juce::Graphics& g)
{
    g.fillAll (Palette::background);
    g.setColour (Palette::text);
    g.setFont (juce::FontOptions (20.0f, juce::Font::bold));
    g.drawText ("ZERO ENGINE", 20, 14, 200, 30, juce::Justification::centredLeft);
}

void ZeroEngineEditor::resized()
{
    auto area = getLocalBounds().reduced (20);

    auto top = area.removeFromTop (36);
    top.removeFromLeft (200);
    auto outputArea = top.removeFromRight (90);
    outputKnob.label.setBounds (outputArea.removeFromLeft (50));
    outputKnob.knob.setBounds (outputArea);
    top.removeFromRight (12);
    devButton.setBounds (top.removeFromRight (50).reduced (0, 4));
    top.removeFromRight (8);
    typeBox.setBounds (top.removeFromRight (130).reduced (0, 4));
    top.removeFromRight (8);
    genreBox.setBounds (top.removeFromRight (150).reduced (0, 4));

    area.removeFromTop (12);
    keyboard.setBounds (area.removeFromBottom (70));
    area.removeFromBottom (12);

    auto knobRow = area.removeFromBottom (120);
    const auto knobWidth = knobRow.getWidth() / (int) macros.size();
    for (auto& macro : macros)
    {
        auto cell = knobRow.removeFromLeft (knobWidth);
        macro.label.setBounds (cell.removeFromBottom (20));
        macro.knob.setBounds (cell.reduced (6));
    }

    area.removeFromBottom (12);
    display.setBounds (area);
    devPanel.setBounds (area);
}
