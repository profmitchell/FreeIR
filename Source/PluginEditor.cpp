#include "PluginEditor.h"

//==============================================================================
FreeIREditor::FreeIREditor(FreeIRAudioProcessor &p)
    : AudioProcessorEditor(&p), proc(p), eqSection(p) {
  setLookAndFeel(&lnf);

  // Responsive / Resizable setup
  setResizable(true, true);
  constrainer.setFixedAspectRatio((double)defaultWidth / (double)defaultHeight);
  constrainer.setMinimumSize(550, 360);
  setConstrainer(&constrainer);

  // Title
  titleLabel.setFont(juce::Font(32.0f, juce::Font::plain));
  titleLabel.setColour(juce::Label::textColourId, juce::Colours::white);
  titleLabel.setJustificationType(juce::Justification::centredLeft);
  addAndMakeVisible(titleLabel);

  // Header knobs
  setupHeaderKnob(bassHeaderKnob, bassHeaderLabel, "BassGainDb",
                  bassHeaderAttach, 0.0);
  setupHeaderKnob(trebleHeaderKnob, trebleHeaderLabel, "TrebleGainDb",
                  trebleHeaderAttach, 0.0);
  setupHeaderKnob(airHeaderKnob, airHeaderLabel, "AirGainDb", airHeaderAttach,
                  0.0);
  setupHeaderKnob(volumeKnob, volumeLabel, "OutputGainDb", volumeAttach, 0.0);

  // 4 Slot strips
  for (int i = 0; i < 4; ++i) {
    slotComponents[i] = std::make_unique<IRSlotComponent>(proc, i);
    slotComponents[i]->onSlotChanged = [this]() {
      refreshIRList();
      refreshWaveform();
    };
    addAndMakeVisible(*slotComponents[i]);
  }

  // IR list buttons (pill styled)
  for (int i = 0; i < 4; ++i) {
    irListButtons[i] = std::make_unique<juce::TextButton>();
    irListButtons[i]->setColour(juce::TextButton::buttonColourId,
                                juce::Colour(0x0affffff));
    irListButtons[i]->setColour(juce::TextButton::buttonOnColourId,
                                juce::Colour(0x1affffff));
    irListButtons[i]->setColour(juce::TextButton::textColourOffId,
                                juce::Colour(0xff888888));
    irListButtons[i]->setColour(juce::TextButton::textColourOnId,
                                juce::Colours::white);

    int slotIdx = i;
    irListButtons[i]->onClick = [this, slotIdx]() { loadIRForSlot(slotIdx); };

    addAndMakeVisible(*irListButtons[i]);
  }

  refreshIRList();

  // Waveform
  addAndMakeVisible(waveformDisplay);

  // Auto Align button (Toggle)
  autoAlignButton.setClickingTogglesState(true);
  autoAlignButton.onClick = [this]() {
    isAutoAlignOn = autoAlignButton.getToggleState();
    updateAutoAlignState();
    if (isAutoAlignOn) {
      proc.getAutoAligner().performAlignment();
      // The alignment sets the delay parameters, which updates the knobs,
      // which updates the waveform via callback.
    }
  };
  addAndMakeVisible(autoAlignButton);

  // Export button
  exportButton.onClick = [this]() {
    auto chooser = std::make_shared<juce::FileChooser>(
        "Export Mixed IR",
        juce::File::getSpecialLocation(juce::File::userDesktopDirectory)
            .getChildFile("FreeIR_Export.wav"),
        "*.wav");

    chooser->launchAsync(
        juce::FileBrowserComponent::saveMode |
            juce::FileBrowserComponent::canSelectFiles,
        [this, chooser](const juce::FileChooser &fc) {
          auto file = fc.getResult();
          if (file != juce::File()) {
            if (!file.hasFileExtension("wav"))
              file = file.withFileExtension("wav");

            bool success = proc.exportMixedIR(file, 48000.0, 8192);

            if (!success) {
              juce::AlertWindow::showMessageBoxAsync(
                  juce::MessageBoxIconType::WarningIcon, "Export Failed",
                  "Could not write the IR file.");
            }
          }
        });
  };
  // Green tint for export
  exportButton.setColour(juce::TextButton::buttonColourId,
                         juce::Colour(0x15228822));
  exportButton.setColour(juce::TextButton::buttonOnColourId,
                         juce::Colour(0x3344aa44));

  addAndMakeVisible(exportButton);

  // EQ Section
  addAndMakeVisible(eqSection);

  // Register for auto-align callbacks & init
  proc.getAutoAligner().addListener(this);
  updateAutoAlignState(); // Init state

  startTimerHz(15);

  // Initial waveform refresh deferred to timer or after layout
  refreshWaveform();

  setSize(defaultWidth, defaultHeight);
}

FreeIREditor::~FreeIREditor() {
  proc.getAutoAligner().removeListener(this);
  setLookAndFeel(nullptr);
}

//==============================================================================
void FreeIREditor::setupHeaderKnob(
    juce::Slider &knob, juce::Label &label, const juce::String &paramID,
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
        &attach,
    double defaultVal) {
  knob.setSliderStyle(juce::Slider::RotaryVerticalDrag);
  knob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
  knob.setDoubleClickReturnValue(true, defaultVal);
  knob.setPopupDisplayEnabled(true, true, this);
  addAndMakeVisible(knob);

  label.setFont(juce::Font(10.0f, juce::Font::bold));
  label.setJustificationType(juce::Justification::centred);
  label.setColour(juce::Label::textColourId, juce::Colour(0xff888888));
  addAndMakeVisible(label);

  attach =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          proc.getAPVTS(), paramID, knob);
}

void FreeIREditor::updateAutoAlignState() {
  if (isAutoAlignOn) {
    // Cyan glow when ON
    autoAlignButton.setColour(juce::TextButton::buttonColourId,
                              juce::Colour(0x3300ccff));
    autoAlignButton.setColour(juce::TextButton::buttonOnColourId,
                              juce::Colour(0x6600ccff));
    autoAlignButton.setColour(juce::TextButton::textColourOnId,
                              juce::Colours::white);

    // Disable delay knobs
    for (auto &slot : slotComponents)
      if (slot)
        slot->setDelayEnabled(false);

  } else {
    // Dim / Standard when OFF
    autoAlignButton.setColour(juce::TextButton::buttonColourId,
                              juce::Colour(0x1affffff));
    autoAlignButton.setColour(juce::TextButton::buttonOnColourId,
                              juce::Colour(0x33ffffff));

    // Enable delay knobs
    for (auto &slot : slotComponents)
      if (slot)
        slot->setDelayEnabled(true);
  }
}

//==============================================================================
void FreeIREditor::paint(juce::Graphics &g) {
  // ShadCN Dark Glass Background
  juce::ColourGradient bgGrad(juce::Colour(0xff000000), 0, 0,
                              juce::Colour(0xff121212), 0, (float)getHeight(),
                              false);
  g.setGradientFill(bgGrad);
  g.fillAll();

  // Subtle Noise
  juce::Random rng(999);
  g.setColour(juce::Colour(0x06ffffff));
  for (int i = 0; i < 4000; ++i) {
    float nx = rng.nextFloat() * (float)getWidth();
    float ny = rng.nextFloat() * (float)getHeight();
    g.fillRect(nx, ny, 1.0f, 1.0f);
  }
}

//==============================================================================
void FreeIREditor::resized() {
  // Map rectangles from 1100x720 design space to current bounds
  auto mapRect = [&](int x, int y, int w, int h) -> juce::Rectangle<int> {
    float sx = (float)getWidth() / 1100.0f;
    float sy = (float)getHeight() / 720.0f;
    return juce::Rectangle<float>(x * sx, y * sy, w * sx, h * sy)
        .toNearestInt();
  };

  // HEADER BAR (Background drawn in Paint? No, let's just layout components)
  // We'll draw the header panel bg in paint if needed, or just let components
  // float. Ideally, paint() should use the same scaling logic for the
  // header-bg. But for now, let's keep it simple.

  // Title
  titleLabel.setBounds(mapRect(24, 12, 200, 60));

  // Header Knobs (Center)
  int knobSize = 46;
  int spacing = 80;
  // Center X = 550. Total width = 3 * 80 = 240. Start X = 430.
  int startX = 430;
  int knobY = 18;

  auto layoutHeaderKnob = [&](juce::Slider &k, juce::Label &l, int x) {
    k.setBounds(mapRect(x, knobY, knobSize, knobSize));
    l.setBounds(mapRect(x, knobY + 46, knobSize, 14));
  };

  layoutHeaderKnob(bassHeaderKnob, bassHeaderLabel, startX);
  layoutHeaderKnob(trebleHeaderKnob, trebleHeaderLabel, startX + spacing);
  layoutHeaderKnob(airHeaderKnob, airHeaderLabel, startX + spacing * 2);

  // Volume (Right)
  layoutHeaderKnob(volumeKnob, volumeLabel, 1100 - 80);

  // EQ SECTIO (Bottom)
  eqSection.setBounds(mapRect(12, 608, 1076, 100));

  // SLOTS (Left)
  int slotW = 120;
  int startY = 100;
  int slotH = 490; // Height between header and EQ

  for (int i = 0; i < 4; ++i) {
    slotComponents[i]->setBounds(
        mapRect(24 + (i * slotW), startY, slotW - 8, slotH));
  }

  // RIGHT PANEL (IR List, Waveform, Buttons)
  int rightPanelX = 24 + (4 * slotW) + 12;   // ~516
  int rightPanelW = 1100 - rightPanelX - 24; // ~560
  int curY = startY;

  // IR List Buttons
  int btnH = 28;
  for (int i = 0; i < 4; ++i) {
    irListButtons[i]->setBounds(mapRect(rightPanelX, curY, rightPanelW, btnH));
    curY += 34;
  }

  curY += 12;

  // Waveform
  int waveH = 250;
  waveformDisplay.setBounds(mapRect(rightPanelX, curY, rightPanelW, waveH));
  curY += waveH + 16;

  // Buttons
  int btnW = (rightPanelW - 12) / 2;
  autoAlignButton.setBounds(mapRect(rightPanelX, curY, btnW, 32));
  exportButton.setBounds(mapRect(rightPanelX + btnW + 12, curY, btnW, 32));
}

//==============================================================================
void FreeIREditor::timerCallback() { refreshIRList(); }

void FreeIREditor::alignmentComplete() { refreshWaveform(); }

//==============================================================================
void FreeIREditor::refreshIRList() {
  for (int i = 0; i < 4; ++i) {
    auto &slot = proc.getIRSlot(i);
    juce::String text = juce::String(i + 1) + ". ";

    if (slot.isLoaded()) {
      text += slot.getSlotName();
      irListButtons[i]->setColour(juce::TextButton::textColourOffId,
                                  juce::Colours::white);
      // Bright glass when loaded
      irListButtons[i]->setColour(juce::TextButton::buttonColourId,
                                  juce::Colour(0x1affffff));
    } else {
      text += "Empty (Click to Load)";
      irListButtons[i]->setColour(juce::TextButton::textColourOffId,
                                  juce::Colour(0xff666666));
      irListButtons[i]->setColour(juce::TextButton::buttonColourId,
                                  juce::Colour(0x0affffff));
    }

    if (irListButtons[i]->getButtonText() != text)
      irListButtons[i]->setButtonText(text);
  }
}

void FreeIREditor::refreshWaveform() {
  for (int i = 0; i < 4; ++i) {
    auto &slot = proc.getIRSlot(i);

    // Read the delay parameter converted to 0..1 then map to ms?
    // No, APVTS parameters are float.
    // But we need the actual value in milliseconds.
    float currentDelayMs = 0.0f;
    auto *param =
        proc.getAPVTS().getParameter("Slot" + juce::String(i + 1) + "_DelayMs");
    if (param) {
      // getParameter returns 0..1 normalized usually if using
      // AudioProcessorParameter::getValue() But convertFrom0to1 gives real
      // value. Actually `getAPVTS().getRawParameterValue` returns the float
      // *value* directly (atomic).
      currentDelayMs = *proc.getAPVTS().getRawParameterValue(
          "Slot" + juce::String(i + 1) + "_DelayMs");
    }

    if (slot.isLoaded())
      waveformDisplay.setIRData(i, &slot.getIRBuffer(), slot.getIRSampleRate(),
                                (double)currentDelayMs);
    else
      waveformDisplay.clearSlot(i);
  }
  waveformDisplay.refresh();
}

void FreeIREditor::loadIRForSlot(int slotIndex) {
  auto chooser = std::make_shared<juce::FileChooser>(
      "Load Impulse Response for Slot " + juce::String(slotIndex + 1),
      juce::File::getSpecialLocation(juce::File::userHomeDirectory),
      "*.wav;*.aif;*.aiff");

  chooser->launchAsync(juce::FileBrowserComponent::openMode |
                           juce::FileBrowserComponent::canSelectFiles,
                       [this, slotIndex, chooser](const juce::FileChooser &fc) {
                         auto file = fc.getResult();
                         if (file.existsAsFile()) {
                           proc.getIRSlot(slotIndex).loadImpulseResponse(file);
                           refreshIRList();
                           refreshWaveform();
                           if (slotComponents[slotIndex])
                             slotComponents[slotIndex]->updateSlotDisplay();
                         }
                       });
}
