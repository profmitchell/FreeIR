#include "IRSlotComponent.h"

IRSlotComponent::IRSlotComponent(FreeIRAudioProcessor &processor, int slotIndex)
    : proc(processor), slotID(slotIndex) {
  auto prefix = "Slot" + juce::String(slotID + 1) + "_";

  // Slot number label
  slotNumLabel.setText(juce::String(slotID + 1), juce::dontSendNotification);
  slotNumLabel.setFont(juce::Font(20.0f, juce::Font::bold));
  slotNumLabel.setColour(juce::Label::textColourId, slotColours[slotID]);
  slotNumLabel.setJustificationType(juce::Justification::centred);
  addAndMakeVisible(slotNumLabel);

  // Clear button (trash icon)
  clearButton.setButtonText(
      juce::String::charToString(0x1F5D1)); // Unicode trash
  clearButton.onClick = [this]() {
    proc.getIRSlot(slotID).clearImpulseResponse();
    updateSlotDisplay();
    if (onSlotChanged)
      onSlotChanged();
  };
  addAndMakeVisible(clearButton);

  // Solo
  soloButton.setClickingTogglesState(true);
  soloButton.setColour(juce::TextButton::buttonOnColourId,
                       juce::Colour(0xffddaa00));
  addAndMakeVisible(soloButton);
  soloAttach =
      std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
          proc.getAPVTS(), prefix + "Solo", soloButton);

  // Mute
  muteButton.setClickingTogglesState(true);
  muteButton.setColour(juce::TextButton::buttonOnColourId,
                       juce::Colour(0xffcc3333));
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

  panLabel.setFont(juce::Font(11.0f));
  panLabel.setJustificationType(juce::Justification::centred);
  addAndMakeVisible(panLabel);

  // Delay knob
  delayKnob.setSliderStyle(juce::Slider::RotaryVerticalDrag);
  delayKnob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
  delayKnob.setDoubleClickReturnValue(true, 0.0);
  delayKnob.setPopupDisplayEnabled(true, true, this);
  addAndMakeVisible(delayKnob);
  delayAttach =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          proc.getAPVTS(), prefix + "DelayMs", delayKnob);

  delayLabel.setFont(juce::Font(11.0f));
  delayLabel.setJustificationType(juce::Justification::centred);
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

  levelLabel.setFont(juce::Font(11.0f));
  levelLabel.setJustificationType(juce::Justification::centred);
  addAndMakeVisible(levelLabel);
}

IRSlotComponent::~IRSlotComponent() {}

void IRSlotComponent::paint(juce::Graphics &g) {
  auto bounds = getLocalBounds().toFloat();
  g.setColour(juce::Colour(0xff1e1e1e));
  g.fillRoundedRectangle(bounds, 4.0f);

  // Slot colour stripe at top
  g.setColour(slotColours[slotID]);
  g.fillRect(bounds.getX() + 4.0f, bounds.getY(), bounds.getWidth() - 8.0f,
             3.0f);
}

void IRSlotComponent::resized() {
  auto area = getLocalBounds().reduced(4);
  int y = area.getY() + 8;

  // Slot number + clear button
  auto topRow = area.removeFromTop(28);
  slotNumLabel.setBounds(topRow.removeFromLeft(topRow.getWidth() / 2));
  clearButton.setBounds(topRow.reduced(2));

  area.removeFromTop(4);

  // S / M buttons side by side
  auto smRow = area.removeFromTop(28);
  int halfW = smRow.getWidth() / 2;
  soloButton.setBounds(smRow.removeFromLeft(halfW).reduced(2));
  muteButton.setBounds(smRow.reduced(2));

  area.removeFromTop(6);

  // Pan knob
  int knobSize = juce::jmin(area.getWidth() - 8, 60);
  auto panArea = area.removeFromTop(knobSize + 18);
  panKnob.setBounds(panArea.removeFromTop(knobSize).withSizeKeepingCentre(
      knobSize, knobSize));
  panLabel.setBounds(panArea);

  area.removeFromTop(4);

  // Delay knob
  auto delayArea = area.removeFromTop(knobSize + 18);
  delayKnob.setBounds(delayArea.removeFromTop(knobSize).withSizeKeepingCentre(
      knobSize, knobSize));
  delayLabel.setBounds(delayArea);

  area.removeFromTop(6);

  // Level fader (fills rest)
  auto levelArea = area.removeFromTop(area.getHeight() - 18);
  levelFader.setBounds(levelArea.withSizeKeepingCentre(
      juce::jmin(area.getWidth() - 8, 40), levelArea.getHeight()));
  levelLabel.setBounds(area);
}

void IRSlotComponent::updateSlotDisplay() { repaint(); }
