#pragma once
// HEX-Q VANTA - six-band EQ engine. Pure C++ (no JUCE). No heap allocation anywhere.
#include "Biquad.h"
#include <array>

namespace hexq::dsp
{

struct BandSettings
{
    bool enabled = false;
    FilterType type = FilterType::Bell;
    float freq = 1000.0f;
    float gainDb = 0.0f;
    float q = 0.707f;
};

namespace detail
{
    // One-pole smoother that snaps exactly onto its target (so "identity" can be detected).
    struct Smoothed
    {
        float current = 0.0f, target = 0.0f, eps = 1.0e-4f;

        void snap() noexcept { current = target; }

        bool step (float coef) noexcept
        {
            if (current == target)
                return false;

            current += (target - current) * coef;
            if (std::abs (target - current) < eps)
                current = target;
            return true;
        }
    };
}

class EqBand
{
public:
    void prepare (double sr) noexcept
    {
        sampleRate = sr;
        rampStep = (float) (1.0 / (0.012 * sr));   // 12 ms fade for enable / type change
        freqS.eps = 1.0e-4f;
        qS.eps = 1.0e-4f;
        gainS.eps = 1.0e-3f;
        reset();
    }

    void reset() noexcept
    {
        stateValid = false;
        mix = 0.0f;
        clearState();
    }

    void setTarget (const BandSettings& s) noexcept
    {
        wanted = s;
        freqS.target = std::log (std::clamp (s.freq, 20.0f, 20000.0f));
        gainS.target = std::clamp (s.gainDb, -18.0f, 18.0f);
        qS.target    = std::log (std::clamp (s.q, 0.1f, 10.0f));
    }

    void process (float* const* data, int numCh, int n, float smoothCoef) noexcept
    {
        if (! stateValid)
        {
            // Dormant band: just follow the target silently, wake up when enabled.
            type = wanted.type;
            snapSmoothers();
            if (! wanted.enabled)
                return;

            clearState();
            mix = 0.0f;
            stateValid = true;
        }

        const bool typeMatch = (type == wanted.type);
        const float desired = (wanted.enabled && typeMatch) ? 1.0f : 0.0f;

        bool moving = freqS.step (smoothCoef);
        moving |= gainS.step (smoothCoef);
        moving |= qS.step (smoothCoef);
        if (moving || coeffsDirty)
            updateCoeffs();

        float mixEnd = mix;
        const float delta = rampStep * (float) n;
        if (desired > mix)      mixEnd = std::min (desired, mix + delta);
        else if (desired < mix) mixEnd = std::max (desired, mix - delta);

        // Bell / shelf at exactly 0 dB is an identity filter: skip the maths.
        const bool identity = (type != FilterType::HighPass && type != FilterType::LowPass)
                              && gainS.current == 0.0f;

        if (identity)
        {
            clearState();
        }
        else
        {
            const float inc = (mixEnd - mix) / (float) n;
            const bool full = (mix == 1.0f && mixEnd == 1.0f);

            for (int c = 0; c < numCh; ++c)
            {
                float* d = data[c];
                BiquadState& st = states[(size_t) c];

                if (full)
                {
                    for (int i = 0; i < n; ++i)
                        d[i] = (float) processSample (coeffs, st, (double) d[i]);
                }
                else
                {
                    float m = mix;
                    for (int i = 0; i < n; ++i)
                    {
                        const double x = (double) d[i];
                        const double y = processSample (coeffs, st, x);
                        m += inc;
                        d[i] = (float) (x + (y - x) * (double) m);
                    }
                }

                flushDenormals (st);
            }
        }

        mix = mixEnd;

        if (mix == 0.0f)
        {
            if (! typeMatch)
            {
                type = wanted.type;       // faded out: safe to switch the filter type
                snapSmoothers();
                clearState();
            }
            else if (! wanted.enabled)
            {
                stateValid = false;       // faded out: go dormant
            }
        }
    }

private:
    void clearState() noexcept { states[0] = {}; states[1] = {}; }

    void snapSmoothers() noexcept
    {
        freqS.snap();
        gainS.snap();
        qS.snap();
        updateCoeffs();
    }

    void updateCoeffs() noexcept
    {
        coeffs = makeCoeffs (type, sampleRate, std::exp ((double) freqS.current),
                             (double) gainS.current, std::exp ((double) qS.current));
        coeffsDirty = false;
    }

    double sampleRate = 44100.0;
    float rampStep = 0.01f;

    BandSettings wanted;
    FilterType type = FilterType::Bell;

    detail::Smoothed freqS, gainS, qS;   // freq and Q smoothed in the log domain
    Coeffs coeffs;
    bool coeffsDirty = true;

    std::array<BiquadState, 2> states {};
    float mix = 0.0f;
    bool stateValid = false;
};

class EqEngine
{
public:
    static constexpr int kNumBands = 6;
    static constexpr int kSubBlock = 32;

    void prepare (double sr) noexcept
    {
        smoothCoef = 1.0f - std::exp (-(float) kSubBlock / (0.010f * (float) sr));
        for (auto& b : bands)
            b.prepare (sr);
    }

    void reset() noexcept
    {
        for (auto& b : bands)
            b.reset();
    }

    void setBand (int index, const BandSettings& s) noexcept
    {
        if (index >= 0 && index < kNumBands)
            bands[(size_t) index].setTarget (s);
    }

    // data: one pointer per channel (1 or 2 channels), processed in place.
    void process (float* const* data, int numCh, int numSamples) noexcept
    {
        numCh = std::min (numCh, 2);
        if (numCh <= 0)
            return;

        float* ptrs[2] = { nullptr, nullptr };
        int offset = 0;

        while (offset < numSamples)
        {
            const int n = std::min (kSubBlock, numSamples - offset);
            for (int c = 0; c < numCh; ++c)
                ptrs[c] = data[c] + offset;

            for (auto& b : bands)
                b.process (ptrs, numCh, n, smoothCoef);

            offset += n;
        }
    }

private:
    std::array<EqBand, kNumBands> bands;
    float smoothCoef = 0.1f;
};

} // namespace hexq::dsp
