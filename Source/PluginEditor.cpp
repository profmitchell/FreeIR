#include "PluginEditor.h"

//==============================================================================
FreeIREditor::FreeIREditor(FreeIRAudioProcessor &p)
    : AudioProcessorEditor(&p), proc(p), eqSection(p) {
  setLookAndFeel(&lnf);
  setResizable(false, false);

  // Title
  titleLabel.setFont(juce::Font(28.0f, juce::Font::bold));
  titleLabel.setColour(juce::Label::textColourId, juce::Colours::white);
  titleLabel.setJustificationType(juce::Justification::centredLeft);
  addAndMakeVisible(titleLabel);

  // Header knobs: Bass, Treble, Air, Volume
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

  // IR list buttons (styled as text, click to load)
  for (int i = 0; i < 4; ++i) {
    irListButtons[i] = std::make_unique<juce::TextButton>();
    irListButtons[i]->setColour(juce::TextButton::buttonColourId,
                                juce::Colours::transparentBlack);
    irListButtons[i]->setColour(juce::TextButton::buttonOnColourId,
                                juce::Colour(0x22ffffff));
    irListButtons[i]->setColour(juce::TextButton::textColourOffId,
                                juce::Colour(0xff666666));
    irListButtons[i]->setColour(juce::TextButton::textColourOnId,
                                juce::Colours::white);

    int slotIdx = i;
    irListButtons[i]->onClick = [this, slotIdx]() { loadIRForSlot(slotIdx); };

    addAndMakeVisible(*irListButtons[i]);
  }

  refreshIRList();

  // Waveform
  addAndMakeVisible(waveformDisplay);
  refreshWaveform();

  // Auto Align button
  autoAlignButton.onClick = [this]() {
    proc.getAutoAligner().performAlignment();
  };
  autoAlignButton.setColour(juce::TextButton::buttonColourId,
                            juce::Colour(0xff2a2a2a));
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

            if (success) {
              juce::AlertWindow::showMessageBoxAsync(
                  juce::MessageBoxIconType::InfoIcon, "Export Complete",
                  "Mixed IR exported successfully to:\n" +
                      file.getFullPathName());
            } else {
              juce::AlertWindow::showMessageBoxAsync(
                  juce::MessageBoxIconType::WarningIcon, "Export Failed",
                  "Could not write the IR file.");
            }
          }
        });
  };
  exportButton.setColour(juce::TextButton::buttonColourId,
                         juce::Colour(0xff2a4a2a));
  addAndMakeVisible(exportButton);

  // EQ Section
  addAndMakeVisible(eqSection);

  // Register for auto-align callbacks
  proc.getAutoAligner().addListener(this);

  startTimerHz(15);

  // IMPORTANT: setSize must be LAST — it triggers resized() which needs all
  // components to exist
  setSize(1100, 720);
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

  label.setFont(juce::Font(11.0f));
  label.setJustificationType(juce::Justification::centred);
  label.setColour(juce::Label::textColourId, juce::Colour(0xffaaaaaa));
  addAndMakeVisible(label);

  attach =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          proc.getAPVTS(), paramID, knob);
}

//==============================================================================
void FreeIREditor::paint(juce::Graphics &g) {
  // Background: dark brushed metal gradient
  juce::ColourGradient bgGrad(juce::Colour(0xff1a1a1a), 0, 0,
                              juce::Colour(0xff0e0e0e), 0, (float)getHeight(),
                              false);
  g.setGradientFill(bgGrad);
  g.fillAll();

  // Subtle noise texture overlay (deterministic for consistency)
  juce::Random rng(42);
  g.setColour(juce::Colour(0x08ffffff));
  for (int i = 0; i < 3000; ++i) {
    float nx = rng.nextFloat() * (float)getWidth();
    float ny = rng.nextFloat() * (float)getHeight();
    g.fillRect(nx, ny, 1.0f, 1.0f);
  }

  // Header bar background
  g.setColour(juce::Colour(0xff222222));
  g.fillRect(0, 0, getWidth(), 80);

  // Separator line below header
  g.setColour(juce::Colour(0xff444444));
  g.drawHorizontalLine(80, 0, (float)getWidth());

  // Separator between slot area and right panel
  int slotAreaRight = 4 * 120 + 20;
  g.setColour(juce::Colour(0xff333333));
  g.drawVerticalLine(slotAreaRight, 80, (float)getHeight() - 100);
}

//==============================================================================
void FreeIREditor::resized() {
  auto bounds = getLocalBounds();

  // ===== HEADER (top 80px) =====
  auto header = bounds.removeFromTop(80).reduced(10, 10);

  titleLabel.setBounds(header.removeFromLeft(180));

  // Volume on far right
  auto volumeArea = header.removeFromRight(70);
  volumeKnob.setBounds(
      volumeArea.removeFromTop(46).withSizeKeepingCentre(46, 46));
  volumeLabel.setBounds(volumeArea);

  header.removeFromRight(20);

  // Bass, Treble, Air knobs centered
  int hKnobSize = 46;
  int hKnobSpacing = 80;
  int totalHeaderKnobs = 3 * hKnobSpacing;
  auto headerKnobArea =
      header.withSizeKeepingCentre(totalHeaderKnobs, header.getHeight());

  auto bassArea = headerKnobArea.removeFromLeft(hKnobSpacing);
  bassHeaderKnob.setBounds(
      bassArea.removeFromTop(hKnobSize).withSizeKeepingCentre(hKnobSize,
                                                              hKnobSize));
  bassHeaderLabel.setBounds(bassArea);

  auto trebleArea = headerKnobArea.removeFromLeft(hKnobSpacing);
  trebleHeaderKnob.setBounds(
      trebleArea.removeFromTop(hKnobSize).withSizeKeepingCentre(hKnobSize,
                                                                hKnobSize));
  trebleHeaderLabel.setBounds(trebleArea);

  auto airArea = headerKnobArea.removeFromLeft(hKnobSpacing);
  airHeaderKnob.setBounds(
      airArea.removeFromTop(hKnobSize).withSizeKeepingCentre(hKnobSize,
                                                             hKnobSize));
  airHeaderLabel.setBounds(airArea);

  // ===== EQ SECTION (bottom 100px) =====
  auto eqArea = bounds.removeFromBottom(100).reduced(10, 5);
  eqSection.setBounds(eqArea);

  // ===== MAIN BODY =====
  auto body = bounds.reduced(10, 5);

  // Left: 4 slot strips
  int slotWidth = 120;
  auto slotsArea = body.removeFromLeft(slotWidth * 4);
  for (int i = 0; i < 4; ++i)
    slotComponents[i]->setBounds(
        slotsArea.removeFromLeft(slotWidth).reduced(2));

  body.removeFromLeft(10);

  // Right panel: IR list, waveform, buttons
  auto rightPanel = body;

  // IR List (top ~120px)
  auto irListArea = rightPanel.removeFromTop(120);
  int labelH = 28;
  for (int i = 0; i < 4; ++i)
    irListButtons[i]->setBounds(irListArea.removeFromTop(labelH));

  rightPanel.removeFromTop(5);

  // Waveform display
  auto waveArea = rightPanel.removeFromTop(rightPanel.getHeight() - 40);
  waveformDisplay.setBounds(waveArea);

  rightPanel.removeFromTop(5);

  // Auto Align + Export buttons
  auto buttonsRow = rightPanel;
  autoAlignButton.setBounds(
      buttonsRow.removeFromLeft(buttonsRow.getWidth() / 2).reduced(2));
  exportButton.setBounds(buttonsRow.reduced(2));
}

//==============================================================================
void FreeIREditor::timerCallback() { refreshIRList(); }

void FreeIREditor::alignmentComplete() {
  refreshWaveform();
  repaint();
}

//==============================================================================
void FreeIREditor::refreshIRList() {
  for (int i = 0; i < 4; ++i) {
    auto &slot = proc.getIRSlot(i);
    juce::String text = juce::String(i + 1) + ". ";

    if (slot.isLoaded()) {
      text += slot.getSlotName();
      irListButtons[i]->setColour(juce::TextButton::textColourOffId,
                                  juce::Colours::white);
    } else {
      text += "Click to load impulse response...";
      irListButtons[i]->setColour(juce::TextButton::textColourOffId,
                                  juce::Colour(0xff666666));
    }

    if (irListButtons[i]->getButtonText() != text)
      irListButtons[i]->setButtonText(text);
  }
}

void FreeIREditor::refreshWaveform() {
  for (int i = 0; i < 4; ++i) {
    auto &slot = proc.getIRSlot(i);
    if (slot.isLoaded())
      waveformDisplay.setIRData(i, &slot.getIRBuffer(), slot.getIRSampleRate(),
                                slot.getAlignmentDelay());
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
                           slotComponents[slotIndex]->updateSlotDisplay();
                         }
                       });
}
