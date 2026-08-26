#pragma once

#include "PluginProcessor.h"

class SliceomatAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit SliceomatAudioProcessorEditor(SliceomatAudioProcessor&);
    ~SliceomatAudioProcessorEditor() override = default;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    struct Knob
    {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };

    void setupKnob(Knob& knob, const juce::String& paramId, const juce::String& title);

    SliceomatAudioProcessor& audioProcessor;

    Knob noise;
    Knob input;
    Knob attack;
    Knob decay;
    Knob volMod;
    Knob pitch;
    Knob reso;
    Knob filterEnvMod;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SliceomatAudioProcessorEditor)
};
