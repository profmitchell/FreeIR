#include "AutoAligner.h"

AutoAligner::AutoAligner(std::array<IRSlot, 4> &s)
    : juce::Thread("AutoAligner"), slots(s) {}

AutoAligner::~AutoAligner() { stopThread(2000); }

void AutoAligner::performAlignment() {
  if (isThreadRunning())
    return;
  startThread();
}

void AutoAligner::run() {
  // 1. Find reference slot (first loaded slot)
  int refIndex = -1;
  for (int i = 0; i < 4; ++i) {
    if (slots[i].isLoaded()) {
      refIndex = i;
      break;
    }
  }

  if (refIndex < 0) {
    // No slots loaded
    juce::MessageManager::callAsync(
        [this]() { listeners.call(&Listener::alignmentComplete); });
    return;
  }

  const auto &refIR = slots[refIndex].getIRBuffer();
  double refSR = slots[refIndex].getIRSampleRate();

  // Reset reference delay to 0
  results[refIndex] = 0.0;

  // 2. For each other loaded slot, compute cross-correlation offset
  for (int i = 0; i < 4; ++i) {
    if (threadShouldExit())
      return;

    if (i == refIndex || !slots[i].isLoaded()) {
      if (i != refIndex)
        results[i] = 0.0;
      continue;
    }

    const auto &targetIR = slots[i].getIRBuffer();
    double targetSR = slots[i].getIRSampleRate();

    // Use the lower sample rate for safety
    double offset = findDelayOffset(refIR, targetIR, refSR);
    results[i] = offset;
  }

  // 3. Notify listeners on message thread
  juce::MessageManager::callAsync(
      [this]() { listeners.call(&Listener::alignmentComplete); });
}

double AutoAligner::findDelayOffset(const juce::AudioBuffer<float> &ref,
                                    const juce::AudioBuffer<float> &target,
                                    double sr) {
  // Cross-correlation in time domain on the early portion of both IRs
  // Limit analysis window to first 5ms as per PRD
  int windowSamples = juce::jmin((int)(sr * 0.005), ref.getNumSamples(),
                                 target.getNumSamples());
  if (windowSamples < 2)
    return 0.0;

  // Use mono (channel 0 or sum) for correlation
  const float *refData = ref.getReadPointer(0);
  const float *tgtData = target.getReadPointer(0);

  // Search range: ±5ms
  int maxLag = (int)(sr * 0.005);
  maxLag = juce::jmin(maxLag, windowSamples - 1);

  double bestCorr = -1e30;
  int bestLag = 0;

  for (int lag = -maxLag; lag <= maxLag; ++lag) {
    double sum = 0.0;
    int count = 0;

    for (int i = 0; i < windowSamples; ++i) {
      int j = i + lag;
      if (j >= 0 && j < target.getNumSamples()) {
        sum += (double)refData[i] * (double)tgtData[j];
        count++;
      }
    }

    if (count > 0)
      sum /= (double)count;

    if (sum > bestCorr) {
      bestCorr = sum;
      bestLag = lag;
    }
  }

  // bestLag > 0 means target needs to be delayed to align with ref
  // bestLag < 0 means target arrives earlier (so we delay it less, or ref needs
  // delay — but we fix ref at 0) We want to apply a positive delay to shift the
  // target forward in time
  double delayMs = (double)bestLag / sr * 1000.0;

  // Clamp to 0..10ms (negative means this slot should play EARLIER than ref; we
  // can't do negative delay, so we clamp to 0 which means it's as aligned as we
  // can get without delaying the ref)
  delayMs = juce::jlimit(0.0, 10.0, delayMs);

  return delayMs;
}
