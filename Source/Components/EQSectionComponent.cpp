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

  label.setFont(juce::Font(10.0f, juce::Font::bold));
  label.setJustificationType(juce::Justification::centred);
  label.setColour(juce::Label::textColourId, juce::Colour(0xff888888));
  addAndMakeVisible(label);

  attach =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          proc.getAPVTS(), paramID, knob);
}

void EQSectionComponent::paint(juce::Graphics &g) {
  auto bounds = getLocalBounds().toFloat();

  // Glass panel background
  g.setColour(juce::Colour(0x0affffff));
  g.fillRoundedRectangle(bounds, 8.0f);
  g.setColour(juce::Colour(0x1affffff));
  g.drawRoundedRectangle(bounds, 8.0f, 1.0f);

  // Mid group box styling
  auto midGroupX = bounds.getWidth() * (4.0f / 9.0f);
  auto midGroupW = bounds.getWidth() * (3.0f / 9.0f);
  auto midGroupRect = juce::Rectangle<float>(midGroupX + 4, 4, midGroupW - 8,
                                             bounds.getHeight() - 8);

  // Inner glass card for Mids
  g.setColour(juce::Colour(0x08ffffff));
  g.fillRoundedRectangle(midGroupRect, 6.0f);
  g.setColour(juce::Colour(0x15ffffff));
  g.drawRoundedRectangle(midGroupRect, 6.0f, 1.0f);

  // "Mid" Label
  g.setColour(juce::Colour(0xff666666));
  g.setFont(juce::Font(10.0f, juce::Font::bold));
  g.drawText("MIDS", midGroupRect.getX(), midGroupRect.getY() + 4,
             midGroupRect.getWidth(), 12, juce::Justification::centred);
}

void EQSectionComponent::resized() {
  auto area = getLocalBounds().reduced(8);

  int numKnobs = 9;
  auto knobW = (float)area.getWidth() / (float)numKnobs;
  int labelH = 14;
  int knobSize = juce::jmin((int)knobW - 4, area.getHeight() - labelH - 4);

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

  // Re-layout knobs
  // Knobs 0-3 (Left)
  for (int i = 0; i < 4; ++i) {
    auto r = area.removeFromLeft((int)knobW);
    auto kR = r.removeFromTop(r.getHeight() - labelH);
    knobs[i].knob->setBounds(kR.withSizeKeepingCentre(knobSize, knobSize));
    knobs[i].label->setBounds(r);
  }

  // Mids 4-6 (Middle - ensure they are centered in the box)
  for (int i = 4; i <= 6; ++i) {
    auto r = area.removeFromLeft((int)knobW);
    r.removeFromTop(12); // Push down for "MIDS" label
    auto kR = r.removeFromTop(r.getHeight() - labelH);
    knobs[i].knob->setBounds(kR.withSizeKeepingCentre(knobSize, knobSize));
    knobs[i].label->setBounds(r);
  }

  // Knobs 7-8 (Right)
  for (int i = 7; i < 9; ++i) {
    auto r = area.removeFromLeft((int)knobW);
    auto kR = r.removeFromTop(r.getHeight() - labelH);
    knobs[i].knob->setBounds(kR.withSizeKeepingCentre(knobSize, knobSize));
    knobs[i].label->setBounds(r);
  }
}
