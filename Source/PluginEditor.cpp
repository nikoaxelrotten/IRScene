#include "PluginEditor.h"

using namespace irscene;
using APVTS = juce::AudioProcessorValueTreeState;

static juce::Font fnt (float h, int style = juce::Font::plain)
{
    return juce::Font (juce::FontOptions (h, style));
}

//==============================================================================
IRSceneLookAndFeel::IRSceneLookAndFeel()
{
    setColour (juce::TextButton::textColourOffId, ink);
    setColour (juce::TextButton::textColourOnId, juce::Colours::white);
    setColour (juce::Label::textColourId, ink);
    setColour (juce::ScrollBar::thumbColourId, juce::Colour (0xff5d7a8c));
}

void IRSceneLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                           float a0, float a1, juce::Slider&)
{
    auto b = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (2.0f);
    const float d = juce::jmin (b.getWidth(), b.getHeight());
    auto r = juce::Rectangle<float> (d, d).withCentre (b.getCentre());

    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.fillEllipse (r.translated (0.0f, 2.0f));
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff5a6068), r.getX() + d * 0.35f, r.getY() + d * 0.3f,
                                             juce::Colour (0xff15181b), r.getRight(), r.getBottom(), true));
    g.fillEllipse (r);
    g.setColour (juce::Colour (0xff0f1113));
    g.drawEllipse (r.reduced (1.5f), 3.0f);

    const float angle = a0 + pos * (a1 - a0);
    const auto c = r.getCentre();
    const float rad = d * 0.5f - d * 0.17f;
    juce::Point<float> p (c.x + rad * std::sin (angle), c.y - rad * std::cos (angle));
    g.setColour (accent);
    g.fillEllipse (juce::Rectangle<float> (d * 0.13f, d * 0.13f).withCentre (p));
}

void IRSceneLookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h, float sliderPos,
                                           float, float, juce::Slider::SliderStyle, juce::Slider&)
{
    const float cx = (float) x + (float) w * 0.5f;
    g.setColour (juce::Colour (0xff0f1113));
    g.fillRoundedRectangle (cx - 5.0f, (float) y + 17.0f, 10.0f, (float) h - 34.0f, 5.0f);

    g.setColour (juce::Colour (0xff8c939b));
    for (int i = 0; i <= 12; ++i)
    {
        const float ty = (float) y + 17.0f + (float) (h - 34) * (float) i / 12.0f;
        const float len = (i % 4 == 0) ? 12.0f : 7.0f;
        g.drawLine (cx - 12.0f - len, ty, cx - 12.0f, ty, 1.0f);
        g.drawLine (cx + 12.0f, ty, cx + 12.0f + len, ty, 1.0f);
    }

    auto thumb = juce::Rectangle<float> (44.0f, 34.0f).withCentre ({ cx, sliderPos });
    g.setColour (juce::Colours::black.withAlpha (0.4f));
    g.fillRoundedRectangle (thumb.translated (0.0f, 3.0f), 3.0f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xffeceef0), 0.0f, thumb.getY(),
                                             juce::Colour (0xff8e939a), 0.0f, thumb.getBottom(), false));
    g.fillRoundedRectangle (thumb, 3.0f);
    g.setColour (accent);
    g.fillRect (thumb.getX() + 4.0f, thumb.getCentreY() - 1.5f, thumb.getWidth() - 8.0f, 3.0f);
}

void IRSceneLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (0.5f);
    const bool on = b.getToggleState();
    if (on)
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff2c3137), 0.0f, 0.0f,
                                                 juce::Colour (0xff15181b), 0.0f, r.getBottom(), false));
    else
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xffeef0f2), 0.0f, 0.0f,
                                                 juce::Colour (0xffc3c7cc), 0.0f, r.getBottom(), false));
    g.fillRoundedRectangle (r, 3.0f);
    if (down)
    {
        g.setColour (juce::Colours::black.withAlpha (0.15f));
        g.fillRoundedRectangle (r, 3.0f);
    }
    g.setColour (juce::Colour (0xff8c9199));
    g.drawRoundedRectangle (r, 3.0f, 1.0f);
}

juce::Font IRSceneLookAndFeel::getTextButtonFont (juce::TextButton&, int) { return fnt (14.0f, juce::Font::bold); }

void IRSceneLookAndFeel::drawCornerResizer (juce::Graphics& g, int w, int h, bool, bool)
{
    g.setColour (juce::Colour (0xff5a6068));
    for (int i = 0; i < 3; ++i)
    {
        const float o = 5.0f + (float) i * 6.0f;
        g.drawLine ((float) w - 3.0f, (float) h - o, (float) w - o, (float) h - 3.0f, 2.0f);
    }
}

//==============================================================================
// Formateo de valores
static juce::String fmtHz (double v)  { return v >= 1000.0 ? juce::String (v / 1000.0, 1) + " KHz" : juce::String ((int) std::round (v)) + " Hz"; }
static juce::String fmtPct (double v) { return juce::String ((int) std::round (v)) + "%"; }
static juce::String fmtMs (double v)  { return juce::String (v, 1) + " ms"; }
static juce::String fmtPan (double v)
{
    const int i = (int) std::round (v);
    return i == 0 ? juce::String ("C") : juce::String (std::abs (i)) + (i < 0 ? "% L" : "% R");
}
static juce::String fmtDb (double v) { return (v > 0.0 ? "+" : "") + juce::String (v, 1) + " dB"; }

static void styleDisplayLabel (juce::Label& l, float size = 12.0f)
{
    l.setFont (fnt (size, juce::Font::bold));
    l.setJustificationType (juce::Justification::centred);
    l.setColour (juce::Label::backgroundColourId, display);
    l.setColour (juce::Label::outlineColourId, displayEdge);
    l.setColour (juce::Label::textColourId, displayText);
    l.setInterceptsMouseClicks (false, false);
}

//==============================================================================
class KnobUnit : public juce::Component
{
public:
    KnobUnit (const juce::String& name, int knobSz, bool lightText, std::function<juce::String (double)> fmt)
        : knobSize (knobSz), format (std::move (fmt))
    {
        slider.setSliderStyle (juce::Slider::RotaryVerticalDrag);
        slider.setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
        slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
        slider.setMouseDragSensitivity (160);
        slider.onValueChange = [this] { value.setText (format (slider.getValue()), juce::dontSendNotification); };
        addAndMakeVisible (slider);

        styleDisplayLabel (value);
        addAndMakeVisible (value);

        label.setText (name, juce::dontSendNotification);
        label.setFont (fnt (12.0f, juce::Font::bold));
        label.setJustificationType (juce::Justification::centred);
        label.setColour (juce::Label::textColourId, lightText ? juce::Colour (0xffe6e9ed) : ink);
        label.setInterceptsMouseClicks (false, false);
        addAndMakeVisible (label);
    }

    void refreshValue() { value.setText (format (slider.getValue()), juce::dontSendNotification); }

    void resized() override
    {
        value.setBounds ((getWidth() - 84) / 2, 0, 84, 22);
        slider.setBounds ((getWidth() - knobSize) / 2, 29, knobSize, knobSize);
        label.setBounds (0, 29 + knobSize + 6, getWidth(), 16);
    }

    juce::Slider slider;

private:
    int knobSize;
    std::function<juce::String (double)> format;
    juce::Label value, label;
};

//==============================================================================
class GainStrip : public juce::Component
{
public:
    GainStrip (IRSceneProcessor& p, int slot, bool mirrored)
    {
        const juce::String suffix = slot == 0 ? "A" : "B";

        fader.setSliderStyle (juce::Slider::LinearVertical);
        fader.setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
        fader.onValueChange = [this] { value.setText (fmtDb (fader.getValue()), juce::dontSendNotification); };
        addAndMakeVisible (fader);

        styleDisplayLabel (value);
        addAndMakeVisible (value);

        title.setText ("GAIN", juce::dontSendNotification);
        title.setFont (fnt (12.0f, juce::Font::bold));
        title.setJustificationType (juce::Justification::centred);
        title.setColour (juce::Label::textColourId, juce::Colour (0xffe6e9ed));
        addAndMakeVisible (title);

        solo.setButtonText ("S");
        solo.setClickingTogglesState (true);
        phase.setButtonText (juce::String::charToString ((juce::juce_wchar) 0x00D8));
        phase.setClickingTogglesState (true);
        solo.setTooltip ("Solo");
        phase.setTooltip ("Invertir fase");
        addAndMakeVisible (solo);
        addAndMakeVisible (phase);
        soloFirst = ! mirrored;

        gainAtt = std::make_unique<APVTS::SliderAttachment> (p.apvts, "gain" + suffix, fader);
        soloAtt = std::make_unique<APVTS::ButtonAttachment> (p.apvts, "solo" + suffix, solo);
        polAtt  = std::make_unique<APVTS::ButtonAttachment> (p.apvts, "pol" + suffix, phase);
        fader.setDoubleClickReturnValue (true, 0.0);
        value.setText (fmtDb (fader.getValue()), juce::dontSendNotification);
    }

    void resized() override
    {
        value.setBounds (7, 0, 56, 22);
        title.setBounds (0, 28, 70, 16);
        fader.setBounds (0, 48, 70, 240);
        auto& first = soloFirst ? solo : phase;
        auto& second = soloFirst ? phase : solo;
        first.setBounds (0, 300, 32, 32);
        second.setBounds (38, 300, 32, 32);
    }

private:
    juce::Slider fader;
    juce::Label value, title;
    juce::TextButton solo, phase;
    bool soloFirst = true;
    std::unique_ptr<APVTS::SliderAttachment> gainAtt;
    std::unique_ptr<APVTS::ButtonAttachment> soloAtt, polAtt;
};

//==============================================================================
class SlotPanel : public juce::Component, private juce::ListBoxModel
{
public:
    SlotPanel (IRSceneProcessor& p, int slotIndex) : proc (p), slot (slotIndex), list ({}, this)
    {
        currentFile = proc.getIRFile (slot);
        dir = currentFile.existsAsFile() ? currentFile.getParentDirectory()
                                         : juce::File::getSpecialLocation (juce::File::userDocumentsDirectory);

        styleDisplayLabel (pathLabel, 14.0f);
        pathLabel.setFont (fnt (14.0f));
        pathLabel.setJustificationType (juce::Justification::centredLeft);
        pathLabel.setBorderSize ({ 0, 8, 0, 8 });
        addAndMakeVisible (pathLabel);

        list.setRowHeight (26);
        list.setColour (juce::ListBox::backgroundColourId, rowA);
        list.setColour (juce::ListBox::outlineColourId, displayEdge);
        list.setOutlineThickness (1);
        addAndMakeVisible (list);

        prev.setButtonText ("<");
        next.setButtonText (">");
        prev.onClick = [this] { step (-1); };
        next.onClick = [this] { step (1); };
        addAndMakeVisible (prev);
        addAndMakeVisible (next);

        fileLabel.setFont (fnt (14.0f));
        fileLabel.setColour (juce::Label::textColourId, displayText);
        fileLabel.setInterceptsMouseClicks (false, false);
        addAndMakeVisible (fileLabel);

        juce::Path folder;
        folder.startNewSubPath (1.0f, 3.0f);
        folder.lineTo (8.0f, 3.0f);
        folder.lineTo (10.0f, 5.0f);
        folder.lineTo (21.0f, 5.0f);
        folder.lineTo (21.0f, 17.0f);
        folder.lineTo (1.0f, 17.0f);
        folder.closeSubPath();
        browse.setShape (folder, true, true, false);
        browse.setOutline (displayText, 1.6f);
        browse.onClick = [this] { openChooser(); };
        browse.setTooltip ("Cargar IR...");
        addAndMakeVisible (browse);

        refresh();
        updateFileLabel();
    }

    void paint (juce::Graphics& g) override
    {
        auto f = juce::Rectangle<int> (0, 300, getWidth(), 38);
        g.setColour (display);
        g.fillRect (f);
        g.setColour (displayEdge);
        g.drawRect (f, 1);
    }

    void resized() override
    {
        pathLabel.setBounds (0, 0, getWidth(), 30);
        list.setBounds (0, 34, getWidth(), 262);
        prev.setBounds (6, 306, 26, 26);
        next.setBounds (36, 306, 26, 26);
        fileLabel.setBounds (68, 300, getWidth() - 68 - 40, 38);
        browse.setBounds (getWidth() - 36, 305, 28, 28);
    }

private:
    struct Entry { juce::File file; bool isDir = false; bool isParent = false; };

    int getNumRows() override { return entries.size(); }

    void paintListBoxItem (int row, juce::Graphics& g, int w, int h, bool selected) override
    {
        if (! juce::isPositiveAndBelow (row, entries.size()))
            return;
        const auto& e = entries.getReference (row);
        g.fillAll (selected ? rowSel : (row % 2 == 0 ? rowA : rowB));
        g.setColour (displayText);
        g.setFont (fnt (14.0f));
        g.drawText (e.isParent ? juce::String ("..") : e.file.getFileName(), 8, 0, w - 80, h, juce::Justification::centredLeft, true);
        if (e.isDir)
        {
            g.setColour (juce::Colour (0xff4b6677));
            g.setFont (fnt (11.0f));
            g.drawText ("FOLDER", w - 66, 0, 60, h, juce::Justification::centredRight);
        }
    }

    void listBoxItemClicked (int row, const juce::MouseEvent&) override
    {
        if (! juce::isPositiveAndBelow (row, entries.size()))
            return;
        const auto e = entries.getReference (row);
        if (e.isDir)
        {
            dir = e.file;
            refresh();
        }
        else
            loadEntry (row);
    }

    void refresh()
    {
        entries.clear();
        if (dir.getParentDirectory() != dir)
            entries.add ({ dir.getParentDirectory(), true, true });

        juce::Array<juce::File> dirs, files;
        dir.findChildFiles (dirs, juce::File::findDirectories, false);
        dir.findChildFiles (files, juce::File::findFiles, false, "*.wav;*.aif;*.aiff;*.flac");
        auto cmp = [] (const juce::File& a, const juce::File& b) { return a.getFileName().compareNatural (b.getFileName()) < 0; };
        std::sort (dirs.begin(), dirs.end(), cmp);
        std::sort (files.begin(), files.end(), cmp);
        for (auto& d : dirs)  entries.add ({ d, true, false });
        for (auto& f : files) entries.add ({ f, false, false });

        pathLabel.setText (dir.getFullPathName(), juce::dontSendNotification);
        list.updateContent();
        list.deselectAllRows();
        const int idx = indexOfCurrent();
        if (idx >= 0)
        {
            list.selectRow (idx, true, true);
            list.scrollToEnsureRowIsOnscreen (idx);
        }
        list.repaint();
    }

    int indexOfCurrent() const
    {
        for (int i = 0; i < entries.size(); ++i)
            if (! entries.getReference (i).isDir && entries.getReference (i).file == currentFile)
                return i;
        return -1;
    }

    void loadEntry (int row)
    {
        currentFile = entries.getReference (row).file;
        proc.loadIR (slot, currentFile);
        list.selectRow (row, true, true);
        list.scrollToEnsureRowIsOnscreen (row);
        updateFileLabel();
    }

    void step (int delta)
    {
        juce::Array<int> fileRows;
        for (int i = 0; i < entries.size(); ++i)
            if (! entries.getReference (i).isDir)
                fileRows.add (i);
        if (fileRows.isEmpty())
            return;

        const int pos = fileRows.indexOf (indexOfCurrent());
        int newPos = pos < 0 ? (delta > 0 ? 0 : fileRows.size() - 1)
                             : (pos + delta + fileRows.size()) % fileRows.size();
        loadEntry (fileRows[newPos]);
    }

    void openChooser()
    {
        chooser = std::make_unique<juce::FileChooser> ("Elegir un IR", dir, "*.wav;*.aif;*.aiff;*.flac");
        chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                              [this] (const juce::FileChooser& fc)
                              {
                                  const auto f = fc.getResult();
                                  if (! f.existsAsFile())
                                      return;
                                  dir = f.getParentDirectory();
                                  currentFile = f;
                                  proc.loadIR (slot, f);
                                  refresh();
                                  updateFileLabel();
                              });
    }

    void updateFileLabel()
    {
        fileLabel.setText ("IR File: " + (currentFile.existsAsFile() ? currentFile.getFileName() : juce::String ("(ninguno)")),
                           juce::dontSendNotification);
    }

    IRSceneProcessor& proc;
    int slot;
    juce::File dir, currentFile;
    juce::Array<Entry> entries;
    juce::Label pathLabel, fileLabel;
    juce::ListBox list;
    juce::TextButton prev, next;
    juce::ShapeButton browse { "browse", juce::Colours::transparentBlack, juce::Colours::transparentBlack, juce::Colours::transparentBlack };
    std::unique_ptr<juce::FileChooser> chooser;
};

//==============================================================================
class IRSceneEditor::Content : public juce::Component
{
public:
    Content (IRSceneProcessor& p, std::function<void (float)> scaleCallback)
        : proc (p), setScale (std::move (scaleCallback)),
          gainA (p, 0, false), gainB (p, 1, true), panelA (p, 0), panelB (p, 1)
    {
        for (auto* c : { (juce::Component*) &gainA, (juce::Component*) &gainB, (juce::Component*) &panelA, (juce::Component*) &panelB })
            addAndMakeVisible (c);

        gainA.setBounds (44, 110, 70, 332);
        gainB.setBounds (1086, 110, 70, 332);
        panelA.setBounds (134, 110, 372, 338);
        panelB.setBounds (696, 110, 372, 338);

        addKnob ("balance", "BALANCE", 526, 118, 150, 66, true, fmtPan);
        addKnob ("room", "ROOM", 526, 292, 150, 66, true, fmtPct);

        const char* left[]  = { "hp", "lp", "res", "dly", "pan" };
        const char* right[] = { "pan", "dly", "res", "lp", "hp" };
        const char* names[] = { "HI PASS", "LO PASS", "RESONANCE", "DELAY", "PANNING" };
        auto fmtFor = [] (const juce::String& k) -> std::function<juce::String (double)>
        {
            if (k == "hp" || k == "lp") return fmtHz;
            if (k == "res") return fmtPct;
            if (k == "dly") return fmtMs;
            return fmtPan;
        };
        auto nameFor = [&] (const juce::String& k)
        {
            const char* keys[] = { "hp", "lp", "res", "dly", "pan" };
            for (int i = 0; i < 5; ++i) if (k == keys[i]) return juce::String (names[i]);
            return juce::String();
        };

        for (int i = 0; i < 5; ++i)
        {
            addKnob (juce::String (left[i]) + "A", nameFor (left[i]), 40 + i * 110, 496, 104, 54, false, fmtFor (left[i]));
            addKnob (juce::String (right[i]) + "B", nameFor (right[i]), 616 + i * 110, 496, 104, 54, false, fmtFor (right[i]));
        }

        const float scales[] = { 0.75f, 1.0f, 1.25f, 1.5f };
        const char* labels[] = { "75%", "100%", "125%", "150%" };
        for (int i = 0; i < 4; ++i)
        {
            auto& b = scaleButtons[(size_t) i];
            b.setButtonText (labels[i]);
            b.setClickingTogglesState (true);
            b.setRadioGroupId (101);
            b.setBounds (930 + i * 58, 22, 52, 34);
            const float s = scales[i];
            b.onClick = [this, s] { setScale (s); };
            addAndMakeVisible (b);
        }
    }

    void updateScaleHighlight (float s)
    {
        const float scales[] = { 0.75f, 1.0f, 1.25f, 1.5f };
        for (int i = 0; i < 4; ++i)
            scaleButtons[(size_t) i].setToggleState (std::abs (scales[i] - s) < 0.02f, juce::dontSendNotification);
    }

    void paint (juce::Graphics& g) override
    {
        g.setGradientFill (juce::ColourGradient (silverTop, 0.0f, 0.0f, silverBottom, 0.0f, (float) designH, false));
        g.fillAll();
        g.setColour (juce::Colour (0xff7a7f87));
        g.drawRect (getLocalBounds(), 2);

        g.setColour (juce::Colour (0xff1f2327));
        g.setFont (fnt (58.0f, juce::Font::bold | juce::Font::italic));
        g.drawText ("IRScene", 44, 8, 420, 64, juce::Justification::centredLeft);
        g.setColour (juce::Colour (0xff3a3f45));
        g.setFont (fnt (12.0f, juce::Font::bold));
        g.drawText ("DUAL IR LOADER  -  VST3", 46, 70, 300, 16, juce::Justification::centredLeft);
        g.drawText ("ESCALA", 850, 22, 70, 34, juce::Justification::centredRight);

        g.setColour (darkPanel);
        g.fillRoundedRectangle (24.0f, 92.0f, 1152.0f, 380.0f, 6.0f);
    }

private:
    void addKnob (const juce::String& pid, const juce::String& name, int x, int y, int w, int knobSize,
                  bool light, std::function<juce::String (double)> fmt)
    {
        auto unit = std::make_unique<KnobUnit> (name, knobSize, light, std::move (fmt));
        unit->setBounds (x, y, w, 29 + knobSize + 22);
        addAndMakeVisible (*unit);
        attachments.push_back (std::make_unique<APVTS::SliderAttachment> (proc.apvts, pid, unit->slider));
        if (auto* prm = proc.apvts.getParameter (pid))
            unit->slider.setDoubleClickReturnValue (true, prm->convertFrom0to1 (prm->getDefaultValue()));
        unit->refreshValue();
        knobs.push_back (std::move (unit));
    }

    IRSceneProcessor& proc;
    std::function<void (float)> setScale;
    GainStrip gainA, gainB;
    SlotPanel panelA, panelB;
    std::vector<std::unique_ptr<KnobUnit>> knobs;
    std::vector<std::unique_ptr<APVTS::SliderAttachment>> attachments;
    std::array<juce::TextButton, 4> scaleButtons;
};

//==============================================================================
IRSceneEditor::IRSceneEditor (IRSceneProcessor& p) : AudioProcessorEditor (&p), proc (p)
{
    content = std::make_unique<Content> (p, [this] (float s)
    {
        setSize (juce::roundToInt ((float) designW * s), juce::roundToInt ((float) designH * s));
    });
    content->setLookAndFeel (&laf);
    addAndMakeVisible (*content);

    setResizeLimits (600, 330, 1800, 990);
    setResizable (true, true);
    getConstrainer()->setFixedAspectRatio ((double) designW / (double) designH);

    const int w = juce::jlimit (600, 1800, (int) proc.apvts.state.getProperty ("uiWidth", designW));
    setSize (w, juce::roundToInt ((double) w * designH / designW));
}

IRSceneEditor::~IRSceneEditor()
{
    content->setLookAndFeel (nullptr);
}

void IRSceneEditor::resized()
{
    const float s = (float) getWidth() / (float) designW;
    content->setBounds (0, 0, designW, designH);
    content->setTransform (juce::AffineTransform::scale (s));
    content->updateScaleHighlight (s);
    proc.apvts.state.setProperty ("uiWidth", getWidth(), nullptr);
}
