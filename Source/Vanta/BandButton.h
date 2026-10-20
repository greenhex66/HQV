#pragma once
#include "../HexQ/HexQTheme.h"

namespace vanta
{
// Band selector: number + a dot that shows whether the band is switched on.
class BandButton : public juce::Button
{
public:
    BandButton (const hexq::Palette& p, int number)
        : juce::Button ("band" + juce::String (number)), palette (p), bandNumber (number)
    {
        setClickingTogglesState (true);
        setRadioGroupId (1001);
    }

    void setBandActive (bool shouldBeActive)
    {
        if (shouldBeActive != active)
        {
            active = shouldBeActive;
            repaint();
        }
    }

    void paintButton (juce::Graphics& g, bool hover, bool) override
    {
        auto r = getLocalBounds().toFloat().reduced (0.5f);
        const bool selected = getToggleState();

        g.setColour (selected ? palette.accent.withAlpha (0.14f) : palette.panel);
        g.fillRoundedRectangle (r, hexq::layout::controlCorner);
        g.setColour (selected ? palette.accent : (hover ? palette.textDim : palette.border));
        g.drawRoundedRectangle (r, hexq::layout::controlCorner, 1.0f);

        g.setFont (hexq::makeFont (13.0f, true));
        g.setColour (selected ? palette.accent : palette.text);
        g.drawText (juce::String (bandNumber), r.withTrimmedLeft (14.0f), juce::Justification::centred, false);

        const juce::Rectangle<float> dot (r.getX() + 12.0f, r.getCentreY() - 3.0f, 6.0f, 6.0f);
        if (active)
        {
            g.setColour (palette.accent);
            g.fillEllipse (dot);
        }
        else
        {
            g.setColour (palette.border);
            g.drawEllipse (dot, 1.0f);
        }
    }

private:
    hexq::Palette palette;
    int bandNumber;
    bool active = false;
};
} // namespace vanta
