#include "PluginEditor.h"

//==============================================================================
FreeIREditor::FreeIREditor(FreeIRAudioProcessor &p)
    : AudioProcessorEditor(&p), proc(p), browser(p), presetBrowser(p),
      eqSection(p) {
  setLookAndFeel(&lnf);

  // Responsive / Resizable setup
  setResizable(true, true);
  constrainer.setFixedAspectRatio((double)defaultWidth / (double)defaultHeight);
  constrainer.setMinimumSize(800, 500);
  setConstrainer(&constrainer);

  // Browser
  browser.onLoadIR = [this](juce::File f) {
    // Logic to load into *currently selected* slot?
    // Or just load into Slot 1 by default?
    // User didn't specify selection logic.
    // But the Slots have their own "Load" buttons now.
    // So the browser is mostly for management/drag?
    // "when selected display IRs... and ability to load them".
    // Let's assume double click loads into the *first empty* slot or Slot 1 if
    // full? Or maybe we need a "Selected Slot" concept in the Editor? For now,
    // let's load into Slot 1 for simplicity or just perform a callback?
    // Actually, standard behavior: Drag and Drop.
    // Double click could replace Slot 1.
    if (proc.getIRSlot(0).isLoaded()) {
      // Find first empty?
      for (int i = 0; i < 4; ++i) {
        if (!proc.getIRSlot(i).isLoaded()) {
          proc.getIRSlot(i).loadImpulseResponse(f);
          // notify slot
          return;
        }
      }
      // All full, replace slot 1
      proc.getIRSlot(0).loadImpulseResponse(f);
    } else {
      proc.getIRSlot(0).loadImpulseResponse(f);
    }
    refreshWaveform();
    // Slots need update? They update on timer/paint usually or we call
    // updateSlotDisplay? We don't have direct access to updateSlotDisplay
    // easily for all unless we expose it. But slots poll? No, we called
    // updateSlotDisplay manually in load. We should really have a listener
    // mechanism. Dirty fix: iterate slots and update.
    for (auto &s : slotComponents)
      if (s)
        s->updateSlotDisplay();
  };

  browser.onLoadIRToSlot = [this](juce::File f, int slotIndex) {
    if (slotIndex >= 0 && slotIndex < 4) {
      proc.getIRSlot(slotIndex).loadImpulseResponse(f);
      if (slotComponents[(size_t)slotIndex])
        slotComponents[(size_t)slotIndex]->updateSlotDisplay();
      refreshWaveform();
    }
  };

  addAndMakeVisible(browser);
  addAndMakeVisible(presetBrowser);

  // Title
  titleLabel.setFont(juce::Font(32.0f, juce::Font::plain));
  titleLabel.setColour(juce::Label::textColourId, juce::Colours::white);
  titleLabel.setJustificationType(juce::Justification::centredLeft);
  addAndMakeVisible(titleLabel);

  // Subtitle
  subtitleLabel.setFont(juce::Font(15.0f));
  subtitleLabel.setColour(juce::Label::textColourId,
                          juce::Colours::white.withAlpha(0.6f));
  subtitleLabel.setJustificationType(juce::Justification::centredLeft);
  addAndMakeVisible(subtitleLabel);

  // Settings Button
  settingsButton.setButtonText("Settings");
  settingsButton.onClick = [this] {
    juce::PopupMenu m;
    m.addSectionHeader("Export Settings");

    m.addItem("Export Mono", true, proc.exportMono,
              [this] { proc.exportMono = !proc.exportMono; });

    juce::PopupMenu srMenu;
    srMenu.addItem("44.1 kHz", true, proc.exportSampleRate == 44100.0,
                   [this] { proc.exportSampleRate = 44100.0; });
    srMenu.addItem("48 kHz", true, proc.exportSampleRate == 48000.0,
                   [this] { proc.exportSampleRate = 48000.0; });
    srMenu.addItem("88.2 kHz", true, proc.exportSampleRate == 88200.0,
                   [this] { proc.exportSampleRate = 88200.0; });
    srMenu.addItem("96 kHz", true, proc.exportSampleRate == 96000.0,
                   [this] { proc.exportSampleRate = 96000.0; });

    m.addSubMenu("Export Sample Rate", srMenu);

    m.showMenuAsync(
        juce::PopupMenu::Options().withTargetComponent(settingsButton));
  };
  addAndMakeVisible(settingsButton);

  // Header knobs
  setupHeaderKnob(bassHeaderKnob, bassHeaderLabel, "BassGainDb",
                  bassHeaderAttach, 0.0);
  setupHeaderKnob(trebleHeaderKnob, trebleHeaderLabel, "TrebleGainDb",
                  trebleHeaderAttach, 0.0);
  setupHeaderKnob(airHeaderKnob, airHeaderLabel, "AirGainDb", airHeaderAttach,
                  0.0);
  setupHeaderKnob(volumeKnob, volumeLabel, "OutputGainDb", volumeAttach, 0.0);

  // 4 Slot strips
  for (size_t i = 0; i < 4; ++i) {
    slotComponents[i] = std::make_unique<IRSlotComponent>(proc, (int)i);
    slotComponents[i]->onSlotChanged = [this]() { refreshWaveform(); };
    addAndMakeVisible(*slotComponents[i]);
  }

  // Waveform
  addAndMakeVisible(waveformDisplay);

  // Auto Align button
  autoAlignButton.setClickingTogglesState(true);
  autoAlignButton.onClick = [this]() {
    isAutoAlignOn = autoAlignButton.getToggleState();
    updateAutoAlignState();
    if (isAutoAlignOn) {
      proc.cacheManualDelays();
      proc.getAutoAligner().performAlignment();
    } else {
      proc.revertAutoAlignment();
      refreshWaveform();
    }
  };
  addAndMakeVisible(autoAlignButton);
  updateAutoAlignState();

  // Export button
  exportButton.onClick = [this]() {
    auto chooser = std::make_shared<juce::FileChooser>(
        "Export Mixed IR",
        juce::File::getSpecialLocation(juce::File::userDesktopDirectory)
            .getChildFile("FreeIR_Export.wav"),
        "*.wav");

    chooser->launchAsync(juce::FileBrowserComponent::saveMode |
                             juce::FileBrowserComponent::canSelectFiles,
                         [this, chooser](const juce::FileChooser &fc) {
                           auto file = fc.getResult();
                           if (file != juce::File()) {
                             if (!file.hasFileExtension("wav"))
                               file = file.withFileExtension("wav");

                             proc.exportMixedIR(
                                 file); // No args, uses internal settings
                           }
                         });
  };
  exportButton.setColour(juce::TextButton::buttonColourId,
                         juce::Colour(0x15228822));
  exportButton.setColour(juce::TextButton::buttonOnColourId,
                         juce::Colour(0x3344aa44));

  addAndMakeVisible(exportButton);

  // EQ Section
  addAndMakeVisible(eqSection);

  // Register for auto-align callbacks & init
  proc.getAutoAligner().addListener(this);

  startTimerHz(15);
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
    autoAlignButton.setColour(juce::TextButton::buttonColourId,
                              juce::Colour(0x3300ccff));
    autoAlignButton.setColour(juce::TextButton::buttonOnColourId,
                              juce::Colour(0x6600ccff));
    autoAlignButton.setColour(juce::TextButton::textColourOnId,
                              juce::Colours::white);
    for (auto &slot : slotComponents)
      if (slot)
        slot->setDelayEnabled(false);
  } else {
    autoAlignButton.setColour(juce::TextButton::buttonColourId,
                              juce::Colour(0x1affffff));
    autoAlignButton.setColour(juce::TextButton::buttonOnColourId,
                              juce::Colour(0x33ffffff));
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
  juce::Random rng(99);
  g.setColour(juce::Colour(0x06ffffff));
  for (int i = 0; i < 4000; ++i) {
    float nx = rng.nextFloat() * (float)getWidth();
    float ny = rng.nextFloat() * (float)getHeight();
    g.fillRect(nx, ny, 1.0f, 1.0f);
  }
}

//==============================================================================
void FreeIREditor::resized() {
  auto mapRect = [&](int x, int y, int w, int h) -> juce::Rectangle<int> {
    float sx = (float)getWidth() / 1100.0f;
    float sy = (float)getHeight() / 720.0f;
    return juce::Rectangle<float>(x * sx, y * sy, w * sx, h * sy)
        .toNearestInt();
  };

  // 1. Header (Full Width)
  // 1. Header (Full Width)
  titleLabel.setBounds(mapRect(24, 14, 200, 40));
  subtitleLabel.setBounds(mapRect(26, 50, 200, 20));

  // Preset Browser (Top Center)
  int presetW = 300;
  int presetH = 30;
  presetBrowser.setBounds(mapRect((1100 - presetW) / 2, 20, presetW, presetH));

  // 2. Main Content Area
  int contentY = 80;
  int contentH = 720 - contentY - 12;

  // BROWSER (Left)
  int browserW = 240;
  browser.setBounds(mapRect(12, contentY, browserW, contentH));

  // MIXER AREA (Right of Browser)
  int mixerX = 12 + browserW + 12;
  int mixerW = 1100 - mixerX - 12;

  // Waveform
  int waveH = 200;
  waveformDisplay.setBounds(mapRect(mixerX, contentY, mixerW, waveH));

  // Slots
  int slotH = 260;
  int slotsY = contentY + waveH + 12;
  // EQ Section Height
  int eqH = 120; // Slightly taller for labels?
  // Let's calculate remaining height for Slots
  slotH = contentH - waveH - 12 - eqH - 12;

  int slotW = mixerW / 4;
  for (size_t i = 0; i < 4; ++i) {
    slotComponents[i]->setBounds(
        mapRect(mixerX + ((int)i * slotW), slotsY, slotW - 8, slotH));
  }

  // EQ Section & Global Buttons (Bottom)
  int eqY = slotsY + slotH + 12;

  // EQ takes 70% width
  int eqReqW = (int)(mixerW * 0.70f);
  eqSection.setBounds(mapRect(mixerX, eqY, eqReqW, eqH));

  // Buttons Area (Right of EQ)
  int btnAreaX = mixerX + eqReqW + 12;
  int btnAreaW = mixerW - eqReqW - 12;
  // int btnAreaH = eqH; // Unused

  // Layout buttons: AutoAlign, Export, Settings
  // Arrange in a grid or column?
  // Let's do a column for "Auto Align" (large?) and "Export/Settings" (smaller)
  // Or just 3 rows.
  int btnH = 30;
  int btnGap = 10;
  int startBtnY = eqY + (eqH - (3 * btnH + 2 * btnGap)) / 2;

  autoAlignButton.setBounds(mapRect(btnAreaX, startBtnY, btnAreaW, btnH));
  exportButton.setBounds(
      mapRect(btnAreaX, startBtnY + btnH + btnGap, btnAreaW, btnH));
  settingsButton.setBounds(
      mapRect(btnAreaX, startBtnY + 2 * (btnH + btnGap), btnAreaW, btnH));
}

//==============================================================================
void FreeIREditor::timerCallback() {
  // Slots update themselves via listeners mostly, but let's keep timer for
  // anything else
}

void FreeIREditor::alignmentComplete() {
  proc.applyAlignmentResults();
  refreshWaveform();
}

//==============================================================================
void FreeIREditor::refreshWaveform() {
  for (int i = 0; i < 4; ++i) {
    auto &slot = proc.getIRSlot(i);
    float currentDelayMs = 0.0f;
    auto *param =
        proc.getAPVTS().getParameter("Slot" + juce::String(i + 1) + "_DelayMs");
    if (param)
      currentDelayMs = *proc.getAPVTS().getRawParameterValue(
          "Slot" + juce::String(i + 1) + "_DelayMs");

    if (slot.isLoaded())
      waveformDisplay.setIRData(i, &slot.getIRBuffer(), slot.getIRSampleRate(),
                                (double)currentDelayMs);
    else
      waveformDisplay.clearSlot(i);
  }
  waveformDisplay.refresh();
}

void FreeIREditor::loadIRForSlot(int /*slotIndex*/) {
  // Deprecated/Unused? Kept for compatibility if needed
  // Slots manage their own loading now.
}
