#include "IRSlotComponent.h"
#include "../FreeIRLookAndFeel.h"

// Internal helper for lambda listener
struct Linker : public juce::Slider::Listener {
  std::function<void()> cb;
  Linker(std::function<void()> c) : cb(c) {}
  void sliderValueChanged(juce::Slider *) override { cb(); }
};

IRSlotComponent::IRSlotComponent(FreeIRAudioProcessor &processor, int slotIndex)
    : proc(processor), slotID(slotIndex) {

  // 1. Navigation Buttons
  prevButton.setButtonText("<");
  prevButton.onClick = [this] {
    proc.getIRSlot(slotID).loadPrevIR();
    updateSlotDisplay();
    if (onSlotChanged)
      onSlotChanged();
  };
  addAndMakeVisible(prevButton);

  nextButton.setButtonText(">");
  nextButton.onClick = [this] {
    proc.getIRSlot(slotID).loadNextIR();
    updateSlotDisplay();
    if (onSlotChanged)
      onSlotChanged();
  };
  addAndMakeVisible(nextButton);

  // 2. Load / Name Button
  loadButton.setButtonText("Empty");
  loadButton.onClick = [this] {
    // Open file chooser
    auto chooser = std::make_shared<juce::FileChooser>(
        "Load Impulse Response",
        juce::File::getSpecialLocation(juce::File::userHomeDirectory),
        "*.wav;*.aif;*.aiff");

    chooser->launchAsync(juce::FileBrowserComponent::openMode |
                             juce::FileBrowserComponent::canSelectFiles,
                         [this, chooser](const juce::FileChooser &fc) {
                           auto file = fc.getResult();
                           if (file.existsAsFile()) {
                             proc.getIRSlot(slotID).loadImpulseResponse(file);
                             updateSlotDisplay();
                             if (onSlotChanged)
                               onSlotChanged();
                           }
                         });
  };
  addAndMakeVisible(loadButton);

  // Clear Button
  clearButton.setButtonText("X");
  // clearButton.setTooltip("Clear"); // TooltipWindow might not be set up
  // globally, but good to have
  clearButton.onClick = [this] {
    proc.getIRSlot(slotID).clearImpulseResponse();
    updateSlotDisplay();
    if (onSlotChanged)
      onSlotChanged();
  };
  addAndMakeVisible(clearButton);

  // 3. Slot Number Label
  slotNumLabel.setText(juce::String(slotID + 1), juce::dontSendNotification);
  slotNumLabel.setFont(juce::Font(14.0f, juce::Font::bold));
  slotNumLabel.setColour(juce::Label::textColourId,
                         juce::Colours::white.withAlpha(0.4f));
  slotNumLabel.setJustificationType(juce::Justification::centred);
  // addAndMakeVisible(slotNumLabel); // Maybe don't need it if we have clear
  // layout? User said "move IR labels to above". The ID 1,2,3,4 is handled by
  // position. Let's keep it subtle or remove. Let's keep it top-left or
  // background? Actually, let's omit it for a cleaner look or put it in the
  // corner.
  addAndMakeVisible(slotNumLabel);

  // 4. Controls
  auto prefix = "Slot" + juce::String(slotID + 1) + "_";

  // Delay
  delayKnob.setSliderStyle(juce::Slider::RotaryVerticalDrag);
  delayKnob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
  delayKnob.setPopupDisplayEnabled(true, true, this);
  delayKnob.setTextValueSuffix(" ms");
  addAndMakeVisible(delayKnob);
  delayAttach =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          proc.getAPVTS(), prefix + "DelayMs", delayKnob);

  delayLabel.setText("Delay", juce::dontSendNotification);
  delayLabel.setFont(10.0f);
  delayLabel.setJustificationType(juce::Justification::centred);
  delayLabel.setColour(juce::Label::textColourId, juce::Colours::grey);
  addAndMakeVisible(delayLabel);

  // Pan
  panKnob.setSliderStyle(juce::Slider::RotaryVerticalDrag);
  panKnob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
  panKnob.setPopupDisplayEnabled(true, true, this);
  addAndMakeVisible(panKnob);
  panAttach =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          proc.getAPVTS(), prefix + "Pan", panKnob);

  panLabel.setText("Pan", juce::dontSendNotification);
  panLabel.setFont(10.0f);
  panLabel.setJustificationType(juce::Justification::centred);
  panLabel.setColour(juce::Label::textColourId, juce::Colours::grey);
  addAndMakeVisible(panLabel);

  // Fader (Vol)
  fader.setSliderStyle(juce::Slider::LinearVertical);
  fader.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 14);
  fader.setColour(juce::Slider::textBoxOutlineColourId,
                  juce::Colours::transparentBlack);
  fader.setColour(juce::Slider::trackColourId, juce::Colour(0x33ffffff));
  fader.setColour(juce::Slider::thumbColourId, juce::Colours::white);
  addAndMakeVisible(fader);
  faderAttach =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          proc.getAPVTS(), prefix + "Level", fader);

  // Mute/Solo
  muteButton.setClickingTogglesState(true);
  muteButton.setColour(juce::TextButton::buttonOnColourId,
                       juce::Colours::red.withAlpha(0.6f));
  addAndMakeVisible(muteButton);
  muteAttach =
      std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
          proc.getAPVTS(), prefix + "Mute", muteButton);

  soloButton.setClickingTogglesState(true);
  soloButton.setColour(juce::TextButton::buttonOnColourId,
                       juce::Colours::yellow.withAlpha(0.6f));
  addAndMakeVisible(soloButton);
  soloAttach =
      std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
          proc.getAPVTS(), prefix + "Solo", soloButton);

  // Waveform linking
  delayKnob.addListener(new Linker([this] {
    if (onSlotChanged)
      onSlotChanged();
  }));

  updateSlotDisplay();
}

IRSlotComponent::~IRSlotComponent() {}

void IRSlotComponent::paint(juce::Graphics &g) {
  auto bounds = getLocalBounds().toFloat();

  // Glass Card Background
  g.setColour(juce::Colour(0x0affffff));
  g.fillRoundedRectangle(bounds, 8.0f);

  g.setColour(juce::Colour(0x1affffff));
  g.drawRoundedRectangle(bounds, 8.0f, 1.0f);

  // Header bg for buttons
  auto header = bounds.removeFromTop(36);
  g.setColour(juce::Colour(0x0affffff));
  g.fillRoundedRectangle(header,
                         8.0f); // Top corners only ideally but this is fine
}

void IRSlotComponent::resized() {
  auto area = getLocalBounds().reduced(6);

  // Top Row: < Name > Clear
  auto topRow = area.removeFromTop(24);

  prevButton.setBounds(topRow.removeFromLeft(24));

  clearButton.setBounds(topRow.removeFromRight(24));
  topRow.removeFromRight(2); // Gap

  nextButton.setBounds(topRow.removeFromRight(24));
  topRow.removeFromRight(2); // Gap

  loadButton.setBounds(topRow); // Center

  area.removeFromTop(12); // Gap

  // Pan & Delay Row
  auto knobRow = area.removeFromTop(60);
  int knobWid = area.getWidth() / 2;

  auto panArea = knobRow.removeFromLeft(knobWid);
  auto delayArea = knobRow;

  panKnob.setBounds(panArea.removeFromTop(46).reduced(4));
  panLabel.setBounds(panArea);

  delayKnob.setBounds(delayArea.removeFromTop(46).reduced(4));
  delayLabel.setBounds(delayArea);

  area.removeFromTop(12);

  // Bottom: Mute/Solo
  auto botRow = area.removeFromBottom(24);
  muteButton.setBounds(botRow.removeFromLeft(area.getWidth() / 2 - 2));
  soloButton.setBounds(botRow.removeFromRight(area.getWidth() / 2 - 2));

  area.removeFromBottom(8);

  // Fader (remaining height)
  fader.setBounds(area.reduced(10, 0));

  // Slot Num Label (Overlay on fader slightly? or Top Right?)
  // Let's hide it for now as requested layout is clean.
  // slotNumLabel.setBounds(0,0,0,0);
}

void IRSlotComponent::updateSlotDisplay() {
  auto &slot = proc.getIRSlot(slotID);
  if (slot.isLoaded()) {
    loadButton.setButtonText(slot.getSlotName());
    // Highlight logic could go here
  } else {
    loadButton.setButtonText("Empty");
  }
}

void IRSlotComponent::setDelayEnabled(bool enabled) {
  delayKnob.setEnabled(enabled);
  delayKnob.setAlpha(enabled ? 1.0f : 0.5f);
}

bool IRSlotComponent::isInterestedInDragSource(
    const juce::DragAndDropTarget::SourceDetails &dragSourceDetails) {
  return dragSourceDetails.description.isArray();
}

void IRSlotComponent::itemDropped(
    const juce::DragAndDropTarget::SourceDetails &dragSourceDetails) {
  if (dragSourceDetails.description.isArray()) {
    auto files = dragSourceDetails.description;
    if (files.size() > 0) {
      juce::File file(files[0].toString());
      if (file.existsAsFile()) {
        proc.getIRSlot(slotID).loadImpulseResponse(file);
        updateSlotDisplay();
        if (onSlotChanged)
          onSlotChanged();
      }
    }
  }
}
