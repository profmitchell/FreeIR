#include "EQProcessor.h"

EQProcessor::EQProcessor(juce::AudioProcessorValueTreeState &state)
    : apvts(state) {}

void EQProcessor::prepare(const juce::dsp::ProcessSpec &spec) {
  sampleRate = spec.sampleRate;

  // Prepare all filters with mono spec (we process L and R separately)
  juce::dsp::ProcessSpec monoSpec;
  monoSpec.sampleRate = spec.sampleRate;
  monoSpec.maximumBlockSize = spec.maximumBlockSize;
  monoSpec.numChannels = 1;

  loCutFilterL.prepare(monoSpec);
  loCutFilterR.prepare(monoSpec);
  bassFilterL.prepare(monoSpec);
  bassFilterR.prepare(monoSpec);
  midFilterL.prepare(monoSpec);
  midFilterR.prepare(monoSpec);
  trebleFilterL.prepare(monoSpec);
  trebleFilterR.prepare(monoSpec);
  airFilterL.prepare(monoSpec);
  airFilterR.prepare(monoSpec);
  hiCutFilterL.prepare(monoSpec);
  hiCutFilterR.prepare(monoSpec);

  updateParameters();
}

void EQProcessor::reset() {
  loCutFilterL.reset();
  loCutFilterR.reset();
  bassFilterL.reset();
  bassFilterR.reset();
  midFilterL.reset();
  midFilterR.reset();
  trebleFilterL.reset();
  trebleFilterR.reset();
  airFilterL.reset();
  airFilterR.reset();
  hiCutFilterL.reset();
  hiCutFilterR.reset();
}

void EQProcessor::process(juce::AudioBuffer<float> &buffer) {
  updateParameters();

  int numSamples = buffer.getNumSamples();
  int numChannels = buffer.getNumChannels();

  // Process L channel
  if (numChannels > 0) {
    auto *dataL = buffer.getWritePointer(0);
    for (int i = 0; i < numSamples; ++i) {
      float s = dataL[i];
      s = loCutFilterL.processSample(s);
      s = bassFilterL.processSample(s);
      s = midFilterL.processSample(s);
      s = trebleFilterL.processSample(s);
      s = airFilterL.processSample(s);
      s = hiCutFilterL.processSample(s);
      dataL[i] = s;
    }
  }

  // Process R channel
  if (numChannels > 1) {
    auto *dataR = buffer.getWritePointer(1);
    for (int i = 0; i < numSamples; ++i) {
      float s = dataR[i];
      s = loCutFilterR.processSample(s);
      s = bassFilterR.processSample(s);
      s = midFilterR.processSample(s);
      s = trebleFilterR.processSample(s);
      s = airFilterR.processSample(s);
      s = hiCutFilterR.processSample(s);
      dataR[i] = s;
    }
  }
}

void EQProcessor::updateParameters() {
  if (sampleRate <= 0.0)
    return;

  // LoCut (HighPass)
  float loCutFreq = apvts.getRawParameterValue("LoCutHz")->load();
  auto loCutCoeffs =
      juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, loCutFreq);
  loCutFilterL.coefficients = loCutCoeffs;
  loCutFilterR.coefficients = loCutCoeffs;

  // Bass (LowShelf at 100Hz)
  float bassGain = apvts.getRawParameterValue("BassGainDb")->load();
  auto bassCoeffs = juce::dsp::IIR::Coefficients<float>::makeLowShelf(
      sampleRate, 100.0f, 0.707f, juce::Decibels::decibelsToGain(bassGain));
  bassFilterL.coefficients = bassCoeffs;
  bassFilterR.coefficients = bassCoeffs;

  // Mid (Peak)
  float midFreq = apvts.getRawParameterValue("MidFreqHz")->load();
  float midQ = apvts.getRawParameterValue("MidQ")->load();
  float midGain = apvts.getRawParameterValue("MidGainDb")->load();
  auto midCoeffs = juce::dsp::IIR::Coefficients<float>::makePeakFilter(
      sampleRate, midFreq, midQ, juce::Decibels::decibelsToGain(midGain));
  midFilterL.coefficients = midCoeffs;
  midFilterR.coefficients = midCoeffs;

  // Treble (HighShelf at ~3kHz)
  float trebleGain = apvts.getRawParameterValue("TrebleGainDb")->load();
  auto trebleCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighShelf(
      sampleRate, 3000.0f, 0.707f, juce::Decibels::decibelsToGain(trebleGain));
  trebleFilterL.coefficients = trebleCoeffs;
  trebleFilterR.coefficients = trebleCoeffs;

  // Air (HighShelf at ~10kHz)
  float airGain = apvts.getRawParameterValue("AirGainDb")->load();
  auto airCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighShelf(
      sampleRate, 10000.0f, 0.707f, juce::Decibels::decibelsToGain(airGain));
  airFilterL.coefficients = airCoeffs;
  airFilterR.coefficients = airCoeffs;

  // HiCut (LowPass)
  float hiCutFreq = apvts.getRawParameterValue("HiCutHz")->load();
  auto hiCutCoeffs =
      juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, hiCutFreq);
  hiCutFilterL.coefficients = hiCutCoeffs;
  hiCutFilterR.coefficients = hiCutCoeffs;
}
