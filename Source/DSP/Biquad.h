#pragma once
// HEX-Q VANTA - biquad filter maths. Pure C++ (no JUCE), so it can be unit-tested offline.
#include <algorithm>
#include <cmath>

namespace hexq::dsp
{

enum class FilterType : int { HighPass = 0, LowShelf = 1, Bell = 2, HighShelf = 3, LowPass = 4 };

// Normalised coefficients (a0 == 1). H(z) = (b0 + b1 z^-1 + b2 z^-2) / (1 + a1 z^-1 + a2 z^-2)
struct Coeffs
{
    double b0 = 1.0, b1 = 0.0, b2 = 0.0, a1 = 0.0, a2 = 0.0;
};

struct BiquadState
{
    double z1 = 0.0, z2 = 0.0;
};

constexpr double kMinFreq = 20.0;
constexpr double kMaxFreq = 20000.0;
constexpr double kMinGainDb = -18.0;
constexpr double kMaxGainDb = 18.0;
constexpr double kMinQ = 0.1;
constexpr double kMaxQ = 10.0;

// RBJ Audio-EQ-Cookbook designs. Inputs are clamped to the plugin's legal ranges, and the
// frequency is additionally kept below 0.45 * sampleRate so the design stays well behaved.
inline Coeffs makeCoeffs (FilterType type, double sampleRate, double freq, double gainDb, double q) noexcept
{
    constexpr double pi = 3.14159265358979323846;

    const double fMax = std::max (kMinFreq, std::min (kMaxFreq, 0.45 * sampleRate));
    freq   = std::clamp (freq, kMinFreq, fMax);
    q      = std::clamp (q, kMinQ, kMaxQ);
    gainDb = std::clamp (gainDb, kMinGainDb, kMaxGainDb);

    const double w0 = 2.0 * pi * freq / sampleRate;
    const double cw = std::cos (w0);
    const double sw = std::sin (w0);
    const double alpha = sw / (2.0 * q);
    const double A = std::pow (10.0, gainDb / 40.0);
    const double sqrtA2alpha = 2.0 * std::sqrt (A) * alpha;

    double b0 = 1.0, b1 = 0.0, b2 = 0.0, a0 = 1.0, a1 = 0.0, a2 = 0.0;

    switch (type)
    {
        case FilterType::HighPass:
            b0 = (1.0 + cw) * 0.5;
            b1 = -(1.0 + cw);
            b2 = (1.0 + cw) * 0.5;
            a0 = 1.0 + alpha;
            a1 = -2.0 * cw;
            a2 = 1.0 - alpha;
            break;

        case FilterType::LowPass:
            b0 = (1.0 - cw) * 0.5;
            b1 = 1.0 - cw;
            b2 = (1.0 - cw) * 0.5;
            a0 = 1.0 + alpha;
            a1 = -2.0 * cw;
            a2 = 1.0 - alpha;
            break;

        case FilterType::LowShelf:
            b0 = A * ((A + 1.0) - (A - 1.0) * cw + sqrtA2alpha);
            b1 = 2.0 * A * ((A - 1.0) - (A + 1.0) * cw);
            b2 = A * ((A + 1.0) - (A - 1.0) * cw - sqrtA2alpha);
            a0 = (A + 1.0) + (A - 1.0) * cw + sqrtA2alpha;
            a1 = -2.0 * ((A - 1.0) + (A + 1.0) * cw);
            a2 = (A + 1.0) + (A - 1.0) * cw - sqrtA2alpha;
            break;

        case FilterType::HighShelf:
            b0 = A * ((A + 1.0) + (A - 1.0) * cw + sqrtA2alpha);
            b1 = -2.0 * A * ((A - 1.0) + (A + 1.0) * cw);
            b2 = A * ((A + 1.0) + (A - 1.0) * cw - sqrtA2alpha);
            a0 = (A + 1.0) - (A - 1.0) * cw + sqrtA2alpha;
            a1 = 2.0 * ((A - 1.0) - (A + 1.0) * cw);
            a2 = (A + 1.0) - (A - 1.0) * cw - sqrtA2alpha;
            break;

        case FilterType::Bell:
        default:
            b0 = 1.0 + alpha * A;
            b1 = -2.0 * cw;
            b2 = 1.0 - alpha * A;
            a0 = 1.0 + alpha / A;
            a1 = -2.0 * cw;
            a2 = 1.0 - alpha / A;
            break;
    }

    Coeffs c;
    c.b0 = b0 / a0;
    c.b1 = b1 / a0;
    c.b2 = b2 / a0;
    c.a1 = a1 / a0;
    c.a2 = a2 / a0;
    return c;
}

// Transposed direct form II - robust when coefficients change between sub-blocks.
inline double processSample (const Coeffs& c, BiquadState& s, double x) noexcept
{
    const double y = c.b0 * x + s.z1;
    s.z1 = c.b1 * x - c.a1 * y + s.z2;
    s.z2 = c.b2 * x - c.a2 * y;
    return y;
}

inline void flushDenormals (BiquadState& s) noexcept
{
    if (std::abs (s.z1) < 1.0e-30) s.z1 = 0.0;
    if (std::abs (s.z2) < 1.0e-30) s.z2 = 0.0;
}

// Exact magnitude response of one biquad in dB. Used by the graph - the same coefficients
// that process audio are used to draw the curve.
inline double magnitudeDb (const Coeffs& c, double sampleRate, double freq) noexcept
{
    constexpr double pi = 3.14159265358979323846;
    const double w = 2.0 * pi * freq / sampleRate;
    const double cw = std::cos (w);
    const double c2w = std::cos (2.0 * w);

    const double num = c.b0 * c.b0 + c.b1 * c.b1 + c.b2 * c.b2
                     + 2.0 * (c.b0 * c.b1 + c.b1 * c.b2) * cw
                     + 2.0 * c.b0 * c.b2 * c2w;
    const double den = 1.0 + c.a1 * c.a1 + c.a2 * c.a2
                     + 2.0 * (c.a1 + c.a1 * c.a2) * cw
                     + 2.0 * c.a2 * c2w;

    return 10.0 * std::log10 (std::max (num, 1.0e-30) / std::max (den, 1.0e-30));
}

} // namespace hexq::dsp
