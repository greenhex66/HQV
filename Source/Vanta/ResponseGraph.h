#pragma once
#include "../PluginProcessor.h"
#include "../HexQ/HexQTheme.h"
#include <array>
#include <functional>

namespace vanta
{
// Frequency-response graph (NOT a spectrum analyser). Draws the exact combined response
// computed from the same biquad coefficients the audio engine uses, and lets the user drag
// the six band nodes. All DSP here is evaluation of the filter formula on the message thread.
class ResponseGraph : public juce::Component,
                      private juce::AsyncUpdater,
                      private juce::AudioProcessorValueTreeState::Listener
{
public:
    struct BandView
    {
        bool on = false;
        hexq::dsp::FilterType type = hexq::dsp::FilterType::Bell;
        float freq = 1000.0f, gain = 0.0f, q = 0.707f;
        hexq::dsp::Coeffs c;
    };

    ResponseGraph (VantaAudioProcessor&, const hexq::Palette&);
    ~ResponseGraph() override;

    void setSelectedBand (int band);
    int getSelectedBand() const noexcept { return selected; }
    const BandView& getBandView (int i) const noexcept { return model[(size_t) i]; }

    std::function<void (int)> onBandSelected;    // user clicked / dragged a node
    std::function<void()> onModelChanged;        // any parameter changed (message thread)

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

private:
    void parameterChanged (const juce::String&, float) override { triggerAsyncUpdate(); }
    void handleAsyncUpdate() override;

    void refreshModel();
    void rebuildCurve();
    double graphSampleRate() const;

    float freqToX (float f) const noexcept;
    float xToFreq (float x) const noexcept;
    float dbToY (float db) const noexcept;
    float yToDb (float y) const noexcept;
    juce::Point<float> nodePos (int band) const;
    int hitTest (juce::Point<float> p) const;

    void setParam (const juce::String& id, float plainValue);
    void beginGestures (int band);
    void endGestures (int band);
    void selectBand (int band, bool notify);

    VantaAudioProcessor& proc;
    hexq::Palette palette;

    std::array<BandView, kNumBands> model;
    bool bypassed = false;
    juce::Rectangle<float> plot;
    juce::Path curvePath, fillPath;
    juce::StringArray listenedIds;

    int selected = 2;
    int hover = -1;
    int dragBand = -1;
    juce::Point<float> anchorMouse, anchorNode;
    bool lastFine = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ResponseGraph)
};
} // namespace vanta
