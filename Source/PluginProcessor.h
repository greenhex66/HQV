#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include "DSP/EqEngine.h"
#include "Parameters/VantaParams.h"

class VantaAudioProcessor : public juce::AudioProcessor
{
public:
    VantaAudioProcessor();
    ~VantaAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "HEX-Q VANTA"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

private:
    std::array<vanta::BandParamPtrs, vanta::kNumBands> bandPtrs;
    std::atomic<float>* bypassPtr = nullptr;

    hexq::dsp::EqEngine engine;
    juce::AudioBuffer<float> dryBuffer;     // pre-allocated in prepareToPlay

    float bypassMix = 0.0f;                 // 0 = processed, 1 = bypassed (smoothed)
    float bypassStep = 0.001f;
    bool engineAsleep = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VantaAudioProcessor)
};
