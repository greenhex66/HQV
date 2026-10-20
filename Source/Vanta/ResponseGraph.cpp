#include "ResponseGraph.h"
#include <cmath>

namespace vanta
{
using hexq::dsp::FilterType;
using hexq::makeFont;

namespace
{
    constexpr float kDbRange = 20.0f;        // graph shows +/- 20 dB
    constexpr float kNodeRadius = 10.0f;
    constexpr float kHitRadius = 15.0f;

    bool hasGain (FilterType t) { return t == FilterType::Bell || t == FilterType::LowShelf || t == FilterType::HighShelf; }
}

ResponseGraph::ResponseGraph (VantaAudioProcessor& p, const hexq::Palette& pal)
    : proc (p), palette (pal)
{
    listenedIds.add (bypassId());
    for (int b = 0; b < kNumBands; ++b)
        for (auto* suffix : { "on", "type", "freq", "gain", "q" })
            listenedIds.add (bandParamId (b, suffix));

    for (auto& id : listenedIds)
        proc.apvts.addParameterListener (id, this);

    refreshModel();
}

ResponseGraph::~ResponseGraph()
{
    cancelPendingUpdate();
    for (auto& id : listenedIds)
        proc.apvts.removeParameterListener (id, this);
}

//==============================================================================
double ResponseGraph::graphSampleRate() const
{
    const double sr = proc.getSampleRate();
    return sr > 8000.0 ? sr : 44100.0;
}

void ResponseGraph::handleAsyncUpdate()
{
    refreshModel();
    repaint();
    if (onModelChanged)
        onModelChanged();
}

void ResponseGraph::refreshModel()
{
    const double sr = graphSampleRate();
    bypassed = proc.apvts.getRawParameterValue (bypassId())->load() > 0.5f;

    for (int i = 0; i < kNumBands; ++i)
    {
        const auto s = readBandSettings (proc.apvts, i);
        auto& m = model[(size_t) i];
        m.on = s.enabled;
        m.type = s.type;
        m.freq = s.freq;
        m.gain = s.gainDb;
        m.q = s.q;
        m.c = hexq::dsp::makeCoeffs (s.type, sr, s.freq, s.gainDb, s.q);
    }

    rebuildCurve();
}

void ResponseGraph::rebuildCurve()
{
    curvePath.clear();
    fillPath.clear();
    if (plot.getWidth() < 8.0f)
        return;

    const double sr = graphSampleRate();
    const float y0 = dbToY (0.0f);
    bool first = true;
    float lastX = plot.getX();

    for (float x = plot.getX(); x < plot.getRight() + 2.0f; x += 2.0f)
    {
        const float xc = juce::jmin (x, plot.getRight());
        const double f = (double) xToFreq (xc);

        double db = 0.0;
        for (const auto& m : model)
            if (m.on)
                db += hexq::dsp::magnitudeDb (m.c, sr, f);

        const float y = dbToY ((float) juce::jlimit (-60.0, 60.0, db));
        if (first)
        {
            curvePath.startNewSubPath (xc, y);
            fillPath.startNewSubPath (xc, y0);
            fillPath.lineTo (xc, y);
            first = false;
        }
        else
        {
            curvePath.lineTo (xc, y);
            fillPath.lineTo (xc, y);
        }
        lastX = xc;
    }

    fillPath.lineTo (lastX, y0);
    fillPath.closeSubPath();
}

//==============================================================================
float ResponseGraph::freqToX (float f) const noexcept
{
    return plot.getX() + plot.getWidth() * std::log (juce::jmax (20.0f, f) / 20.0f) / std::log (1000.0f);
}

float ResponseGraph::xToFreq (float x) const noexcept
{
    const float p = juce::jlimit (0.0f, 1.0f, (x - plot.getX()) / juce::jmax (1.0f, plot.getWidth()));
    return 20.0f * std::pow (1000.0f, p);
}

float ResponseGraph::dbToY (float db) const noexcept
{
    return plot.getY() + plot.getHeight() * (0.5f - db / (2.0f * kDbRange));
}

float ResponseGraph::yToDb (float y) const noexcept
{
    return (0.5f - (y - plot.getY()) / juce::jmax (1.0f, plot.getHeight())) * (2.0f * kDbRange);
}

juce::Point<float> ResponseGraph::nodePos (int band) const
{
    const auto& m = model[(size_t) band];
    float db = m.gain;
    if (! hasGain (m.type))   // High-Pass / Low-Pass: sit on the band's own curve
        db = (float) hexq::dsp::magnitudeDb (m.c, graphSampleRate(), (double) m.freq);

    return { freqToX (m.freq), dbToY (juce::jlimit (-kDbRange, kDbRange, db)) };
}

int ResponseGraph::hitTest (juce::Point<float> p) const
{
    int best = -1;
    float bestDist = kHitRadius;
    for (int i = 0; i < kNumBands; ++i)
    {
        const float d = nodePos (i).getDistanceFrom (p);
        if (d <= bestDist)
        {
            best = i;
            bestDist = d;
        }
    }
    return best;
}

//==============================================================================
void ResponseGraph::resized()
{
    plot = getLocalBounds().toFloat().withTrimmedLeft (36.0f).withTrimmedRight (14.0f)
                                     .withTrimmedTop (14.0f).withTrimmedBottom (24.0f);
    rebuildCurve();
}

void ResponseGraph::setSelectedBand (int band)
{
    selected = juce::jlimit (0, kNumBands - 1, band);
    repaint();
}

void ResponseGraph::selectBand (int band, bool notify)
{
    if (band == selected && ! notify)
        return;

    selected = band;
    repaint();
    if (notify && onBandSelected)
        onBandSelected (band);
}

void ResponseGraph::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    g.setColour (palette.panel);
    g.fillRoundedRectangle (bounds, hexq::layout::corner);
    g.setColour (palette.border);
    g.drawRoundedRectangle (bounds.reduced (0.5f), hexq::layout::corner, 1.0f);

    // ---- grid ------------------------------------------------------------------------
    g.setFont (makeFont (10.0f));
    const float freqs[] = { 20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000 };
    const char* labels[] = { "20", "50", "100", "200", "500", "1k", "2k", "5k", "10k", "20k" };
    for (int i = 0; i < 10; ++i)
    {
        const float x = freqToX (freqs[i]);
        g.setColour (palette.border.withAlpha (0.45f));
        g.drawVerticalLine ((int) x, plot.getY(), plot.getBottom());
        g.setColour (palette.textDim);
        g.drawText (labels[i], (int) x - 20, (int) plot.getBottom() + 5, 40, 14, juce::Justification::centred, false);
    }

    for (int db = -18; db <= 18; db += 6)
    {
        const float y = dbToY ((float) db);
        g.setColour (db == 0 ? palette.border : palette.border.withAlpha (0.4f));
        g.drawHorizontalLine ((int) y, plot.getX(), plot.getRight());
        g.setColour (palette.textDim);
        g.drawText ((db > 0 ? "+" : "") + juce::String (db), 2, (int) y - 7, 30, 14, juce::Justification::centredRight, false);
    }

    // ---- response curve --------------------------------------------------------------
    {
        juce::Graphics::ScopedSaveState state (g);
        g.reduceClipRegion (plot.toNearestInt().expanded (0, 2));

        if (! bypassed)
        {
            g.setColour (palette.accent.withAlpha (0.10f));
            g.fillPath (fillPath);
        }

        g.setColour (bypassed ? palette.textDim.withAlpha (0.55f) : palette.accent);
        g.strokePath (curvePath, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // ---- nodes ------------------------------------------------------------------------
    for (int i = 0; i < kNumBands; ++i)
    {
        if (i == selected)
            continue;

        const auto& m = model[(size_t) i];
        const auto p = nodePos (i);
        const float r = kNodeRadius - 1.0f + (i == hover ? 1.0f : 0.0f);
        const juce::Rectangle<float> c (p.x - r, p.y - r, r * 2.0f, r * 2.0f);

        if (m.on && ! bypassed)
        {
            g.setColour (palette.accent);
            g.fillEllipse (c);
            g.setColour (palette.background);
        }
        else
        {
            g.setColour (palette.panel);
            g.fillEllipse (c);
            g.setColour (palette.textDim.withAlpha (0.7f));
            g.drawEllipse (c, 1.3f);
        }

        g.setFont (makeFont (11.0f, true));
        g.drawText (juce::String (i + 1), c, juce::Justification::centred, false);
    }

    {   // selected node last, on top, with a ring
        const auto& m = model[(size_t) selected];
        const auto p = nodePos (selected);
        const float r = kNodeRadius;
        const juce::Rectangle<float> c (p.x - r, p.y - r, r * 2.0f, r * 2.0f);

        g.setColour (palette.accent.withAlpha (0.25f));
        g.fillEllipse (c.expanded (5.0f));
        g.setColour (palette.accent);
        g.drawEllipse (c.expanded (3.0f), 1.3f);

        if (m.on && ! bypassed)
        {
            g.setColour (palette.accent);
            g.fillEllipse (c);
            g.setColour (palette.background);
        }
        else
        {
            g.setColour (palette.panel);
            g.fillEllipse (c);
            g.setColour (palette.accent);
            g.drawEllipse (c, 1.5f);
        }

        g.setFont (makeFont (11.0f, true));
        g.drawText (juce::String (selected + 1), c, juce::Justification::centred, false);
    }

    // ---- readout of the selected band --------------------------------------------------
    {
        const auto& m = model[(size_t) selected];
        juce::String text = "BAND " + juce::String (selected + 1) + "  " + filterTypeNames()[(int) m.type].toUpperCase()
                          + "   " + formatFreq (m.freq);
        if (hasGain (m.type))
            text << "   " << formatGain (m.gain);
        text << "   Q " << formatQ (m.q);
        if (! m.on)
            text << "   (OFF)";

        g.setFont (makeFont (11.0f, true));
        g.setColour (palette.textDim);
        g.drawText (text, plot.withHeight (16.0f).toNearestInt(), juce::Justification::centredRight, false);
    }
}

//==============================================================================
void ResponseGraph::setParam (const juce::String& id, float plainValue)
{
    if (auto* p = proc.apvts.getParameter (id))
        p->setValueNotifyingHost (p->convertTo0to1 (plainValue));
}

void ResponseGraph::beginGestures (int band)
{
    for (auto* suffix : { "freq", "gain" })
        if (auto* p = proc.apvts.getParameter (bandParamId (band, suffix)))
            p->beginChangeGesture();
}

void ResponseGraph::endGestures (int band)
{
    for (auto* suffix : { "freq", "gain" })
        if (auto* p = proc.apvts.getParameter (bandParamId (band, suffix)))
            p->endChangeGesture();
}

void ResponseGraph::mouseMove (const juce::MouseEvent& e)
{
    const int h = hitTest (e.position);
    if (h != hover)
    {
        hover = h;
        setMouseCursor (h >= 0 ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
        repaint();
    }
}

void ResponseGraph::mouseExit (const juce::MouseEvent&)
{
    if (hover != -1)
    {
        hover = -1;
        repaint();
    }
}

void ResponseGraph::mouseDown (const juce::MouseEvent& e)
{
    const int h = hitTest (e.position);
    if (h < 0)
        return;

    selectBand (h, true);
    dragBand = h;
    beginGestures (h);
    anchorMouse = e.position;
    anchorNode = nodePos (h);
    lastFine = e.mods.isShiftDown();
}

void ResponseGraph::mouseDrag (const juce::MouseEvent& e)
{
    if (dragBand < 0)
        return;

    const bool fine = e.mods.isShiftDown();       // Shift = fine adjustment
    if (fine != lastFine)
    {
        anchorMouse = e.position;
        anchorNode = nodePos (dragBand);
        lastFine = fine;
    }

    const float k = fine ? 0.2f : 1.0f;
    const auto target = anchorNode + (e.position - anchorMouse) * k;

    setParam (bandParamId (dragBand, "freq"), xToFreq (target.x));
    if (hasGain (model[(size_t) dragBand].type))
        setParam (bandParamId (dragBand, "gain"), juce::jlimit (-18.0f, 18.0f, yToDb (target.y)));
}

void ResponseGraph::mouseUp (const juce::MouseEvent&)
{
    if (dragBand >= 0)
    {
        endGestures (dragBand);
        dragBand = -1;
    }
}

void ResponseGraph::mouseDoubleClick (const juce::MouseEvent& e)
{
    const int h = hitTest (e.position);
    if (h < 0)
        return;

    // Double-click a node = switch that band on / off (per-band bypass).
    if (auto* p = proc.apvts.getParameter (bandParamId (h, "on")))
    {
        p->beginChangeGesture();
        p->setValueNotifyingHost (model[(size_t) h].on ? 0.0f : 1.0f);
        p->endChangeGesture();
    }
}

void ResponseGraph::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w)
{
    const int h = hitTest (e.position);
    if (h < 0)
        return;

    // Mouse wheel over a node = change Q.
    const float newQ = juce::jlimit (0.1f, 10.0f, model[(size_t) h].q * std::exp (w.deltaY * 0.6f));
    if (auto* p = proc.apvts.getParameter (bandParamId (h, "q")))
    {
        p->beginChangeGesture();
        p->setValueNotifyingHost (p->convertTo0to1 (newQ));
        p->endChangeGesture();
    }
}
} // namespace vanta
