#pragma once

#include <JuceHeader.h>

//==============================================================================
// WaveformDisplay: shows overlaid IR waveforms with time axis
//==============================================================================
class WaveformDisplay : public juce::Component, public juce::Timer {
public:
  WaveformDisplay();

  void setIRData(int slotIndex, const juce::AudioBuffer<float> *buffer,
                 double sampleRate, double alignOffsetMs);
  void clearSlot(int slotIndex);
  void refresh();

  void paint(juce::Graphics &g) override;
  void resized() override {}
  void timerCallback() override;

private:
  struct SlotData {
    const juce::AudioBuffer<float> *buffer = nullptr;
    double sampleRate = 48000.0;
    double alignOffsetMs = 0.0;
    bool valid = false;
  };

  std::array<SlotData, 4> slotData;
  bool needsRepaint = false;

  // Slot colours matching the reference (red, green, blue, purple)
  const std::array<juce::Colour, 4> slotColours = {
      juce::Colour(0xffff4444), // Slot 1: Red
      juce::Colour(0xff44ff44), // Slot 2: Green
      juce::Colour(0xff4488ff), // Slot 3: Blue
      juce::Colour(0xffcc44ff)  // Slot 4: Purple
  };

  // Display window
  static constexpr double displayWindowMs = 5.0;
};
