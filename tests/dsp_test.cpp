// HEX-Q VANTA - offline DSP self-test (no JUCE needed).
#include "DSP/EqEngine.h"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <new>
#include <random>
#include <vector>

// ---- allocation counter (proves the audio path never touches the heap) ----------------
static std::size_t g_allocs = 0;
static bool g_count = false;
void* operator new (std::size_t n)
{
    if (g_count) ++g_allocs;
    if (void* p = std::malloc (n ? n : 1)) return p;
    throw std::bad_alloc();
}
void operator delete (void* p) noexcept { std::free (p); }
void operator delete (void* p, std::size_t) noexcept { std::free (p); }

using namespace hexq::dsp;
static const double PI = 3.14159265358979323846;
static int g_pass = 0, g_fail = 0;

static void check (bool ok, const char* name, double v = 0.0)
{
    ok ? ++g_pass : ++g_fail;
    std::printf ("[%s] %-66s %.5f\n", ok ? "PASS" : "FAIL", name, v);
}

static BandSettings S (bool en, FilterType t, float f, float g, float q)
{
    BandSettings s; s.enabled = en; s.type = t; s.freq = f; s.gainDb = g; s.q = q; return s;
}

static std::array<BandSettings, 6> defaults()
{
    return { S (false, FilterType::HighPass, 25.0f, 0.0f, 0.707f),
             S (true,  FilterType::LowShelf, 120.0f, 0.0f, 0.707f),
             S (true,  FilterType::Bell, 220.0f, 0.0f, 1.0f),
             S (true,  FilterType::Bell, 500.0f, 0.0f, 1.0f),
             S (true,  FilterType::HighShelf, 8000.0f, 0.0f, 0.707f),
             S (false, FilterType::LowPass, 20000.0f, 0.0f, 0.707f) };
}

static double mag (FilterType t, double sr, double f0, double g, double q, double f)
{
    return magnitudeDb (makeCoeffs (t, sr, f0, g, q), sr, f);
}

static double rmsDb (const std::vector<float>& v, size_t from)
{
    double s = 0; size_t n = 0;
    for (size_t i = from; i < v.size(); ++i) { s += (double) v[i] * v[i]; ++n; }
    return 10.0 * std::log10 (std::max (s / (double) n, 1e-30));
}

static void applyCfg (EqEngine& e, const std::array<BandSettings, 6>& c)
{
    for (int b = 0; b < 6; ++b) e.setBand (b, c[(size_t) b]);
}

static double measureEngineDb (const std::array<BandSettings, 6>& cfg, double sr, double f, int block = 512)
{
    EqEngine e; e.prepare (sr); applyCfg (e, cfg);
    const int total = (int) (1.4 * sr);
    std::vector<float> in ((size_t) total), out;
    for (int i = 0; i < total; ++i) in[(size_t) i] = (float) (0.25 * std::sin (2.0 * PI * f * i / sr));
    out = in;
    for (int pos = 0; pos < total; pos += block)
    {
        float* p[1] = { out.data() + pos };
        e.process (p, 1, std::min (block, total - pos));
    }
    const size_t from = (size_t) (1.0 * sr);
    return rmsDb (out, from) - rmsDb (in, from);
}

static bool stable (const Coeffs& c)
{
    return std::isfinite (c.b0) && std::isfinite (c.b1) && std::isfinite (c.b2)
        && std::isfinite (c.a1) && std::isfinite (c.a2)
        && std::abs (c.a2) < 1.0 && std::abs (c.a1) < 1.0 + c.a2;
}

int main()
{
    std::printf ("=== HEX-Q VANTA DSP self-test ===\n\n-- 1. defaults are neutral\n");
    {
        EqEngine e; e.prepare (48000.0); applyCfg (e, defaults());
        std::mt19937 rng (1); std::uniform_real_distribution<float> d (-0.5f, 0.5f);
        std::vector<float> L (512), R (512), L0, R0; double maxDiff = 0;
        for (int blk = 0; blk < 300; ++blk)
        {
            for (auto& x : L) x = d (rng);
            for (auto& x : R) x = d (rng);
            L0 = L; R0 = R;
            float* p[2] = { L.data(), R.data() };
            e.process (p, 2, 512);
            for (size_t i = 0; i < 512; ++i)
                maxDiff = std::max ({ maxDiff, (double) std::abs (L[i] - L0[i]), (double) std::abs (R[i] - R0[i]) });
        }
        check (maxDiff == 0.0, "default settings: output identical to input (max diff)", maxDiff);

        double worst = 0;
        for (auto t : { FilterType::LowShelf, FilterType::Bell, FilterType::HighShelf })
            for (double f : { 20.0, 120.0, 1000.0, 8000.0, 20000.0 })
                for (double fr : { 20.0, 200.0, 2000.0, 15000.0 })
                    worst = std::max (worst, std::abs (mag (t, 48000.0, f, 0.0, 1.0, fr)));
        check (worst < 1e-9, "0 dB bell/shelf response is flat (max |dB|)", worst);
    }

    std::printf ("\n-- 2. analytic filter responses (48 kHz)\n");
    {
        const double sr = 48000.0;
        check (std::abs (mag (FilterType::Bell, sr, 1000, 12, 1.0, 1000) - 12.0) < 0.01, "Bell +12 dB @1k: peak = +12", mag (FilterType::Bell, sr, 1000, 12, 1.0, 1000));
        check (std::abs (mag (FilterType::Bell, sr, 1000, -18, 1.0, 1000) + 18.0) < 0.01, "Bell -18 dB @1k: dip = -18", mag (FilterType::Bell, sr, 1000, -18, 1.0, 1000));
        check (std::abs (mag (FilterType::Bell, sr, 1000, 12, 1.0, 20)) < 0.2, "Bell far from centre (20 Hz) ~ 0 dB", mag (FilterType::Bell, sr, 1000, 12, 1.0, 20));
        check (mag (FilterType::Bell, sr, 1000, 12, 10.0, 1500) < mag (FilterType::Bell, sr, 1000, 12, 0.5, 1500), "higher Q = narrower bell", 0);
        check (std::abs (mag (FilterType::HighPass, sr, 100, 0, 0.7071, 100) + 3.01) < 0.05, "High-Pass: -3 dB at cutoff", mag (FilterType::HighPass, sr, 100, 0, 0.7071, 100));
        check (mag (FilterType::HighPass, sr, 100, 0, 0.7071, 10) < -35.0, "High-Pass: steep below cutoff (10 Hz)", mag (FilterType::HighPass, sr, 100, 0, 0.7071, 10));
        check (std::abs (mag (FilterType::HighPass, sr, 100, 0, 0.7071, 5000)) < 0.01, "High-Pass: flat in the passband", mag (FilterType::HighPass, sr, 100, 0, 0.7071, 5000));
        check (std::abs (mag (FilterType::LowPass, sr, 5000, 0, 0.7071, 5000) + 3.01) < 0.05, "Low-Pass: -3 dB at cutoff", mag (FilterType::LowPass, sr, 5000, 0, 0.7071, 5000));
        check (mag (FilterType::LowPass, sr, 5000, 0, 0.7071, 15000) < -15.0, "Low-Pass: rolls off above cutoff", mag (FilterType::LowPass, sr, 5000, 0, 0.7071, 15000));
        check (std::abs (mag (FilterType::LowShelf, sr, 120, 9, 0.7071, 20) - 9.0) < 0.5, "Low Shelf +9: ~ +9 dB at 20 Hz", mag (FilterType::LowShelf, sr, 120, 9, 0.7071, 20));
        check (std::abs (mag (FilterType::LowShelf, sr, 120, 9, 0.7071, 120) - 4.5) < 0.3, "Low Shelf +9: ~ half gain at corner", mag (FilterType::LowShelf, sr, 120, 9, 0.7071, 120));
        check (std::abs (mag (FilterType::LowShelf, sr, 120, 9, 0.7071, 5000)) < 0.1, "Low Shelf: flat far above corner", mag (FilterType::LowShelf, sr, 120, 9, 0.7071, 5000));
        check (std::abs (mag (FilterType::HighShelf, sr, 8000, -12, 0.7071, 20000) + 12.0) < 1.0, "High Shelf -12: ~ -12 dB at 20 kHz", mag (FilterType::HighShelf, sr, 8000, -12, 0.7071, 20000));
        check (std::abs (mag (FilterType::HighShelf, sr, 8000, -12, 0.7071, 200)) < 0.1, "High Shelf: flat far below corner", mag (FilterType::HighShelf, sr, 8000, -12, 0.7071, 200));
    }

    std::printf ("\n-- 3. limits, clamping and stability grid\n");
    {
        const double rates[] = { 22050, 44100, 48000, 88200, 96000, 176400, 192000 };
        const double qs[] = { 0.1, 0.3, 0.7071, 2.0, 10.0 };
        const double gains[] = { -18, -6, 0, 6, 18 };
        long total = 0, bad = 0;
        for (double sr : rates)
            for (int t = 0; t < 5; ++t)
                for (int fi = 0; fi < 40; ++fi)
                {
                    const double f = 20.0 * std::pow (1000.0, fi / 39.0);
                    for (double g : gains) for (double q : qs)
                    { ++total; if (! stable (makeCoeffs ((FilterType) t, sr, f, g, q))) ++bad; }
                }
        check (bad == 0, "all filter types stable over rate/freq/gain/Q grid (unstable count)", (double) bad);
        std::printf ("      (%ld combinations checked)\n", total);

        auto a = makeCoeffs (FilterType::Bell, 48000, 5.0, 100.0, 50.0);
        auto b = makeCoeffs (FilterType::Bell, 48000, 20.0, 18.0, 10.0);
        check (a.b0 == b.b0 && a.a1 == b.a1 && a.a2 == b.a2, "out-of-range freq/gain/Q are clamped to 20 Hz, +18 dB, Q 10", 0);
        auto c = makeCoeffs (FilterType::Bell, 48000, 99999.0, -99.0, 0.0);
        auto d = makeCoeffs (FilterType::Bell, 48000, 0.45 * 48000, -18.0, 0.1);
        check (c.b0 == d.b0 && c.a1 == d.a1, "upper limits clamp safely below Nyquist", 0);
    }

    std::printf ("\n-- 4. engine output matches the analytic curve\n");
    {
        auto cfg = defaults();
        cfg[2] = S (true, FilterType::Bell, 1000.0f, 12.0f, 1.0f);
        double worst = 0;
        for (double f : { 250.0, 500.0, 1000.0, 2000.0, 8000.0 })
            worst = std::max (worst, std::abs (measureEngineDb (cfg, 48000, f) - mag (FilterType::Bell, 48000, 1000, 12, 1.0, f)));
        check (worst < 0.05, "single bell: measured vs analytic (max error dB)", worst);

        cfg = { S (true, FilterType::HighPass, 80, 0, 0.707f), S (true, FilterType::LowShelf, 120, 6, 0.707f),
                S (true, FilterType::Bell, 300, -8, 1.5f), S (true, FilterType::Bell, 2000, 5, 2.0f),
                S (true, FilterType::HighShelf, 8000, -4, 0.707f), S (true, FilterType::LowPass, 16000, 0, 0.707f) };
        worst = 0;
        for (double f : { 40.0, 80.0, 150.0, 300.0, 700.0, 2000.0, 5000.0, 9000.0, 14000.0 })
        {
            double ref = 0;
            for (auto& s : cfg) ref += mag (s.type, 48000, s.freq, s.gainDb, s.q, f);
            worst = std::max (worst, std::abs (measureEngineDb (cfg, 48000, f) - ref));
        }
        check (worst < 0.1, "all six bands active: measured vs analytic (max error dB)", worst);
    }

    std::printf ("\n-- 5. smoothing / click behaviour\n");
    {
        EqEngine e; e.prepare (48000.0); auto cfg = defaults(); cfg[2] = S (true, FilterType::Bell, 1000, 0, 1.0f); applyCfg (e, cfg);
        const int block = 256; double maxStep = 0, prev = 0; long pos = 0;
        for (int blk = 0; blk < 120; ++blk)
        {
            if (blk == 40) { cfg[2].gainDb = 18.0f; applyCfg (e, cfg); }
            if (blk == 80) { cfg[2].gainDb = -18.0f; applyCfg (e, cfg); }
            std::vector<float> x (block);
            for (auto& s : x) s = (float) (0.1 * std::sin (2 * PI * 1000.0 * (double) pos++ / 48000.0));
            float* p[1] = { x.data() }; e.process (p, 1, block);
            for (float s : x) { maxStep = std::max (maxStep, std::abs ((double) s - prev)); prev = s; }
        }
        check (maxStep < 0.25, "gain jump 0 -> +18 -> -18 dB: no sample discontinuity", maxStep);

        EqEngine e2; e2.prepare (48000.0); auto c2 = defaults(); c2[3] = S (true, FilterType::Bell, 200, 12, 4.0f); applyCfg (e2, c2);
        maxStep = 0; prev = 0; pos = 0;
        for (int blk = 0; blk < 300; ++blk)
        {
            c2[3].freq = (float) (200.0 * std::pow (20.0, blk / 299.0)); applyCfg (e2, c2);
            std::vector<float> x (128);
            for (auto& s : x) s = (float) (0.1 * std::sin (2 * PI * 800.0 * (double) pos++ / 48000.0));
            float* p[1] = { x.data() }; e2.process (p, 1, 128);
            for (float s : x) { maxStep = std::max (maxStep, std::abs ((double) s - prev)); prev = s; }
        }
        check (maxStep < 0.3, "fast frequency sweep on a +12 dB narrow bell: no zipper steps", maxStep);

        EqEngine e3; e3.prepare (48000.0); auto c3 = defaults(); c3[0] = S (false, FilterType::HighPass, 400, 0, 0.707f); applyCfg (e3, c3);
        maxStep = 0; prev = 0; pos = 0;
        for (int blk = 0; blk < 100; ++blk)
        {
            if (blk == 20) { c3[0].enabled = true; applyCfg (e3, c3); }
            if (blk == 50) { c3[0].enabled = false; applyCfg (e3, c3); }
            if (blk == 70) { c3[0].enabled = true; c3[0].type = FilterType::LowPass; c3[0].freq = 150; applyCfg (e3, c3); }
            std::vector<float> x (128);
            for (auto& s : x) s = (float) (0.5 * std::sin (2 * PI * 200.0 * (double) pos++ / 48000.0));
            float* p[1] = { x.data() }; e3.process (p, 1, 128);
            for (float s : x) { maxStep = std::max (maxStep, std::abs ((double) s - prev)); prev = s; }
        }
        check (maxStep < 0.05, "band enable / disable / type change: no click", maxStep);
    }

    std::printf ("\n-- 6. mono, stereo, block sizes, sample rates\n");
    {
        auto cfg = defaults(); cfg[0].enabled = true; cfg[2] = S (true, FilterType::Bell, 300, -9, 2.0f); cfg[4].gainDb = 4.0f;
        std::mt19937 rng (7); std::uniform_real_distribution<float> d (-0.5f, 0.5f);
        std::vector<float> src (48000); for (auto& x : src) x = d (rng);

        auto run = [&] (int block, int ch)
        {
            EqEngine e; e.prepare (48000.0); applyCfg (e, cfg);
            std::vector<float> L = src, R = src;
            for (size_t pos = 0; pos < src.size(); pos += (size_t) block)
            {
                const int n = (int) std::min ((size_t) block, src.size() - pos);
                float* p[2] = { L.data() + pos, R.data() + pos };
                e.process (p, ch, n);
            }
            return L;
        };
        auto ref = run (512, 2);
        double worst = 0;
        for (int blk : { 1, 7, 32, 33, 64, 100, 128, 480, 1024, 4096 })
        {
            auto o = run (blk, 2);
            for (size_t i = 4096; i < ref.size(); ++i) worst = std::max (worst, (double) std::abs (o[i] - ref[i]));
        }
        check (worst < 1e-4, "same result for block sizes 1..4096, after the 12 ms fade-in (max diff)", worst);

        auto mono = run (512, 1);
        double w2 = 0; for (size_t i = 0; i < ref.size(); ++i) w2 = std::max (w2, (double) std::abs (mono[i] - ref[i]));
        check (w2 < 1e-6, "mono path equals stereo path (max diff)", w2);

        EqEngine e; e.prepare (48000.0); applyCfg (e, cfg);
        std::vector<float> L = src, R (src.size(), 0.0f);
        float* p[2] = { L.data(), R.data() }; e.process (p, 2, (int) src.size());
        double rmsR = 0; for (float x : R) rmsR += (double) x * x;
        check (rmsR == 0.0, "channels are independent (silent R stays silent)", rmsR);
    }
    {
        const double rates[] = { 22050, 44100, 48000, 88200, 96000, 192000 };
        bool allOk = true; double peak = 0;
        for (double sr : rates)
        {
            EqEngine e; e.prepare (sr);
            std::mt19937 rng (3); std::uniform_real_distribution<float> d (-1.0f, 1.0f), u (0.0f, 1.0f);
            for (int blk = 0; blk < 400; ++blk)
            {
                for (int b = 0; b < 6; ++b)
                    e.setBand (b, S (u (rng) > 0.3f, (FilterType) (int) (u (rng) * 4.999f),
                                     20.0f * std::pow (1000.0f, u (rng)), -18.0f + 36.0f * u (rng),
                                     0.1f * std::pow (100.0f, u (rng))));
                std::vector<float> L (256), R (256);
                for (auto& x : L) x = d (rng);
                for (auto& x : R) x = d (rng);
                float* p[2] = { L.data(), R.data() }; e.process (p, 2, 256);
                for (size_t i = 0; i < 256; ++i)
                {
                    if (! std::isfinite (L[i]) || ! std::isfinite (R[i])) allOk = false;
                    peak = std::max ({ peak, (double) std::abs (L[i]), (double) std::abs (R[i]) });
                }
            }
        }
        check (allOk && peak < 1e6, "random automation at 6 sample rates: finite and bounded (peak)", peak);
    }

    std::printf ("\n-- 7. denormals, allocations, CPU\n");
    {
        EqEngine e; e.prepare (48000.0);
        auto cfg = defaults(); cfg[0] = S (true, FilterType::HighPass, 20, 0, 0.707f); cfg[1].gainDb = 6; cfg[2].gainDb = -6; applyCfg (e, cfg);
        std::mt19937 rng (5); std::uniform_real_distribution<float> d (-0.5f, 0.5f);
        std::vector<float> x (48000); for (auto& v : x) v = d (rng);
        float* p[1] = { x.data() }; e.process (p, 1, 48000);
        double worst = 0;
        for (int sec = 0; sec < 4; ++sec) { std::fill (x.begin(), x.end(), 0.0f); e.process (p, 1, 48000); }
        for (float v : x) worst = std::max (worst, (double) std::abs (v));
        check (worst == 0.0, "after 4 s of silence the output is exactly 0 (no denormal tails)", worst);

        auto full = std::array<BandSettings, 6> { S (true, FilterType::HighPass, 30, 0, 0.707f), S (true, FilterType::LowShelf, 120, 3, 0.707f),
            S (true, FilterType::Bell, 250, -4, 1.0f), S (true, FilterType::Bell, 3000, 3, 1.5f),
            S (true, FilterType::HighShelf, 9000, 2, 0.707f), S (true, FilterType::LowPass, 18000, 0, 0.707f) };
        EqEngine b; b.prepare (48000.0); applyCfg (b, full);
        std::vector<float> L (512), R (512); for (auto& v : L) v = d (rng); for (auto& v : R) v = d (rng);
        float* pp[2] = { L.data(), R.data() };
        b.process (pp, 2, 512);
        const int blocks = 48000 * 20 / 512;
        g_allocs = 0; g_count = true;
        const auto t0 = std::chrono::steady_clock::now();
        for (int i = 0; i < blocks; ++i)
        {
            full[3].freq = 3000.0f + (float) (i % 50);
            b.setBand (3, full[3]);
            b.process (pp, 2, 512);
        }
        const auto t1 = std::chrono::steady_clock::now();
        g_count = false;
        const double secs = std::chrono::duration<double> (t1 - t0).count();
        const double pct = 100.0 * secs / 20.0;
        check (g_allocs == 0, "heap allocations during processing", (double) g_allocs);
        check (pct < 2.0, "CPU for 6 active bands, stereo, 48 kHz (% of one core, this PC)", pct);
    }

    std::printf ("\nRESULT: %d passed, %d failed\n", g_pass, g_fail);
    return g_fail == 0 ? 0 : 1;
}
