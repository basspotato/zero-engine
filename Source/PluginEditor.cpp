#include "PluginEditor.h"
#include "Parameters.h"

namespace Palette
{
    const juce::Colour background { 0xff121418 };
    const juce::Colour panel      { 0xff1c1f26 };
    const juce::Colour accent     { 0xff36d6c3 };
    const juce::Colour text       { 0xffd8dde6 };
    const juce::Colour warn       { 0xffffa94d };
    const juce::Colour track      { 0xff2a2e38 };
}

//==============================================================================
DisplayPanel::DisplayPanel (const FinishingChain::Meters& m)
    : meters (m)
{
    startTimerHz (30);
}

void DisplayPanel::timerCallback()
{
    auto ease = [] (float& v, float target) { v += 0.25f * (target - v); };

    ease (drive, meters.driveDb.load());
    for (int b = 0; b < 3; ++b)
        ease (bands[b], meters.bandGainDb[b].load());
    ease (tilt, meters.tiltDb.load());
    ease (air, meters.airDb.load());
    ease (monoFreq, meters.monoFreq.load());
    ease (width, meters.width.load());
    ease (rms, meters.rmsDb.load());
    ease (momentary, meters.momentaryLufs.load());
    shortTerm = meters.shortTermLufs.load(); // already a 3 s average

    // Fast attack, slow fall, so short peaks stay readable.
    clip = juce::jmax (meters.clipDb.load(), clip * 0.9f);
    peak = juce::jmax (meters.peakDb.load(), peak - 1.0f);

    if (isShowing())
        repaint();
}

void DisplayPanel::paint (juce::Graphics& g)
{
    using juce::String;

    g.setColour (Palette::panel);
    g.fillRoundedRectangle (getLocalBounds().toFloat(), 8.0f);

    auto area = getLocalBounds().reduced (14, 12);
    const auto columnWidth = area.getWidth() / 6;

    // Each column: title, big value, meter, caption.
    auto column = [&] (const String& title, const String& value, const String& caption, auto&& drawMeter)
    {
        auto c = area.removeFromLeft (columnWidth).reduced (6, 0);

        g.setColour (Palette::accent.withAlpha (0.8f));
        g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
        g.drawText (title, c.removeFromTop (16), juce::Justification::centredLeft);

        g.setColour (Palette::text);
        g.setFont (juce::FontOptions (17.0f));
        g.drawText (value, c.removeFromTop (24), juce::Justification::centredLeft);

        g.setColour (Palette::text.withAlpha (0.6f));
        g.setFont (juce::FontOptions (12.0f));
        g.drawFittedText (caption, c.removeFromBottom (32), juce::Justification::topLeft, 2);

        c.removeFromBottom (6);
        drawMeter (c.reduced (0, 4).toFloat());
    };

    auto hBar = [&] (juce::Rectangle<float> r, float fraction, juce::Colour colour)
    {
        auto bar = r.withSizeKeepingCentre (r.getWidth(), 8.0f);
        g.setColour (Palette::track);
        g.fillRoundedRectangle (bar, 4.0f);
        g.setColour (colour);
        g.fillRoundedRectangle (bar.withWidth (bar.getWidth() * juce::jlimit (0.0f, 1.0f, fraction)), 4.0f);
    };

    // Saturation.
    column ("GRIT", "+" + String (drive, 1) + " dB",
            drive < 3.0f ? "Clean, barely driven" : drive < 12.0f ? "Warm saturation" : "Heavy drive",
            [&] (juce::Rectangle<float> r) { hBar (r, drive / 21.6f, Palette::accent); });

    // 3-band upward/downward compression.
    {
        int loudest = 0;
        for (int b = 1; b < 3; ++b)
            if (std::abs (bands[b]) > std::abs (bands[loudest]))
                loudest = b;

        const char* names[] { "Lows", "Mids", "Highs" };
        const auto amount = bands[loudest];
        const auto caption = std::abs (amount) < 0.5f ? String ("Barely working")
                           : String (names[loudest]) + (amount > 0 ? " lifted " : " tamed ") + String (std::abs (amount), 1) + " dB";

        column ("OTT", String (bands[0], 1) + " / " + String (bands[1], 1) + " / " + String (bands[2], 1), caption,
                [&] (juce::Rectangle<float> r)
                {
                    const auto barWidth = r.getWidth() / 3.0f;
                    const auto centreY = r.getCentreY();
                    for (int b = 0; b < 3; ++b)
                    {
                        auto slot = r.withX (r.getX() + barWidth * (float) b).withWidth (barWidth).reduced (6.0f, 0.0f);
                        g.setColour (Palette::track);
                        g.fillRoundedRectangle (slot, 3.0f);

                        const auto h = juce::jlimit (0.0f, 1.0f, std::abs (bands[b]) / 12.0f) * slot.getHeight() * 0.5f;
                        g.setColour (bands[b] > 0 ? Palette::accent : Palette::warn);
                        g.fillRect (slot.withHeight (h).withY (bands[b] > 0 ? centreY - h : centreY));
                    }
                    g.setColour (Palette::text.withAlpha (0.3f));
                    g.drawHorizontalLine ((int) centreY, r.getX(), r.getRight());
                });
    }

    // Tilt and air EQ, drawn as a rough response curve.
    column ("TONE", "Air +" + String (air, 1) + " dB",
            air > 0.5f ? "Highs opened above 8 kHz"
                       : tilt < -0.5f ? "Tilted darker" : tilt > 0.5f ? "Tilted brighter" : "Flat",
            [&] (juce::Rectangle<float> r)
            {
                g.setColour (Palette::track);
                g.fillRoundedRectangle (r, 3.0f);

                juce::Path curve;
                for (int i = 0; i <= 40; ++i)
                {
                    const auto t = (float) i / 40.0f;
                    const auto octavesFrom1k = (t - 0.55f) * 10.0f;   // ~20 Hz to 20 kHz
                    const auto octavesFrom8k = octavesFrom1k - 3.0f;
                    const auto dB = -tilt + 2.0f * tilt / (1.0f + std::exp (-octavesFrom1k * 1.5f))
                                  + air / (1.0f + std::exp (-octavesFrom8k * 2.0f));
                    const auto y = r.getCentreY() - juce::jlimit (-1.0f, 1.0f, dB / 10.0f) * r.getHeight() * 0.45f;
                    const auto x = r.getX() + t * r.getWidth();
                    if (i == 0) curve.startNewSubPath (x, y); else curve.lineTo (x, y);
                }
                g.setColour (Palette::accent);
                g.strokePath (curve, juce::PathStrokeType (2.0f));
            });

    // Mono bass and width.
    column ("WIDTH", String (juce::roundToInt (width * 100.0f)) + "%",
            "Mono below " + String (juce::roundToInt (monoFreq)) + " Hz",
            [&] (juce::Rectangle<float> r) { hBar (r, width, Palette::accent); });

    // Soft clipper.
    column ("CLIP", String (clip, 1) + " dB",
            clip < 0.1f ? "Clean" : clip < 2.0f ? "Shaving peaks" : "Working hard",
            [&] (juce::Rectangle<float> r) { hBar (r, clip / 6.0f, clip < 2.0f ? Palette::accent : Palette::warn); });

    // Output level: short-term LUFS as the headline, momentary LUFS on the bar.
    auto level = [] (float dB) { return dB > -99.0f ? String (dB, 1) : String ("-inf"); };
    column ("OUT", level (shortTerm) + " LUFS",
            "RMS " + level (rms) + ", peak " + level (peak) + " dB",
            [&] (juce::Rectangle<float> r)
            {
                auto meter = r.withSizeKeepingCentre (14.0f, r.getHeight());
                g.setColour (Palette::track);
                g.fillRoundedRectangle (meter, 3.0f);

                auto toFraction = [] (float dB) { return juce::jlimit (0.0f, 1.0f, (dB + 36.0f) / 36.0f); };
                const auto h = toFraction (momentary) * meter.getHeight();
                g.setColour (peak > -0.5f ? Palette::warn : Palette::accent);
                g.fillRoundedRectangle (meter.withTop (meter.getBottom() - h), 3.0f);

                const auto peakY = meter.getBottom() - toFraction (peak) * meter.getHeight();
                g.setColour (Palette::text);
                g.drawHorizontalLine ((int) peakY, meter.getX() - 3.0f, meter.getRight() + 3.0f);
            });
}

//==============================================================================
ZeroEngineEditor::ZeroEngineEditor (ZeroEngineProcessor& p)
    : AudioProcessorEditor (p),
      processor (p),
      display (p.getMeters()),
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
