#include "PluginEditor.h"
#include "Parameters.h"
#include "Wavetable.h"
#include "BinaryData.h"
#include <numeric>
#if ZERO_SPACE_IMAGES
 #include "SpaceImages.h"
#endif

namespace Palette
{
    // Flat, muted slate-navy hardware with a hint of purple; light-blue LEDs and
    // screens; orange-to-yellow rings.
    const juce::Colour backgroundTop    { 0xff181a2b };
    const juce::Colour backgroundBottom { 0xff1c1a2e };
    const juce::Colour panel      { 0xff23263a };
    const juce::Colour panelEdge  { 0xff34384f };
    const juce::Colour header     { 0xff2b2f47 };
    const juce::Colour screen     { 0xff0b0c15 };
    const juce::Colour accent     { 0xff72c9e6 }; // light blue
    const juce::Colour orange     { 0xffff7417 };
    const juce::Colour yellow     { 0xffffc531 };
    const juce::Colour magenta    { 0xfff736aa }; // the pink-magenta the tesseract passes through between blue and orange
    const juce::Colour text       { 0xffdfe4ff };
    const juce::Colour warn       { 0xffff9a2e };
    const juce::Colour track      { 0xff141621 };
    const juce::Colour clip       { 0xffff4a4a };
}

// Potato Kicker's type: Barlow Condensed for labels, Share Tech Mono for numbers.
namespace Fonts
{
    struct Faces
    {
        juce::Typeface::Ptr semi = juce::Typeface::createSystemTypefaceFor (BinaryData::BarlowCondensedSemiBold_ttf, BinaryData::BarlowCondensedSemiBold_ttfSize);
        juce::Typeface::Ptr bold = juce::Typeface::createSystemTypefaceFor (BinaryData::BarlowCondensedBold_ttf, BinaryData::BarlowCondensedBold_ttfSize);
        juce::Typeface::Ptr mono = juce::Typeface::createSystemTypefaceFor (BinaryData::ShareTechMonoRegular_ttf, BinaryData::ShareTechMonoRegular_ttfSize);
    };

    const Faces& faces()
    {
        static Faces f;
        return f;
    }

    // Condensed faces read small, so heights run a little larger than the old sans.
    juce::FontOptions ui (float height, bool bold = false, float kerning = 0.04f)
    {
        return juce::FontOptions().withTypeface (bold ? faces().bold : faces().semi).withHeight (height * 1.2f).withKerningFactor (kerning);
    }

    juce::FontOptions mono (float height)
    {
        return juce::FontOptions().withTypeface (faces().mono).withHeight (height * 1.1f);
    }
}

namespace
{
    // A dark recessed screen with a faint light-blue glass edge, for the display,
    // meter and generate panels.
    void fillScreen (juce::Graphics& g, juce::Rectangle<float> r)
    {
        g.setGradientFill (juce::ColourGradient (Palette::screen.brighter (0.12f), 0.0f, r.getY(),
                                                 Palette::screen, 0.0f, r.getBottom(), false));
        g.fillRoundedRectangle (r, 5.0f);
        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.drawRoundedRectangle (r.reduced (0.5f), 5.0f, 1.0f);
        g.setColour (Palette::accent.withAlpha (0.12f));
        g.drawRoundedRectangle (r.reduced (1.5f), 4.0f, 1.0f);
    }
}

//==============================================================================
ZeroLookAndFeel::ZeroLookAndFeel()
{
    setColour (juce::ComboBox::backgroundColourId, Palette::screen);
    setColour (juce::ComboBox::outlineColourId, Palette::panelEdge);
    setColour (juce::ComboBox::textColourId, Palette::accent);
    setColour (juce::ComboBox::arrowColourId, Palette::accent);
    setColour (juce::PopupMenu::backgroundColourId, Palette::panel);
    setColour (juce::PopupMenu::textColourId, Palette::text);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, Palette::header);
    setColour (juce::PopupMenu::highlightedTextColourId, Palette::yellow);
    setColour (juce::TextButton::buttonColourId, Palette::screen);
    setColour (juce::TextButton::buttonOnColourId, Palette::header);
    setColour (juce::TextButton::textColourOffId, Palette::accent);
    setColour (juce::TextButton::textColourOnId, Palette::yellow);
    setColour (juce::Label::textColourId, Palette::accent);
    setColour (juce::TooltipWindow::backgroundColourId, Palette::panel);
    setColour (juce::TooltipWindow::textColourId, Palette::text);
    setColour (juce::Slider::textBoxTextColourId, Palette::text.withAlpha (0.8f));
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxHighlightColourId, Palette::orange.withAlpha (0.4f));
    setColour (juce::ToggleButton::textColourId, Palette::accent);
    setColour (juce::ToggleButton::tickColourId, Palette::yellow);
    setColour (juce::ToggleButton::tickDisabledColourId, Palette::text.withAlpha (0.3f));
    setColour (juce::TextEditor::backgroundColourId, Palette::screen);
    setColour (juce::TextEditor::textColourId, Palette::text);
    setColour (juce::TextEditor::outlineColourId, Palette::panelEdge);
    setColour (juce::TextEditor::focusedOutlineColourId, Palette::accent);
    setColour (juce::TextEditor::highlightColourId, Palette::orange.withAlpha (0.4f));
    setColour (juce::CaretComponent::caretColourId, Palette::yellow);
    setColour (juce::AlertWindow::backgroundColourId, Palette::panel);
    setColour (juce::AlertWindow::textColourId, Palette::text);
    setColour (juce::AlertWindow::outlineColourId, Palette::panelEdge);
}

void ZeroLookAndFeel::drawComboBox (juce::Graphics& g, int width, int height, bool isButtonDown,
                                    int, int, int, int, juce::ComboBox& box)
{
    const auto r = juce::Rectangle<float> ((float) width, (float) height).reduced (0.5f);
    const auto hover = box.isMouseOver (true) || isButtonDown;

    g.setColour (Palette::screen.withAlpha (0.85f));
    g.fillRoundedRectangle (r, 3.0f);
    g.setColour (hover ? Palette::accent.withAlpha (0.6f) : Palette::panelEdge);
    g.drawRoundedRectangle (r, 3.0f, 1.0f);

    // Small chevron.
    const auto cx = (float) width - 9.0f, cy = (float) height * 0.5f;
    juce::Path chevron;
    chevron.startNewSubPath (cx - 3.5f, cy - 1.5f);
    chevron.lineTo (cx, cy + 2.0f);
    chevron.lineTo (cx + 3.5f, cy - 1.5f);
    g.setColour (hover ? Palette::yellow : Palette::accent.withAlpha (0.8f));
    g.strokePath (chevron, juce::PathStrokeType (1.3f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

void ZeroLookAndFeel::positionComboBoxText (juce::ComboBox& box, juce::Label& label)
{
    label.setBounds (7, 0, box.getWidth() - 22, box.getHeight());
    label.setFont (getComboBoxFont (box));
}

juce::Font ZeroLookAndFeel::getComboBoxFont (juce::ComboBox& box)
{
    return Fonts::ui (juce::jmin (12.0f, (float) box.getHeight() * 0.62f), false, 0.06f);
}

juce::Font ZeroLookAndFeel::getPopupMenuFont()
{
    return Fonts::ui (12.5f, false, 0.04f);
}

juce::Font ZeroLookAndFeel::getTextButtonFont (juce::TextButton&, int buttonHeight)
{
    return Fonts::ui (juce::jmin (12.0f, (float) buttonHeight * 0.62f), true, 0.08f);
}

juce::Font ZeroLookAndFeel::getAlertWindowTitleFont()   { return Fonts::ui (15.0f, true, 0.06f); }
juce::Font ZeroLookAndFeel::getAlertWindowMessageFont() { return Fonts::ui (12.5f); }
juce::Font ZeroLookAndFeel::getAlertWindowFont()        { return Fonts::ui (12.0f); }

void ZeroLookAndFeel::drawAlertBox (juce::Graphics& g, juce::AlertWindow& alert, const juce::Rectangle<int>& textArea, juce::TextLayout& text)
{
    // A panel with the console's header strip and an orange title.
    const auto plate = alert.getLocalBounds().toFloat();
    g.fillAll (Palette::panel);
    g.setColour (Palette::header);
    g.fillRect (plate.withHeight (4.0f));
    g.setColour (Palette::panelEdge);
    g.drawRect (plate, 1.0f);

    g.setColour (Palette::orange);
    text.draw (g, textArea.toFloat());
}

juce::Font ZeroLookAndFeel::getLabelFont (juce::Label& label)
{
    if (dynamic_cast<juce::Slider*> (label.getParentComponent()) != nullptr)
        return Fonts::mono (11.0f);
    return LookAndFeel_V4::getLabelFont (label);
}

void ZeroLookAndFeel::drawPopupMenuBackground (juce::Graphics& g, int width, int height)
{
    g.fillAll (Palette::panel);
    g.setColour (Palette::panelEdge);
    g.drawRect (0, 0, width, height);
}

void ZeroLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float position,
                                        float startAngle, float endAngle, juce::Slider& slider)
{
    using juce::MathConstants;

    // Backlit light-pipe ring and lit scale (from Potato Kicker) around a machined
    // black knob. The lit part fades orange to yellow along the travel, and the
    // whole light gets brighter as the knob turns up.
    const auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat();
    const auto size = juce::jmin (bounds.getWidth(), bounds.getHeight());
    const auto centre = bounds.getCentre();
    const auto n = juce::jlimit (0.0f, 1.0f, position);
    const auto k = 0.3f + 0.7f * n; // light intensity
    const auto big = size >= 84.0f;

    const auto outer = size * 0.5f - 1.5f;
    const auto bodyRadius = size * (big ? 0.25f : 0.3f);
    const auto pipe = juce::jmax (2.0f, size * 0.036f);
    const auto ringRadius = bodyRadius + pipe * 0.5f + size * 0.04f;

    auto ramp = [] (float t) { return Palette::orange.interpolatedWith (Palette::yellow, juce::jlimit (0.0f, 1.0f, t)); };
    auto angleAt = [&] (float t) { return startAngle + t * (endAngle - startAngle); };
    auto box = [&] (float r) { return juce::Rectangle<float> (r * 2.0f, r * 2.0f).withCentre (centre); };

    // Arc from t0 to t1 of the travel, in short segments shaded along the ramp.
    auto rampArc = [&] (float t0, float t1, float thickness, float alpha, float bright = 0.0f)
    {
        const auto segments = juce::jmax (2, (int) ((t1 - t0) * 40.0f));
        for (int i = 0; i < segments; ++i)
        {
            const auto a = t0 + (t1 - t0) * (float) i / (float) segments;
            const auto b = t0 + (t1 - t0) * (float) (i + 1) / (float) segments;
            juce::Path p;
            p.addCentredArc (centre.x, centre.y, ringRadius, ringRadius, 0.0f, angleAt (a), angleAt (b) + 0.005f, true);
            g.setColour (ramp (0.5f * (a + b)).brighter (bright).withAlpha (juce::jlimit (0.0f, 1.0f, alpha)));
            g.strokePath (p, juce::PathStrokeType (thickness, juce::PathStrokeType::curved, juce::PathStrokeType::butt));
        }
    };

    // Scale ticks: lit (with a glow) up to the value, dim beyond.
    if (size > 40.0f)
        for (int i = 0; i <= 40; ++i)
        {
            const auto major = i % 4 == 0;
            const auto t = (float) i / 40.0f;
            const auto a = angleAt (t);
            const auto inner = centre.getPointOnCircumference (major ? outer - 5.5f : outer - 3.0f, a);
            const auto tip = centre.getPointOnCircumference (outer, a);
            if (n > 0.001f && t <= n + 0.0001f)
            {
                g.setColour (ramp (t).withAlpha (0.35f * k));
                g.drawLine ({ inner, tip }, major ? 4.0f : 3.0f);
                g.setColour (ramp (t).brighter (0.35f));
                g.drawLine ({ inner, tip }, major ? 1.5f : 1.0f);
            }
            else
            {
                g.setColour (Palette::text.withAlpha (major ? 0.45f : 0.22f));
                g.drawLine ({ inner, tip }, major ? 1.3f : 0.8f);
            }
        }

    // 0-10 numbers on the big knobs, lit up to the value.
    if (big)
    {
        const auto numberRadius = 0.5f * ((ringRadius + pipe * 0.5f + 1.2f) + (outer - 5.5f));
        g.setFont (Fonts::ui (size * 0.095f, true));
        for (int j = 0; j <= 10; ++j)
        {
            const auto t = (float) j / 10.0f;
            const auto lit = n > 0.001f && t <= n + 0.0001f;
            g.setColour (lit ? ramp (t).brighter (0.55f) : Palette::text.withAlpha (0.35f));
            g.drawText (juce::String (j), juce::Rectangle<float> (14.0f, 11.0f).withCentre (centre.getPointOnCircumference (numberRadius, angleAt (t))),
                        juce::Justification::centred, false);
        }
    }

    // Groove, the dim backlit ring over the whole travel, then the lit value arc
    // with layered bloom and a bright tip.
    g.setColour (juce::Colour (0xff040306));
    g.drawEllipse (box (ringRadius), pipe + 3.0f);
    rampArc (0.0f, 1.0f, pipe * 0.8f, 0.10f + 0.12f * k);
    if (n > 0.001f)
    {
        rampArc (0.0f, n, pipe * 4.0f, 0.12f * k);
        rampArc (0.0f, n, pipe * 2.3f, 0.28f * k);
        rampArc (0.0f, n, pipe * 1.1f, 0.65f + 0.35f * k);
        rampArc (0.0f, n, pipe * 0.6f, 1.0f, 0.45f);
        const auto tip = centre.getPointOnCircumference (ringRadius, angleAt (n));
        g.setColour (ramp (n).brighter (0.6f).withAlpha (0.35f));
        g.fillEllipse (juce::Rectangle<float> (pipe * 3.2f, pipe * 3.2f).withCentre (tip));
        g.setColour (juce::Colour (0xfffff4d6));
        g.fillEllipse (juce::Rectangle<float> (pipe * 1.3f, pipe * 1.3f).withCentre (tip));
    }

    // Light spilling onto the knob's edge.
    g.setColour (ramp (n).withAlpha (0.12f + 0.3f * k));
    g.drawEllipse (box (bodyRadius + 2.2f), 1.6f);

    // The machined body, lit from the upper left: cast shadow, knurled skirt,
    // bevelled rim, a cap with fine turning rings and a highlight.
    const auto body = box (bodyRadius);
    const auto light = centre.translated (-bodyRadius * 0.45f, -bodyRadius * 0.55f);

    for (int i = 4; i >= 1; --i)
    {
        const auto spread = (float) i * bodyRadius * 0.05f;
        g.setColour (juce::Colours::black.withAlpha (0.12f));
        g.fillEllipse (body.translated (spread * 0.5f, spread * 0.9f).expanded (spread));
    }

    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff2c2e38), light.x, light.y,
                                             juce::Colour (0xff050507), centre.x + bodyRadius, centre.y + bodyRadius, true));
    g.fillEllipse (body);
    constexpr int ridges = 40;
    for (int i = 0; i < ridges; ++i)
    {
        const auto a = MathConstants<float>::twoPi * (float) i / ridges;
        const auto lit = 0.5f + 0.5f * std::cos (a + MathConstants<float>::pi * 0.25f); // brightest upper left
        g.setColour (juce::Colours::white.withAlpha (0.03f + 0.12f * lit));
        g.drawLine ({ centre.getPointOnCircumference (bodyRadius * 0.86f, a), centre.getPointOnCircumference (bodyRadius * 0.99f, a) }, 1.0f);
    }

    const auto cap = body.reduced (bodyRadius * 0.16f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff5a5d6c), cap.getX(), cap.getY(),
                                             juce::Colour (0xff060608), cap.getRight(), cap.getBottom(), false));
    g.fillEllipse (cap.expanded (bodyRadius * 0.05f));

    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff3c3e4a), light.x, light.y,
                                             juce::Colour (0xff0a0a0e), cap.getRight(), cap.getBottom(), true));
    g.fillEllipse (cap);
    g.setColour (juce::Colours::white.withAlpha (0.035f));
    for (float r = 0.2f; r < 1.0f; r += 0.12f)
        g.drawEllipse (cap.reduced (cap.getWidth() * 0.5f * (1.0f - r)), 0.8f);

    g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.32f), light.x, light.y,
                                             juce::Colours::transparentWhite, light.x + bodyRadius * 0.55f, light.y + bodyRadius * 0.55f, true));
    g.fillEllipse (cap.reduced (cap.getWidth() * 0.06f));

    // Pointer: a lit line in the ramp colour for the current position, in a dark groove.
    const auto from = centre.getPointOnCircumference (bodyRadius * 0.15f, angleAt (n));
    const auto to = centre.getPointOnCircumference (bodyRadius * 0.95f, angleAt (n));
    const auto thickness = juce::jmax (2.0f, size * 0.026f);
    g.setColour (juce::Colours::black.withAlpha (0.8f));
    g.drawLine ({ from, to }, thickness + 2.0f);
    g.setColour (ramp (n).withAlpha (0.25f + 0.4f * k));
    g.drawLine ({ from, to }, 5.0f);
    g.setColour (juce::Colour (0xfffff1d8));
    g.drawLine ({ from, to }, thickness);

    // Value readout in the gap at the bottom of the travel, brighter while dragging.
    if (size >= 56.0f) // the small Output knob in the top bar goes without
    {
        const auto dragging = slider.isMouseButtonDown();
        g.setColour (dragging ? ramp (n).brighter (0.4f) : Palette::text.withAlpha (0.55f));
        g.setFont (Fonts::mono (juce::jmax (8.5f, size * 0.1f)));
        // 0..1 amounts read on the 0-10 scale printed round the knob; anything with a
        // unit (s, Hz, dB, st, oct) in that unit, with sensible decimals.
        const auto value = slider.getValue();
        const auto unit = slider.getTextValueSuffix();
        const auto unitRange = slider.getMinimum() == 0.0 && slider.getMaximum() == 1.0 && unit.isEmpty();
        const auto magnitude = std::abs (value);
        const auto decimals = slider.getInterval() >= 1.0 || magnitude >= 100.0 ? 0 : magnitude >= 10.0 ? 1 : 2;
        // Times under a second read in milliseconds ("0.00 s" says nothing).
        const auto millis = unit.trim() == "s" && magnitude < 1.0;
        const auto text = unitRange ? juce::String (value * 10.0, 1)
                        : millis   ? juce::String (value * 1000.0, magnitude < 0.01 ? 1 : 0) + " ms"
                                   : juce::String (value, decimals) + unit;
        g.drawText (text,
                    juce::Rectangle<float> (size * 0.95f, size * 0.12f).withCentre ({ centre.x, centre.y + outer - size * 0.06f }),
                    juce::Justification::centred, false);
    }
}

//==============================================================================
void DislikeButton::paintButton (juce::Graphics& g, bool highlighted, bool down)
{
    // A thumb pointing down: the cuff on the left, the fist (a rounded block),
    // and the thumb hanging down from it; outlined like the heart.
    const auto r = getLocalBounds().toFloat().reduced (3.0f + (down ? 1.0f : 0.0f));
    const auto size = juce::jmin (r.getWidth(), r.getHeight());
    const auto b = r.withSizeKeepingCentre (size, size);
    const auto x = b.getX(), y = b.getY(), w = b.getWidth(), h = b.getHeight();
    juce::Path thumb;
    thumb.addRectangle (x + 0.06f * w, y + 0.06f * h, 0.18f * w, 0.46f * h);                     // the cuff, on the left
    thumb.addRoundedRectangle (x + 0.28f * w, y + 0.06f * h, 0.64f * w, 0.46f * h, 0.12f * w); // the fist
    thumb.addRoundedRectangle (x + 0.28f * w, y + 0.44f * h, 0.22f * w, 0.5f * h, 0.11f * w);  // the thumb, pointing down
    g.setColour (Palette::accent.withAlpha (highlighted ? 1.0f : 0.75f));
    g.strokePath (thumb, juce::PathStrokeType (1.3f));
}

void HeartButton::paintButton (juce::Graphics& g, bool highlighted, bool down)
{
    // Two lobes and a point, drawn from cubic curves inside the button's square.
    const auto r = getLocalBounds().toFloat().reduced (3.0f + (down ? 1.0f : 0.0f));
    const auto size = juce::jmin (r.getWidth(), r.getHeight());
    const auto box = r.withSizeKeepingCentre (size, size);
    const auto x = box.getX(), y = box.getY(), w = box.getWidth(), h = box.getHeight();
    juce::Path heart;
    heart.startNewSubPath (x + 0.5f * w, y + 0.95f * h);
    heart.cubicTo (x - 0.05f * w, y + 0.55f * h, x + 0.05f * w, y + 0.02f * h, x + 0.5f * w, y + 0.28f * h);
    heart.cubicTo (x + 0.95f * w, y + 0.02f * h, x + 1.05f * w, y + 0.55f * h, x + 0.5f * w, y + 0.95f * h);
    heart.closeSubPath();

    if (filled)
    {
        g.setColour (Palette::orange.withAlpha (highlighted ? 1.0f : 0.9f));
        g.fillPath (heart);
    }
    g.setColour ((filled ? Palette::yellow : Palette::accent).withAlpha (highlighted ? 1.0f : 0.75f));
    g.strokePath (heart, juce::PathStrokeType (1.4f));
}

void ChevronButton::paintButton (juce::Graphics& g, bool highlighted, bool down)
{
    const auto r = getLocalBounds().toFloat();
    if (highlighted && isEnabled())
    {
        g.setColour (Palette::screen.withAlpha (0.8f));
        g.fillRoundedRectangle (r, 3.0f);
    }

    const auto c = r.getCentre().translated (down ? 0.5f : 0.0f, down ? 0.5f : 0.0f);
    const auto dx = left ? 2.5f : -2.5f;
    juce::Path p;
    p.startNewSubPath (c.x + dx, c.y - 4.0f);
    p.lineTo (c.x - dx, c.y);
    p.lineTo (c.x + dx, c.y + 4.0f);
    g.setColour (! isEnabled() ? Palette::text.withAlpha (0.2f) : highlighted ? Palette::yellow : Palette::accent);
    g.strokePath (p, juce::PathStrokeType (1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

//==============================================================================
TesseractButton::TesseractButton (bool small)
    : juce::Button (small ? "Generate any genre" : "Generate"), mini (small)
{
    startTimerHz (60);
}

void TesseractButton::fold()
{
    foldStart = juce::Time::getMillisecondCounterHiRes();
    // Gravity waves from the collapse: three, a moment apart, starting as the
    // tesseract reaches the black hole (mid-fold).
    for (int i = 0; i < 3; ++i)
        waves.push_back ({ spaceTime + 0.4 + 0.22 * i, juce::Random::getSystemRandom().nextFloat() * juce::MathConstants<float>::twoPi });
    if (waves.size() > 9)
        waves.erase (waves.begin(), waves.begin() + (std::ptrdiff_t) (waves.size() - 9));
}

void TesseractButton::timerCallback()
{
    constexpr double foldMs = 900.0;

    if (foldStart >= 0.0)
    {
        foldProgress = (float) juce::jlimit (0.0, 1.0, (juce::Time::getMillisecondCounterHiRes() - foldStart) / foldMs);
        if (foldProgress >= 1.0f)
        {
            foldStart = -1.0;
            foldProgress = 0.0f;
        }
    }

    angle += 0.008f + 0.12f * std::sin (juce::MathConstants<float>::pi * foldProgress); // spins up mid-fold
    colourPhase += 0.008f; // the resting turn's pace, whatever a fold does
    spaceTime += 1.0 / 60.0;
    if (isShowing())
        repaint();
}

#if ZERO_SPACE_IMAGES
// A picture from Resources/Space by its file name (empty if it isn't there).
static juce::Image spaceImage (const char* fileName)
{
    for (int i = 0; i < SpaceImages::namedResourceListSize; ++i)
        if (juce::String (SpaceImages::originalFilenames[i]) == fileName)
        {
            int size = 0;
            if (const auto* data = SpaceImages::getNamedResource (SpaceImages::namedResourceList[i], size))
                return juce::ImageCache::getFromMemory (data, size);
        }
    return {};
}
#endif

void TesseractButton::paintSpace (juce::Graphics& g, juce::Rectangle<float> screen)
{
    // Kept faint, so the tesseract stays the thing to look at.
    using juce::MathConstants;
    if (stars.empty())
    {
        juce::Random random (7);
        for (int i = 0; i < 280; ++i)
            stars.push_back ({ random.nextFloat(), random.nextFloat(), 0.5f + 1.3f * random.nextFloat() * random.nextFloat(),
                               0.3f + 1.5f * random.nextFloat(), MathConstants<float>::twoPi * random.nextFloat() });
    }
    const auto t = (float) spaceTime;
    juce::Graphics::ScopedSaveState saved (g);
    g.reduceClipRegion (screen.toNearestInt());

   #if ZERO_SPACE_IMAGES
    // The owner's nebula, stretched over the whole screen and almost
    // transparent: everything else plays in front of it.
    const auto background = spaceImage ("background.jpg"); // ImageCache keeps it decoded
    if (background.isValid())
    {
        g.setOpacity (0.16f);
        g.drawImage (background, screen, juce::RectanglePlacement::stretchToFit);
        g.setOpacity (1.0f);
    }
   #endif

    // Stars, each twinkling at its own slow rate.
    for (const auto& s : stars)
    {
        const auto twinkle = 0.5f + 0.5f * std::sin (s.phase + t * s.rate);
        g.setColour (juce::Colour (0xffdfe9ff).withAlpha (0.12f + 0.45f * twinkle * juce::jmin (1.0f, s.size)));
        g.fillEllipse (screen.getX() + s.x * screen.getWidth() - 0.5f * s.size, screen.getY() + s.y * screen.getHeight() - 0.5f * s.size,
                       s.size, s.size);
    }

    // Two planets, drifting a few pixels over minutes: lit from the upper left.
    auto planet = [&] (float x, float y, float radius, juce::Colour colour, float drift)
    {
        const auto centre = juce::Point<float> (screen.getX() + (x + 0.01f * std::sin (t * drift)) * screen.getWidth(),
                                                screen.getY() + (y + 0.008f * std::cos (t * drift * 0.7f)) * screen.getHeight());
        juce::ColourGradient lit (colour.withAlpha (0.38f), centre.x - radius * 0.4f, centre.y - radius * 0.4f,
                                  colour.darker (0.8f).withAlpha (0.05f), centre.x + radius, centre.y + radius, true);
        g.setGradientFill (lit);
        g.fillEllipse (centre.x - radius, centre.y - radius, 2.0f * radius, 2.0f * radius);
    };
    const auto h = screen.getHeight();
    planet (0.13f, 0.28f, 0.075f * h, Palette::accent, 0.031f);
    planet (0.86f, 0.70f, 0.045f * h, Palette::magenta, 0.023f);

    // Glowing nodes spread over the whole screen, drifting round the black hole
    // on wide oval orbits (the inner ones faster), each shimmering; faint oval
    // orbit lines under them.
    if (nodes.empty())
    {
        juce::Random random (11);
        const juce::Colour colours[] { Palette::orange, Palette::magenta, Palette::accent, juce::Colour (0xffb39cf0), Palette::yellow };
        for (int i = 0; i < 110; ++i)
            nodes.push_back ({ 0.12f + 0.98f * std::sqrt (random.nextFloat()), MathConstants<float>::twoPi * random.nextFloat(),
                               0.8f + 2.2f * random.nextFloat() * random.nextFloat(), colours[random.nextInt ((int) std::size (colours))],
                               false, MathConstants<float>::twoPi * random.nextFloat() });
    }
    const auto c = screen.getCentre();
    const auto rx = 0.5f * screen.getWidth(), ry = 0.62f * screen.getHeight();

    for (int ring = 1; ring <= 5; ++ring)
    {
        const auto k = (float) ring / 5.0f;
        g.setColour (Palette::accent.withAlpha (0.04f));
        g.drawEllipse (c.x - rx * k, c.y - ry * k, 2.0f * rx * k, 2.0f * ry * k, 0.8f);
    }

    for (const auto& n : nodes)
    {
        const auto a = n.angle + t * 0.015f / n.radius;
        const auto p = c + juce::Point<float> (std::cos (a) * rx, std::sin (a) * ry) * n.radius;
        const auto shimmer = 0.6f + 0.4f * std::sin (n.phase + t * 1.3f);
        const auto halo = 3.0f * n.size;
        juce::ColourGradient glow (n.colour.withAlpha (0.3f * shimmer), p.x, p.y, n.colour.withAlpha (0.0f), p.x + halo, p.y, true);
        g.setGradientFill (glow);
        g.fillEllipse (p.x - halo, p.y - halo, 2.0f * halo, 2.0f * halo);
        g.setColour (n.colour.brighter (0.4f).withAlpha (0.75f * shimmer));
        g.fillEllipse (p.x - 0.5f * n.size, p.y - 0.5f * n.size, n.size, n.size);
    }

    // Gravity waves from a collapse: faint rings rippling out from the black
    // hole across the whole screen, uneven (a few slow wobbles round each),
    // fading as they spread; light blue going to magenta.
    {
        const auto farthest = std::hypot (0.5f * screen.getWidth(), 0.5f * screen.getHeight());
        for (const auto& w : waves)
        {
            const auto age = (float) (spaceTime - w.start);
            const auto life = 3.2f;
            if (age <= 0.0f || age >= life)
                continue;
            const auto k = age / life;
            const auto reach = farthest * 1.1f * (1.0f - std::pow (1.0f - k, 2.0f)); // fast at first, easing out
            juce::Path ring;
            for (int s = 0; s <= 96; ++s)
            {
                const auto a = juce::MathConstants<float>::twoPi * (float) s / 96.0f;
                const auto wobble = 1.0f + 0.05f * std::sin (3.0f * a + w.phase + age) + 0.035f * std::sin (5.0f * a - 1.7f * w.phase - 0.6f * age);
                const auto p = c + juce::Point<float> (std::cos (a), std::sin (a) * 0.78f) * (reach * wobble);
                if (s == 0)
                    ring.startNewSubPath (p);
                else
                    ring.lineTo (p);
            }
            const auto alpha = 0.16f * (1.0f - k) * std::sqrt (k * 4.0f > 1.0f ? 1.0f : k * 4.0f);
            g.setColour (Palette::accent.interpolatedWith (Palette::magenta, k).withAlpha (alpha));
            g.strokePath (ring, juce::PathStrokeType (1.4f + 2.0f * (1.0f - k)));
            g.setColour (Palette::accent.withAlpha (alpha * 0.25f)); // a soft halo
            g.strokePath (ring, juce::PathStrokeType (8.0f * (1.0f - k) + 2.0f));
        }
    }

    // The black hole at the centre: an accretion disc of tilted rings, magenta
    // into orange, turning slowly (steady through a fold; only the rim brightens);
    // a dark core with a faint rim of light.
    const auto flare = std::sin (MathConstants<float>::pi * foldProgress);
    const auto r = 0.36f * juce::jmin (screen.getWidth(), screen.getHeight());
    for (int ring = 0; ring < 7; ++ring)
    {
        const auto k = (float) ring / 6.0f;
        const auto radius = r * (0.55f + 0.5f * k);
        juce::Path disc;
        disc.addEllipse (-radius, -radius * 0.32f, 2.0f * radius, 2.0f * radius * 0.32f);
        const auto turn = juce::AffineTransform::rotation (-0.35f + 0.05f * std::sin (t * 0.05f)).translated (c.x, c.y);
        g.setColour (Palette::magenta.interpolatedWith (Palette::orange, k).withAlpha (0.08f * (1.0f - 0.6f * k)));
        g.strokePath (disc, juce::PathStrokeType (2.5f), turn);
        juce::Path dashed;
        const float dashes[] { 6.0f + 10.0f * k, 9.0f };
        juce::PathStrokeType (1.2f).createDashedStroke (dashed, disc, dashes, 2);
        g.setColour (Palette::yellow.withAlpha (0.06f * (1.0f - k)));
        g.fillPath (dashed, juce::AffineTransform::rotation (t * (0.1f - 0.05f * k)).followedBy (turn));
    }
    const auto core = r * 0.28f;
    juce::ColourGradient rim (juce::Colour (0xff05060c), c.x, c.y, Palette::accent.withAlpha (0.0f), c.x + core * 1.6f, c.y, true);
    rim.addColour (0.62, juce::Colour (0xff05060c));
    rim.addColour (0.72, Palette::accent.withAlpha (0.3f + 0.5f * flare));
    g.setGradientFill (rim);
    g.fillEllipse (c.x - core * 1.6f, c.y - core * 1.6f, core * 3.2f, core * 3.2f);
}

void TesseractButton::paintButton (juce::Graphics& g, bool highlighted, bool down)
{
    using juce::MathConstants;

    auto bounds = getLocalBounds().toFloat();
    if (! mini)
    {
        fillScreen (g, bounds);
        paintSpace (g, bounds.reduced (2.0f));
    }

    // Fold: shrink towards the centre while turning a half turn through w (the
    // outer cube passes through the inner one), then open out again.
    const auto pulse = std::sin (MathConstants<float>::pi * foldProgress);
    const auto shrink = 1.0f - 0.7f * pulse - (down ? 0.08f : 0.0f);
    const auto wTurn = angle + MathConstants<float>::pi * foldProgress;

    const auto centre = getLocalBounds().toFloat().getCentre(); // the black hole's centre
    const auto radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.38f * shrink;

    // The 16 corners of a 4D cube, turned in the xw and yw planes, then zx;
    // projected 4D -> 3D -> 2D with perspective.
    juce::Point<float> points[16];
    float depth[16];
    for (int i = 0; i < 16; ++i)
    {
        float x = (i & 1) ? 1.0f : -1.0f, y = (i & 2) ? 1.0f : -1.0f;
        float z = (i & 4) ? 1.0f : -1.0f, w = (i & 8) ? 1.0f : -1.0f;

        auto turn = [] (float& a, float& b, float t)
        {
            const auto c = std::cos (t), s = std::sin (t);
            const auto a2 = a * c - b * s;
            b = a * s + b * c;
            a = a2;
        };
        turn (x, w, wTurn);
        turn (y, w, wTurn * 0.7f);
        turn (z, x, angle * 0.5f);

        const auto f4 = 2.2f / (3.0f - w);
        x *= f4; y *= f4; z *= f4;
        const auto f3 = 2.5f / (4.0f - z);
        const auto px = x * f3 * radius, py = y * f3 * radius;
        points[i] = { centre.x + px, centre.y + py };
        depth[i] = w;
    }

    // Edges join corners that differ in one coordinate; nearer in w is brighter.
    // Light blue at rest; a wide faint pass adds glow.
    const auto glow = highlighted ? 1.0f : 0.75f;
    // A smooth fade that stays saturated: blue goes round the colour wheel through
    // violet and magenta into orange (a straight blend would pass through grey),
    // then on to yellow at the deepest point.
    auto aroundTheWheel = [] (juce::Colour from, juce::Colour to, float t)
    {
        auto hueTo = to.getHue();
        if (hueTo < from.getHue())
            hueTo += 1.0f; // always travel upwards: blue -> violet -> magenta -> red/orange
        auto hue = from.getHue() + t * (hueTo - from.getHue());
        hue -= std::floor (hue);
        return juce::Colour (hue, juce::jmap (t, from.getSaturation(), to.getSaturation()),
                             juce::jmap (t, from.getBrightness(), to.getBrightness()), 1.0f);
    };
    // The colour drifts at the resting turn's pace, one slow round trip per turn
    // (blue, violet, magenta, orange, yellow and back), smooth as a cosine; a
    // fold flashes on top of wherever it is.
    const auto drift = 0.5f - 0.5f * std::cos (colourPhase);
    const auto shade = juce::jlimit (0.0f, 1.0f, juce::jmax (drift, pulse));
    const auto colour = shade < 0.6f ? aroundTheWheel (Palette::accent, Palette::orange, shade / 0.6f)
                                     : Palette::orange.interpolatedWith (Palette::yellow, (shade - 0.6f) / 0.4f);
    for (const auto pass : { 0, 1 })
        for (int a = 0; a < 16; ++a)
            for (int bit = 1; bit < 16; bit <<= 1)
                if (const auto b = a | bit; b != a)
                {
                    const auto near = juce::jlimit (0.15f, 1.0f, (0.5f + 0.25f * (depth[a] + depth[b]) * 0.5f) * glow + 0.4f * pulse);
                    g.setColour (colour.withAlpha (pass == 0 ? near * 0.15f : near));
                    g.drawLine ({ points[a], points[b] }, (pass == 0 ? 5.0f : 1.4f) + 1.2f * pulse);
                }
}

//==============================================================================
namespace
{
    constexpr float meterFloorDb = -60.0f;

    // dB to 0..1 up a meter: finer near the top, where mixing decisions happen.
    float meterFraction (float db)
    {
        const auto x = juce::jlimit (0.0f, 1.0f, (db - meterFloorDb) / -meterFloorDb);
        return std::pow (x, 1.6f);
    }

    juce::String dbText (float db)
    {
        return db <= -99.0f ? juce::String ("-inf") : juce::String (db, 1);
    }
}

LevelMeter::LevelMeter (const FinishingChain::Meters& m)
    : meters (m)
{
    startTimerHz (30);
}

void LevelMeter::timerCallback()
{
    constexpr float frameMs = 1000.0f / 30.0f;

    for (int ch = 0; ch < 2; ++ch)
    {
        // Peak: instant rise, ~20 dB/s fall. RMS: eased.
        const auto newPeak = meters.channelPeakDb[ch].load();
        peak[ch] = juce::jmax (newPeak, peak[ch] - 20.0f * frameMs / 1000.0f);
        rms[ch] += 0.3f * (meters.channelRmsDb[ch].load() - rms[ch]);

        // Hold for 1.5 s, then fall.
        holdMs[ch] += frameMs;
        if (newPeak >= hold[ch]) { hold[ch] = newPeak; holdMs[ch] = 0.0f; }
        else if (holdMs[ch] > 1500.0f) hold[ch] -= 30.0f * frameMs / 1000.0f;

        maxPeak = juce::jmax (maxPeak, newPeak);
        clipped[ch] = clipped[ch] || newPeak > -0.1f;
    }

    momentary = meters.momentaryLufs.load();
    shortTerm = meters.shortTermLufs.load();
    clipDb = juce::jmax (meters.clipDb.load(), clipDb * 0.9f);
    correlation += 0.2f * (meters.correlation.load() - correlation);

    if (isShowing())
        repaint();
}

void LevelMeter::mouseDown (const juce::MouseEvent&)
{
    maxPeak = -100.0f;
    clipped[0] = clipped[1] = false;
    hold[0] = hold[1] = -100.0f;
}

// The meter keeps the plain sans (owner's choice); the rest of the UI uses Fonts.
void LevelMeter::paint (juce::Graphics& g)
{
    fillScreen (g, getLocalBounds().toFloat());

    auto area = getLocalBounds().reduced (10).toFloat();


    // Readouts at the bottom, or in a column on the right when the meter is wide
    // (so the bars keep their height).
    const auto wide = area.getWidth() > 2.0f * area.getHeight();
    const auto readoutWidth = area.getWidth() - 130.0f;
    auto readouts = wide ? area.removeFromRight (readoutWidth).withSizeKeepingCentre (readoutWidth, 76.0f)
                         : area.removeFromBottom (76.0f);
    if (wide)
        area.removeFromRight (12.0f);
    auto corrArea = readouts.removeFromBottom (18.0f);

    auto row = [&] (const juce::String& name, const juce::String& value, juce::Colour colour)
    {
        auto r = readouts.removeFromTop (14.0f);
        g.setFont (juce::FontOptions (11.0f));
        g.setColour (Palette::text.withAlpha (0.55f));
        g.drawText (name, r, juce::Justification::centredLeft);
        g.setColour (colour);
        g.setFont (juce::FontOptions (11.0f));
        g.drawText (value, r, juce::Justification::centredRight);
    };
    row ("Peak max", dbText (maxPeak) + " dB", maxPeak > -0.1f ? Palette::clip : Palette::text);
    row ("LUFS M / S", dbText (momentary) + " / " + dbText (shortTerm), Palette::text);
    row ("Clipper", clipDb < 0.05f ? juce::String ("idle") : "-" + juce::String (clipDb, 1) + " dB",
         clipDb > 3.0f ? Palette::warn : Palette::text);
    row ("RMS L / R", dbText (rms[0]) + " / " + dbText (rms[1]), Palette::text);

    // Correlation: -1 (out of phase) .. +1 (mono).
    {
        auto bar = corrArea.reduced (0.0f, 5.0f);
        g.setColour (Palette::track);
        g.fillRoundedRectangle (bar, 3.0f);
        const auto centre = bar.getCentreX();
        const auto x = centre + juce::jlimit (-1.0f, 1.0f, correlation) * bar.getWidth() * 0.5f;
        g.setColour (correlation < 0.0f ? Palette::warn : Palette::accent);
        g.fillRect (juce::Rectangle<float>::leftTopRightBottom (juce::jmin (centre, x), bar.getY(), juce::jmax (centre, x), bar.getBottom()));
        g.setColour (Palette::text.withAlpha (0.5f));
        g.setFont (juce::FontOptions (9.0f));
        g.drawText ("-1", bar.withWidth (14.0f).translated (0.0f, -8.0f), juce::Justification::centredLeft);
        g.drawText ("+1", bar.withTrimmedLeft (bar.getWidth() - 14.0f).translated (0.0f, -8.0f), juce::Justification::centredRight);
    }

    if (! wide)
        area.removeFromBottom (6.0f);

    // Clip lights.
    auto lights = area.removeFromTop (10.0f);
    area.removeFromTop (4.0f);

    // Scale on the left, two bars on the right.
    auto scale = area.removeFromLeft (28.0f);
    const auto barGap = 6.0f;
    const auto barWidth = (area.getWidth() - barGap) / 2.0f;

    auto yFor = [&] (float db) { return area.getBottom() - meterFraction (db) * area.getHeight(); };

    g.setFont (juce::FontOptions (9.0f));
    for (const auto db : { 0.0f, -6.0f, -12.0f, -18.0f, -24.0f, -36.0f, -48.0f })
    {
        const auto y = yFor (db);
        g.setColour (Palette::text.withAlpha (0.45f));
        g.drawText (juce::String ((int) db), scale.withY (y - 6.0f).withHeight (12.0f).withTrimmedRight (4.0f),
                    juce::Justification::centredRight);
        g.setColour (Palette::track.brighter (0.15f));
        g.drawHorizontalLine ((int) y, area.getX(), area.getRight());
    }

    for (int ch = 0; ch < 2; ++ch)
    {
        auto bar = area.withX (area.getX() + (float) ch * (barWidth + barGap)).withWidth (barWidth);

        g.setColour (Palette::track);
        g.fillRoundedRectangle (bar, 3.0f);

        // Colour by level: light blue low down, fading to orange as it rises, yellow
        // near the top, red at 0 dB. The RMS body is the wide bar, the peak a thinner
        // one behind it; each gets a lit cap and a soft rounded sheen.
        auto fill = [&] (float db, float inset, float alpha, bool isPeak)
        {
            const auto top = yFor (db);
            auto r = bar.reduced (inset, 0.0f).withTop (top);
            if (r.getHeight() <= 0.0f)
                return;

            juce::ColourGradient gradient (Palette::accent.withAlpha (alpha), 0.0f, bar.getBottom(),
                                           Palette::clip.withAlpha (alpha), 0.0f, bar.getY(), false);
            gradient.addColour (meterFraction (-48.0f), Palette::accent.withAlpha (alpha));
            gradient.addColour (meterFraction (-18.0f), Palette::orange.withAlpha (alpha));
            gradient.addColour (meterFraction (-6.0f), Palette::yellow.withAlpha (alpha));
            g.setGradientFill (gradient);
            g.fillRect (r);

            juce::ColourGradient sheen (juce::Colours::black.withAlpha (0.35f), r.getX(), 0.0f,
                                        juce::Colours::black.withAlpha (0.35f), r.getRight(), 0.0f, false);
            sheen.addColour (0.4, juce::Colours::white.withAlpha (0.14f));
            g.setGradientFill (sheen);
            g.fillRect (r);

            // The top of the bar blends from its own colour into the tesseract's
            // magenta, so the pink grows out of the orange rather than sitting on it.
            {
                const auto blend = juce::jmin (r.getHeight(), 14.0f);
                juce::ColourGradient glow (Palette::magenta.withAlpha (0.0f), 0.0f, r.getY() + blend,
                                           Palette::magenta.withAlpha (0.75f * alpha), 0.0f, r.getY(), false);
                g.setGradientFill (glow);
                g.fillRect (r.withHeight (blend));
            }

            const auto capColour = gradient.getColourAtPosition (juce::jlimit (0.0, 1.0, (double) meterFraction (db)));
            g.setColour ((isPeak ? Palette::magenta : capColour).withAlpha (0.35f));
            g.fillRect (r.withHeight (6.0f).translated (0.0f, -3.0f));
            g.setColour ((isPeak ? Palette::magenta : capColour.brighter (0.5f)).withAlpha (1.0f));
            g.fillRect (r.withHeight (1.5f));
        };
        fill (peak[ch], barWidth * 0.3f, 0.55f, true); // the peak's cap in the tesseract's magenta
        fill (rms[ch], 0.0f, 0.95f, false);

        // Peak hold.
        if (hold[ch] > meterFloorDb)
        {
            g.setColour (hold[ch] > -0.1f ? Palette::clip : Palette::magenta);
            g.fillRect (bar.getX(), yFor (hold[ch]) - 1.0f, bar.getWidth(), 2.0f);
        }

        // Clip light.
        auto light = lights.withX (bar.getX()).withWidth (bar.getWidth());
        g.setColour (clipped[ch] ? Palette::clip : Palette::track);
        g.fillRoundedRectangle (light, 2.0f);

        g.setColour (Palette::text.withAlpha (0.5f));
        g.setFont (juce::FontOptions (9.0f, juce::Font::bold));
        g.drawText (ch == 0 ? "L" : "R", bar.withTop (bar.getBottom() - 14.0f), juce::Justification::centred);
    }
}

//==============================================================================
ScopeView::ScopeView (ZeroEngineProcessor& p)
    : processor (p)
{
    startTimerHz (30);
}

void ScopeView::timerCallback()
{
    // Follow the sound: the voices' live wavetable position (envelopes and LFOs
    // included), or the knob's position when nothing plays; and the output level.
    const auto live = processor.getLivePosition();
    const auto target = live >= 0.0f ? live : processor.state.getRawParameterValue (ParamIDs::wavePos)->load();
    shownPosition = shownPosition < 0.0f ? target : shownPosition + 0.5f * (target - shownPosition);
    const auto rms = juce::Decibels::decibelsToGain (processor.getMeters().rmsDb.load());
    level += 0.3f * (juce::jlimit (0.0f, 1.0f, rms * 3.0f) - level);

    if (! isShowing())
        return;

    if (mode == spectrum)
    {
        processor.copyScope (samples.data(), fftSize);
        std::copy (samples.begin(), samples.end(), fftData.begin());
        std::fill (fftData.begin() + fftSize, fftData.end(), 0.0f);
        window.multiplyWithWindowingTable (fftData.data(), (size_t) fftSize);
        fft.performFrequencyOnlyForwardTransform (fftData.data());

        // Instant rise, slow fall, so the picture is readable.
        for (size_t i = 0; i < spectrumDb.size(); ++i)
        {
            const auto db = juce::Decibels::gainToDecibels (fftData[i] * 4.0f / (float) fftSize, -100.0f);
            spectrumDb[i] = juce::jmax (db, spectrumDb[i] - 1.5f);
        }
    }

    repaint();
}

void ScopeView::paint (juce::Graphics& g)
{
    fillScreen (g, getLocalBounds().toFloat());

    auto area = getLocalBounds().reduced (10).toFloat();

    switch (mode)
    {
        case spectrum:  drawSpectrum (g, area); break;
        case wavetable: drawWavetable (g, area); break;
        default:        drawWaveform (g, area); break;
    }

    drawMatchOverlay (g, getLocalBounds().toFloat());
}

void ScopeView::drawMatchOverlay (juce::Graphics& g, juce::Rectangle<float> bounds)
{
    // A file held over the plugin, or a match running / just finished: one line
    // along the bottom of the display.
    const auto status = dropHint ? juce::String ("DROP TO MATCH THIS SOUND") : processor.getMatchStatus();
    if (status.isEmpty())
        return;

    if (dropHint)
    {
        g.setColour (Palette::accent.withAlpha (0.8f));
        g.drawRoundedRectangle (bounds.reduced (3.0f), 4.0f, 1.5f);
    }
    auto strip = bounds.removeFromBottom (22.0f).reduced (6.0f, 3.0f);
    g.setColour (Palette::panel.withAlpha (0.85f));
    g.fillRoundedRectangle (strip, 3.0f);
    g.setColour (status.startsWith ("MATCHED") || dropHint ? Palette::accent : Palette::yellow);
    g.setFont (Fonts::ui (10.0f));
    g.drawText (status, strip, juce::Justification::centred);
}

void ScopeView::drawWaveform (juce::Graphics& g, juce::Rectangle<float> area)
{
    // About 43 ms at 48 kHz, starting on a rising zero crossing so a held note stands still.
    constexpr int shown = 2048;
    processor.copyScope (samples.data(), shown * 2);

    int start = 0;
    for (int i = 1; i < shown; ++i)
        if (samples[(size_t) i - 1] <= 0.0f && samples[(size_t) i] > 0.0f) { start = i; break; }

    g.setColour (Palette::track.brighter (0.15f));
    g.drawHorizontalLine ((int) area.getCentreY(), area.getX(), area.getRight());
    for (const auto gridLevel : { -0.5f, 0.5f })
        g.drawHorizontalLine ((int) (area.getCentreY() - gridLevel * area.getHeight() * 0.5f), area.getX(), area.getRight());

    juce::Path path;
    for (int i = 0; i < shown; ++i)
    {
        const auto x = area.getX() + area.getWidth() * (float) i / (float) (shown - 1);
        const auto y = area.getCentreY() - juce::jlimit (-1.0f, 1.0f, samples[(size_t) (start + i)]) * area.getHeight() * 0.5f;
        i == 0 ? path.startNewSubPath (x, y) : path.lineTo (x, y);
    }
    g.setColour (Palette::accent);
    g.strokePath (path, juce::PathStrokeType (1.5f));
}

void ScopeView::drawSpectrum (juce::Graphics& g, juce::Rectangle<float> area)
{
    constexpr float minHz = 20.0f, maxHz = 20000.0f, floorDb = -90.0f;
    const auto sampleRate = (float) juce::jmax (1.0, processor.getSampleRate());

    auto xFor = [&] (float hz) { return area.getX() + area.getWidth() * std::log (hz / minHz) / std::log (maxHz / minHz); };
    auto yFor = [&] (float db) { return area.getBottom() - area.getHeight() * juce::jlimit (0.0f, 1.0f, (db - floorDb) / -floorDb); };

    g.setFont (Fonts::ui (9.0f));
    for (const auto hz : { 50.0f, 100.0f, 200.0f, 500.0f, 1000.0f, 2000.0f, 5000.0f, 10000.0f })
    {
        g.setColour (Palette::track.brighter (0.15f));
        g.drawVerticalLine ((int) xFor (hz), area.getY(), area.getBottom());
        g.setColour (Palette::text.withAlpha (0.4f));
        g.drawText (hz >= 1000.0f ? juce::String ((int) (hz / 1000.0f)) + "k" : juce::String ((int) hz),
                    juce::Rectangle<float> (xFor (hz) + 2.0f, area.getBottom() - 12.0f, 30.0f, 12.0f), juce::Justification::centredLeft);
    }
    for (const auto db : { -18.0f, -36.0f, -54.0f, -72.0f })
    {
        g.setColour (Palette::track.brighter (0.15f));
        g.drawHorizontalLine ((int) yFor (db), area.getX(), area.getRight());
    }

    juce::Path path;
    path.startNewSubPath (area.getX(), area.getBottom());
    for (size_t i = 1; i < spectrumDb.size(); ++i)
    {
        const auto hz = (float) i * sampleRate / (float) fftSize;
        if (hz < minHz || hz > maxHz)
            continue;
        path.lineTo (xFor (hz), yFor (spectrumDb[i]));
    }
    path.lineTo (area.getRight(), area.getBottom());
    path.closeSubPath();

    g.setColour (Palette::accent.withAlpha (0.25f));
    g.fillPath (path);
    g.setColour (Palette::accent);
    g.strokePath (path, juce::PathStrokeType (1.2f));
}

void ScopeView::drawWavetable (juce::Graphics& g, juce::Rectangle<float> area)
{
    auto param = [this] (const char* id) { return processor.state.getRawParameterValue (id)->load(); };
    const auto bankA = juce::jlimit (0, WavetableBank::numBanks - 1, (int) param (ParamIDs::waveTable));
    const auto bankB = juce::jlimit (0, WavetableBank::numBanks - 1, (int) param (ParamIDs::waveTableB));
    const auto mix = bankB != bankA ? juce::jlimit (0.0f, 1.0f, param (ParamIDs::tableMix)) : 0.0f;
    const auto position = juce::jlimit (0.0f, 1.0f, shownPosition);
    const auto& tablesA = processor.bankFor (bankA);
    const auto& tablesB = processor.bankFor (bankB);

    // The voice's own blend of table A and B, so the picture is what plays.
    auto sample = [&] (float pos, double phase)
    {
        const auto a = tablesA.lookup (bankA, 0, pos, phase);
        return mix > 0.0f ? a + mix * (tablesB.lookup (bankB, 0, pos, phase) - a) : a;
    };

    auto title = area.removeFromTop (16.0f);
    g.setFont (Fonts::ui (11.0f, true));
    g.setColour (Palette::accent.withAlpha (0.75f));
    g.drawText (waveTableNames[bankA].toUpperCase() + (mix > 0.0f ? "  +  " + waveTableNames[bankB].toUpperCase() : juce::String()),
                title, juce::Justification::centredLeft);
    g.setFont (Fonts::mono (11.0f));
    g.setColour (Palette::yellow.withAlpha (0.8f));
    g.drawText ("POS " + juce::String (position, 2), title, juce::Justification::centredRight);

    // Every frame stacked back to front, then the playing position on top, which
    // glows with the output level.
    constexpr int points = 256;
    constexpr int layers = 12;
    const auto depthX = area.getWidth() * 0.18f, depthY = area.getHeight() * 0.3f;
    auto face = area.withTrimmedRight (depthX).withTrimmedTop (depthY);

    auto trace = [&] (float pos, float shift)
    {
        juce::Path path;
        for (int i = 0; i < points; ++i)
        {
            const auto phase = (double) i / (double) points;
            const auto px = face.getX() + shift * depthX + face.getWidth() * (float) i / (float) (points - 1);
            const auto py = face.getCentreY() - shift * depthY - juce::jlimit (-1.0f, 1.0f, sample (pos, phase)) * face.getHeight() * 0.45f;
            i == 0 ? path.startNewSubPath (px, py) : path.lineTo (px, py);
        }
        return path;
    };

    for (int layer = layers - 1; layer >= 0; --layer)
    {
        const auto pos = (float) layer / (float) (layers - 1);
        const auto near = 1.0f - std::abs (pos - position) * 3.0f; // frames near the play position light up
        g.setColour (Palette::accent.withAlpha (0.1f + 0.08f * (1.0f - pos) + 0.25f * juce::jmax (0.0f, near) * (0.4f + level)));
        g.strokePath (trace (pos, pos), juce::PathStrokeType (1.0f));
    }

    const auto current = trace (position, position);
    const auto colour = Palette::orange.interpolatedWith (Palette::yellow, position);
    g.setColour (colour.withAlpha (0.12f + 0.3f * level));
    g.strokePath (current, juce::PathStrokeType (6.0f + 6.0f * level));
    g.setColour (colour);
    g.strokePath (current, juce::PathStrokeType (2.0f));
}

//==============================================================================
PlayingKeyboard::PlayingKeyboard (ZeroEngineProcessor& p)
    : juce::MidiKeyboardComponent (p.keyboardState, horizontalKeyboard), processor (p)
{
    // Played with the mouse only: computer keys belong to the host (in Cubase a
    // key pressed here and released elsewhere left a note held).
    setWantsKeyboardFocus (false);
    clearKeyMappings();
    lights.startTimerHz (30);
}

void PlayingKeyboard::refreshLights()
{
    std::array<juce::uint32, 4> now {};
    for (int note = 0; note < 128; ++note)
        if (processor.isNoteSounding (note))
            now[(size_t) note >> 5] |= 1u << (note & 31);
    if (now != shown)
    {
        shown = now;
        repaint();
    }
}

void PlayingKeyboard::drawWhiteNote (int note, juce::Graphics& g, juce::Rectangle<float> area, bool isDown, bool isOver,
                                     juce::Colour lineColour, juce::Colour textColour)
{
    juce::MidiKeyboardComponent::drawWhiteNote (note, g, area, isDown, isOver, lineColour, textColour);
    if (! isDown && processor.isNoteSounding (note))
    {
        g.setColour (Palette::accent.withAlpha (0.85f));
        g.fillRect (area.reduced (1.0f, 0.0f));
    }
}

void PlayingKeyboard::drawBlackNote (int note, juce::Graphics& g, juce::Rectangle<float> area, bool isDown, bool isOver,
                                     juce::Colour noteFillColour)
{
    juce::MidiKeyboardComponent::drawBlackNote (note, g, area, isDown, isOver, noteFillColour);
    if (! isDown && processor.isNoteSounding (note))
    {
        g.setColour (Palette::accent.withAlpha (0.85f));
        g.fillRect (area.reduced (area.getWidth() * 0.125f, 0.0f).withTrimmedBottom (area.getHeight() * 0.1f));
    }
}

//==============================================================================
SequencerGrid::SequencerGrid (juce::AudioProcessorValueTreeState& state, std::function<int()> step)
    : playingStep (std::move (step))
{
    auto attach = [&] (const juce::String& id, std::function<void (float)> onChange)
    {
        auto attachment = std::make_unique<juce::ParameterAttachment> (*state.getParameter (id), std::move (onChange));
        attachment->sendInitialUpdate();
        return attachment;
    };

    for (int s = 0; s < steps; ++s)
    {
        onAttachments[s]     = attach (SeqIDs::on (s),     [this, s] (float v) { on[s] = v > 0.5f; repaint(); });
        pitchAttachments[s]  = attach (SeqIDs::pitch (s),  [this, s] (float v) { pitch[s] = juce::roundToInt (v); repaint(); });
        accentAttachments[s] = attach (SeqIDs::accent (s), [this, s] (float v) { accent[s] = v > 0.5f; repaint(); });
    }
    lengthAttachment = attach (ParamIDs::seqLength, [this] (float v) { length = juce::roundToInt (v); repaint(); });

    setMouseCursor (juce::MouseCursor::PointingHandCursor);
    startTimerHz (30);
}

void SequencerGrid::timerCallback()
{
    if (const auto now = playingStep(); now != lit)
    {
        lit = now;
        repaint();
    }
}

juce::Rectangle<float> SequencerGrid::cell (int step, Row row) const
{
    // Columns of pads, in groups of four; rows: pad, pitch, accent.
    auto area = getLocalBounds().toFloat();
    const auto groupGap = 8.0f;
    const auto columnWidth = (area.getWidth() - groupGap * 3.0f) / (float) steps;
    const auto x = area.getX() + (float) step * columnWidth + (float) (step / 4) * groupGap;
    const auto column = juce::Rectangle<float> (x, area.getY(), columnWidth, area.getHeight()).reduced (2.0f, 0.0f);

    const auto padHeight = juce::jmin (column.getWidth(), area.getHeight() * 0.45f);
    switch (row)
    {
        case padRow:    return column.withHeight (padHeight);
        case pitchRow:  return column.withY (column.getY() + padHeight + 6.0f).withHeight (22.0f);
        case accentRow: return column.withY (column.getY() + padHeight + 34.0f).withHeight (14.0f);
        default:        return {};
    }
}

std::pair<int, SequencerGrid::Row> SequencerGrid::hit (juce::Point<float> p) const
{
    for (int s = 0; s < steps; ++s)
        for (const auto row : { padRow, pitchRow, accentRow })
            if (cell (s, row).expanded (1.0f, 2.0f).contains (p))
                return { s, row };
    return { -1, noRow };
}

void SequencerGrid::paint (juce::Graphics& g)
{
    for (int s = 0; s < steps; ++s)
    {
        const auto active = s < length;
        const auto playing = s == lit;
        const auto dim = active ? 1.0f : 0.3f;

        // Pad: lit orange-to-yellow when on (brighter on accents), outlined when playing.
        auto pad = cell (s, padRow);
        g.setColour (Palette::screen);
        g.fillRoundedRectangle (pad, 4.0f);
        if (on[s])
        {
            const auto colour = (accent[s] ? Palette::yellow : Palette::orange).withMultipliedAlpha (dim);
            g.setColour (colour.withAlpha (0.25f * dim));
            g.fillRoundedRectangle (pad.expanded (2.0f), 5.0f);
            g.setGradientFill (juce::ColourGradient (colour.brighter (0.3f), pad.getX(), pad.getY(),
                                                     colour.darker (0.3f), pad.getX(), pad.getBottom(), false));
            g.fillRoundedRectangle (pad.reduced (2.0f), 3.0f);
        }
        g.setColour (playing ? Palette::accent : Palette::panelEdge.withMultipliedAlpha (dim));
        g.drawRoundedRectangle (pad, 4.0f, playing ? 2.0f : 1.0f);

        // Step number every four.
        if (s % 4 == 0)
        {
            g.setColour (Palette::text.withAlpha (0.4f * dim));
            g.setFont (Fonts::mono (9.0f));
            g.drawText (juce::String (s + 1), pad.withHeight (12.0f).translated (3.0f, 2.0f), juce::Justification::topLeft);
        }

        // Pitch: drag up or down.
        auto box = cell (s, pitchRow);
        g.setColour (Palette::screen.withAlpha (0.8f));
        g.fillRoundedRectangle (box, 3.0f);
        g.setColour ((pitch[s] == 0 ? Palette::text.withAlpha (0.45f) : Palette::accent).withMultipliedAlpha (dim));
        g.setFont (Fonts::mono (10.5f));
        g.drawText (pitch[s] > 0 ? "+" + juce::String (pitch[s]) : juce::String (pitch[s]), box, juce::Justification::centred);

        // Accent dot.
        const auto dot = juce::Rectangle<float> (8.0f, 8.0f).withCentre (cell (s, accentRow).getCentre());
        g.setColour ((accent[s] ? Palette::yellow : Palette::track.brighter (0.4f)).withMultipliedAlpha (dim));
        g.fillEllipse (dot);
    }

    g.setColour (Palette::text.withAlpha (0.4f));
    g.setFont (Fonts::ui (9.0f, true, 0.06f));
    const auto legend = cell (0, accentRow).withX (0.0f).withWidth ((float) getWidth()).translated (0.0f, 16.0f);
    g.drawText ("PAD: ON / OFF    PITCH: DRAG, DOUBLE-CLICK = 0    DOT: ACCENT", legend, juce::Justification::centredLeft);
}


void SequencerGrid::mouseDown (const juce::MouseEvent& e)
{
    const auto [s, row] = hit (e.position);
    dragStep = -1;
    if (s < 0)
        return;


    if (row == padRow)
        onAttachments[s]->setValueAsCompleteGesture (on[s] ? 0.0f : 1.0f);
    else if (row == accentRow)
        accentAttachments[s]->setValueAsCompleteGesture (accent[s] ? 0.0f : 1.0f);
    else if (row == pitchRow)
    {
        dragStep = s;
        dragStartPitch = pitch[s];
        pitchAttachments[s]->beginGesture();
    }
}

void SequencerGrid::mouseDrag (const juce::MouseEvent& e)
{
    if (dragStep < 0)
        return;
    const auto semitones = juce::jlimit (-12, 12, dragStartPitch - e.getDistanceFromDragStartY() / 6);
    pitchAttachments[dragStep]->setValueAsPartOfGesture ((float) semitones);
}

void SequencerGrid::mouseUp (const juce::MouseEvent&)
{
    if (dragStep >= 0)
        pitchAttachments[dragStep]->endGesture();
    dragStep = -1;
}

void SequencerGrid::mouseDoubleClick (const juce::MouseEvent& e)
{
    if (const auto [s, row] = hit (e.position); s >= 0 && row == pitchRow)
    {
        pitchAttachments[s]->setValueAsCompleteGesture (0.0f);
    }
}

//==============================================================================
ArpScope::ArpScope (juce::AudioProcessorValueTreeState& state, std::function<int()> step, std::function<void (float*, int)> scope)
    : playingStep (std::move (step)), copyScope (std::move (scope))
{
    for (int s = 0; s < steps; ++s)
    {
        on[(size_t) s] = state.getRawParameterValue (SeqIDs::on (s));
        pitch[(size_t) s] = state.getRawParameterValue (SeqIDs::pitch (s));
        accent[(size_t) s] = state.getRawParameterValue (SeqIDs::accent (s));
    }
    length = state.getRawParameterValue (ParamIDs::seqLength);
    setInterceptsMouseClicks (false, false);
    startTimerHz (30); // as Magic Touch: the smoke fades per frame
}

juce::Rectangle<float> ArpScope::plotArea() const
{
    return getLocalBounds().toFloat().reduced (6.0f).withTrimmedBottom (13.0f);
}

void ArpScope::resized()
{
    const auto plot = plotArea();
    ghost = juce::Image (juce::Image::ARGB, juce::jmax (1, (int) plot.getWidth()), juce::jmax (1, (int) plot.getHeight()), true);
}

juce::Path ArpScope::spectrumPath (const std::vector<float>& db) const
{
    // 30 Hz to 16 kHz across, -84 to 0 dB up (a little headroom on top).
    const auto w = (float) ghost.getWidth(), h = (float) ghost.getHeight();
    juce::Path path;
    for (int i = 0; i < points; ++i)
    {
        const auto x = w * (float) i / (float) (points - 1);
        const auto y = h - h * 0.92f * juce::jlimit (0.0f, 1.0f, (db[(size_t) i] + 84.0f) / 84.0f);
        if (i == 0)
            path.startNewSubPath (x, y);
        else
            path.lineTo (x, y);
    }
    return path;
}

juce::Rectangle<float> ArpScope::barFor (int step, float height) const
{
    // A note bar across the lane at the step's pitch (-12..+12 semitones from the
    // middle), in plot coordinates.
    const auto w = (float) ghost.getWidth(), h = (float) ghost.getHeight();
    const auto lane = w / steps;
    const auto y = h * 0.5f - height * h * 0.36f;
    return { lane * (float) step + lane * 0.14f, y - 3.0f, lane * 0.72f, 6.0f };
}

void ArpScope::timerCallback()
{
    if (isShowing())
        update();
}

void ArpScope::update()
{
    if (! ghost.isValid())
        return;

    // The spectrum of the newest samples: peak per point, falling back slowly.
    copyScope (fftData.data(), fftSize);
    window.multiplyWithWindowingTable (fftData.data(), (size_t) fftSize);
    fft.performFrequencyOnlyForwardTransform (fftData.data());
    constexpr double sampleRate = 48000.0; // close enough for drawing
    for (int i = 0; i < points; ++i)
    {
        const auto lo = 30.0 * std::pow (16000.0 / 30.0, (double) i / points);
        const auto hi = 30.0 * std::pow (16000.0 / 30.0, (double) (i + 1) / points);
        // Where a point spans less than a bin (the lows), read between bins; else the
        // loudest bin in its span. Tilted +3 dB an octave around 1 kHz so a full
        // mix reads level, as analysers do.
        const auto centre = (lo + hi) * 0.5 * fftSize / sampleRate;
        const auto b0 = juce::jlimit (1, fftSize / 2 - 2, (int) (lo * fftSize / sampleRate));
        const auto b1 = juce::jlimit (b0, fftSize / 2 - 1, (int) (hi * fftSize / sampleRate));
        float peak = 0.0f;
        if (b1 - b0 < 1)
        {
            const auto i0 = juce::jlimit (1, fftSize / 2 - 2, (int) centre);
            const auto frac = (float) juce::jlimit (0.0, 1.0, centre - i0);
            peak = fftData[(size_t) i0] + frac * (fftData[(size_t) i0 + 1] - fftData[(size_t) i0]);
        }
        else
            for (int b = b0; b <= b1; ++b)
                peak = juce::jmax (peak, fftData[(size_t) b]);
        const auto db = juce::Decibels::gainToDecibels (peak / (fftSize * 0.25f), -100.0f)
                      + 3.0f * (float) std::log2 ((lo + hi) * 0.5 / 1000.0);
        auto& s = spectrum[(size_t) i];
        s = db > s ? db : juce::jmax (db, s - 2.5f);
    }
    for (int i = 0; i < points; ++i) // the slow line follows the softened trace
    {
        float sum = 0.0f;
        for (int k = -3; k <= 3; ++k)
            sum += spectrum[(size_t) juce::jlimit (0, points - 1, i + k)];
        slowAverage[(size_t) i] += 0.06f * (sum / 7.0f - slowAverage[(size_t) i]);
    }

    // The smoke: everything fades a little each frame, then this frame's trace
    // (softened over its neighbours) goes in as Magic Touch draws it (wide faint
    // glow, medium, thin bright).
    std::vector<float> soft (spectrum.size());
    for (int i = 0; i < points; ++i)
    {
        float sum = 0.0f, weight = 0.0f;
        for (int k = -3; k <= 3; ++k)
            if (const auto j = i + k; j >= 0 && j < points)
            {
                const auto w = 4.0f - (float) std::abs (k);
                sum += w * spectrum[(size_t) j];
                weight += w;
            }
        soft[(size_t) i] = sum / weight;
    }
    ghost.multiplyAllAlphas (0.88f);
    juce::Graphics g (ghost);
    const auto trace = spectrumPath (soft);
    const auto smoke = Palette::accent.interpolatedWith (juce::Colours::white, 0.35f);
    g.setColour (smoke.withAlpha (0.05f));
    g.strokePath (trace, juce::PathStrokeType (9.0f));
    g.setColour (smoke.withAlpha (0.10f));
    g.strokePath (trace, juce::PathStrokeType (3.5f));
    g.setColour (smoke.withAlpha (0.22f));
    g.strokePath (trace, juce::PathStrokeType (1.2f));

    // A new step that plays (on, not a rest): its bar and beam go into the smoke
    // (they fade with it, leaving the trail), and it flashes for a moment.
    const auto now = this->now();
    const auto step = playingStep();
    if (step != lastStep && step >= 0 && step < steps && on[(size_t) step]->load() > 0.5f)
        hits.push_back ({ step, now, juce::jlimit (-1.0f, 1.0f, pitch[(size_t) step]->load() / 12.0f), accent[(size_t) step]->load() > 0.5f });
    lastStep = step;
    for (const auto& h : hits)
    {
        if (now - h.start > 0.25)
            continue; // its trail is already laid down
        const auto colour = h.accented ? Palette::orange : Palette::accent;
        const auto bar = barFor (h.step, h.height);
        // A beam of light falling from the bar, narrowing and fading as it goes.
        juce::Path beam;
        const auto top = bar.getBottom(), bottom = (float) ghost.getHeight();
        beam.startNewSubPath (bar.getX() + bar.getWidth() * 0.18f, top);
        beam.lineTo (bar.getRight() - bar.getWidth() * 0.18f, top);
        beam.lineTo (bar.getCentreX() + bar.getWidth() * 0.06f, bottom);
        beam.lineTo (bar.getCentreX() - bar.getWidth() * 0.06f, bottom);
        beam.closeSubPath();
        g.setGradientFill (juce::ColourGradient (colour.withAlpha (0.12f), bar.getCentreX(), top, colour.withAlpha (0.0f), bar.getCentreX(), bottom, false));
        g.fillPath (beam);
        g.setColour (colour.withAlpha (0.06f));
        g.fillRoundedRectangle (bar.expanded (5.0f, 6.0f), 6.0f);
        g.setColour (colour.withAlpha (0.18f));
        g.fillRoundedRectangle (bar.expanded (1.5f, 1.5f), 4.0f);
    }
    hits.erase (std::remove_if (hits.begin(), hits.end(), [now] (const Hit& h) { return now - h.start > 0.6; }), hits.end());
    repaint();
}

void ArpScope::paint (juce::Graphics& g)
{
    const auto box = getLocalBounds().toFloat();
    juce::ColourGradient bg (Palette::screen.brighter (0.04f), 0.0f, box.getY(), Palette::screen.darker (0.4f), 0.0f, box.getBottom(), false);
    g.setGradientFill (bg);
    g.fillRoundedRectangle (box, 5.0f);
    const auto plot = plotArea();
    const auto lane = plot.getWidth() / steps;
    const auto active = juce::jlimit (1, steps, juce::roundToInt (length->load()));
    const auto now = this->now();

    // Lanes: hairlines (brighter every four), steps past Seq Length shaded, the
    // playing lane softly lit from below.
    for (int s = 0; s < steps; ++s)
    {
        const auto r = juce::Rectangle<float> (plot.getX() + (float) s * lane, plot.getY(), lane, plot.getHeight());
        if (s >= active)
        {
            g.setColour (Palette::panel.withAlpha (0.5f));
            g.fillRect (r);
        }
        if (s == lastStep)
        {
            juce::ColourGradient lit (Palette::accent.withAlpha (0.0f), 0.0f, r.getY(), Palette::accent.withAlpha (0.10f), 0.0f, r.getBottom(), false);
            g.setGradientFill (lit);
            g.fillRect (r);
        }
        if (s > 0)
        {
            g.setColour (Palette::panelEdge.withAlpha (s % 4 == 0 ? 0.55f : 0.22f));
            g.drawVerticalLine (juce::roundToInt (r.getX()), r.getY(), r.getBottom());
        }
        g.setColour ((s == lastStep ? Palette::accent : Palette::text).withAlpha (s < active ? 0.75f : 0.3f));
        g.setFont (Fonts::mono (9.0f));
        g.drawText (juce::String (s + 1), juce::Rectangle<float> (r.getX(), plot.getBottom() + 1.0f, lane, 12.0f), juce::Justification::centred);
    }
    g.setColour (Palette::panelEdge.withAlpha (0.3f)); // the pitch centre line
    g.drawHorizontalLine (juce::roundToInt (plot.getCentreY()), plot.getX(), plot.getRight());

    // The smoke, then the slow average as a thin bright line.
    g.drawImageAt (ghost, (int) plot.getX(), (int) plot.getY());
    g.setColour (Palette::accent.interpolatedWith (juce::Colours::white, 0.35f).withAlpha (0.8f));
    g.strokePath (spectrumPath (slowAverage), juce::PathStrokeType (1.0f), juce::AffineTransform::translation (plot.getX(), plot.getY()));

    // Fresh hits flash: the bar white-hot at first, spreading to the lane's
    // width as it cools to its colour.
    for (const auto& h : hits)
    {
        const auto age = (float) (now - h.start);
        const auto heat = std::exp (-age / 0.12f);
        const auto colour = (h.accented ? Palette::orange : Palette::accent).interpolatedWith (juce::Colours::white, 0.6f * heat);
        auto bar = barFor (h.step, h.height).translated (plot.getX(), plot.getY());
        bar = bar.withSizeKeepingCentre (bar.getWidth() * (0.75f + 0.33f * (1.0f - heat)), bar.getHeight());
        const auto fade = std::exp (-age / 0.25f);
        g.setColour (colour.withAlpha (0.25f * fade));
        g.fillRoundedRectangle (bar.expanded (4.0f, 5.0f), 6.0f);
        g.setColour (colour.withAlpha (0.95f * fade));
        g.fillRoundedRectangle (bar, 3.0f);
    }
}


//==============================================================================
juce::Component* AdvancedPanel::Control::widget() const
{
    if (knob != nullptr)   return knob.get();
    if (menu != nullptr)   return menu.get();
    return toggle.get();
}

AdvancedPanel::AdvancedPanel (juce::AudioProcessorValueTreeState& state, std::function<int()> playingStep, std::function<void (float*, int)> scope)
    : grid (std::make_unique<SequencerGrid> (state, playingStep)),
      arpScope (std::make_unique<ArpScope> (state, playingStep, std::move (scope)))
{
    using namespace ParamIDs;
    using Ids = std::vector<const char*>;
    struct SectionSpec { const char* name; Ids ids; };
    struct PageSpec { const char* name; std::vector<std::vector<SectionSpec>> rows; std::vector<int> weights {}; };

    const std::vector<PageSpec> layout {
        // Everything but the arp on one page, a row per stage: tone, voice,
        // movement, mix.
        { "SOUND & MIX", { { { "OSCILLATOR",   { waveTable, waveTableB, tableMix, wavePos, unison, detune, blend } },
                             { "FILTER",       { filterMode, filterCutoff, filterRes, filterEnv, filterKeytrack, velToCutoff } } },
                           { { "AMP ENVELOPE", { ampAttack, ampDecay, ampSustain, ampRelease } },
                             { "SUB",          { subOn, subOctave, subSplit, midHighPass } },
                             { "PLAY",         { playMode, glide, bendRange, velSens } } },
                           { { "MOD ENVELOPE", { modAttack, modDecay, modSustain, modRelease, modToPos, modToPitch } },
                             { "LFO",          { lfoShape, lfoSync, lfoRate, lfoToPos, lfoToPitch, lfoToCutoff } } },
                           { { "SPACE",        { delayTime, delayMix, delayFeedback, reverbMix, reverbSize } },
                             { "FINISH",       { inputTrim, ottDepth, spectral, tilt, monoFreq, stereoWidth, levelTrim } } } } },
        // The pattern on top (the output and each step as it plays), the steps,
        // then the arp's settings.
        { "ARP",    { { { "PATTERN  (the output, and each step as it plays)", {} } },
                      { { "STEP SEQUENCER  (rests, pitch and accents for every arp mode)", {} } },
                      { { "ARPEGGIATOR",  { arpOn, arpLatch, arpMode, arpRate, arpOctaves, arpGate, seqLength } } } },
                    { 9, 6, 5 } },
    };

    auto makeControl = [&] (const char* id) -> std::unique_ptr<Control>
    {
        auto* param = state.getParameter (id);
        if (param == nullptr)
            return {};

        auto control = std::make_unique<Control>();
        control->name = param->getName (24).toUpperCase();

        if (auto* choice = dynamic_cast<juce::AudioParameterChoice*> (param))
        {
            control->menu = std::make_unique<juce::ComboBox>();
            control->menu->addItemList (choice->choices, 1);
            control->menuAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (state, id, *control->menu);
        }
        else if (auto* steps = dynamic_cast<juce::AudioParameterInt*> (param);
                 steps != nullptr && (steps->getRange().getLength() <= 3 || juce::String (id) == ParamIDs::seqLength))
        {
            // A setting with up to four positions (and the sequence length) reads better as a menu.
            const auto lowest = steps->getRange().getStart();
            auto stepName = [id] (int v)
            {
                if (juce::String (id) == ParamIDs::subOctave)
                    return juce::String (v < 0 ? "Octave down" : "At note");
                return juce::String (v);
            };

            control->menu = std::make_unique<juce::ComboBox>();
            for (int v = lowest; v <= steps->getRange().getEnd(); ++v)
                control->menu->addItem (stepName (v), v - lowest + 1);

            auto* menu = control->menu.get();
            control->stepAttachment = std::make_unique<juce::ParameterAttachment> (*param, [menu, lowest] (float value)
            {
                menu->setSelectedId (juce::roundToInt (value) - lowest + 1, juce::dontSendNotification);
            });
            auto* attachment = control->stepAttachment.get();
            menu->onChange = [menu, attachment, lowest]
            {
                attachment->setValueAsCompleteGesture ((float) (menu->getSelectedId() - 1 + lowest));
            };
            attachment->sendInitialUpdate();
        }
        else if (dynamic_cast<juce::AudioParameterBool*> (param) != nullptr)
        {
            control->toggle = std::make_unique<juce::ToggleButton> ("ON");
            control->toggleAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (state, id, *control->toggle);
        }
        else
        {
            control->knob = std::make_unique<juce::Slider> (juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox);
            control->knob->setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
            control->knob->setMouseCursor (juce::MouseCursor::UpDownResizeCursor);
            if (const auto unit = param->getLabel(); unit.isNotEmpty())
                control->knob->setTextValueSuffix (" " + unit); // the knob readout shows it
            control->knobAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (state, id, *control->knob);
            control->knob->setDoubleClickReturnValue (true, param->convertFrom0to1 (param->getDefaultValue()));
        }

        addChildComponent (control->widget());
        return control;
    };

    for (const auto& pageSpec : layout)
    {
        Page page;
        page.name = pageSpec.name;
        page.rowWeights = pageSpec.weights;
        for (const auto& rowSpec : pageSpec.rows)
        {
            std::vector<Section> row;
            for (const auto& sectionSpec : rowSpec)
            {
                Section section;
                section.name = sectionSpec.name;
                for (const auto* id : sectionSpec.ids)
                    if (auto control = makeControl (id))
                    {
                        // The section's title says it once: "CUTOFF" under FILTER,
                        // "ATTACK" under AMP ENVELOPE, "TO PITCH" under LFO.
                        auto& name = control->name;
                        const auto lead = section.name.upToFirstOccurrenceOf (" ", true, false);
                        if (name.startsWith (lead) && name.length() > lead.length())
                            name = name.substring (lead.length()).trimStart();
                        if (name.startsWith (">"))
                            name = "TO" + name.substring (1);
                        name = name.replace ("VELOCITY", "VEL");
                        section.controls.push_back (std::move (control));
                    }
                row.push_back (std::move (section));
            }
            page.rows.push_back (std::move (row));
        }

        page.button = std::make_unique<juce::TextButton> (page.name);
        page.button->setClickingTogglesState (true);
        page.button->setRadioGroupId (0x7ab5);
        const auto index = pages.size();
        page.button->onClick = [this, index] { show (index); };
        addAndMakeVisible (page.button.get());
        pages.push_back (std::move (page));
    }

    // The step grid and the pattern display draw their own sections.
    addChildComponent (grid.get());
    addChildComponent (arpScope.get());
    for (auto& page : pages)
        for (auto& row : page.rows)
            for (auto& section : row)
            {
                if (section.name.startsWith ("STEP SEQUENCER"))
                    section.custom = grid.get();
                if (section.name.startsWith ("PATTERN"))
                    section.custom = arpScope.get();
            }

    show (0);
}

void AdvancedPanel::show (size_t page)
{
    current = juce::jmin (page, pages.size() - 1);
    for (size_t p = 0; p < pages.size(); ++p)
    {
        pages[p].button->setToggleState (p == current, juce::dontSendNotification);
        for (auto& row : pages[p].rows)
            for (auto& section : row)
            {
                for (auto& control : section.controls)
                    control->widget()->setVisible (p == current);
                if (section.custom != nullptr)
                    section.custom->setVisible (p == current);
            }
    }
    resized();
    repaint();
}

void AdvancedPanel::paint (juce::Graphics& g)
{
    // Same plate and header strip as the other panels.
    const auto plate = getLocalBounds().toFloat();
    g.setGradientFill (juce::ColourGradient (Palette::panel.brighter (0.03f), 0.0f, plate.getY(),
                                             Palette::panel.darker (0.08f), 0.0f, plate.getBottom(), false));
    g.fillRoundedRectangle (plate, 7.0f);
    g.setColour (Palette::panelEdge);
    g.drawRoundedRectangle (plate.reduced (0.5f), 7.0f, 1.0f);

    auto strip = plate.withHeight (22.0f).reduced (1.0f, 1.0f);
    g.setColour (Palette::header);
    g.fillRoundedRectangle (strip, 6.0f);
    g.fillRect (strip.withTrimmedTop (8.0f));
    g.setColour (Palette::accent);
    g.setFont (Fonts::ui (11.0f, true));
    g.drawText ("ADVANCED", strip.reduced (10.0f, 0.0f), juce::Justification::centredLeft);

    for (auto& row : pages[current].rows)
        for (auto& section : row)
        {
            // Bordered section: recessed plate, title and LED on its top edge.
            const auto box = section.bounds.toFloat();
            g.setColour (Palette::screen.withAlpha (0.35f));
            g.fillRoundedRectangle (box, 5.0f);
            g.setColour (Palette::panelEdge);
            g.drawRoundedRectangle (box.reduced (0.5f), 5.0f, 1.0f);

            const auto led = juce::Rectangle<float> (5.0f, 5.0f).withCentre ({ box.getX() + 12.0f, box.getY() + 11.0f });
            g.setColour (Palette::accent.withAlpha (0.25f));
            g.fillEllipse (led.expanded (2.5f));
            g.setColour (Palette::accent);
            g.fillEllipse (led);
            g.setFont (Fonts::ui (10.5f, true, 0.08f));
            g.setColour (Palette::orange);
            g.drawText (section.name, box.withHeight (22.0f).withTrimmedLeft (22.0f), juce::Justification::centredLeft);

            // Parameter names above each control.
            g.setFont (Fonts::ui (9.5f, true, 0.05f));
            g.setColour (Palette::accent.withAlpha (0.85f));
            for (auto& control : section.controls)
            {
                const auto r = control->widget()->getBounds();
                g.drawFittedText (control->name, juce::Rectangle<int> (r.getX() - 8, r.getY() - 16, r.getWidth() + 16, 14),
                                  juce::Justification::centredBottom, 1);
            }
        }
}

void AdvancedPanel::resized()
{
    if (pages.empty())
        return;

    // Page tabs across the header strip, right of the title.
    auto strip = getLocalBounds().removeFromTop (22).reduced (6, 3);
    strip.removeFromLeft (80);
    for (auto& page : pages)
        page.button->setBounds (strip.removeFromLeft (110).reduced (2, 0));

    // Rows share the height; sections in a row share the width by control count.
    auto area = getLocalBounds().withTrimmedTop (22).reduced (12, 10);
    auto& rows = pages[current].rows;
    const auto rowGap = 10, sectionGap = 10;
    const auto& weights = pages[current].rowWeights;
    const auto rowSpace = area.getHeight() - rowGap * ((int) rows.size() - 1);
    const auto totalWeight = weights.size() == rows.size() ? std::accumulate (weights.begin(), weights.end(), 0) : (int) rows.size();

    for (size_t r = 0; r < rows.size(); ++r)
    {
        auto& row = rows[r];
        const auto weight = weights.size() == rows.size() ? weights[r] : 1;
        auto rowArea = area.removeFromTop (r + 1 == rows.size() ? area.getHeight() : rowSpace * weight / juce::jmax (1, totalWeight));
        area.removeFromTop (rowGap);

        int total = 0;
        for (auto& section : row)
            total += juce::jmax (1, (int) section.controls.size());
        const auto usable = rowArea.getWidth() - sectionGap * ((int) row.size() - 1);

        for (size_t s = 0; s < row.size(); ++s)
        {
            auto& section = row[s];
            const auto width = s + 1 == row.size() ? rowArea.getWidth()
                                                   : usable * juce::jmax (1, (int) section.controls.size()) / total;
            section.bounds = rowArea.removeFromLeft (width);
            if (section.custom != nullptr)
                section.custom->setBounds (section.bounds.withTrimmedTop (22).reduced (8, 6));
            rowArea.removeFromLeft (sectionGap);

            // Controls in one line under the section title, a name above each.
            auto inner = section.bounds.withTrimmedTop (22).reduced (6, 4);
            const auto count = juce::jmax (1, (int) section.controls.size());
            const auto cellWidth = inner.getWidth() / count;
            for (int i = 0; i < (int) section.controls.size(); ++i)
            {
                auto cell = juce::Rectangle<int> (inner.getX() + i * cellWidth, inner.getY(), cellWidth, inner.getHeight()).reduced (3, 0);
                cell.removeFromTop (16); // the name
                auto& control = section.controls[(size_t) i];

                if (control->knob != nullptr)
                {
                    const auto size = juce::jmin (cell.getWidth(), cell.getHeight(), 104);
                    control->knob->setBounds (cell.withSizeKeepingCentre (cell.getWidth(), size)); // centred in the section
                }
                else
                {
                    control->widget()->setBounds (cell.withSizeKeepingCentre (juce::jmin (cell.getWidth(), 110), 22)
                                                      .withY (cell.getY() + juce::jmax (0, (cell.getHeight() - 22) / 2 - 6)));
                }
            }
        }
    }
}

//==============================================================================
ZeroEngineEditor::ZeroEngineEditor (ZeroEngineProcessor& p)
    : AudioProcessorEditor (p),
      processor (p),
      display (p),
      levelMeter (p.getMeters()),
      advancedPanel (p.state, [&p] { return p.getSequenceStep(); }, [&p] (float* dest, int n) { p.copyScope (dest, n); }),
      keyboard (p)
{
    setLookAndFeel (&lookAndFeel);
    const std::array<std::pair<const char*, const char*>, 6> macroParams { {
        { ParamIDs::weight, "Weight" }, { ParamIDs::grit,   "Grit" },   { ParamIDs::punch, "Punch" },
        { ParamIDs::width,  "Width" },  { ParamIDs::motion, "Motion" }, { ParamIDs::air,   "Air" } } };

    for (size_t i = 0; i < macros.size(); ++i)
        setUpMacro (macros[i], macroParams[i].first, macroParams[i].second);

    // Right-click a macro for MIDI learn.
    addMouseListener (this, true);
    startTimerHz (8);

    matchButton.setTooltip ("Reference Match: pick (or drop) a sound and Zero Engine finds its closest version of it, across every genre and type. "
                            "Single notes and one-instrument loops both work: a loop's notes are played back and compared whole. Takes about 20 seconds.");
    matchButton.onClick = [this]
    {
        matchChooser = std::make_unique<juce::FileChooser> ("Match a sound", juce::File(), "*.wav;*.aif;*.aiff;*.flac;*.mp3;*.ogg");
        matchChooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                                   [this] (const juce::FileChooser& chooser)
                                   {
                                       if (const auto file = chooser.getResult(); file.existsAsFile())
                                           processor.startMatch (file);
                                   });
    };
    addAndMakeVisible (matchButton);

    quickSaveButton.onClick = [this]
    {
        if (const auto name = processor.quickSave(); name.isNotEmpty())
        {
            currentFavourite = name;
            refreshFavourites();
        }
    };
    quickSaveButton.setTooltip ("Quick save: the sound as it is now goes into Favourites under a random name.");
    addAndMakeVisible (quickSaveButton);
    undoButton.onClick = [this] { processor.undo(); };
    redoButton.onClick = [this] { processor.redo(); };
    addAndMakeVisible (undoButton);
    addAndMakeVisible (redoButton);
    setWantsKeyboardFocus (true);

    setUpMacro (outputKnob, ParamIDs::output, "Output");
    setUpMacro (panKnob, ParamIDs::pan, "Pan");
    panKnob.knob.setDoubleClickReturnValue (true, 0.0); // double-click: back to centre
    // Clones of the Advanced page's amp envelope knobs: the same parameters, on the main page.
    {
        const std::pair<const char*, const char*> adsr[] { { ParamIDs::ampAttack, "Attack" }, { ParamIDs::ampDecay, "Decay" },
                                                           { ParamIDs::ampSustain, "Sustain" }, { ParamIDs::ampRelease, "Release" } };
        for (size_t i = 0; i < envelope.size(); ++i)
            setUpMacro (envelope[i], adsr[i].first, adsr[i].second);
    }

    genreBox.addItemList (genreNames, 1);
    typeBox.addItemList (typeNames, 1);
    addAndMakeVisible (genreBox);
    addAndMakeVisible (typeBox);

    genrePrev.onClick = [this] { stepChoice (ParamIDs::genre, -1); };
    genreNext.onClick = [this] { stepChoice (ParamIDs::genre, 1); };
    typePrev.onClick  = [this] { stepChoice (ParamIDs::type, -1); };
    typeNext.onClick  = [this] { stepChoice (ParamIDs::type, 1); };
    for (auto* arrow : { &genrePrev, &genreNext, &typePrev, &typeNext })
        addAndMakeVisible (arrow);
    genreAttachment = std::make_unique<ComboAttachment> (processor.state, ParamIDs::genre, genreBox);
    typeAttachment  = std::make_unique<ComboAttachment> (processor.state, ParamIDs::type, typeBox);

    addAndMakeVisible (display);
    displayMode.addItem ("Wavetable", ScopeView::wavetable);
    displayMode.addItem ("Waveform", ScopeView::waveform);
    displayMode.addItem ("Spectrum", ScopeView::spectrum);
    displayMode.setSelectedId (ScopeView::wavetable, juce::dontSendNotification);
    displayMode.onChange = [this] { display.setMode (displayMode.getSelectedId()); };
    addAndMakeVisible (displayMode);
    addChildComponent (levelMeter); // the main page shows the envelope knobs in its place

    generateButton.onClick = [this]
    {
        // Fold at once; the sound is worked out in the background and lands
        // mid-fold.
        generateButton.fold();
        processor.generateAsync (processor.isMoodMode(), false, [self = juce::Component::SafePointer<ZeroEngineEditor> (this)]
        {
            if (self != nullptr)
                self->refreshHistoryButtons();
        });
    };
    addAndMakeVisible (generateButton);

    // Under the tesseract: MOOD (free, creative: pads, drones, keys... each mood
    // picks its own type) or GENRE (stays in the selected genre and type).
    for (auto* b : { &moodButton, &genreModeButton })
    {
        b->setClickingTogglesState (false);
        addAndMakeVisible (*b);
    }
    moodButton.onClick = [this] { processor.setMoodMode (true); refreshGenerateMode(); };
    genreModeButton.onClick = [this] { processor.setMoodMode (false); refreshGenerateMode(); };
    moodButton.setTooltip ("Generate from moods: lush pads, airy drones, warm keys and more, free of the genres.");
    genreModeButton.setTooltip ("Generate within the genre and type you have selected.");
    refreshGenerateMode();

    // MUTATE: small variations on the sound playing; the thumbs down: less like
    // this, then the next sound. Both fold the tesseract like a Generate click.
    auto whenDone = [self = juce::Component::SafePointer<ZeroEngineEditor> (this)]
    {
        if (self != nullptr)
            self->refreshHistoryButtons();
    };
    mutateButton.onClick = [this, whenDone]
    {
        generateButton.fold();
        processor.generateAsync (processor.isMoodMode(), true, whenDone);
    };
    mutateButton.setTooltip ("Mutate: small variations on the sound playing, keeping its character.");
    addAndMakeVisible (mutateButton);
    dislikeButton.onClick = [this, whenDone]
    {
        processor.dislikeCurrent();
        heartButton.setFilled (processor.isCurrentHearted());
        generateButton.fold();
        processor.generateAsync (processor.isMoodMode(), false, whenDone);
    };
    dislikeButton.setTooltip ("Less like this: Generate learns from it and moves on to the next sound.");
    addAndMakeVisible (dislikeButton);


    heartButton.onClick = [this]
    {
        processor.heartCurrent();
        heartButton.setFilled (processor.isCurrentHearted());
        refreshFavourites(); // the LIKED section
    };
    heartButton.setTooltip ("Heart this sound: both Generate buttons learn what you like from your hearts "
                            "(and from what you play or skip), and breed new sounds from hearted ones. Your last 10 likes are in the Favourites menu.");

    backButton.onClick = [this] { processor.stepHistory (-1); refreshHistoryButtons(); };
    forwardButton.onClick = [this] { processor.stepHistory (1); refreshHistoryButtons(); };
    addAndMakeVisible (backButton);
    addAndMakeVisible (forwardButton);
    refreshHistoryButtons();

    // Favourites: save the current sound (generated or tweaked) and recall it later.
    saveButton.onClick = [this]
    {
        if (const auto name = processor.saveFavourite(); name.isNotEmpty())
        {
            currentFavourite = name;
            refreshFavourites();
        }
    };
    favouritesBox.setTextWhenNothingSelected ("Favourites");
    favouritesBox.setTextWhenNoChoicesAvailable ("Nothing saved yet");
    favouritesBox.onChange = [this]
    {
        const auto id = favouritesBox.getSelectedId();
        if (id >= likeIdBase && id < likeIdBase + recentLikes.size())
        {
            const auto name = recentLikes[id - likeIdBase];
            processor.recallLike (name);
            currentFavourite.clear();
            refreshFavourites();
            favouritesBox.setSelectedId (likeIdBase + recentLikes.indexOf (name), juce::dontSendNotification);
        }
        else if (id == renameId)
            renameCurrentFavourite();
        else if (id == deleteId)
        {
            processor.deleteFavourite (currentFavourite);
            currentFavourite.clear();
            refreshFavourites();
        }
        else if (id > 0)
        {
            currentFavourite = favouritesBox.getText();
            processor.loadFavourite (currentFavourite);
            refreshFavourites(); // so Rename and Delete name it
        }
    };
    addAndMakeVisible (saveButton);
    addAndMakeVisible (favouritesBox);
    refreshFavourites();
    addAndMakeVisible (keyboard);

    // MAIN and ADVANCED switch views; ADVANCED clicked again also goes back to MAIN.
    advancedButton.onClick = [this] { showAdvanced (! advancedPanel.isVisible()); };
    mainButton.onClick = [this] { showAdvanced (false); };
    addAndMakeVisible (mainButton);
    addAndMakeVisible (advancedButton);
    addAndMakeVisible (heartButton);
    addChildComponent (advancedPanel); // last, so it sits over the panels
    showAdvanced (false);

    keyboard.setColour (juce::MidiKeyboardComponent::whiteNoteColourId, juce::Colour (0xffd6daf2));
    keyboard.setColour (juce::MidiKeyboardComponent::blackNoteColourId, juce::Colour (0xff0c0d22));
    keyboard.setColour (juce::MidiKeyboardComponent::keySeparatorLineColourId, Palette::panel);
    keyboard.setColour (juce::MidiKeyboardComponent::keyDownOverlayColourId, Palette::orange.withAlpha (0.85f));
    keyboard.setColour (juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, Palette::accent.withAlpha (0.35f));
    keyboard.setColour (juce::MidiKeyboardComponent::shadowColourId, juce::Colours::black.withAlpha (0.4f));

    // Resizable, keeping proportions; everything scales from the 900 x 600 design.
    setResizable (true, true);
    setResizeLimits (designWidth, designHeight, designWidth * 2, designHeight * 2);
    getConstrainer()->setFixedAspectRatio ((double) designWidth / designHeight);
    setSize (designWidth, designHeight);
}

ZeroEngineEditor::~ZeroEngineEditor()
{
    if (renameDialog != nullptr)
        delete renameDialog.getComponent(); // never outlive the look-and-feel it uses
    setLookAndFeel (nullptr);
}

void ZeroEngineEditor::stepChoice (const char* paramID, int direction)
{
    // Wraps round, and loads the new profile at once rather than on the next timer tick.
    if (auto* choice = dynamic_cast<juce::AudioParameterChoice*> (processor.state.getParameter (paramID)))
    {
        const auto count = choice->choices.size();
        const auto next = (choice->getIndex() + direction + count) % count;
        choice->beginChangeGesture();
        choice->setValueNotifyingHost (choice->convertTo0to1 ((float) next));
        choice->endChangeGesture();
        processor.syncProfile();
    }
}

void ZeroEngineEditor::showAdvanced (bool show)
{
    advancedPanel.setVisible (show);
    advancedPanel.toFront (false);
    advancedButton.setToggleState (show, juce::dontSendNotification);
    mainButton.setToggleState (! show, juce::dontSendNotification);
}

void ZeroEngineEditor::mouseDown (const juce::MouseEvent& e)
{
    if (! e.mods.isPopupMenu())
        return;

    for (size_t m = 0; m < macros.size(); ++m)
        if (e.eventComponent == &macros[m].knob)
        {
            const auto cc = processor.getMacroCC ((int) m);
            juce::PopupMenu menu;
            menu.addItem (1, "MIDI Learn (move a controller knob)");
            menu.addItem (2, cc >= 0 ? "Clear CC " + juce::String (cc) : juce::String ("No CC learned"), cc >= 0);
            menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&macros[m].knob),
                                [this, m] (int choice)
                                {
                                    if (choice == 1) processor.learnMacro ((int) m);
                                    if (choice == 2) processor.clearMacroCC ((int) m);
                                    timerCallback();
                                });
            return;
        }
}

bool ZeroEngineEditor::isInterestedInFileDrag (const juce::StringArray& files)
{
    return files.size() == 1 && juce::File (files[0]).hasFileExtension ("wav;aif;aiff;flac;mp3;ogg");
}

void ZeroEngineEditor::fileDragEnter (const juce::StringArray&, int, int) { display.setDropHint (true); }
void ZeroEngineEditor::fileDragExit (const juce::StringArray&) { display.setDropHint (false); }

void ZeroEngineEditor::filesDropped (const juce::StringArray& files, int, int)
{
    display.setDropHint (false);
    processor.startMatch (juce::File (files[0]));
}

bool ZeroEngineEditor::keyPressed (const juce::KeyPress& key)
{
    const auto command = key.getModifiers().isCommandDown();
    const auto shift = key.getModifiers().isShiftDown();
    const auto code = juce::CharacterFunctions::toUpperCase (key.getTextCharacter() > 31 ? key.getTextCharacter() : (juce::juce_wchar) key.getKeyCode());

    if (command && code == 'Z')
        return shift ? processor.redo() : processor.undo();
    if (command && code == 'Y')
        return processor.redo();
    return false;
}

void ZeroEngineEditor::timerCallback()
{
    undoButton.setEnabled (processor.canUndo());
    redoButton.setEnabled (processor.canRedo());
    heartButton.setFilled (processor.isCurrentHearted());
    refreshGenerateMode(); // a session or favourite may bring its own

    for (size_t m = 0; m < macros.size(); ++m)
    {
        auto& macro = macros[m];
        const auto learning = processor.isLearning ((int) m);
        const auto cc = processor.getMacroCC ((int) m);
        const auto text = learning ? macro.name + "  LEARNING..." : cc >= 0 ? macro.name + "  CC" + juce::String (cc) : macro.name;
        if (macro.label.getText() != text)
        {
            macro.label.setText (text, juce::dontSendNotification);
            macro.label.setColour (juce::Label::textColourId, learning ? Palette::yellow : Palette::accent);
        }
    }
}

void ZeroEngineEditor::refreshHistoryButtons()
{
    backButton.setEnabled (processor.canStepHistory (-1));
    forwardButton.setEnabled (processor.canStepHistory (1));
}

void ZeroEngineEditor::refreshFavourites()
{
    favouritesBox.clear (juce::dontSendNotification);

    // The hearted sounds first, by type (the newest 6 of each), then the saved favourites.
    recentLikes.clear();
    for (int type = 0; type < typeNames.size(); ++type)
    {
        const auto likes = processor.getRecentLikes (type, 6);
        if (likes.isEmpty())
            continue;
        favouritesBox.addSectionHeading ("LIKED " + typeNames[type].toUpperCase());
        for (const auto& name : likes)
        {
            favouritesBox.addItem (name, likeIdBase + recentLikes.size());
            recentLikes.add (name);
        }
    }
    if (! recentLikes.isEmpty())
        favouritesBox.addSectionHeading ("FAVOURITES");
    const auto names = processor.getFavourites();
    favouritesBox.addItemList (names, 1);

    // Managing the current one lives at the bottom of the same list.
    if (const auto index = names.indexOf (currentFavourite); index >= 0)
    {
        favouritesBox.addSeparator();
        favouritesBox.addItem ("Rename \"" + currentFavourite + "\"...", renameId);
        favouritesBox.addItem ("Delete \"" + currentFavourite + "\"", deleteId);
        favouritesBox.setSelectedId (index + 1, juce::dontSendNotification);
    }
}

void ZeroEngineEditor::renameCurrentFavourite()
{
    refreshFavourites(); // put the selection back on the current sound while the box is open

    auto* dialog = new juce::AlertWindow ("Rename favourite", {}, juce::MessageBoxIconType::NoIcon, this);
    dialog->setLookAndFeel (&lookAndFeel);
    dialog->addTextEditor ("name", currentFavourite);
    dialog->addButton ("Rename", 1, juce::KeyPress (juce::KeyPress::returnKey));
    dialog->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));
    renameDialog = dialog;

    // The editor may close while the box is open; the destructor deletes the box,
    // and the callback then finds the editor gone and does nothing.
    juce::Component::SafePointer<ZeroEngineEditor> editor (this);
    dialog->enterModalState (true, juce::ModalCallbackFunction::create ([editor, dialog] (int result)
    {
        if (editor == nullptr)
            return;
        if (result == 1)
            if (const auto name = editor->processor.renameFavourite (editor->currentFavourite, dialog->getTextEditorContents ("name")); name.isNotEmpty())
                editor->currentFavourite = name;
        editor->refreshFavourites();
    }), true);
}

void ZeroEngineEditor::setUpMacro (Macro& macro, const char* paramID, const juce::String& name)
{
    macro.name = name.toUpperCase();
    macro.label.setText (macro.name, juce::dontSendNotification);
    macro.label.setJustificationType (juce::Justification::centred);
    macro.label.setFont (Fonts::ui (12.0f, true));
    macro.label.setColour (juce::Label::textColourId, Palette::accent);
    // 270 degrees of travel, like Potato Kicker; double-click resets to the default.
    macro.knob.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
    macro.knob.setMouseCursor (juce::MouseCursor::UpDownResizeCursor);
    addAndMakeVisible (macro.knob);
    addAndMakeVisible (macro.label);
    if (auto* param = processor.state.getParameter (paramID); param != nullptr && param->getLabel().isNotEmpty())
        macro.knob.setTextValueSuffix (" " + param->getLabel()); // the readout in its unit, as on the Advanced page (envelope times in s)
    macro.attachment = std::make_unique<SliderAttachment> (processor.state, paramID, macro.knob);
    if (auto* param = processor.state.getParameter (paramID))
        macro.knob.setDoubleClickReturnValue (true, param->convertFrom0to1 (param->getDefaultValue()));
}

void ZeroEngineEditor::paint (juce::Graphics& g)
{
    // Indigo to purple, like an anodised panel under coloured light.
    g.setGradientFill (juce::ColourGradient (Palette::backgroundTop, 0.0f, 0.0f,
                                             Palette::backgroundBottom, 0.0f, (float) getHeight(), false));
    g.fillAll();
    g.addTransform (juce::AffineTransform::scale (uiScale())); // drawn at the design size, scaled

    // Title.
    g.setColour (Palette::text);
    g.setFont (Fonts::ui (22.0f, true));
    g.drawText ("ZERO ENGINE", 18, 12, 220, 26, juce::Justification::centredLeft);
    g.setColour (Palette::orange);
    g.setFont (Fonts::ui (10.0f, true));
    g.drawText ("FINISHED-SOUND SYNTHESIZER", 19, 36, 220, 12, juce::Justification::centredLeft);

    // Panels: a raised plate with a header strip, a title and a light-blue LED.
    auto section = [&] (juce::Rectangle<int> r, const juce::String& title)
    {
        if (r.isEmpty())
            return;
        const auto plate = r.toFloat();
        g.setGradientFill (juce::ColourGradient (Palette::panel.brighter (0.03f), 0.0f, plate.getY(),
                                                 Palette::panel.darker (0.08f), 0.0f, plate.getBottom(), false));
        g.fillRoundedRectangle (plate, 7.0f);
        g.setColour (Palette::panelEdge);
        g.drawRoundedRectangle (plate.reduced (0.5f), 7.0f, 1.0f);

        auto strip = plate.withHeight (22.0f).reduced (1.0f, 1.0f);
        g.setColour (Palette::header);
        g.fillRoundedRectangle (strip, 6.0f);
        g.fillRect (strip.withTrimmedTop (8.0f));

        g.setColour (Palette::accent);
        g.setFont (Fonts::ui (11.0f, true));
        g.drawText (title, strip.reduced (10.0f, 0.0f), juce::Justification::centredLeft);

        const auto led = juce::Rectangle<float> (7.0f, 7.0f).withCentre ({ strip.getRight() - 12.0f, strip.getCentreY() });
        g.setColour (Palette::accent.withAlpha (0.25f));
        g.fillEllipse (led.expanded (3.0f));
        g.setColour (Palette::accent);
        g.fillEllipse (led);
    };

    section (displaySection, "DISPLAY");
    section (meterSection, "ENVELOPE");
    section (generateSection, "GENERATE");
    if (! favouritesSection.isEmpty())
        section (favouritesSection, "FAVOURITES");
    section (macroSection, "MACROS");
}

void ZeroEngineEditor::resized()
{
    auto area = juce::Rectangle<int> (designWidth, designHeight).reduced (16);

    auto top = area.removeFromTop (44);
    top.removeFromLeft (130);
    quickSaveButton.setBounds (top.removeFromLeft (72).withSizeKeepingCentre (72, 22));
    top.removeFromLeft (4);
    undoButton.setBounds (top.removeFromLeft (44).withSizeKeepingCentre (44, 22));
    top.removeFromLeft (4);
    redoButton.setBounds (top.removeFromLeft (44).withSizeKeepingCentre (44, 22));
    auto outputArea = top.removeFromRight (104);
    outputKnob.label.setBounds (outputArea.removeFromLeft (60));
    outputKnob.knob.setBounds (outputArea);
    top.removeFromRight (6);
    auto panArea = top.removeFromRight (78);
    panKnob.label.setBounds (panArea.removeFromLeft (34));
    panKnob.knob.setBounds (panArea);
    top.removeFromRight (10);
    advancedButton.setBounds (top.removeFromRight (74).withSizeKeepingCentre (74, 22));
    top.removeFromRight (4);
    mainButton.setBounds (top.removeFromRight (50).withSizeKeepingCentre (50, 22));
    top.removeFromRight (8);
    // Each menu sits between its back / forward arrows.
    typeNext.setBounds (top.removeFromRight (16).withSizeKeepingCentre (16, 22));
    typeBox.setBounds (top.removeFromRight (100).withSizeKeepingCentre (100, 22));
    typePrev.setBounds (top.removeFromRight (16).withSizeKeepingCentre (16, 22));
    top.removeFromRight (8);
    genreNext.setBounds (top.removeFromRight (16).withSizeKeepingCentre (16, 22));
    genreBox.setBounds (top.removeFromRight (112).withSizeKeepingCentre (112, 22));
    genrePrev.setBounds (top.removeFromRight (16).withSizeKeepingCentre (16, 22));

    area.removeFromTop (10);
    keyboard.setBounds (area.removeFromBottom (60));
    area.removeFromBottom (10);

    // Each panel: a 22 px header strip, then its content inset.
    auto content = [] (juce::Rectangle<int> r) { return r.withTrimmedTop (22).reduced (8); };

    macroSection = area.removeFromBottom (140);
    auto knobRow = content (macroSection);
    const auto knobWidth = knobRow.getWidth() / (int) macros.size();
    for (auto& macro : macros)
    {
        auto cell = knobRow.removeFromLeft (knobWidth);
        macro.label.setBounds (cell.removeFromBottom (18));
        macro.knob.setBounds (cell.reduced (4));
    }
    area.removeFromBottom (10);

    // Under Generate: the display (wide) and the meters, side by side.
    auto lower = area.removeFromBottom (150);
    area.removeFromBottom (10);
    displaySection = lower.removeFromLeft (560);
    lower.removeFromLeft (10);
    meterSection = lower;

    // Generate across the full width: the tesseract on a wide screen, MOOD /
    // GENRE under it in the middle, Favourites (the heart, the list, SAVE) on
    // the left of that row.
    generateSection = area;
    favouritesSection = {};
    auto generateArea = content (generateSection);
    auto controls = generateArea.removeFromBottom (26);
    generateArea.removeFromBottom (6);
    generateButton.setBounds (generateArea);
    const auto modeWidth = 96;
    // [MUTATE][MOOD][GENRE] centred, the heart and thumbs down right of GENRE.
    auto modeRow = controls.withSizeKeepingCentre (3 * modeWidth + 12, 24);
    mutateButton.setBounds (modeRow.removeFromLeft (modeWidth));
    modeRow.removeFromLeft (6);
    moodButton.setBounds (modeRow.removeFromLeft (modeWidth));
    modeRow.removeFromLeft (6);
    genreModeButton.setBounds (modeRow);
    heartButton.setBounds (genreModeButton.getRight() + 8, genreModeButton.getY() + 2, 22, 20);
    dislikeButton.setBounds (heartButton.getRight() + 4, genreModeButton.getY() + 1, 22, 22);
    auto favouritesRow = controls.removeFromLeft (300).withSizeKeepingCentre (300, 24);
    saveButton.setBounds (favouritesRow.removeFromRight (50));
    favouritesRow.removeFromRight (4);
    favouritesBox.setBounds (favouritesRow);
    // History arrows sit in the header strip, left of the LED.
    forwardButton.setBounds (generateSection.getRight() - 26 - 18, generateSection.getY() + 3, 18, 16);
    backButton.setBounds (forwardButton.getX() - 20, generateSection.getY() + 3, 18, 16);

    levelMeter.setBounds (content (meterSection));
    // The amp envelope's four knobs, the same parameters the Advanced page sets.
    {
        auto row = content (meterSection);
        const auto cellWidth = row.getWidth() / (int) envelope.size();
        for (auto& knob : envelope)
        {
            auto cell = row.removeFromLeft (cellWidth);
            knob.label.setBounds (cell.removeFromBottom (18));
            knob.knob.setBounds (cell.reduced (4));
        }
    }
    display.setBounds (content (displaySection));
    // The mode menu sits in the header strip, left of the LED.
    displayMode.setBounds (displaySection.getRight() - 26 - 96, displaySection.getY() + 3, 96, 16);
    matchButton.setBounds (displayMode.getX() - 56, displaySection.getY() + 3, 52, 16);
    advancedPanel.setBounds (generateSection.getUnion (macroSection).getUnion (meterSection));

    // Scale every child with the window: layout is done at the design size.
    const auto transform = juce::AffineTransform::scale (uiScale());
    for (auto* child : getChildren())
        if (dynamic_cast<juce::ResizableCornerComponent*> (child) == nullptr) // the resize handle stays put
            child->setTransform (transform);
}

void ZeroEngineEditor::refreshGenerateMode()
{
    const auto mood = processor.isMoodMode();
    moodButton.setToggleState (mood, juce::dontSendNotification);
    genreModeButton.setToggleState (! mood, juce::dontSendNotification);
}
