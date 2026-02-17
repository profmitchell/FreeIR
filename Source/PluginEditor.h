#pragma once

#include "AutoAligner.h"
#include "Components/EQSectionComponent.h"
#include "Components/IRBrowserComponent.h"
#include "Components/IRSlotComponent.h"
#include "Components/PresetBrowserComponent.h"
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
  juce::Label subtitleLabel{{}, "Cohen Concepts"};
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

  // Browser Panel
  IRBrowserComponent browser;
  PresetBrowserComponent presetBrowser;

  // Waveform display
  WaveformDisplay waveformDisplay;

  // Auto Align toggle button (changed from text button)
  juce::TextButton autoAlignButton{"Auto Align"};
  bool isAutoAlignOn = false;

  // Settings & Export
  juce::TextButton settingsButton{"Settings"};
  juce::TextButton exportButton{"Export IR"};

  // EQ Section
  EQSectionComponent eqSection;

  // Responsive resizing constrainer
  juce::ComponentBoundsConstrainer constrainer;
  static constexpr int defaultWidth = 1100;
  static constexpr int defaultHeight = 720;

  // Helpers
  void refreshIRList();
  void refreshWaveform();
  void updateAutoAlignState();
  void loadIRForSlot(int slotIndex);

  void setupHeaderKnob(
      juce::Slider &knob, juce::Label &label, const juce::String &paramID,
      std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
          &attach,
      double defaultVal);

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FreeIREditor)
};
