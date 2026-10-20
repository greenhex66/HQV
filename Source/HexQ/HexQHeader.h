#pragma once
#include "HexQTheme.h"

namespace hexq
{
// Shared header: brand + product name + version + global bypass button.
class HexQHeader : public juce::Component
{
public:
    HexQHeader (const Palette& p, juce::String brandName, juce::String productName, juce::String versionText)
        : palette (p), brand (std::move (brandName)), product (std::move (productName)), version (std::move (versionText))
    {
        bypassButton.setClickingTogglesState (true);
        addAndMakeVisible (bypassButton);
    }

    juce::TextButton& getBypassButton() noexcept { return bypassButton; }

    void paint (juce::Graphics& g) override
    {
        const int h = getHeight();

        g.setColour (palette.border);
        g.fillRect (0, h - 1, getWidth(), 1);

        g.setFont (makeFont (20.0f, true));
        g.setColour (palette.text);
        g.drawText (brand, 20, 0, 80, h, juce::Justification::centredLeft, false);

        g.setColour (palette.accent);
        g.drawText (product, 98, 0, 120, h, juce::Justification::centredLeft, false);

        g.setFont (makeFont (11.0f));
        g.setColour (palette.textDim);
        g.drawText (version, 190, 0, 80, h, juce::Justification::centredLeft, false);
    }

    void resized() override
    {
        bypassButton.setBounds (getWidth() - 16 - 96, (getHeight() - 30) / 2, 96, 30);
    }

private:
    Palette palette;
    juce::String brand, product, version;
    juce::TextButton bypassButton { "BYPASS" };
};
} // namespace hexq
