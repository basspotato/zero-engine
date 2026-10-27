#pragma once

#include "PluginProcessor.h"
#include <juce_dsp/juce_dsp.h>

// Glossy black knobs with orange-to-yellow value rings; indigo panels with
// light-blue text and screens.
class ZeroLookAndFeel : public juce::LookAndFeel_V4
{
public:
    ZeroLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height, float position,
                           float startAngle, float endAngle, juce::Slider&) override;

    // Slim, flat dropdowns with a small chevron.
    void drawComboBox (juce::Graphics&, int width, int height, bool isButtonDown,
                       int buttonX, int buttonY, int buttonW, int buttonH, juce::ComboBox&) override;
    void positionComboBoxText (juce::ComboBox&, juce::Label&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
    juce::Font getPopupMenuFont() override;
    juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override;
    void drawPopupMenuBackground (juce::Graphics&, int width, int height) override;
};

// Output meter: L/R peak and RMS bars on a dB scale with peak hold and clip
// lights (click to reset), plus loudness, clipper and stereo correlation readouts.
class LevelMeter : public juce::Component,
                   private juce::Timer
{
public:
    explicit LevelMeter (const FinishingChain::Meters& meters);
    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent&) override;

private:
    void timerCallback() override;

    const FinishingChain::Meters& meters;

    float peak[2] { -100, -100 }, rms[2] { -100, -100 };
    float hold[2] { -100, -100 }, holdMs[2] {};   // peak hold and how long it has held
    float maxPeak = -100;                          // highest peak since reset
    bool clipped[2] {};
    float momentary = -100, shortTerm = -100, clipDb = 0, correlation = 1;
};

// The display window: the output as a waveform or spectrum, or the current
// wavetable following the playing position live. The mode menu sits in the
// panel's header strip (owned by the editor).
class ScopeView : public juce::Component,
                  private juce::Timer
{
public:
    enum Mode { waveform = 1, spectrum, wavetable };

    explicit ScopeView (ZeroEngineProcessor&);
    void paint (juce::Graphics& g) override;
    void setMode (int newMode) { mode = newMode; repaint(); }

private:
    void timerCallback() override;
    void drawWaveform (juce::Graphics&, juce::Rectangle<float>);
    void drawSpectrum (juce::Graphics&, juce::Rectangle<float>);
    void drawWavetable (juce::Graphics&, juce::Rectangle<float>);

    ZeroEngineProcessor& processor;
    int mode = wavetable;
    float shownPosition = -1.0f; // eased toward the live position
    float level = 0.0f;          // eased output level, for the trace glow

    static constexpr int fftOrder = 12, fftSize = 1 << fftOrder;
    juce::dsp::FFT fft { fftOrder };
    juce::dsp::WindowingFunction<float> window { (size_t) fftSize, juce::dsp::WindowingFunction<float>::hann };
    std::vector<float> samples = std::vector<float> ((size_t) fftSize), fftData = std::vector<float> ((size_t) fftSize * 2);
    std::vector<float> spectrumDb = std::vector<float> ((size_t) fftSize / 2, -100.0f);
};

// A small chevron button for stepping through Generate's history.
class ChevronButton : public juce::Button
{
public:
    explicit ChevronButton (bool pointsLeft) : juce::Button (pointsLeft ? "Back" : "Forward"), left (pointsLeft) {}
    void paintButton (juce::Graphics&, bool highlighted, bool down) override;

private:
    bool left;
};

// The Generate button: a slowly turning tesseract (4D cube) that folds in on
// itself, turning inside out through the fourth dimension, when clicked.
class TesseractButton : public juce::Button,
                        private juce::Timer
{
public:
    TesseractButton();

    // Starts the fold animation.
    void fold();

    // Freezes the animation at a point of the fold (0..1), for preview images.
    void showFoldAt (float progress) { foldStart = -1.0; foldProgress = progress; repaint(); }

    void paintButton (juce::Graphics&, bool highlighted, bool down) override;

private:
    void timerCallback() override;

    float angle = 0.0f;       // idle rotation, radians
    double foldStart = -1.0;  // ms timestamp; negative when not folding
    float foldProgress = 0.0f; // 0..1 while folding
};

class ZeroEngineEditor : public juce::AudioProcessorEditor
{
public:
    explicit ZeroEngineEditor (ZeroEngineProcessor&);
    ~ZeroEngineEditor() override;

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

    ZeroLookAndFeel lookAndFeel; // first, so it outlives every component using it
    ZeroEngineProcessor& processor;

    std::array<Macro, 6> macros;
    Macro outputKnob;
    juce::ComboBox genreBox, typeBox;
    std::unique_ptr<ComboAttachment> genreAttachment, typeAttachment;
    ScopeView display;
    juce::ComboBox displayMode; // in the DISPLAY header strip
    LevelMeter levelMeter;
    TesseractButton generateButton;
    juce::ComboBox favouritesBox;            // recall a saved sound
    juce::TextButton saveButton { "SAVE" };  // save the current sound as a favourite
    ChevronButton backButton { true }, forwardButton { false }; // Generate history, in the header strip

    // Temporary: all engine parameters as plain sliders, shown in place of the display.
    juce::GenericAudioProcessorEditor devPanel;
    juce::TextButton devButton { "Dev" };
    juce::MidiKeyboardComponent keyboard;

    void setUpMacro (Macro&, const char* paramID, const juce::String& name);
    void refreshFavourites();
    void refreshHistoryButtons();

    // Panel areas, set in resized() and drawn in paint() behind the components.
    juce::Rectangle<int> displaySection, meterSection, generateSection, macroSection;
    juce::TooltipWindow tooltips { this };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ZeroEngineEditor)
};
