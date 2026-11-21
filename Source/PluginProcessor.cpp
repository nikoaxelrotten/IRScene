#include "PluginProcessor.h"
#include "PluginEditor.h"

using APVTS = juce::AudioProcessorValueTreeState;

static juce::String id (const char* base, int slot) { return juce::String (base) + (slot == 0 ? "A" : "B"); }

APVTS::ParameterLayout IRSceneProcessor::createLayout()
{
    using namespace juce;
    APVTS::ParameterLayout layout;

    for (int s = 0; s < 2; ++s)
    {
        const String n = s == 0 ? "A " : "B ";
        layout.add (std::make_unique<AudioParameterFloat> (ParameterID { id ("gain", s), 1 }, n + "Gain",
                    NormalisableRange<float> (-24.0f, 12.0f, 0.1f), 0.0f,
                    AudioParameterFloatAttributes().withLabel ("dB")));
        layout.add (std::make_unique<AudioParameterFloat> (ParameterID { id ("hp", s), 1 }, n + "Hi Pass",
                    NormalisableRange<float> (20.0f, 1000.0f, 1.0f, 0.4f), 50.0f,
                    AudioParameterFloatAttributes().withLabel ("Hz")));
        layout.add (std::make_unique<AudioParameterFloat> (ParameterID { id ("lp", s), 1 }, n + "Lo Pass",
                    NormalisableRange<float> (2000.0f, 20000.0f, 10.0f, 0.4f), 16000.0f,
                    AudioParameterFloatAttributes().withLabel ("Hz")));
        layout.add (std::make_unique<AudioParameterFloat> (ParameterID { id ("res", s), 1 }, n + "Resonance",
                    NormalisableRange<float> (0.0f, 100.0f, 1.0f), 0.0f,
                    AudioParameterFloatAttributes().withLabel ("%")));
        layout.add (std::make_unique<AudioParameterFloat> (ParameterID { id ("dly", s), 1 }, n + "Delay",
                    NormalisableRange<float> (0.0f, 20.0f, 0.1f), 0.0f,
                    AudioParameterFloatAttributes().withLabel ("ms")));
        layout.add (std::make_unique<AudioParameterFloat> (ParameterID { id ("pan", s), 1 }, n + "Panning",
                    NormalisableRange<float> (-100.0f, 100.0f, 1.0f), s == 0 ? -100.0f : 100.0f,
                    AudioParameterFloatAttributes().withLabel ("%")));
        layout.add (std::make_unique<AudioParameterBool> (ParameterID { id ("pol", s), 1 }, n + "Phase Invert", false));
        layout.add (std::make_unique<AudioParameterBool> (ParameterID { id ("solo", s), 1 }, n + "Solo", false));
    }

    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "balance", 1 }, "Balance",
                NormalisableRange<float> (-100.0f, 100.0f, 1.0f), 0.0f));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "room", 1 }, "Room",
                NormalisableRange<float> (0.0f, 100.0f, 1.0f), 0.0f));
    return layout;
}

IRSceneProcessor::IRSceneProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "IRSceneState", createLayout())
{
    for (int s = 0; s < 2; ++s)
    {
        auto& sl = slots[(size_t) s];
        sl.gain = apvts.getRawParameterValue (id ("gain", s));
        sl.hp   = apvts.getRawParameterValue (id ("hp", s));
        sl.lp   = apvts.getRawParameterValue (id ("lp", s));
        sl.res  = apvts.getRawParameterValue (id ("res", s));
        sl.dly  = apvts.getRawParameterValue (id ("dly", s));
        sl.pan  = apvts.getRawParameterValue (id ("pan", s));
        sl.pol  = apvts.getRawParameterValue (id ("pol", s));
        sl.solo = apvts.getRawParameterValue (id ("solo", s));
    }
    balance = apvts.getRawParameterValue ("balance");
    room    = apvts.getRawParameterValue ("room");
}

bool IRSceneProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    return l.getMainInputChannelSet() == juce::AudioChannelSet::stereo()
        && l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void IRSceneProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;
    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) samplesPerBlock, 2 };

    for (auto& sl : slots)
    {
        sl.convolution.prepare (spec);
        sl.delay.setMaximumDelayInSamples ((int) std::ceil (sampleRate * 0.025));
        sl.delay.prepare (spec);
        sl.highPass.prepare (spec);
        sl.highPass.setType (juce::dsp::StateVariableTPTFilterType::highpass);
        sl.highPass.setResonance (0.7071f);
        sl.lowPass.prepare (spec);
        sl.lowPass.setType (juce::dsp::StateVariableTPTFilterType::lowpass);
        sl.scratch.setSize (2, samplesPerBlock);
        sl.lastGainL = sl.lastGainR = 0.0f;
    }

    reverb.setSampleRate (sampleRate);
    reverb.reset();
}

void IRSceneProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    if (buffer.getNumChannels() < 2 || n == 0)
        return;

    const bool soloA = slots[0].solo->load() > 0.5f, soloB = slots[1].solo->load() > 0.5f;
    const bool anySolo = soloA || soloB;

    const float bal = balance->load() / 100.0f;
    const float balGain[2] = { bal > 0.0f ? 1.0f - bal : 1.0f, bal < 0.0f ? 1.0f + bal : 1.0f };

    for (int s = 0; s < 2; ++s)
    {
        auto& sl = slots[(size_t) s];
        if (sl.scratch.getNumSamples() < n)
            sl.scratch.setSize (2, n, false, false, true);

        sl.scratch.copyFrom (0, 0, buffer, 0, 0, n);
        sl.scratch.copyFrom (1, 0, buffer, 1, 0, n);

        juce::dsp::AudioBlock<float> full (sl.scratch);
        auto block = full.getSubBlock (0, (size_t) n);
        juce::dsp::ProcessContextReplacing<float> ctx (block);

        sl.delay.setDelay ((float) (sl.dly->load() * 0.001 * sr));
        sl.delay.process (ctx);
        sl.convolution.process (ctx);

        sl.highPass.setCutoffFrequency (sl.hp->load());
        sl.highPass.process (ctx);
        sl.lowPass.setCutoffFrequency (juce::jmin (sl.lp->load(), (float) (sr * 0.45)));
        sl.lowPass.setResonance (0.7071f + sl.res->load() * 0.05f); // 0..100 % -> Q 0.71..5.7
        sl.lowPass.process (ctx);

        const bool muted = anySolo && ! (s == 0 ? soloA : soloB);
        float g = muted ? 0.0f : juce::Decibels::decibelsToGain (sl.gain->load()) * balGain[s];
        if (sl.pol->load() > 0.5f) g = -g;

        const float pan = sl.pan->load() / 100.0f;
        const float gL = g * (pan > 0.0f ? 1.0f - pan : 1.0f);
        const float gR = g * (pan < 0.0f ? 1.0f + pan : 1.0f);

        sl.scratch.applyGainRamp (0, 0, n, sl.lastGainL, gL);
        sl.scratch.applyGainRamp (1, 0, n, sl.lastGainR, gR);
        sl.lastGainL = gL;
        sl.lastGainR = gR;
    }

    for (int ch = 0; ch < 2; ++ch)
    {
        buffer.copyFrom (ch, 0, slots[0].scratch, ch, 0, n);
        buffer.addFrom (ch, 0, slots[1].scratch, ch, 0, n);
    }

    const float roomAmt = room->load() / 100.0f;
    if (roomAmt > 0.001f)
    {
        juce::Reverb::Parameters p;
        p.roomSize = 0.35f; p.damping = 0.5f; p.width = 1.0f;
        p.dryLevel = 0.5f;               // JUCE multiplica por 2 -> ganancia unitaria
        p.wetLevel = roomAmt * 0.15f;    // JUCE multiplica por 3
        p.freezeMode = 0.0f;
        reverb.setParameters (p);
        reverb.processStereo (buffer.getWritePointer (0), buffer.getWritePointer (1), n);
    }
}

void IRSceneProcessor::loadIR (int slot, const juce::File& file)
{
    if (! file.existsAsFile())
        return;
    apvts.state.setProperty (slot == 0 ? "irPathA" : "irPathB", file.getFullPathName(), nullptr);
    slots[(size_t) slot].convolution.loadImpulseResponse (file,
        juce::dsp::Convolution::Stereo::yes,
        juce::dsp::Convolution::Trim::yes, 0,
        juce::dsp::Convolution::Normalise::yes);
}

juce::File IRSceneProcessor::getIRFile (int slot) const
{
    return juce::File (apvts.state.getProperty (slot == 0 ? "irPathA" : "irPathB").toString());
}

void IRSceneProcessor::restoreIRsFromState()
{
    for (int s = 0; s < 2; ++s)
        loadIR (s, getIRFile (s));
}

void IRSceneProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, dest);
}

void IRSceneProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
        {
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
            restoreIRsFromState();
        }
}

juce::AudioProcessorEditor* IRSceneProcessor::createEditor() { return new IRSceneEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new IRSceneProcessor(); }
