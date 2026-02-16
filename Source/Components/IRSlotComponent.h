#pragma once

#include "../PluginProcessor.h"
#include <JuceHeader.h>

//==============================================================================
// IRSlotComponent: UI strip for a single IR slot
//==============================================================================
class IRSlotComponent : public juce::Component {
public:
  IRSlotComponent(FreeIRAudioProcessor &processor, int slotIndex);
  ~IRSlotComponent() override;

  void resized() override;
  void paint(juce::Graphics &g) override;
  void updateSlotDisplay();

  void setDelayEnabled(bool enabled);

  // Slider::Listener — fires when delay knob moves
  // Slider::Listener removed

  // Callback for parent to refresh waveform/IR list
  std::function<void()> onSlotChanged;

private:
  FreeIRAudioProcessor &proc;
  int slotID;

  // UI Components
  juce::TextButton prevButton{"<"};
  juce::TextButton nextButton{">"};
  juce::TextButton loadButton; // Displays IR Name
  juce::TextButton clearButton;

  juce::Slider delayKnob;
  juce::Label delayLabel;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      delayAttach;

  juce::Slider panKnob;
  juce::Label panLabel;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      panAttach;

  // Fader & Mute/Solo
  juce::Slider fader;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      faderAttach;

  juce::TextButton muteButton{"M"};
  juce::TextButton soloButton{"S"};

  std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>
      muteAttach;
  std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>
      soloAttach;

  juce::Label slotNumLabel;
  juce::SharedResourcePointer<juce::TooltipWindow> tooltipWindow;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(IRSlotComponent)
};
