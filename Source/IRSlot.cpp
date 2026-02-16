#include "IRSlot.h"

IRSlot::IRSlot() {}

void IRSlot::init(int index, juce::AudioProcessorValueTreeState *apvtsPtr) {
  slotID = index;
  apvts = apvtsPtr;
}

juce::String IRSlot::paramPrefix() const {
  return "Slot" + juce::String(slotID + 1) + "_";
}

void IRSlot::prepare(const juce::dsp::ProcessSpec &spec) {
  sampleRate = spec.sampleRate;
  blockSize = (int)spec.maximumBlockSize;
  convolution.prepare(spec);
  delayLine.prepare(spec);
  delayLine.setMaximumDelayInSamples(4800);
  delaySmoothed.reset(sampleRate, 0.02); // 20ms ramp

  slotBuffer.setSize(2, blockSize);
}

void IRSlot::reset() {
  convolution.reset();
  delayLine.reset();
}

void IRSlot::process(const juce::AudioBuffer<float> &input,
                     juce::AudioBuffer<float> &mixBuffer) {
  if (!isLoaded() || apvts == nullptr)
    return;

  if (isMuted())
    return;

  int numSamples = input.getNumSamples();
  int numChannels = juce::jmin(input.getNumChannels(), 2);

  slotBuffer.setSize(2, numSamples, false, false, true);
  slotBuffer.clear();

  // Copy input (may be mono -> duplicate to stereo)
  for (int ch = 0; ch < 2; ++ch) {
    int srcCh = juce::jmin(ch, numChannels - 1);
    slotBuffer.copyFrom(ch, 0, input, srcCh, 0, numSamples);
  }

  // 1. Convolution
  juce::dsp::AudioBlock<float> block(slotBuffer);
  juce::dsp::ProcessContextReplacing<float> context(block);
  convolution.process(context);

  // 2. Delay (User Delay + Alignment Delay)
  float userDelayMs =
      apvts->getRawParameterValue(paramPrefix() + "DelayMs")->load();
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

  // 3. Pan (constant power)
  float panVal = apvts->getRawParameterValue(paramPrefix() + "Pan")->load();
  float pan01 = panVal / 100.0f; // -1..1
  float angle = (pan01 + 1.0f) * juce::MathConstants<float>::pi * 0.25f;
  float gainL = std::cos(angle);
  float gainR = std::sin(angle);

  for (int i = 0; i < numSamples; ++i) {
    slotBuffer.setSample(0, i, slotBuffer.getSample(0, i) * gainL);
    slotBuffer.setSample(1, i, slotBuffer.getSample(1, i) * gainR);
  }

  // 4. Level
  float levelDb = apvts->getRawParameterValue(paramPrefix() + "Level")->load();
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

  // Load raw IR data for visualization and auto-align
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
  if (apvts == nullptr)
    return false;
  return apvts->getRawParameterValue(paramPrefix() + "Mute")->load() > 0.5f;
}

bool IRSlot::isSoloed() const {
  if (apvts == nullptr)
    return false;
  return apvts->getRawParameterValue(paramPrefix() + "Solo")->load() > 0.5f;
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
    // Loop forward
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
    // Loop backward
    int prevIndex = (index - 1 + files.size()) % files.size();
    loadImpulseResponse(files[prevIndex]);
  }
}
