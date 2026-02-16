#pragma once

#include "IRSlot.h"
#include <JuceHeader.h>

class AutoAligner : public juce::Thread {
public:
  AutoAligner(std::array<IRSlot, 4> &slots);
  ~AutoAligner() override;

  void performAlignment(); // Triggers the background thread

  void run() override;

  std::array<double, 4> results = {0.0, 0.0, 0.0, 0.0};

  // Listener interface to notify editor when alignment is done
  struct Listener {
    virtual ~Listener() = default;
    virtual void alignmentComplete() = 0;
  };

  void addListener(Listener *l) { listeners.add(l); }
  void removeListener(Listener *l) { listeners.remove(l); }

private:
  std::array<IRSlot, 4> &slots;
  juce::ListenerList<Listener> listeners;

  // Helper to calculate cross-correlation and find max lag
  double findDelayOffset(const juce::AudioBuffer<float> &ref,
                         const juce::AudioBuffer<float> &target,
                         double sampleRate);
};
