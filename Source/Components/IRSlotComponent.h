#pragma once

#include "../PluginProcessor.h"
#include <JuceHeader.h>

//==============================================================================
// IRSlotComponent: UI strip for a single IR slot
//==============================================================================
class IRSlotComponent : public juce::Component, public juce::Slider::Listener {
public:
  IRSlotComponent(FreeIRAudioProcessor &processor, int slotIndex);
  ~IRSlotComponent() override;

  void resized() override;
  void paint(juce::Graphics &g) override;
  void updateSlotDisplay();

  void setDelayEnabled(bool enabled);

  // Slider::Listener — fires when delay knob moves
  void sliderValueChanged(juce::Slider *slider) override;

  // Callback for parent to refresh waveform/IR list
  std::function<void()> onSlotChanged;

private:
  FreeIRAudioProcessor &proc;
  int slotID;

  juce::TextButton soloButton{"S"};
  juce::TextButton muteButton{"M"};
  juce::TextButton clearButton;

  juce::Slider panKnob;
  juce::Slider delayKnob;
  juce::Slider levelFader;

  juce::Label panLabel{{}, "Pan"};
  juce::Label delayLabel{{}, "Delay"};
  juce::Label levelLabel{{}, "Level"};
  juce::Label slotNumLabel;

  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      panAttach;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      delayAttach;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      levelAttach;
  std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>
      soloAttach;
  std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>
      muteAttach;

  // Slot colours
  const std::array<juce::Colour, 4> slotColours = {
      juce::Colour(0xffff4444), juce::Colour(0xff44ff44),
      juce::Colour(0xff4488ff), juce::Colour(0xffcc44ff)};

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(IRSlotComponent)
};
