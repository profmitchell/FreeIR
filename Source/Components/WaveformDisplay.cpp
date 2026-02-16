#include "WaveformDisplay.h"

WaveformDisplay::WaveformDisplay() { startTimerHz(30); }

void WaveformDisplay::setIRData(int slotIndex,
                                const juce::AudioBuffer<float> *buffer,
                                double sampleRate, double alignOffsetMs) {
  if (slotIndex < 0 || slotIndex >= 4)
    return;
  slotData[slotIndex] = {buffer, sampleRate, alignOffsetMs,
                         buffer != nullptr && buffer->getNumSamples() > 0};
  needsRepaint = true;
}

void WaveformDisplay::clearSlot(int slotIndex) {
  if (slotIndex < 0 || slotIndex >= 4)
    return;
  slotData[slotIndex] = {};
  needsRepaint = true;
}

void WaveformDisplay::refresh() { needsRepaint = true; }

void WaveformDisplay::timerCallback() {
  if (needsRepaint) {
    needsRepaint = false;
    repaint();
  }
}

void WaveformDisplay::paint(juce::Graphics &g) {
  auto bounds = getLocalBounds().toFloat();

  // Background
  g.setColour(juce::Colour(0xff111111));
  g.fillRoundedRectangle(bounds, 4.0f);

  // Border
  g.setColour(juce::Colour(0xff333333));
  g.drawRoundedRectangle(bounds, 4.0f, 1.0f);

  float pad = 8.0f;
  auto plotArea = bounds.reduced(pad);

  // Grid lines and time labels
  g.setColour(juce::Colour(0xff333333));
  for (int i = 0; i <= 4; ++i) {
    float x = plotArea.getX() + plotArea.getWidth() * (float)i / 4.0f;
    g.drawVerticalLine((int)x, plotArea.getY(), plotArea.getBottom());
  }

  // Time labels
  g.setColour(juce::Colour(0xff888888));
  g.setFont(11.0f);
  g.drawText("0ms", (int)plotArea.getX() - 10, (int)plotArea.getBottom() + 2,
             40, 14, juce::Justification::centred);
  g.drawText("2.5ms", (int)(plotArea.getX() + plotArea.getWidth() * 0.5f) - 20,
             (int)plotArea.getBottom() + 2, 50, 14,
             juce::Justification::centred);
  g.drawText("5ms", (int)(plotArea.getRight()) - 20,
             (int)plotArea.getBottom() + 2, 40, 14,
             juce::Justification::centred);

  // Zero line
  float zeroY = plotArea.getCentreY();
  g.setColour(juce::Colour(0xff2a2a2a));
  g.drawHorizontalLine((int)zeroY, plotArea.getX(), plotArea.getRight());

  // Draw each slot waveform
  for (int s = 0; s < 4; ++s) {
    if (!slotData[s].valid || slotData[s].buffer == nullptr)
      continue;

    const auto &buf = *slotData[s].buffer;
    double sr = slotData[s].sampleRate;
    double offsetMs = slotData[s].alignOffsetMs;

    if (buf.getNumSamples() == 0 || sr <= 0.0)
      continue;

    int displaySamples = (int)(sr * displayWindowMs * 0.001);
    int offsetSamples = (int)(sr * offsetMs * 0.001);

    juce::Path waveform;
    bool started = false;

    for (int px = 0; px < (int)plotArea.getWidth(); ++px) {
      // Map pixel to sample index
      double t = (double)px / (double)plotArea.getWidth(); // 0..1 across 5ms
      int sampleIdx = (int)(t * (double)displaySamples) - offsetSamples;

      float value = 0.0f;
      if (sampleIdx >= 0 && sampleIdx < buf.getNumSamples())
        value = buf.getSample(0, sampleIdx); // Use channel 0

      // Clamp for display
      value = juce::jlimit(-1.0f, 1.0f, value);

      float xPos = plotArea.getX() + (float)px;
      float yPos = zeroY - value * plotArea.getHeight() * 0.45f;

      if (!started) {
        waveform.startNewSubPath(xPos, yPos);
        started = true;
      } else {
        waveform.lineTo(xPos, yPos);
      }
    }

    g.setColour(slotColours[s]);
    g.strokePath(waveform, juce::PathStrokeType(1.5f));
  }
}
