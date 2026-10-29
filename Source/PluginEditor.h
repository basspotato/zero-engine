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
    juce::Font getLabelFont (juce::Label&) override; // knob value boxes in the mono face
    juce::Font getAlertWindowTitleFont() override;
    juce::Font getAlertWindowMessageFont() override;
    juce::Font getAlertWindowFont() override;
    void drawAlertBox (juce::Graphics&, juce::AlertWindow&, const juce::Rectangle<int>& textArea, juce::TextLayout&) override;
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
    void setDropHint (bool show) { dropHint = show; repaint(); } // a file is being dragged over

private:
    void timerCallback() override;
    void drawWaveform (juce::Graphics&, juce::Rectangle<float>);
    void drawSpectrum (juce::Graphics&, juce::Rectangle<float>);
    void drawWavetable (juce::Graphics&, juce::Rectangle<float>);

    ZeroEngineProcessor& processor;
    int mode = wavetable;
    float shownPosition = -1.0f; // eased toward the live position
    float level = 0.0f;          // eased output level, for the trace glow
    bool dropHint = false;
    void drawMatchOverlay (juce::Graphics&, juce::Rectangle<float>);

    static constexpr int fftOrder = 12, fftSize = 1 << fftOrder;
    juce::dsp::FFT fft { fftOrder };
    juce::dsp::WindowingFunction<float> window { (size_t) fftSize, juce::dsp::WindowingFunction<float>::hann };
    std::vector<float> samples = std::vector<float> ((size_t) fftSize), fftData = std::vector<float> ((size_t) fftSize * 2);
    std::vector<float> spectrumDb = std::vector<float> ((size_t) fftSize / 2, -100.0f);
};

// The 16-step sequencer: per step a lit on/off pad, a pitch you drag up or down
// (double-click for 0) and an accent dot; steps past Seq Length are dimmed and
// the playing step is lit.
// The on-screen keyboard, lit by what the synth is actually playing: keys held
// (mouse, computer keys, DAW MIDI) in orange, notes the arp or sequencer adds in
// light blue.
class PlayingKeyboard : public juce::MidiKeyboardComponent
{
public:
    explicit PlayingKeyboard (ZeroEngineProcessor&);

private:
    void refreshLights();
    void drawWhiteNote (int note, juce::Graphics&, juce::Rectangle<float> area, bool isDown, bool isOver,
                        juce::Colour lineColour, juce::Colour textColour) override;
    void drawBlackNote (int note, juce::Graphics&, juce::Rectangle<float> area, bool isDown, bool isOver,
                        juce::Colour noteFillColour) override;

    ZeroEngineProcessor& processor;
    std::array<juce::uint32, 4> shown {}; // the sounding notes last painted
    juce::TimedCallback lights { [this] { refreshLights(); } }; // the base class has its own Timer
};

class SequencerGrid : public juce::Component,
                      private juce::Timer
{
public:
    SequencerGrid (juce::AudioProcessorValueTreeState&, std::function<int()> playingStep);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;

private:
    static constexpr int steps = 16;
    enum Row { padRow, pitchRow, accentRow, noRow };

    void timerCallback() override;
    juce::Rectangle<float> cell (int step, Row row) const;
    std::pair<int, Row> hit (juce::Point<float>) const;

    std::function<int()> playingStep;
    std::unique_ptr<juce::ParameterAttachment> onAttachments[steps], pitchAttachments[steps], accentAttachments[steps], lengthAttachment;
    bool on[steps] {}, accent[steps] {};
    int pitch[steps] {}, length = steps, lit = -1;
    int dragStep = -1, dragStartPitch = 0;
};

// Every engine parameter in the console style: knobs for numbers, slim menus
// for choices, toggles for switches. Three pages, each with bordered sections
// arranged in rows (small sections share a row). Covers the panels between the
// top bar and the keyboard while open.
class AdvancedPanel : public juce::Component
{
public:
    AdvancedPanel (juce::AudioProcessorValueTreeState&, std::function<int()> playingStep);
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct Control
    {
        juce::String name;
        std::unique_ptr<juce::Slider> knob;
        std::unique_ptr<juce::ComboBox> menu;
        std::unique_ptr<juce::ToggleButton> toggle;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> knobAttachment;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> menuAttachment;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> toggleAttachment;
        std::unique_ptr<juce::ParameterAttachment> stepAttachment; // a menu over a small integer range
        juce::Component* widget() const;
    };

    struct Section
    {
        juce::String name;
        std::vector<std::unique_ptr<Control>> controls;
        juce::Rectangle<int> bounds; // set in resized()
        juce::Component* custom = nullptr; // a section drawn by its own component (the step grid)
    };

    struct Page
    {
        juce::String name;
        std::vector<std::vector<Section>> rows; // sections side by side within a row
        std::unique_ptr<juce::TextButton> button;
    };

    void show (size_t page);

    std::vector<Page> pages;
    size_t current = 0;
    std::unique_ptr<SequencerGrid> grid;
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

// The heart beside the mini tesseract: an outline, filled when the sound
// playing is hearted. Generate learns from hearts and breeds from them.
class HeartButton : public juce::Button
{
public:
    HeartButton() : juce::Button ("Heart") {}
    void setFilled (bool shouldFill) { if (filled != shouldFill) { filled = shouldFill; repaint(); } }
    void paintButton (juce::Graphics&, bool highlighted, bool down) override;

private:
    bool filled = false;
};

// The thumbs down beside the heart: "less like this"; Generate learns from it
// and moves on to the next sound.
class DislikeButton : public juce::Button
{
public:
    DislikeButton() : juce::Button ("Thumbs down") {}
    void paintButton (juce::Graphics&, bool highlighted, bool down) override;
};

// The Generate button: a slowly turning tesseract (4D cube) that folds in on
// itself, turning inside out through the fourth dimension, when clicked.
class TesseractButton : public juce::Button,
                        private juce::Timer
{
public:
    // mini: the small one in the display corner (Generate from any genre): no
    // screen of its own, no hint.
    explicit TesseractButton (bool mini = false);

    // Starts the fold animation.
    void fold();

    // Freezes the animation at a point of the fold (0..1), for preview images.
    void showFoldAt (float progress) { foldStart = -1.0; foldProgress = progress; repaint(); }
    // Shows the gravity waves a click sends out, some seconds after it (previews).
    void showWavesAt (double secondsAfterClick) { waves = { { 0.4, 0.7f }, { 0.62, 2.1f }, { 0.84, 4.0f } }; spaceTime = secondsAfterClick; foldStart = -1.0; foldProgress = 0.0f; repaint(); }

    void paintButton (juce::Graphics&, bool highlighted, bool down) override;

private:
    void timerCallback() override;

    float angle = 0.0f;       // idle rotation, radians
    float colourPhase = 0.0f; // the colour's drift: advances at the resting pace only
    double foldStart = -1.0;  // ms timestamp; negative when not folding
    float foldProgress = 0.0f; // 0..1 while folding
    // Space behind it: stars (position 0..1, size, twinkle rate and phase), and
    // the time they twinkle and the black hole turns by (seconds).
    struct Star { float x, y, size, rate, phase; };
    std::vector<Star> stars;
    double spaceTime = 0.0;
    struct Node { float radius, angle, size; juce::Colour colour; bool ray; float phase; }; // the burst at the centre (radius 0..1)
    std::vector<Node> nodes;
    struct Wave { double start; float phase; }; // a gravity wave from a collapse: when it starts (spaceTime) and how it wobbles
    std::vector<Wave> waves;
    void paintSpace (juce::Graphics&, juce::Rectangle<float> screen);
    bool mini = false;
};

class ZeroEngineEditor : public juce::AudioProcessorEditor,
                         public juce::FileDragAndDropTarget,
                         private juce::Timer
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
        juce::String name; // the label without its CC
    };

    ZeroLookAndFeel lookAndFeel; // first, so it outlives every component using it
    ZeroEngineProcessor& processor;

    std::array<Macro, 6> macros;
    Macro outputKnob;
    std::array<Macro, 4> envelope; // attack, decay, sustain, release on the main page
    juce::ComboBox genreBox, typeBox;
    std::unique_ptr<ComboAttachment> genreAttachment, typeAttachment;
    ScopeView display;
    juce::ComboBox displayMode; // in the DISPLAY header strip
    LevelMeter levelMeter;
    TesseractButton generateButton;
    juce::TextButton moodButton { "MOOD" }, genreModeButton { "GENRE" }; // under the tesseract: what Generate draws from (lit = active)
    void refreshGenerateMode();
    HeartButton heartButton; // right of GENRE: like the sound playing
    DislikeButton dislikeButton; // beside the heart: less like this, and the next sound
    juce::TextButton mutateButton { "MUTATE" }; // left of MOOD: small variations on the sound playing
    juce::ComboBox favouritesBox;            // recall a saved sound
    juce::String currentFavourite;           // the last one saved or recalled, which Rename and Delete act on
    juce::TextButton saveButton { "SAVE" };  // save the current sound as a favourite
    ChevronButton backButton { true }, forwardButton { false }; // Generate history, in the header strip
    ChevronButton genrePrev { true }, genreNext { false }, typePrev { true }, typeNext { false }; // step through the menus
    void stepChoice (const char* paramID, int direction);

    // Every engine parameter, in tabs; ADVANCED opens it over the panels.
    AdvancedPanel advancedPanel;
    juce::TextButton mainButton { "MAIN" }, advancedButton { "ADVANCED" }; // lit = the view showing
    juce::TextButton quickSaveButton { "QUICK SAVE" }; // the current sound into Favourites under a random name
    juce::TextButton undoButton { "UNDO" }, redoButton { "REDO" };
    bool keyPressed (const juce::KeyPress&) override; // Ctrl+Z, Ctrl+Y / Ctrl+Shift+Z

    // Reference Match: drop an audio file anywhere, or MATCH in the display header.
    bool isInterestedInFileDrag (const juce::StringArray& files) override;
    void fileDragEnter (const juce::StringArray&, int, int) override;
    void fileDragExit (const juce::StringArray&) override;
    void filesDropped (const juce::StringArray& files, int, int) override;
    juce::TextButton matchButton { "MATCH" };
    std::unique_ptr<juce::FileChooser> matchChooser;
    void showAdvanced (bool show);
    PlayingKeyboard keyboard;

    void setUpMacro (Macro&, const char* paramID, const juce::String& name);
    void refreshFavourites();
    void refreshHistoryButtons();
    void mouseDown (const juce::MouseEvent&) override; // right-click a macro: MIDI learn
    void timerCallback() override;                     // keeps the macro labels' CC text current
    static constexpr int designWidth = 960, designHeight = 760;
    float uiScale() const { return (float) getWidth() / (float) designWidth; }
    void renameCurrentFavourite();
    juce::Component::SafePointer<juce::AlertWindow> renameDialog;
    static constexpr int renameId = 10001, deleteId = 10002; // Favourites menu actions
    static constexpr int likeIdBase = 20001;                  // the LIKED section's items
    juce::StringArray recentLikes;                            // as listed in it

    // Panel areas, set in resized() and drawn in paint() behind the components.
    juce::Rectangle<int> displaySection, meterSection, generateSection, favouritesSection, macroSection;
    juce::TooltipWindow tooltips { this };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ZeroEngineEditor)
};
