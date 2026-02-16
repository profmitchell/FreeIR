#pragma once

#include <JuceHeader.h>

class EQProcessor {
public:
  EQProcessor(juce::AudioProcessorValueTreeState &apvts);

  void prepare(const juce::dsp::ProcessSpec &spec);
  void reset();

  void process(juce::AudioBuffer<float> &buffer);

  void updateParameters();

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
};
