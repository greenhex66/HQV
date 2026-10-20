#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

// HEX-Q shared design system: palette, spacing, typography.
// Every HEX-Q plugin takes its look from here - change the accent, keep the rest.
namespace hexq
{
struct Palette
{
    juce::Colour background { 0xff17191f };
    juce::Colour panel      { 0xff222630 };
    juce::Colour border     { 0xff414550 };
    juce::Colour accent     { 0xffffb547 };   // per-plugin accent (VANTA = amber)
    juce::Colour text       { 0xffe6e8ec };
    juce::Colour textDim    { 0xff9297a2 };
};

namespace layout
{
    constexpr int margin = 16;
    constexpr int gap = 12;
    constexpr int headerHeight = 56;
    constexpr int footerHeight = 30;
    constexpr float corner = 8.0f;
    constexpr float controlCorner = 6.0f;
}

inline juce::Font makeFont (float height, bool bold = false)
{
#if JUCE_MAJOR_VERSION >= 8
    return juce::Font (juce::FontOptions (height, bold ? juce::Font::bold : juce::Font::plain));
#else
    return juce::Font (height, bold ? juce::Font::bold : juce::Font::plain);
#endif
}
} // namespace hexq
