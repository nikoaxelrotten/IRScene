#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>

class IRSceneProcessor : public juce::AudioProcessor
{
public:
    IRSceneProcessor();
    ~IRSceneProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "IRScene"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 2.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    // Carga de IRs (hilo de interfaz). slot: 0 = A, 1 = B.
    void loadIR (int slot, const juce::File& file);
    juce::File getIRFile (int slot) const;

    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void restoreIRsFromState();

    struct Slot
    {
        juce::dsp::Convolution convolution;
        juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> delay;
        juce::dsp::StateVariableTPTFilter<float> highPass, lowPass;
        juce::AudioBuffer<float> scratch;
        float lastGainL = 0.0f, lastGainR = 0.0f;

        std::atomic<float> *gain = nullptr, *hp = nullptr, *lp = nullptr, *res = nullptr,
                           *dly = nullptr, *pan = nullptr, *pol = nullptr, *solo = nullptr;
    };

    std::array<Slot, 2> slots;
    std::atomic<float> *balance = nullptr, *room = nullptr;
    juce::Reverb reverb;
    double sr = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (IRSceneProcessor)
};
