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

  // Glass panel background
  g.setColour(juce::Colour(0x0affffff));
  g.fillRoundedRectangle(bounds, 8.0f);

  // Border (subtle glass edge)
  g.setColour(juce::Colour(0x1affffff));
  g.drawRoundedRectangle(bounds, 8.0f, 1.0f);

  float pad = 12.0f;
  auto plotArea = bounds.reduced(pad);

  // Inner dark tracking area
  g.setColour(juce::Colour(0xff050505));
  g.fillRoundedRectangle(plotArea, 4.0f);
  g.setColour(juce::Colour(0x1affffff));
  g.drawRoundedRectangle(plotArea, 4.0f, 1.0f);

  // Grid lines
  g.setColour(juce::Colour(0x0dffffff));
  int numDivs = 8;
  for (int i = 1; i < numDivs; ++i) {
    float x = plotArea.getX() + plotArea.getWidth() * (float)i / (float)numDivs;
    g.drawVerticalLine((int)x, plotArea.getY(), plotArea.getBottom());
  }

  // Time labels (subtle)
  g.setColour(juce::Colour(0xff666666));
  g.setFont(juce::Font(10.0f));

  // 0ms
  g.drawText("0ms", (int)plotArea.getX() + 4, (int)plotArea.getBottom() - 14,
             40, 14, juce::Justification::left);
  // End (5ms)
  g.drawText("5ms", (int)plotArea.getRight() - 44,
             (int)plotArea.getBottom() - 14, 40, 14,
             juce::Justification::right);

  // Zero line
  float zeroY = plotArea.getCentreY();
  g.setColour(juce::Colour(0x26ffffff));
  g.drawHorizontalLine((int)zeroY, plotArea.getX(), plotArea.getRight());

  // Draw each slot waveform
  // Draw in reverse order so Slot 1 is on top
  // Monochrome shades
  const juce::Colour slotShades[4] = {
      juce::Colour(0xffffffff), // Pure White
      juce::Colour(0xddeeeeee), // Slightly dimmed
      juce::Colour(0xbbcccccc), // Grey
      juce::Colour(0x99aaaaaa)  // Darker Grey
  };

  for (int s = 3; s >= 0; --s) {
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

    // Optimize: step by 1 pixel
    for (int px = 0; px < (int)plotArea.getWidth(); ++px) {
      double t = (double)px / (double)plotArea.getWidth();
      int sampleIdx = (int)(t * (double)displaySamples) - offsetSamples;

      float value = 0.0f;
      if (sampleIdx >= 0 && sampleIdx < buf.getNumSamples())
        value = buf.getSample(0, sampleIdx);

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

    g.setColour(slotShades[s]);
    g.strokePath(waveform, juce::PathStrokeType(1.0f));

    // Fill with very low opacity for body
    juce::Path fillPath = waveform;
    fillPath.lineTo(plotArea.getRight(), zeroY);
    fillPath.lineTo(plotArea.getX(), zeroY);
    fillPath.closeSubPath();
    g.setColour(slotShades[s].withAlpha(0.1f));
    g.fillPath(fillPath);
  }
}
