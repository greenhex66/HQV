#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "../DSP/EqEngine.h"

// HEX-Q VANTA - parameter definitions (IDs, ranges, defaults, text formatting).
namespace vanta
{
constexpr int kNumBands = 6;

struct BandDefault
{
    hexq::dsp::FilterType type;
    float freq;
    float gain;
    float q;
    bool enabled;
};

const BandDefault& bandDefault (int band);

// e.g. bandParamId (0, "freq") -> "b1_freq".  Suffixes: on, type, freq, gain, q
juce::String bandParamId (int band, const char* suffix);
inline juce::String bypassId() { return "bypass"; }

juce::StringArray filterTypeNames();

juce::String formatFreq (float hz);
juce::String formatGain (float db);
juce::String formatQ (float q);

juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

// Message-thread helper (builds Strings - do NOT call from the audio thread).
hexq::dsp::BandSettings readBandSettings (juce::AudioProcessorValueTreeState& apvts, int band);

// Cached raw pointers for the audio thread.
struct BandParamPtrs
{
    std::atomic<float>* on = nullptr;
    std::atomic<float>* type = nullptr;
    std::atomic<float>* freq = nullptr;
    std::atomic<float>* gain = nullptr;
    std::atomic<float>* q = nullptr;
};

BandParamPtrs getBandParamPtrs (juce::AudioProcessorValueTreeState& apvts, int band);
} // namespace vanta
