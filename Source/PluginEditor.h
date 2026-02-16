#pragma once

#include "AutoAligner.h"
#include "Components/EQSectionComponent.h"
#include "Components/IRSlotComponent.h"
#include "Components/WaveformDisplay.h"
#include "FreeIRLookAndFeel.h"
#include "PluginProcessor.h"
#include <JuceHeader.h>

//==============================================================================
class FreeIREditor : public juce::AudioProcessorEditor,
                     public juce::Timer,
                     public AutoAligner::Listener {
public:
  explicit FreeIREditor(FreeIRAudioProcessor &processor);
  ~FreeIREditor() override;

  void paint(juce::Graphics &g) override;
  void resized() override;

  void timerCallback() override;
  void alignmentComplete() override;

private:
  FreeIRAudioProcessor &proc;
  FreeIRLookAndFeel lnf;

  // Header controls
  juce::Label titleLabel{{}, "FreeIR"};
  juce::Slider bassHeaderKnob, trebleHeaderKnob, airHeaderKnob, volumeKnob;
  juce::Label bassHeaderLabel{{}, "Bass"};
  juce::Label trebleHeaderLabel{{}, "Treble"};
  juce::Label airHeaderLabel{{}, "Air"};
  juce::Label volumeLabel{{}, "Volume"};

  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      bassHeaderAttach;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      trebleHeaderAttach;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      airHeaderAttach;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      volumeAttach;

  // 4 Slot strips
  std::array<std::unique_ptr<IRSlotComponent>, 4> slotComponents;

  // IR List area (clickable buttons styled as text)
  std::array<std::unique_ptr<juce::TextButton>, 4> irListButtons;

  // Waveform display
  WaveformDisplay waveformDisplay;

  // Auto Align button
  juce::TextButton autoAlignButton{"Auto Align"};

  // Export button
  juce::TextButton exportButton{"Export IR"};

  // EQ Section
  EQSectionComponent eqSection;

  // Helpers
  void refreshIRList();
  void refreshWaveform();
  void loadIRForSlot(int slotIndex);

  void setupHeaderKnob(
      juce::Slider &knob, juce::Label &label, const juce::String &paramID,
      std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
          &attach,
      double defaultVal);

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FreeIREditor)
};
