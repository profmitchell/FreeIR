#include "IRSlotComponent.h"

IRSlotComponent::IRSlotComponent(FreeIRAudioProcessor &processor, int slotIndex)
    : proc(processor), slotID(slotIndex) {
  auto prefix = "Slot" + juce::String(slotID + 1) + "_";

  // Slot number label
  slotNumLabel.setText(juce::String(slotID + 1), juce::dontSendNotification);
  slotNumLabel.setFont(juce::Font(18.0f, juce::Font::bold));
  slotNumLabel.setColour(juce::Label::textColourId, slotColours[slotID]);
  slotNumLabel.setJustificationType(juce::Justification::centred);
  addAndMakeVisible(slotNumLabel);

  // Clear button
  clearButton.setButtonText(juce::String::charToString(0x2715)); // ✕
  clearButton.setColour(juce::TextButton::buttonColourId,
                        juce::Colour(0x10ffffff));
  clearButton.setColour(juce::TextButton::textColourOffId,
                        juce::Colour(0xff666666));
  clearButton.onClick = [this]() {
    proc.getIRSlot(slotID).clearImpulseResponse();
    updateSlotDisplay();
    if (onSlotChanged)
      onSlotChanged();
  };
  addAndMakeVisible(clearButton);

  // Solo
  soloButton.setClickingTogglesState(true);
  soloButton.setColour(juce::TextButton::buttonColourId,
                       juce::Colour(0x1affffff));
  soloButton.setColour(juce::TextButton::buttonOnColourId,
                       juce::Colour(0xffddaa00));
  soloButton.setColour(juce::TextButton::textColourOffId,
                       juce::Colour(0xff888888));
  addAndMakeVisible(soloButton);
  soloAttach =
      std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
          proc.getAPVTS(), prefix + "Solo", soloButton);

  // Mute
  muteButton.setClickingTogglesState(true);
  muteButton.setColour(juce::TextButton::buttonColourId,
                       juce::Colour(0x1affffff));
  muteButton.setColour(juce::TextButton::buttonOnColourId,
                       juce::Colour(0xffcc3333));
  muteButton.setColour(juce::TextButton::textColourOffId,
                       juce::Colour(0xff888888));
  addAndMakeVisible(muteButton);
  muteAttach =
      std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
          proc.getAPVTS(), prefix + "Mute", muteButton);

  // Pan knob
  panKnob.setSliderStyle(juce::Slider::RotaryVerticalDrag);
  panKnob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
  panKnob.setDoubleClickReturnValue(true, 0.0);
  panKnob.setPopupDisplayEnabled(true, true, this);
  addAndMakeVisible(panKnob);
  panAttach =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          proc.getAPVTS(), prefix + "Pan", panKnob);

  panLabel.setFont(juce::Font(10.0f));
  panLabel.setJustificationType(juce::Justification::centred);
  panLabel.setColour(juce::Label::textColourId, juce::Colour(0xff888888));
  addAndMakeVisible(panLabel);

  // Delay knob
  delayKnob.setSliderStyle(juce::Slider::RotaryVerticalDrag);
  delayKnob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
  delayKnob.setDoubleClickReturnValue(true, 0.0);
  delayKnob.setPopupDisplayEnabled(true, true, this);
  delayKnob.addListener(this);
  addAndMakeVisible(delayKnob);
  delayAttach =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          proc.getAPVTS(), prefix + "DelayMs", delayKnob);

  delayLabel.setFont(juce::Font(10.0f));
  delayLabel.setJustificationType(juce::Justification::centred);
  delayLabel.setColour(juce::Label::textColourId, juce::Colour(0xff888888));
  addAndMakeVisible(delayLabel);

  // Level fader
  levelFader.setSliderStyle(juce::Slider::LinearVertical);
  levelFader.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
  levelFader.setDoubleClickReturnValue(true, -6.0);
  levelFader.setPopupDisplayEnabled(true, true, this);
  addAndMakeVisible(levelFader);
  levelAttach =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          proc.getAPVTS(), prefix + "Level", levelFader);

  levelLabel.setFont(juce::Font(10.0f));
  levelLabel.setJustificationType(juce::Justification::centred);
  levelLabel.setColour(juce::Label::textColourId, juce::Colour(0xff888888));
  addAndMakeVisible(levelLabel);
}

IRSlotComponent::~IRSlotComponent() { delayKnob.removeListener(this); }

void IRSlotComponent::sliderValueChanged(juce::Slider *slider) {
  if (slider == &delayKnob) {
    if (onSlotChanged)
      onSlotChanged();
  }
}

void IRSlotComponent::setDelayEnabled(bool enabled) {
  delayKnob.setEnabled(enabled);
  delayKnob.setAlpha(enabled ? 1.0f : 0.35f);
  delayLabel.setAlpha(enabled ? 1.0f : 0.35f);
}

void IRSlotComponent::paint(juce::Graphics &g) {
  auto bounds = getLocalBounds().toFloat();

  // Glass card background
  g.setColour(juce::Colour(0x0dffffff));
  g.fillRoundedRectangle(bounds, 12.0f);

  // Border
  g.setColour(juce::Colour(0x1affffff));
  g.drawRoundedRectangle(bounds.reduced(0.5f), 12.0f, 1.0f);

  // Slot colour accent stripe at top
  auto stripe = juce::Rectangle<float>(bounds.getX() + 12, bounds.getY() + 1,
                                       bounds.getWidth() - 24, 2.0f);
  g.setColour(slotColours[slotID]);
  g.fillRoundedRectangle(stripe, 1.0f);
}

void IRSlotComponent::resized() {
  auto area = getLocalBounds().reduced(6);

  // Slot number + clear button
  auto topRow = area.removeFromTop(26);
  slotNumLabel.setBounds(topRow.removeFromLeft(topRow.getWidth() / 2));
  clearButton.setBounds(topRow.reduced(2));

  area.removeFromTop(4);

  // S / M buttons side by side
  auto smRow = area.removeFromTop(26);
  int halfW = smRow.getWidth() / 2;
  soloButton.setBounds(smRow.removeFromLeft(halfW).reduced(2));
  muteButton.setBounds(smRow.reduced(2));

  area.removeFromTop(6);

  // Pan knob
  int knobSize = juce::jmin(area.getWidth() - 8, 54);
  auto panArea = area.removeFromTop(knobSize + 16);
  panKnob.setBounds(panArea.removeFromTop(knobSize).withSizeKeepingCentre(
      knobSize, knobSize));
  panLabel.setBounds(panArea);

  area.removeFromTop(4);

  // Delay knob
  auto delayArea = area.removeFromTop(knobSize + 16);
  delayKnob.setBounds(delayArea.removeFromTop(knobSize).withSizeKeepingCentre(
      knobSize, knobSize));
  delayLabel.setBounds(delayArea);

  area.removeFromTop(6);

  // Level fader (fills rest)
  auto levelArea = area.removeFromTop(area.getHeight() - 16);
  levelFader.setBounds(levelArea.withSizeKeepingCentre(
      juce::jmin(area.getWidth() - 8, 36), levelArea.getHeight()));
  levelLabel.setBounds(area);
}

void IRSlotComponent::updateSlotDisplay() { repaint(); }
