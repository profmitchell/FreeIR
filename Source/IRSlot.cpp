#include "IRSlot.h"

IRSlot::IRSlot() {}

void IRSlot::init(int index, juce::AudioProcessorValueTreeState *apvtsPtr) {
  slotID = index;
  apvts = apvtsPtr;

  // Cache parameter pointers once -- avoids String construction on audio thread
  if (apvts != nullptr) {
    auto prefix = "Slot" + juce::String(slotID + 1) + "_";
    delayParam = apvts->getRawParameterValue(prefix + "DelayMs");
    panParam = apvts->getRawParameterValue(prefix + "Pan");
    levelParam = apvts->getRawParameterValue(prefix + "Level");
    muteParam = apvts->getRawParameterValue(prefix + "Mute");
    soloParam = apvts->getRawParameterValue(prefix + "Solo");
  }
}

void IRSlot::prepare(const juce::dsp::ProcessSpec &spec) {
  sampleRate = spec.sampleRate;
  blockSize = (int)spec.maximumBlockSize;
  convolution.prepare(spec);
  delayLine.prepare(spec);
  delayLine.setMaximumDelayInSamples(4800);
  delaySmoothed.reset(sampleRate, 0.02);

  slotBuffer.setSize(2, blockSize);
}

void IRSlot::reset() {
  convolution.reset();
  delayLine.reset();
}

void IRSlot::process(const juce::AudioBuffer<float> &input,
                     juce::AudioBuffer<float> &mixBuffer) {
  if (!isLoaded() || delayParam == nullptr)
    return;

  if (muteParam->load() > 0.5f)
    return;

  int numSamples = input.getNumSamples();
  int numChannels = juce::jmin(input.getNumChannels(), 2);

  slotBuffer.setSize(2, numSamples, false, false, true);
  slotBuffer.clear();

  for (int ch = 0; ch < 2; ++ch) {
    int srcCh = juce::jmin(ch, numChannels - 1);
    slotBuffer.copyFrom(ch, 0, input, srcCh, 0, numSamples);
  }

  // 1. Convolution
  juce::dsp::AudioBlock<float> block(slotBuffer);
  juce::dsp::ProcessContextReplacing<float> context(block);
  convolution.process(context);

  // 2. Delay (User Delay + Alignment Delay)
  float userDelayMs = delayParam->load();
  float totalDelayMs = userDelayMs + (float)alignmentDelayMs;
  float delaySamples = (float)(totalDelayMs * 0.001 * sampleRate);
  delaySamples = juce::jlimit(0.0f, 4799.0f, delaySamples);
  delaySmoothed.setTargetValue(delaySamples);

  for (int i = 0; i < numSamples; ++i) {
    float d = delaySmoothed.getNextValue();
    delayLine.setDelay(d);
    for (int ch = 0; ch < 2; ++ch) {
      float in = slotBuffer.getSample(ch, i);
      delayLine.pushSample(ch, in);
      slotBuffer.setSample(ch, i, delayLine.popSample(ch));
    }
  }

  // 3. Pan (constant power) -- raw pointer access
  float panVal = panParam->load();
  float pan01 = panVal / 100.0f;
  float angle = (pan01 + 1.0f) * juce::MathConstants<float>::pi * 0.25f;
  float gainL = std::cos(angle);
  float gainR = std::sin(angle);

  {
    float *dataL = slotBuffer.getWritePointer(0);
    float *dataR = slotBuffer.getWritePointer(1);
    for (int i = 0; i < numSamples; ++i) {
      dataL[i] *= gainL;
      dataR[i] *= gainR;
    }
  }

  // 4. Level
  float levelDb = levelParam->load();
  float levelGain = juce::Decibels::decibelsToGain(levelDb, -60.0f);
  slotBuffer.applyGain(levelGain);

  // 5. Sum to mix bus
  for (int ch = 0; ch < 2; ++ch)
    mixBuffer.addFrom(ch, 0, slotBuffer, ch, 0, numSamples);
}

void IRSlot::loadImpulseResponse(const juce::File &file) {
  if (!file.existsAsFile())
    return;

  currentFile = file;
  convolution.loadImpulseResponse(file, juce::dsp::Convolution::Stereo::yes,
                                  juce::dsp::Convolution::Trim::yes, 0);

  juce::AudioFormatManager formatManager;
  formatManager.registerBasicFormats();

  std::unique_ptr<juce::AudioFormatReader> reader(
      formatManager.createReaderFor(file));
  if (reader) {
    irSampleRate = reader->sampleRate;
    irBuffer.setSize((int)reader->numChannels, (int)reader->lengthInSamples);
    reader->read(&irBuffer, 0, (int)reader->lengthInSamples, 0, true, true);
  }
}

void IRSlot::clearImpulseResponse() {
  currentFile = juce::File();
  irBuffer.setSize(0, 0);
  alignmentDelayMs = 0.0;
}

juce::String IRSlot::getSlotName() const {
  if (currentFile.existsAsFile())
    return currentFile.getFileNameWithoutExtension();
  return "[Empty]";
}

bool IRSlot::isMuted() const {
  if (muteParam == nullptr)
    return false;
  return muteParam->load() > 0.5f;
}

bool IRSlot::isSoloed() const {
  if (soloParam == nullptr)
    return false;
  return soloParam->load() > 0.5f;
}

void IRSlot::setAlignmentDelay(double ms) { alignmentDelayMs = ms; }
double IRSlot::getAlignmentDelay() const { return alignmentDelayMs; }

void IRSlot::loadNextIR() {
  if (!currentFile.exists())
    return;

  auto parent = currentFile.getParentDirectory();
  auto files =
      parent.findChildFiles(juce::File::findFiles, false, "*.wav;*.aif;*.aiff");

  if (files.isEmpty())
    return;

  int index = -1;
  for (int i = 0; i < files.size(); ++i) {
    if (files[i] == currentFile) {
      index = i;
      break;
    }
  }

  if (index != -1) {
    int nextIndex = (index + 1) % files.size();
    loadImpulseResponse(files[nextIndex]);
  }
}

void IRSlot::loadPrevIR() {
  if (!currentFile.exists())
    return;

  auto parent = currentFile.getParentDirectory();
  auto files =
      parent.findChildFiles(juce::File::findFiles, false, "*.wav;*.aif;*.aiff");

  if (files.isEmpty())
    return;

  int index = -1;
  for (int i = 0; i < files.size(); ++i) {
    if (files[i] == currentFile) {
      index = i;
      break;
    }
  }

  if (index != -1) {
    int prevIndex = (index - 1 + files.size()) % files.size();
    loadImpulseResponse(files[prevIndex]);
  }
}
