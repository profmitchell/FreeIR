#include "EQProcessor.h"

EQProcessor::EQProcessor(juce::AudioProcessorValueTreeState &state)
    : apvts(state) {}

void EQProcessor::cacheParameterPointers() {
  loCutParam = apvts.getRawParameterValue("LoCutHz");
  bassParam = apvts.getRawParameterValue("BassGainDb");
  midFreqParam = apvts.getRawParameterValue("MidFreqHz");
  midQParam = apvts.getRawParameterValue("MidQ");
  midGainParam = apvts.getRawParameterValue("MidGainDb");
  trebleParam = apvts.getRawParameterValue("TrebleGainDb");
  airParam = apvts.getRawParameterValue("AirGainDb");
  hiCutParam = apvts.getRawParameterValue("HiCutHz");
}

void EQProcessor::prepare(const juce::dsp::ProcessSpec &spec) {
  sampleRate = spec.sampleRate;

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

  cacheParameterPointers();

  // Force initial coefficient calculation
  prevLoCut = -1.0f;
  prevBass = -999.0f;
  prevMidFreq = -1.0f;
  prevMidQ = -1.0f;
  prevMidGain = -999.0f;
  prevTreble = -999.0f;
  prevAir = -999.0f;
  prevHiCut = -1.0f;
  updateParametersIfNeeded();
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
  updateParametersIfNeeded();

  int numSamples = buffer.getNumSamples();
  int numChannels = buffer.getNumChannels();

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

void EQProcessor::updateParametersIfNeeded() {
  if (sampleRate <= 0.0 || loCutParam == nullptr)
    return;

  float loCut = loCutParam->load();
  float bass = bassParam->load();
  float midFreq = midFreqParam->load();
  float midQ = midQParam->load();
  float midGain = midGainParam->load();
  float treble = trebleParam->load();
  float air = airParam->load();
  float hiCut = hiCutParam->load();

  if (loCut != prevLoCut) {
    auto c =
        juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, loCut);
    loCutFilterL.coefficients = c;
    loCutFilterR.coefficients = c;
    prevLoCut = loCut;
  }

  if (bass != prevBass) {
    auto c = juce::dsp::IIR::Coefficients<float>::makeLowShelf(
        sampleRate, 100.0f, 0.707f, juce::Decibels::decibelsToGain(bass));
    bassFilterL.coefficients = c;
    bassFilterR.coefficients = c;
    prevBass = bass;
  }

  if (midFreq != prevMidFreq || midQ != prevMidQ || midGain != prevMidGain) {
    auto c = juce::dsp::IIR::Coefficients<float>::makePeakFilter(
        sampleRate, midFreq, midQ, juce::Decibels::decibelsToGain(midGain));
    midFilterL.coefficients = c;
    midFilterR.coefficients = c;
    prevMidFreq = midFreq;
    prevMidQ = midQ;
    prevMidGain = midGain;
  }

  if (treble != prevTreble) {
    auto c = juce::dsp::IIR::Coefficients<float>::makeHighShelf(
        sampleRate, 3000.0f, 0.707f, juce::Decibels::decibelsToGain(treble));
    trebleFilterL.coefficients = c;
    trebleFilterR.coefficients = c;
    prevTreble = treble;
  }

  if (air != prevAir) {
    auto c = juce::dsp::IIR::Coefficients<float>::makeHighShelf(
        sampleRate, 10000.0f, 0.707f, juce::Decibels::decibelsToGain(air));
    airFilterL.coefficients = c;
    airFilterR.coefficients = c;
    prevAir = air;
  }

  if (hiCut != prevHiCut) {
    auto c =
        juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, hiCut);
    hiCutFilterL.coefficients = c;
    hiCutFilterR.coefficients = c;
    prevHiCut = hiCut;
  }
}
