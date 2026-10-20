#pragma once
#include "HexQTheme.h"

namespace hexq
{
// Shared footer: one line of usage hints on the left, brand on the right.
class HexQFooter : public juce::Component
{
public:
    HexQFooter (const Palette& p, juce::String hintText, juce::String brandText = "HEX-Q AUDIO")
        : palette (p), hint (std::move (hintText)), brand (std::move (brandText)) {}

    void paint (juce::Graphics& g) override
    {
        g.setFont (makeFont (10.5f));
        g.setColour (palette.textDim);
        g.drawText (hint, 20, 0, getWidth() - 160, getHeight(), juce::Justification::centredLeft, false);
        g.setColour (palette.border.brighter (0.4f));
        g.drawText (brand, getWidth() - 140, 0, 120, getHeight(), juce::Justification::centredRight, false);
    }

private:
    Palette palette;
    juce::String hint, brand;
};
} // namespace hexq
