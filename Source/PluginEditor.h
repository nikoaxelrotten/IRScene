#pragma once
#include "PluginProcessor.h"

namespace irscene
{
    const juce::Colour silverTop    { 0xffe4e6e9 };
    const juce::Colour silverBottom { 0xffb6bac0 };
    const juce::Colour darkPanel    { 0xff2a2e33 };
    const juce::Colour display      { 0xffb9d3e3 };
    const juce::Colour displayEdge  { 0xff7d98aa };
    const juce::Colour displayText  { 0xff1d3342 };
    const juce::Colour rowA         { 0xffcfe3ef };
    const juce::Colour rowB         { 0xffbcd5e5 };
    const juce::Colour rowSel       { 0xff7fb2d6 };
    const juce::Colour accent       { 0xff1ec8ff };
    const juce::Colour ink          { 0xff2a2e33 };

    constexpr int designW = 1200;
    constexpr int designH = 660;
}

class IRSceneLookAndFeel : public juce::LookAndFeel_V4
{
public:
    IRSceneLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override;
    void drawLinearSlider (juce::Graphics&, int x, int y, int w, int h, float sliderPos,
                           float minPos, float maxPos, juce::Slider::SliderStyle, juce::Slider&) override;
    int getSliderThumbRadius (juce::Slider&) override { return 17; }
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool, bool) override;
    juce::Font getTextButtonFont (juce::TextButton&, int) override;
    void drawCornerResizer (juce::Graphics&, int w, int h, bool, bool) override;
};

class IRSceneEditor : public juce::AudioProcessorEditor
{
public:
    explicit IRSceneEditor (IRSceneProcessor&);
    ~IRSceneEditor() override;
    void resized() override;

private:
    class Content;
    IRSceneProcessor& proc;
    IRSceneLookAndFeel laf;
    std::unique_ptr<Content> content;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (IRSceneEditor)
};
