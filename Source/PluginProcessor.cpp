#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
constexpr float defaultAttackSec = 0.001f;  // Axoloti a = -60, very short
constexpr float defaultDecaySec = 0.5f;     // Axoloti d = -6
constexpr float defaultPitchMidi = 18.0f;   // Axoloti pitch 18
}

SliceomatAudioProcessor::SliceomatAudioProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "PARAMS", createParameterLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout SliceomatAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"gain1", 1}, "Noise",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.5f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"gain2", 1}, "Input",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.5f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"attack", 1}, "Attack",
        juce::NormalisableRange<float>(0.0001f, 2.0f, 0.0f, 0.3f),
        defaultAttackSec,
        juce::AudioParameterFloatAttributes().withLabel("s")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"decay", 1}, "Decay",
        juce::NormalisableRange<float>(0.001f, 8.0f, 0.0f, 0.3f),
        defaultDecaySec,
        juce::AudioParameterFloatAttributes().withLabel("s")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"pitch", 1}, "Pitch",
        juce::NormalisableRange<float>(0.0f, 127.0f, 0.01f),
        defaultPitchMidi,
        juce::AudioParameterFloatAttributes().withLabel("st")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"reso", 1}, "Reso",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"volMod", 1}, "Vol Mod",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 1.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"filterEnvMod", 1}, "Filter Env Mod",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 1.0f));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{"filterType", 1}, "Filter",
        sliceomat::filterTypeNames(), 0));

    return {params.begin(), params.end()};
}

void SliceomatAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    engine.prepare(sampleRate, samplesPerBlock);
}

void SliceomatAudioProcessor::releaseResources()
{
    engine.reset();
}

bool SliceomatAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto mainIn = layouts.getMainInputChannelSet();
    const auto mainOut = layouts.getMainOutputChannelSet();

    if (mainOut != juce::AudioChannelSet::mono() && mainOut != juce::AudioChannelSet::stereo())
        return false;

    if (mainIn != mainOut)
        return false;

    return true;
}

void SliceomatAudioProcessor::handleMidi(const juce::MidiMessage& message)
{
    if (message.isNoteOn() && message.getVelocity() > 0)
        engine.trigger();
}

void SliceomatAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;

    for (int ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear(ch, 0, buffer.getNumSamples());

    engine.setTargets(*apvts.getRawParameterValue("gain1"),
                      *apvts.getRawParameterValue("gain2"),
                      *apvts.getRawParameterValue("attack"),
                      *apvts.getRawParameterValue("decay"),
                      *apvts.getRawParameterValue("pitch"),
                      *apvts.getRawParameterValue("reso"),
                      *apvts.getRawParameterValue("volMod"),
                      *apvts.getRawParameterValue("filterEnvMod"),
                      (int) *apvts.getRawParameterValue("filterType"));

    int sample = 0;
    const int numSamples = buffer.getNumSamples();

    for (const auto metadata : midi)
    {
        const int pos = metadata.samplePosition;
        if (pos > sample)
        {
            juce::AudioBuffer<float> slice(buffer.getArrayOfWritePointers(),
                                           buffer.getNumChannels(),
                                           sample,
                                           pos - sample);
            engine.process(slice);
            sample = pos;
        }

        handleMidi(metadata.getMessage());
    }

    if (sample < numSamples)
    {
        juce::AudioBuffer<float> slice(buffer.getArrayOfWritePointers(),
                                       buffer.getNumChannels(),
                                       sample,
                                       numSamples - sample);
        engine.process(slice);
    }
}

juce::AudioProcessorEditor* SliceomatAudioProcessor::createEditor()
{
    return new SliceomatAudioProcessorEditor(*this);
}

void SliceomatAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary(*xml, destData);
}

void SliceomatAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary(data, sizeInBytes))
        if (xml->hasTagName(apvts.state.getType()))
            apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SliceomatAudioProcessor();
}
