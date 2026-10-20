#include "PluginEditor.h"
#include "Diag.h"

VantaAudioProcessorEditor::VantaAudioProcessorEditor (VantaAudioProcessor& p)
    : juce::AudioProcessorEditor (&p),
      proc (p),
      laf (palette),
      header (palette, "HEX-Q", "VANTA", "v1.0.0"),
      graph (p, palette),
      footer (palette, "DRAG NODE: FREQ + GAIN    SHIFT: FINE    WHEEL OVER NODE: Q    DOUBLE-CLICK NODE: BAND ON/OFF")
{
    vanta::trace ("editor ctor: members built");
    setLookAndFeel (&laf);

    addAndMakeVisible (header);
    addAndMakeVisible (graph);
    addAndMakeVisible (footer);

    for (int i = 0; i < vanta::kNumBands; ++i)
    {
        auto& btn = bandButtons[(size_t) i];
        btn = std::make_unique<vanta::BandButton> (palette, i + 1);
        btn->onClick = [this, i]
        {
            if (bandButtons[(size_t) i]->getToggleState())
                selectBand (i, true);
        };
        addAndMakeVisible (*btn);
    }

    auto setupField = [this] (juce::Slider& s, const juce::String& name)
    {
        s.setName (name);
        s.setSliderStyle (juce::Slider::RotaryVerticalDrag);
        s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 150, 26);
        s.setMouseDragSensitivity (220);
        addAndMakeVisible (s);
    };
    setupField (freqSlider, "Freq");
    setupField (gainSlider, "Gain");
    setupField (qSlider, "Q");

    typeBox.addItemList (vanta::filterTypeNames(), 1);
    addAndMakeVisible (typeBox);

    bandOnButton.setClickingTogglesState (true);
    addAndMakeVisible (bandOnButton);

    resetButton.onClick = [this] { resetAll(); };
    addAndMakeVisible (resetButton);

    vanta::trace ("editor ctor: widgets created");
    bypassAtt = std::make_unique<BA> (proc.apvts, vanta::bypassId(), header.getBypassButton());
    vanta::trace ("editor ctor: bypass attached");

    graph.onBandSelected = [this] (int band) { selectBand (band, false); };
    graph.onModelChanged = [this] { syncControlStates(); };

    selectBand (selectedBand, false);
    vanta::trace ("editor ctor: widgets ready, setting size");

    setSize (860, 548);      // must stay LAST: it calls resized(), which needs every child to exist
    vanta::trace ("editor ctor: done");
}

VantaAudioProcessorEditor::~VantaAudioProcessorEditor()
{
    vanta::trace ("editor dtor");
    graph.onBandSelected = nullptr;
    graph.onModelChanged = nullptr;
    setLookAndFeel (nullptr);
}

void VantaAudioProcessorEditor::selectBand (int band, bool fromButton)
{
    selectedBand = juce::jlimit (0, vanta::kNumBands - 1, band);

    if (! fromButton)
        bandButtons[(size_t) selectedBand]->setToggleState (true, juce::dontSendNotification);

    graph.setSelectedBand (selectedBand);
    attachToBand (selectedBand);
    syncControlStates();
}

void VantaAudioProcessorEditor::attachToBand (int band)
{
    // Destroy the old attachments first, then bind the controls to the newly selected band.
    freqAtt.reset();
    gainAtt.reset();
    qAtt.reset();
    typeAtt.reset();
    onAtt.reset();

    vanta::trace ("attachToBand " + juce::String (band));
    freqAtt = std::make_unique<SA> (proc.apvts, vanta::bandParamId (band, "freq"), freqSlider);
    gainAtt = std::make_unique<SA> (proc.apvts, vanta::bandParamId (band, "gain"), gainSlider);
    qAtt    = std::make_unique<SA> (proc.apvts, vanta::bandParamId (band, "q"), qSlider);
    typeAtt = std::make_unique<CA> (proc.apvts, vanta::bandParamId (band, "type"), typeBox);
    onAtt   = std::make_unique<BA> (proc.apvts, vanta::bandParamId (band, "on"), bandOnButton);
}

void VantaAudioProcessorEditor::syncControlStates()
{
    for (int i = 0; i < vanta::kNumBands; ++i)
        bandButtons[(size_t) i]->setBandActive (graph.getBandView (i).on);

    const auto type = graph.getBandView (selectedBand).type;
    const bool usesGain = (type == hexq::dsp::FilterType::Bell
                           || type == hexq::dsp::FilterType::LowShelf
                           || type == hexq::dsp::FilterType::HighShelf);
    gainSlider.setEnabled (usesGain);
    gainSlider.repaint();
}

void VantaAudioProcessorEditor::resetAll()
{
    for (auto* param : proc.getParameters())
    {
        param->beginChangeGesture();
        param->setValueNotifyingHost (param->getDefaultValue());
        param->endChangeGesture();
    }
}

void VantaAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (palette.background);

    g.setColour (palette.panel);
    g.fillRoundedRectangle (panelArea.toFloat(), hexq::layout::corner);
    g.setColour (palette.border);
    g.drawRoundedRectangle (panelArea.toFloat().reduced (0.5f), hexq::layout::corner, 1.0f);

    g.setColour (palette.textDim);
    g.setFont (hexq::makeFont (11.0f, true));
    g.drawText ("TYPE", typeCaptionArea, juce::Justification::centredLeft, false);
}

void VantaAudioProcessorEditor::resized()
{
    for (auto& b : bandButtons)
        if (b == nullptr)
            return;

    auto r = getLocalBounds();
    header.setBounds (r.removeFromTop (hexq::layout::headerHeight));
    footer.setBounds (r.removeFromBottom (hexq::layout::footerHeight));

    r = r.reduced (hexq::layout::margin, 0);
    r.removeFromTop (8);
    graph.setBounds (r.removeFromTop (312));
    r.removeFromTop (hexq::layout::gap);

    panelArea = r.removeFromTop (128);
    auto p = panelArea.reduced (14, 12);

    auto rowA = p.removeFromTop (30);
    for (int i = 0; i < vanta::kNumBands; ++i)
    {
        bandButtons[(size_t) i]->setBounds (rowA.removeFromLeft (64));
        rowA.removeFromLeft (8);
    }
    resetButton.setBounds (rowA.removeFromRight (100));
    rowA.removeFromRight (8);
    bandOnButton.setBounds (rowA.removeFromRight (100));

    p.removeFromTop (12);
    const int fw = 150, gap = 24;
    const int x0 = p.getX();

    typeCaptionArea = { x0, p.getY(), fw, 16 };
    typeBox.setBounds (x0, p.getBottom() - 30, fw, 30);
    freqSlider.setBounds (x0 + 1 * (fw + gap), p.getY(), fw, p.getHeight());
    gainSlider.setBounds (x0 + 2 * (fw + gap), p.getY(), fw, p.getHeight());
    qSlider.setBounds    (x0 + 3 * (fw + gap), p.getY(), fw, p.getHeight());
}
