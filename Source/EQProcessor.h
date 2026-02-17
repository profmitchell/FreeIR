#pragma once

#include <JuceHeader.h>

class EQProcessor {
public:
  EQProcessor(juce::AudioProcessorValueTreeState &apvts);

  void prepare(const juce::dsp::ProcessSpec &spec);
  void reset();

  void process(juce::AudioBuffer<float> &buffer);

private:
  juce::AudioProcessorValueTreeState &apvts;
  double sampleRate = 48000.0;

  // Individual filters (not a chain, for clarity and control)
  juce::dsp::IIR::Filter<float> loCutFilterL, loCutFilterR;
  juce::dsp::IIR::Filter<float> bassFilterL, bassFilterR;
  juce::dsp::IIR::Filter<float> midFilterL, midFilterR;
  juce::dsp::IIR::Filter<float> trebleFilterL, trebleFilterR;
  juce::dsp::IIR::Filter<float> airFilterL, airFilterR;
  juce::dsp::IIR::Filter<float> hiCutFilterL, hiCutFilterR;

  // Cached raw parameter pointers (resolved once in prepare, never reallocated)
  std::atomic<float> *loCutParam = nullptr;
  std::atomic<float> *bassParam = nullptr;
  std::atomic<float> *midFreqParam = nullptr;
  std::atomic<float> *midQParam = nullptr;
  std::atomic<float> *midGainParam = nullptr;
  std::atomic<float> *trebleParam = nullptr;
  std::atomic<float> *airParam = nullptr;
  std::atomic<float> *hiCutParam = nullptr;

  // Previous values for change detection (avoids coefficient recalc every block)
  float prevLoCut = -1.0f;
  float prevBass = -999.0f;
  float prevMidFreq = -1.0f;
  float prevMidQ = -1.0f;
  float prevMidGain = -999.0f;
  float prevTreble = -999.0f;
  float prevAir = -999.0f;
  float prevHiCut = -1.0f;

  void updateParametersIfNeeded();
  void cacheParameterPointers();
};
