#pragma once

#include <JuceHeader.h>

//==============================================================================
// Custom LookAndFeel: ShadCN-inspired dark glass aesthetic
//==============================================================================
class FreeIRLookAndFeel : public juce::LookAndFeel_V4 {
public:
  FreeIRLookAndFeel() {
    // Dark glass colour scheme
    setColour(juce::ResizableWindow::backgroundColourId,
              juce::Colour(0xff0a0a0a));

    // Sliders
    setColour(juce::Slider::backgroundColourId, juce::Colour(0xff1a1a1a));
    setColour(juce::Slider::trackColourId, juce::Colour(0x44ffffff));
    setColour(juce::Slider::thumbColourId, juce::Colour(0xddffffff));
    setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0x66ffffff));
    setColour(juce::Slider::rotarySliderOutlineColourId,
              juce::Colour(0x1affffff));

    // Labels
    setColour(juce::Label::textColourId, juce::Colour(0xffcccccc));

    // Buttons
    setColour(juce::TextButton::buttonColourId, juce::Colour(0x1affffff));
    setColour(juce::TextButton::textColourOnId, juce::Colours::white);
    setColour(juce::TextButton::textColourOffId, juce::Colour(0xffaaaaaa));

    // Menus
    setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff0a0a0a));
    setColour(juce::ComboBox::textColourId, juce::Colour(0xffcccccc));
    setColour(juce::PopupMenu::backgroundColourId, juce::Colour(0xff111111));
    setColour(juce::PopupMenu::textColourId, juce::Colour(0xffcccccc));
  }

  //--------------------------------------------------------------------------
  // Rotary knob: frosted glass with subtle glow
  //--------------------------------------------------------------------------
  void drawRotarySlider(juce::Graphics &g, int x, int y, int width, int height,
                        float sliderPos, float rotaryStartAngle,
                        float rotaryEndAngle, juce::Slider &slider) override {
    auto radius = (float)juce::jmin(width, height) * 0.38f;
    auto centreX = (float)x + (float)width * 0.5f;
    auto centreY = (float)y + (float)height * 0.5f;
    auto rx = centreX - radius;
    auto ry = centreY - radius;
    auto rw = radius * 2.0f;
    auto angle =
        rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

    // Outer glow ring
    g.setColour(juce::Colour(0x0dffffff));
    g.fillEllipse(rx - 3, ry - 3, rw + 6, rw + 6);

    // Outer ring — subtle border
    g.setColour(juce::Colour(0x1affffff));
    g.fillEllipse(rx - 1.5f, ry - 1.5f, rw + 3, rw + 3);

    // Knob body — dark glass gradient
    juce::ColourGradient knobGrad(juce::Colour(0xff2a2a2a), centreX,
                                  centreY - radius, juce::Colour(0xff141414),
                                  centreX, centreY + radius, false);
    g.setGradientFill(knobGrad);
    g.fillEllipse(rx, ry, rw, rw);

    // Inner highlight (top-left)
    g.setColour(juce::Colour(0x0affffff));
    g.drawEllipse(rx + 0.5f, ry + 0.5f, rw - 1, rw - 1, 1.0f);

    // Arc track (background)
    juce::Path arcBg;
    arcBg.addCentredArc(centreX, centreY, radius - 4.0f, radius - 4.0f, 0.0f,
                        rotaryStartAngle, rotaryEndAngle, true);
    g.setColour(juce::Colour(0x15ffffff));
    g.strokePath(arcBg, juce::PathStrokeType(2.5f));

    // Arc track (fill)
    if (sliderPos > 0.001f) {
      juce::Path arcFill;
      arcFill.addCentredArc(centreX, centreY, radius - 4.0f, radius - 4.0f,
                            0.0f, rotaryStartAngle, angle, true);
      g.setColour(juce::Colour(0x66ffffff));
      g.strokePath(arcFill, juce::PathStrokeType(2.5f));
    }

    // Pointer dot
    float dotRadius = 3.0f;
    float dotDist = radius - 4.0f;
    float dotX = centreX + dotDist * std::sin(angle);
    float dotY = centreY - dotDist * std::cos(angle);
    g.setColour(juce::Colour(0xeeffffff));
    g.fillEllipse(dotX - dotRadius, dotY - dotRadius, dotRadius * 2,
                  dotRadius * 2);

    // Center indicator line
    juce::Path pointer;
    auto pointerLen = radius * 0.35f;
    auto pointerThickness = 2.0f;
    pointer.addRoundedRectangle(-pointerThickness * 0.5f, -radius + 6.0f,
                                pointerThickness, pointerLen, 1.0f);
    pointer.applyTransform(
        juce::AffineTransform::rotation(angle).translated(centreX, centreY));
    g.setColour(juce::Colour(0xccffffff));
    g.fillPath(pointer);
  }

  //--------------------------------------------------------------------------
  // Linear slider: sleek glass fader
  //--------------------------------------------------------------------------
  void drawLinearSlider(juce::Graphics &g, int x, int y, int width, int height,
                        float sliderPos, float minSliderPos, float maxSliderPos,
                        const juce::Slider::SliderStyle style,
                        juce::Slider &slider) override {
    if (style == juce::Slider::LinearVertical ||
        style == juce::Slider::LinearBarVertical) {
      // Track
      float trackWidth = 3.0f;
      float trackX = (float)x + (float)width * 0.5f - trackWidth * 0.5f;

      // Track background
      g.setColour(juce::Colour(0x15ffffff));
      g.fillRoundedRectangle(trackX, (float)y, trackWidth, (float)height, 1.5f);

      // Fill below thumb
      g.setColour(juce::Colour(0x44ffffff));
      g.fillRoundedRectangle(trackX, sliderPos, trackWidth,
                             (float)(y + height) - sliderPos, 1.5f);

      // Thumb — glass capsule
      float thumbWidth = (float)width * 0.6f;
      float thumbHeight = 12.0f;
      float thumbX = (float)x + (float)width * 0.5f - thumbWidth * 0.5f;
      float thumbY = sliderPos - thumbHeight * 0.5f;

      // Thumb glow
      g.setColour(juce::Colour(0x0dffffff));
      g.fillRoundedRectangle(thumbX - 2, thumbY - 2, thumbWidth + 4,
                             thumbHeight + 4, 5.0f);

      // Thumb body
      juce::ColourGradient thumbGrad(juce::Colour(0xff3a3a3a), thumbX, thumbY,
                                     juce::Colour(0xff222222), thumbX,
                                     thumbY + thumbHeight, false);
      g.setGradientFill(thumbGrad);
      g.fillRoundedRectangle(thumbX, thumbY, thumbWidth, thumbHeight, 4.0f);

      // Thumb border
      g.setColour(juce::Colour(0x26ffffff));
      g.drawRoundedRectangle(thumbX, thumbY, thumbWidth, thumbHeight, 4.0f,
                             1.0f);

      // Center groove
      g.setColour(juce::Colour(0x33ffffff));
      g.drawHorizontalLine((int)(thumbY + thumbHeight * 0.5f), thumbX + 4.0f,
                           thumbX + thumbWidth - 4.0f);
    } else {
      LookAndFeel_V4::drawLinearSlider(g, x, y, width, height, sliderPos,
                                       minSliderPos, maxSliderPos, style,
                                       slider);
    }
  }

  //--------------------------------------------------------------------------
  // Buttons: frosted glass pills
  //--------------------------------------------------------------------------
  void drawButtonBackground(juce::Graphics &g, juce::Button &button,
                            const juce::Colour &backgroundColour,
                            bool shouldDrawButtonAsHighlighted,
                            bool shouldDrawButtonAsDown) override {
    auto bounds = button.getLocalBounds().toFloat().reduced(0.5f);
    float cornerRadius = juce::jmin(bounds.getHeight() * 0.4f, 10.0f);

    auto baseColour = backgroundColour;

    if (button.getToggleState()) {
      // Active toggle — use the buttonOnColour with glow
      auto onColour = button.findColour(juce::TextButton::buttonOnColourId);
      g.setColour(onColour.withAlpha(0.15f));
      g.fillRoundedRectangle(bounds.expanded(2), cornerRadius + 2);
      baseColour = onColour;
    }

    if (shouldDrawButtonAsDown)
      baseColour = baseColour.brighter(0.15f);
    else if (shouldDrawButtonAsHighlighted)
      baseColour = baseColour.brighter(0.08f);

    g.setColour(baseColour);
    g.fillRoundedRectangle(bounds, cornerRadius);

    // Border
    float borderAlpha = shouldDrawButtonAsHighlighted ? 0.18f : 0.10f;
    g.setColour(juce::Colours::white.withAlpha(borderAlpha));
    g.drawRoundedRectangle(bounds, cornerRadius, 1.0f);
  }

  void drawButtonText(juce::Graphics &g, juce::TextButton &button,
                      bool shouldDrawButtonAsHighlighted,
                      bool shouldDrawButtonAsDown) override {
    auto colour = button.getToggleState()
                      ? button.findColour(juce::TextButton::textColourOnId)
                      : button.findColour(juce::TextButton::textColourOffId);
    g.setColour(colour);
    g.setFont(juce::Font(12.0f, juce::Font::bold));
    g.drawText(button.getButtonText(), button.getLocalBounds(),
               juce::Justification::centred);
  }
};
