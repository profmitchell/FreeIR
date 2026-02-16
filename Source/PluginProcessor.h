#pragma once

#include "AutoAligner.h"
#include "EQProcessor.h"
#include "IRSlot.h"
#include <JuceHeader.h>

class FreeIRAudioProcessor : public juce::AudioProcessor {
public:
  FreeIRAudioProcessor();
  ~FreeIRAudioProcessor() override;

  void prepareToPlay(double sampleRate, int samplesPerBlock) override;
  void releaseResources() override;

#ifndef JucePlugin_PreferredChannelConfigurations
  bool isBusesLayoutSupported(const BusesLayout &layouts) const override;
#endif

  void processBlock(juce::AudioBuffer<float> &, juce::MidiBuffer &) override;

  juce::AudioProcessorEditor *createEditor() override;
  bool hasEditor() const override;

  const juce::String getName() const override;

  bool acceptsMidi() const override;
  bool producesMidi() const override;
  bool isMidiEffect() const override;
  double getTailLengthSeconds() const override;

  int getNumPrograms() override;
  int getCurrentProgram() override;
  void setCurrentProgram(int index) override;
  const juce::String getProgramName(int index) override;
  void changeProgramName(int index, const juce::String &newName) override;

  void getStateInformation(juce::MemoryBlock &destData) override;
  void setStateInformation(const void *data, int sizeInBytes) override;

  juce::AudioProcessorValueTreeState &getAPVTS() { return apvts; }

  IRSlot &getIRSlot(int index) { return slots[index]; }
  const IRSlot &getIRSlot(int index) const { return slots[index]; }
  EQProcessor &getEQ() { return eqProcessor; }
  AutoAligner &getAutoAligner() { return autoAligner; }

  static constexpr int numSlots = 4;

  // Export mixed IR to a WAV file
  bool exportMixedIR(const juce::File &outputFile,
                     double exportSampleRate = 48000.0,
                     int exportLengthSamples = 4096);

private:
  juce::AudioProcessorValueTreeState apvts;
  juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

  std::array<IRSlot, numSlots> slots;
  EQProcessor eqProcessor;
  AutoAligner autoAligner;

  juce::AudioBuffer<float> mixBuffer;

  double currentSampleRate = 48000.0;
  int currentBlockSize = 512;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FreeIRAudioProcessor)
};
