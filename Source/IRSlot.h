#pragma once

#include <JuceHeader.h>

class IRSlot {
public:
  IRSlot();
  void init(int slotIndex, juce::AudioProcessorValueTreeState *apvtsPtr);

  void prepare(const juce::dsp::ProcessSpec &spec);
  void reset();

  // Process input and ADD result into mixBuffer (stereo)
  void process(const juce::AudioBuffer<float> &input,
               juce::AudioBuffer<float> &mixBuffer);

  void loadImpulseResponse(const juce::File &file);
  void clearImpulseResponse();

  juce::File getCurrentFile() const { return currentFile; }
  juce::String getSlotName() const;
  bool isLoaded() const { return currentFile.existsAsFile(); }

  const juce::AudioBuffer<float> &getIRBuffer() const { return irBuffer; }
  double getIRSampleRate() const { return irSampleRate; }

  void setAlignmentDelay(double delayMs);
  double getAlignmentDelay() const;
  double manualDelayMs = 0.0;

  // Navigation
  void loadNextIR();
  void loadPrevIR();

  bool isMuted() const;
  bool isSoloed() const;

  int getSlotID() const { return slotID; }

private:
  int slotID = 0;
  juce::AudioProcessorValueTreeState *apvts = nullptr;

  juce::dsp::Convolution convolution;
  juce::AudioBuffer<float> irBuffer;
  double irSampleRate = 48000.0;
  juce::File currentFile;

  juce::dsp::DelayLine<float,
                       juce::dsp::DelayLineInterpolationTypes::Lagrange3rd>
      delayLine{4800};
  juce::SmoothedValue<float> delaySmoothed;

  juce::AudioBuffer<float> slotBuffer;

  double sampleRate = 48000.0;
  int blockSize = 512;
  double alignmentDelayMs = 0.0;

  // Cached raw parameter pointers (resolved once in init, no string alloc)
  std::atomic<float> *delayParam = nullptr;
  std::atomic<float> *panParam = nullptr;
  std::atomic<float> *levelParam = nullptr;
  std::atomic<float> *muteParam = nullptr;
  std::atomic<float> *soloParam = nullptr;
};
