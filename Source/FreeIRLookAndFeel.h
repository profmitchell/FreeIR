#pragma once

#include <JuceHeader.h>

//==============================================================================
// Custom LookAndFeel for the dark brushed-metal aesthetic
//==============================================================================
class FreeIRLookAndFeel : public juce::LookAndFeel_V4 {
public:
  FreeIRLookAndFeel() {
    // Dark colour scheme
    setColour(juce::ResizableWindow::backgroundColourId,
              juce::Colour(0xff1a1a1a));
    setColour(juce::Slider::backgroundColourId, juce::Colour(0xff2a2a2a));
    setColour(juce::Slider::trackColourId, juce::Colour(0xff888888));
    setColour(juce::Slider::thumbColourId, juce::Colour(0xffcccccc));
    setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xff888888));
    setColour(juce::Slider::rotarySliderOutlineColourId,
              juce::Colour(0xff3a3a3a));
    setColour(juce::Label::textColourId, juce::Colour(0xffcccccc));
    setColour(juce::TextButton::buttonColourId, juce::Colour(0xff333333));
    setColour(juce::TextButton::textColourOnId, juce::Colours::white);
    setColour(juce::TextButton::textColourOffId, juce::Colour(0xff999999));
    setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff2a2a2a));
    setColour(juce::ComboBox::textColourId, juce::Colour(0xffcccccc));
    setColour(juce::PopupMenu::backgroundColourId, juce::Colour(0xff222222));
    setColour(juce::PopupMenu::textColourId, juce::Colour(0xffcccccc));
  }

  //--------------------------------------------------------------------------
  // Rotary knob: metallic 3D look similar to the reference
  //--------------------------------------------------------------------------
  void drawRotarySlider(juce::Graphics &g, int x, int y, int width, int height,
                        float sliderPosProportional, float rotaryStartAngle,
                        float rotaryEndAngle, juce::Slider &slider) override {
    auto radius = (float)juce::jmin(width, height) * 0.4f;
    auto centreX = (float)x + (float)width * 0.5f;
    auto centreY = (float)y + (float)height * 0.5f;
    auto rx = centreX - radius;
    auto ry = centreY - radius;
    auto rw = radius * 2.0f;
    auto angle = rotaryStartAngle +
                 sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

    // Outer ring (dark)
    g.setColour(juce::Colour(0xff2a2a2a));
    g.fillEllipse(rx - 2, ry - 2, rw + 4, rw + 4);

    // Knob body gradient (metallic)
    juce::ColourGradient knobGradient(
        juce::Colour(0xff555555), centreX, centreY - radius,
        juce::Colour(0xff222222), centreX, centreY + radius, false);
    g.setGradientFill(knobGradient);
    g.fillEllipse(rx, ry, rw, rw);

    // Inner shadow
    g.setColour(juce::Colour(0x44000000));
    g.drawEllipse(rx + 1, ry + 1, rw - 2, rw - 2, 1.5f);

    // Highlight
    g.setColour(juce::Colour(0x22ffffff));
    g.drawEllipse(rx, ry, rw, rw, 1.0f);

    // Pointer line
    juce::Path pointer;
    auto pointerLen = radius * 0.6f;
    auto pointerThickness = 2.5f;
    pointer.addRoundedRectangle(-pointerThickness * 0.5f, -radius + 4.0f,
                                pointerThickness, pointerLen, 1.0f);
    pointer.applyTransform(
        juce::AffineTransform::rotation(angle).translated(centreX, centreY));

    g.setColour(juce::Colour(0xffeeeeee));
    g.fillPath(pointer);
  }

  //--------------------------------------------------------------------------
  // Linear slider (fader-style) for Level
  //--------------------------------------------------------------------------
  void drawLinearSlider(juce::Graphics &g, int x, int y, int width, int height,
                        float sliderPos, float minSliderPos, float maxSliderPos,
                        const juce::Slider::SliderStyle style,
                        juce::Slider &slider) override {
    if (style == juce::Slider::LinearVertical ||
        style == juce::Slider::LinearBarVertical) {
      // Track
      float trackWidth = 4.0f;
      float trackX = (float)x + (float)width * 0.5f - trackWidth * 0.5f;
      g.setColour(juce::Colour(0xff333333));
      g.fillRoundedRectangle(trackX, (float)y, trackWidth, (float)height, 2.0f);

      // Fill below thumb
      g.setColour(juce::Colour(0xff777777));
      g.fillRoundedRectangle(trackX, sliderPos, trackWidth,
                             (float)(y + height) - sliderPos, 2.0f);

      // Thumb
      float thumbWidth = (float)width * 0.7f;
      float thumbHeight = 10.0f;
      float thumbX = (float)x + (float)width * 0.5f - thumbWidth * 0.5f;
      float thumbY = sliderPos - thumbHeight * 0.5f;

      juce::ColourGradient thumbGrad(juce::Colour(0xff888888), thumbX, thumbY,
                                     juce::Colour(0xff555555), thumbX,
                                     thumbY + thumbHeight, false);
      g.setGradientFill(thumbGrad);
      g.fillRoundedRectangle(thumbX, thumbY, thumbWidth, thumbHeight, 3.0f);
      g.setColour(juce::Colour(0xff999999));
      g.drawRoundedRectangle(thumbX, thumbY, thumbWidth, thumbHeight, 3.0f,
                             1.0f);

      // Center groove on thumb
      g.setColour(juce::Colour(0xff444444));
      g.drawHorizontalLine((int)(thumbY + thumbHeight * 0.5f), thumbX + 4.0f,
                           thumbX + thumbWidth - 4.0f);
    } else {
      LookAndFeel_V4::drawLinearSlider(g, x, y, width, height, sliderPos,
                                       minSliderPos, maxSliderPos, style,
                                       slider);
    }
  }

  //--------------------------------------------------------------------------
  // Buttons (for Mute/Solo/Auto Align etc.)
  //--------------------------------------------------------------------------
  void drawButtonBackground(juce::Graphics &g, juce::Button &button,
                            const juce::Colour &backgroundColour,
                            bool shouldDrawButtonAsHighlighted,
                            bool shouldDrawButtonAsDown) override {
    auto bounds = button.getLocalBounds().toFloat().reduced(0.5f);
    auto baseColour = backgroundColour;

    if (shouldDrawButtonAsDown || button.getToggleState())
      baseColour = baseColour.brighter(0.3f);
    else if (shouldDrawButtonAsHighlighted)
      baseColour = baseColour.brighter(0.1f);

    g.setColour(baseColour);
    g.fillRoundedRectangle(bounds, 4.0f);

    g.setColour(juce::Colour(0xff555555));
    g.drawRoundedRectangle(bounds, 4.0f, 1.0f);
  }

  void drawButtonText(juce::Graphics &g, juce::TextButton &button,
                      bool shouldDrawButtonAsHighlighted,
                      bool shouldDrawButtonAsDown) override {
    auto colour = button.getToggleState() ? juce::Colours::white
                                          : juce::Colour(0xff999999);
    g.setColour(colour);
    g.setFont(juce::Font(13.0f, juce::Font::bold));
    g.drawText(button.getButtonText(), button.getLocalBounds(),
               juce::Justification::centred);
  }
};
