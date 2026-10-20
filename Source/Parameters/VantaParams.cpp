#include "VantaParams.h"
#include <cmath>

namespace vanta
{
using hexq::dsp::FilterType;

// Bands 1 and 6 (High-Pass / Low-Pass) start switched OFF so a freshly loaded plugin is
// perfectly neutral. Bands 2-5 start ON at 0 dB, which is also an exact identity.
static const BandDefault kDefaults[kNumBands] = {
    { FilterType::HighPass,  25.0f,    0.0f, 0.707f, false },
    { FilterType::LowShelf,  120.0f,   0.0f, 0.707f, true  },
    { FilterType::Bell,      220.0f,   0.0f, 1.0f,   true  },
    { FilterType::Bell,      500.0f,   0.0f, 1.0f,   true  },
    { FilterType::HighShelf, 8000.0f,  0.0f, 0.707f, true  },
    { FilterType::LowPass,   20000.0f, 0.0f, 0.707f, false },
};

const BandDefault& bandDefault (int band)
{
    return kDefaults[juce::jlimit (0, kNumBands - 1, band)];
}

juce::String bandParamId (int band, const char* suffix)
{
    return "b" + juce::String (band + 1) + "_" + suffix;
}

juce::StringArray filterTypeNames()
{
    return { "High-Pass", "Low Shelf", "Bell", "High Shelf", "Low-Pass" };
}

juce::String formatFreq (float hz)
{
    if (hz >= 1000.0f)
        return juce::String (hz / 1000.0f, 2) + " kHz";
    return juce::String (hz, hz < 100.0f ? 1 : 0) + " Hz";
}

juce::String formatGain (float db)
{
    return juce::String (db > 0.05f ? "+" : "") + juce::String (db, 1) + " dB";
}

juce::String formatQ (float q)
{
    return juce::String (q, 2);
}

static float parseFreq (const juce::String& text)
{
    auto s = text.trim().toLowerCase();
    float v = s.getFloatValue();
    if (s.contains ("k"))
        v *= 1000.0f;
    return juce::jlimit (20.0f, 20000.0f, v);
}

static juce::NormalisableRange<float> logRange (float lo, float hi)
{
    return juce::NormalisableRange<float> (
        lo, hi,
        [] (float s, float e, float p) { return s * std::pow (e / s, p); },
        [] (float s, float e, float v) { return std::log (juce::jlimit (s, e, v) / s) / std::log (e / s); },
        [] (float s, float e, float v) { return juce::jlimit (s, e, v); });
}

juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<AudioParameterBool> (ParameterID { bypassId(), 1 }, "Bypass", false));

    auto freqAttr = AudioParameterFloatAttributes()
                        .withStringFromValueFunction ([] (float v, int) { return formatFreq (v); })
                        .withValueFromStringFunction ([] (const String& t) { return parseFreq (t); });
    auto gainAttr = AudioParameterFloatAttributes()
                        .withStringFromValueFunction ([] (float v, int) { return formatGain (v); })
                        .withValueFromStringFunction ([] (const String& t) { return jlimit (-18.0f, 18.0f, t.getFloatValue()); });
    auto qAttr = AudioParameterFloatAttributes()
                        .withStringFromValueFunction ([] (float v, int) { return formatQ (v); })
                        .withValueFromStringFunction ([] (const String& t) { return jlimit (0.1f, 10.0f, t.getFloatValue()); });

    for (int b = 0; b < kNumBands; ++b)
    {
        const auto& d = kDefaults[b];
        const String name = "Band " + String (b + 1) + " ";

        layout.add (std::make_unique<AudioParameterBool> (ParameterID { bandParamId (b, "on"), 1 }, name + "On", d.enabled));
        layout.add (std::make_unique<AudioParameterChoice> (ParameterID { bandParamId (b, "type"), 1 }, name + "Type",
                                                            filterTypeNames(), (int) d.type));
        layout.add (std::make_unique<AudioParameterFloat> (ParameterID { bandParamId (b, "freq"), 1 }, name + "Freq",
                                                           logRange (20.0f, 20000.0f), d.freq, freqAttr));
        layout.add (std::make_unique<AudioParameterFloat> (ParameterID { bandParamId (b, "gain"), 1 }, name + "Gain",
                                                           NormalisableRange<float> (-18.0f, 18.0f, 0.01f), d.gain, gainAttr));
        layout.add (std::make_unique<AudioParameterFloat> (ParameterID { bandParamId (b, "q"), 1 }, name + "Q",
                                                           logRange (0.1f, 10.0f), d.q, qAttr));
    }

    return layout;
}

hexq::dsp::BandSettings readBandSettings (juce::AudioProcessorValueTreeState& apvts, int band)
{
    hexq::dsp::BandSettings s;
    s.enabled = apvts.getRawParameterValue (bandParamId (band, "on"))->load() > 0.5f;
    s.type    = (FilterType) juce::jlimit (0, 4, (int) apvts.getRawParameterValue (bandParamId (band, "type"))->load());
    s.freq    = apvts.getRawParameterValue (bandParamId (band, "freq"))->load();
    s.gainDb  = apvts.getRawParameterValue (bandParamId (band, "gain"))->load();
    s.q       = apvts.getRawParameterValue (bandParamId (band, "q"))->load();
    return s;
}

BandParamPtrs getBandParamPtrs (juce::AudioProcessorValueTreeState& apvts, int band)
{
    BandParamPtrs p;
    p.on   = apvts.getRawParameterValue (bandParamId (band, "on"));
    p.type = apvts.getRawParameterValue (bandParamId (band, "type"));
    p.freq = apvts.getRawParameterValue (bandParamId (band, "freq"));
    p.gain = apvts.getRawParameterValue (bandParamId (band, "gain"));
    p.q    = apvts.getRawParameterValue (bandParamId (band, "q"));
    return p;
}
} // namespace vanta
