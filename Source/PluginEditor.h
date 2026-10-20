#pragma once
#include "PluginProcessor.h"
#include "HexQ/HexQLookAndFeel.h"
#include "HexQ/HexQHeader.h"
#include "HexQ/HexQFooter.h"
#include "Vanta/ResponseGraph.h"
#include "Vanta/BandButton.h"
#include <array>
#include <memory>

class VantaAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit VantaAudioProcessorEditor (VantaAudioProcessor&);
    ~VantaAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void selectBand (int band, bool fromButton);
    void attachToBand (int band);
    void syncControlStates();
    void resetAll();

    VantaAudioProcessor& proc;
    hexq::Palette palette;
    hexq::HexQLookAndFeel laf;

    hexq::HexQHeader header;
    vanta::ResponseGraph graph;
    hexq::HexQFooter footer;

    std::array<std::unique_ptr<vanta::BandButton>, vanta::kNumBands> bandButtons;
    juce::Slider freqSlider, gainSlider, qSlider;
    juce::ComboBox typeBox;
    juce::TextButton bandOnButton { "BAND ON" }, resetButton { "RESET ALL" };

    juce::Rectangle<int> panelArea, typeCaptionArea;
    int selectedBand = 2;

    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;
    using BA = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using CA = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    std::unique_ptr<SA> freqAtt, gainAtt, qAtt;
    std::unique_ptr<CA> typeAtt;
    std::unique_ptr<BA> onAtt, bypassAtt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VantaAudioProcessorEditor)
};
