#include "PluginEditor.h"

namespace
{
constexpr int kKnobW = 96;
constexpr int kKnobH = 118;
constexpr int kPad = 16;
constexpr int kTitleH = 36;
constexpr int kFilterH = 36;
}

SliceomatAudioProcessorEditor::SliceomatAudioProcessorEditor(SliceomatAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    filterLabel.setText("Filter", juce::dontSendNotification);
    filterLabel.setJustificationType(juce::Justification::centredLeft);
    filterLabel.setColour(juce::Label::textColourId, juce::Colour(0xfff2e6d8));
    filterLabel.setFont(juce::FontOptions(13.0f, juce::Font::bold));
    addAndMakeVisible(filterLabel);

    filterBox.addItemList(sliceomat::filterTypeNames(), 1);
    filterBox.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff2a2622));
    filterBox.setColour(juce::ComboBox::textColourId, juce::Colour(0xfff2e6d8));
    filterBox.setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff3a3530));
    filterBox.setColour(juce::ComboBox::arrowColourId, juce::Colour(0xffe07a3d));
    addAndMakeVisible(filterBox);
    filterAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        audioProcessor.apvts, "filterType", filterBox);

    setupKnob(noise, "gain1", "Noise");
    setupKnob(input, "gain2", "Input");
    setupKnob(attack, "attack", "Attack");
    setupKnob(decay, "decay", "Decay");
    setupKnob(volMod, "volMod", "Vol Mod");
    setupKnob(pitch, "pitch", "Pitch");
    setupKnob(reso, "reso", "Reso");
    setupKnob(filterEnvMod, "filterEnvMod", "Flt Env");

    attack.slider.setTextValueSuffix(" s");
    decay.slider.setTextValueSuffix(" s");
    pitch.slider.setNumDecimalPlacesToDisplay(1);

    setSize(kPad * 2 + kKnobW * 4, kTitleH + kFilterH + kPad + kKnobH * 2 + kPad);
}

void SliceomatAudioProcessorEditor::setupKnob(Knob& knob, const juce::String& paramId, const juce::String& title)
{
    knob.slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    knob.slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 82, 18);
    knob.slider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xffe07a3d));
    knob.slider.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xff3a3530));
    knob.slider.setColour(juce::Slider::thumbColourId, juce::Colour(0xfff2e6d8));
    knob.slider.setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xfff2e6d8));
    knob.slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    addAndMakeVisible(knob.slider);

    knob.label.setText(title, juce::dontSendNotification);
    knob.label.setJustificationType(juce::Justification::centred);
    knob.label.setColour(juce::Label::textColourId, juce::Colour(0xfff2e6d8));
    knob.label.setFont(juce::FontOptions(13.0f, juce::Font::bold));
    addAndMakeVisible(knob.label);

    knob.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.apvts, paramId, knob.slider);
}

void SliceomatAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1c1a18));

    g.setColour(juce::Colour(0xffe07a3d));
    g.setFont(juce::FontOptions(18.0f, juce::Font::bold));
    g.drawText("SLICE-O-MAT", getLocalBounds().removeFromTop(kTitleH), juce::Justification::centred);

    auto body = getLocalBounds().withTrimmedTop(kTitleH);
    g.setColour(juce::Colour(0xff2a2622));
    g.fillRoundedRectangle(body.reduced(8).toFloat(), 8.0f);
}

void SliceomatAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().withTrimmedTop(kTitleH).reduced(kPad);
    auto filterRow = area.removeFromTop(kFilterH);
    filterLabel.setBounds(filterRow.removeFromLeft(56));
    filterBox.setBounds(filterRow.withHeight(24).withY(filterRow.getY() + 4));
    area.removeFromTop(4);

    const int rowH = kKnobH;
    auto place = [&](Knob& knob, juce::Rectangle<int> cell)
    {
        knob.label.setBounds(cell.removeFromTop(18));
        knob.slider.setBounds(cell);
    };

    auto row1 = area.removeFromTop(rowH);
    place(noise, row1.removeFromLeft(kKnobW));
    place(input, row1.removeFromLeft(kKnobW));
    place(attack, row1.removeFromLeft(kKnobW));
    place(decay, row1.removeFromLeft(kKnobW));

    auto row2 = area.removeFromTop(rowH);
    place(volMod, row2.removeFromLeft(kKnobW));
    place(pitch, row2.removeFromLeft(kKnobW));
    place(reso, row2.removeFromLeft(kKnobW));
    place(filterEnvMod, row2.removeFromLeft(kKnobW));
}
