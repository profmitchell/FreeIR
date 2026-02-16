#pragma once

#include "../PluginProcessor.h"
#include <JuceHeader.h>

//==============================================================================
// EQSectionComponent: Bottom row of EQ knobs
//==============================================================================
class EQSectionComponent : public juce::Component {
public:
  EQSectionComponent(FreeIRAudioProcessor &processor);
  ~EQSectionComponent() override;

  void resized() override;
  void paint(juce::Graphics &g) override;

private:
  FreeIRAudioProcessor &proc;

  // EQ knobs
  juce::Slider loCutKnob, hiCutKnob, bassKnob, trebleKnob, airKnob;
  juce::Slider midGainKnob, midFreqKnob, midQKnob;
  juce::Slider outputKnob;

  juce::Label loCutLabel{{}, "Lo Cut"};
  juce::Label hiCutLabel{{}, "Hi Cut"};
  juce::Label bassLabel{{}, "Bass"};
  juce::Label trebleLabel{{}, "Treble"};
  juce::Label airLabel{{}, "Air"};
  juce::Label midGainLabel{{}, "Gain"};
  juce::Label midFreqLabel{{}, "Freq"};
  juce::Label midQLabel{{}, "Q"};
  juce::Label outputLabel{{}, "Out"};

  // APVTS attachments
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      loCutAttach;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      hiCutAttach;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      bassAttach;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      trebleAttach;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      airAttach;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      midGainAttach;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      midFreqAttach;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      midQAttach;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      outputAttach;

  void setupKnob(
      juce::Slider &knob, juce::Label &label, const juce::String &paramID,
      std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
          &attach,
      double defaultVal);

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EQSectionComponent)
};
