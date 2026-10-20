#include "HexQLookAndFeel.h"
#include <algorithm>
#include <cmath>

namespace hexq
{
HexQLookAndFeel::HexQLookAndFeel (const Palette& p) : palette (p)
{
    setColour (juce::ResizableWindow::backgroundColourId, palette.background);

    setColour (juce::Slider::textBoxTextColourId, palette.text);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxHighlightColourId, palette.accent.withAlpha (0.35f));

    setColour (juce::Label::textColourId, palette.text);
    setColour (juce::Label::textWhenEditingColourId, palette.text);
    setColour (juce::Label::backgroundWhenEditingColourId, palette.background);
    setColour (juce::Label::outlineWhenEditingColourId, palette.accent);

    setColour (juce::TextEditor::backgroundColourId, palette.background);
    setColour (juce::TextEditor::textColourId, palette.text);
    setColour (juce::TextEditor::highlightColourId, palette.accent.withAlpha (0.35f));
    setColour (juce::TextEditor::highlightedTextColourId, palette.text);
    setColour (juce::TextEditor::outlineColourId, palette.border);
    setColour (juce::TextEditor::focusedOutlineColourId, palette.accent);
    setColour (juce::CaretComponent::caretColourId, palette.accent);

    setColour (juce::ComboBox::backgroundColourId, palette.panel);
    setColour (juce::ComboBox::textColourId, palette.text);
    setColour (juce::ComboBox::outlineColourId, palette.border);
    setColour (juce::ComboBox::arrowColourId, palette.textDim);

    setColour (juce::PopupMenu::backgroundColourId, palette.panel);
    setColour (juce::PopupMenu::textColourId, palette.text);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, palette.accent.withAlpha (0.25f));
    setColour (juce::PopupMenu::highlightedTextColourId, palette.text);
}

void HexQLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&,
                                            bool hover, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (0.5f);
    const bool on = b.getToggleState();

    auto fill = on ? palette.accent.withAlpha (0.14f) : palette.panel;
    if (down)       fill = fill.brighter (0.12f);
    else if (hover) fill = fill.brighter (0.05f);

    g.setColour (fill);
    g.fillRoundedRectangle (r, layout::controlCorner);
    g.setColour (on ? palette.accent : (hover ? palette.textDim : palette.border));
    g.drawRoundedRectangle (r, layout::controlCorner, 1.0f);
}

void HexQLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& b, bool hover, bool)
{
    g.setFont (makeFont (12.0f, true));
    g.setColour (b.getToggleState() ? palette.accent : (hover ? palette.text : palette.textDim));
    g.drawText (b.getButtonText(), b.getLocalBounds(), juce::Justification::centred, false);
}

juce::Font HexQLookAndFeel::getTextButtonFont (juce::TextButton&, int)
{
    return makeFont (12.0f, true);
}

void HexQLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                        float, float, juce::Slider& s)
{
    const float alpha = s.isEnabled() ? 1.0f : 0.35f;
    auto r = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h);

    g.setColour (palette.textDim.withMultipliedAlpha (alpha));
    g.setFont (makeFont (11.0f, true));
    g.drawText (s.getName().toUpperCase(), r.removeFromTop (16.0f), juce::Justification::centredLeft, false);

    const float trackH = 4.0f;
    const float trackY = r.getBottom() - 8.0f;
    const juce::Rectangle<float> track (r.getX(), trackY, r.getWidth(), trackH);

    g.setColour (palette.border.withMultipliedAlpha (alpha));
    g.fillRoundedRectangle (track, 2.0f);

    float x0 = track.getX();
    const float x1 = track.getX() + pos * track.getWidth();
    if (s.getMinimum() < 0.0 && s.getMaximum() > 0.0)            // bipolar (gain): fill from the centre
        x0 = track.getX() + (float) s.valueToProportionOfLength (0.0) * track.getWidth();

    g.setColour (palette.accent.withMultipliedAlpha (alpha));
    g.fillRoundedRectangle (juce::Rectangle<float> (std::min (x0, x1), trackY, std::abs (x1 - x0), trackH), 2.0f);
    g.fillEllipse (x1 - 5.0f, track.getCentreY() - 5.0f, 10.0f, 10.0f);
}

juce::Label* HexQLookAndFeel::createSliderTextBox (juce::Slider& s)
{
    auto* l = juce::LookAndFeel_V4::createSliderTextBox (s);
    l->setFont (makeFont (17.0f, true));
    l->setJustificationType (juce::Justification::centredLeft);
    l->setBorderSize (juce::BorderSize<int> (0, 0, 0, 0));
    l->setColour (juce::TextEditor::backgroundColourId, palette.background);
    l->setColour (juce::TextEditor::outlineColourId, palette.accent);
    l->setColour (juce::TextEditor::focusedOutlineColourId, palette.accent);
    return l;
}

void HexQLookAndFeel::drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& box)
{
    auto r = juce::Rectangle<float> (0.0f, 0.0f, (float) w, (float) h).reduced (0.5f);
    g.setColour (palette.panel.brighter (0.04f));
    g.fillRoundedRectangle (r, layout::controlCorner);
    g.setColour (box.hasKeyboardFocus (true) ? palette.accent : palette.border);
    g.drawRoundedRectangle (r, layout::controlCorner, 1.0f);

    juce::Path arrow;
    const float cx = (float) w - 16.0f, cy = (float) h * 0.5f;
    arrow.addTriangle (cx - 4.0f, cy - 2.0f, cx + 4.0f, cy - 2.0f, cx, cy + 3.0f);
    g.setColour (palette.textDim);
    g.fillPath (arrow);
}

juce::Font HexQLookAndFeel::getComboBoxFont (juce::ComboBox&)
{
    return makeFont (14.0f, true);
}

void HexQLookAndFeel::positionComboBoxText (juce::ComboBox& box, juce::Label& label)
{
    label.setBounds (10, 1, box.getWidth() - 30, box.getHeight() - 2);
    label.setFont (getComboBoxFont (box));
}

juce::Font HexQLookAndFeel::getPopupMenuFont()
{
    return makeFont (14.0f);
}
} // namespace hexq
