#include "EQSectionComponent.h"

EQSectionComponent::EQSectionComponent(FreeIRAudioProcessor &processor)
    : proc(processor) {
  setupKnob(loCutKnob, loCutLabel, "LoCutHz", loCutAttach, 80.0);
  setupKnob(hiCutKnob, hiCutLabel, "HiCutHz", hiCutAttach, 12000.0);
  setupKnob(bassKnob, bassLabel, "BassGainDb", bassAttach, 0.0);
  setupKnob(midGainKnob, midGainLabel, "MidGainDb", midGainAttach, 0.0);
  setupKnob(midFreqKnob, midFreqLabel, "MidFreqHz", midFreqAttach, 1000.0);
  setupKnob(midQKnob, midQLabel, "MidQ", midQAttach, 1.0);
  setupKnob(trebleKnob, trebleLabel, "TrebleGainDb", trebleAttach, 0.0);
  setupKnob(airKnob, airLabel, "AirGainDb", airAttach, 0.0);
  setupKnob(outputKnob, outputLabel, "OutputGainDb", outputAttach, 0.0);
}

EQSectionComponent::~EQSectionComponent() {}

void EQSectionComponent::setupKnob(
    juce::Slider &knob, juce::Label &label, const juce::String &paramID,
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
        &attach,
    double defaultVal) {
  knob.setSliderStyle(juce::Slider::RotaryVerticalDrag);
  knob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
  knob.setDoubleClickReturnValue(true, defaultVal);
  knob.setPopupDisplayEnabled(true, true, this);
  addAndMakeVisible(knob);

  label.setFont(juce::Font(11.0f));
  label.setJustificationType(juce::Justification::centred);
  label.setColour(juce::Label::textColourId, juce::Colour(0xffaaaaaa));
  addAndMakeVisible(label);

  attach =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          proc.getAPVTS(), paramID, knob);
}

void EQSectionComponent::paint(juce::Graphics &g) {
  auto bounds = getLocalBounds().toFloat();

  // Subtle background
  g.setColour(juce::Colour(0xff181818));
  g.fillRoundedRectangle(bounds, 4.0f);

  // Mid group box
  // The reference shows a boxed "Mid" section containing Gain, Freq, Q
  auto midGroupX = bounds.getWidth() * (4.0f / 9.0f);
  auto midGroupW = bounds.getWidth() * (3.0f / 9.0f);
  auto midGroupRect = juce::Rectangle<float>(midGroupX - 4, 0, midGroupW + 8,
                                             bounds.getHeight());

  g.setColour(juce::Colour(0xff252525));
  g.fillRoundedRectangle(midGroupRect, 4.0f);
  g.setColour(juce::Colour(0xff444444));
  g.drawRoundedRectangle(midGroupRect, 4.0f, 1.0f);

  // "Mid" label at top of group
  g.setColour(juce::Colour(0xffaaaaaa));
  g.setFont(juce::Font(12.0f, juce::Font::bold));
  g.drawText("Mid", midGroupRect.removeFromTop(16),
             juce::Justification::centred);
}

void EQSectionComponent::resized() {
  auto area = getLocalBounds().reduced(4);
  int numKnobs = 9;
  int knobW = area.getWidth() / numKnobs;
  int knobSize = juce::jmin(knobW - 4, area.getHeight() - 22);
  int labelH = 18;

  struct KnobInfo {
    juce::Slider *knob;
    juce::Label *label;
  };
  KnobInfo knobs[] = {
      {&loCutKnob, &loCutLabel},     {&hiCutKnob, &hiCutLabel},
      {&bassKnob, &bassLabel},       {&airKnob, &airLabel},

      {&midGainKnob, &midGainLabel}, {&midFreqKnob, &midFreqLabel},
      {&midQKnob, &midQLabel},

      {&trebleKnob, &trebleLabel},   {&outputKnob, &outputLabel},
  };

  for (int i = 0; i < numKnobs; ++i) {
    auto col = area.removeFromLeft(knobW);
    auto knobArea = col.removeFromTop(col.getHeight() - labelH);
    knobs[i].knob->setBounds(
        knobArea.withSizeKeepingCentre(knobSize, knobSize));
    knobs[i].label->setBounds(col);
  }
}
