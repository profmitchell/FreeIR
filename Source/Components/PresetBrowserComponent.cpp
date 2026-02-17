#include "PresetBrowserComponent.h"

PresetBrowserComponent::PresetBrowserComponent(FreeIRAudioProcessor &p)
    : proc(p) {
  // Navigation
  prevButton.setButtonText("<");
  prevButton.onClick = [this] { loadPrev(); };
  addAndMakeVisible(prevButton);

  nextButton.setButtonText(">");
  nextButton.onClick = [this] { loadNext(); };
  addAndMakeVisible(nextButton);

  // Preset Label
  presetNameLabel.setText(proc.currentPresetName, juce::dontSendNotification);
  presetNameLabel.setJustificationType(juce::Justification::centred);
  presetNameLabel.setFont(juce::Font(16.0f, juce::Font::bold));
  presetNameLabel.setColour(juce::Label::textColourId, juce::Colours::white);
  addAndMakeVisible(presetNameLabel);

  // Save
  saveButton.setButtonText("Save");
  saveButton.onClick = [this] { showSaveDialog(); };
  addAndMakeVisible(saveButton);

  // Menu
  menuButton.setButtonText("...");
  menuButton.onClick = [this] { showMenu(); };
  addAndMakeVisible(menuButton);

  updateDisplay();
}

PresetBrowserComponent::~PresetBrowserComponent() {}

void PresetBrowserComponent::paint(juce::Graphics &g) {
  // Background for preset area
  g.setColour(juce::Colour(0x1affffff));
  g.fillRoundedRectangle(getLocalBounds().toFloat(), 6.0f);
}

void PresetBrowserComponent::resized() {
  auto area = getLocalBounds().reduced(4);

  // Left: <
  prevButton.setBounds(area.removeFromLeft(24));
  area.removeFromLeft(4);

  // Right: ... Save >
  nextButton.setBounds(area.removeFromRight(24));
  area.removeFromRight(4);

  menuButton.setBounds(area.removeFromRight(30));
  area.removeFromRight(4);

  saveButton.setBounds(area.removeFromRight(50));
  area.removeFromRight(4);

  // Center: Label
  presetNameLabel.setBounds(area);
}

void PresetBrowserComponent::updateDisplay() {
  presetNameLabel.setText(proc.currentPresetName, juce::dontSendNotification);
}

void PresetBrowserComponent::loadNext() {
  auto next = proc.getPresetManager().getNextPreset(proc.currentPresetName);
  if (next.isNotEmpty()) {
    loadPreset(next);
  }
}

void PresetBrowserComponent::loadPrev() {
  auto prev = proc.getPresetManager().getPrevPreset(proc.currentPresetName);
  if (prev.isNotEmpty()) {
    loadPreset(prev);
  }
}

void PresetBrowserComponent::showMenu() {
  juce::PopupMenu m;
  m.addItem("Show Presets Folder", [this] {
    proc.getPresetManager().getPresetsFolder().revealToUser();
  });
  m.addSeparator();
  auto list = proc.getPresetManager().getPresetList();
  if (list.isEmpty()) {
    m.addItem("No Presets Found", false, false, nullptr);
  } else {
    juce::PopupMenu presetsSub;
    for (const auto &name : list) {
      bool isActive = (name == proc.currentPresetName);
      presetsSub.addItem(name, true, isActive,
                         [this, name] { loadPreset(name); });
    }
    m.addSubMenu("Load Preset", presetsSub);
  }

  if (proc.currentPresetName != "Init" &&
      proc.currentPresetName != "Default") { // Simple check
    m.addSeparator();
    m.addItem("Delete '" + proc.currentPresetName + "'", [this] {
      proc.getPresetManager().deletePreset(proc.currentPresetName);
      // fallback to something else?
      loadPreset("Init"); // Assuming 'Init' or just clear
    });
  }

  m.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(menuButton));
}

void PresetBrowserComponent::showSaveDialog() {
  // Better way: Custom Dialog
  auto *w = new juce::AlertWindow(
      "Save Preset", "Enter preset name:", juce::AlertWindow::QuestionIcon);
  w->addTextEditor("name", proc.currentPresetName, "Preset Name");
  w->addButton("Save", 1, juce::KeyPress(juce::KeyPress::returnKey, 0, 0));
  w->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey, 0, 0));

  w->enterModalState(true,
                     juce::ModalCallbackFunction::create([this, w](int result) {
                       if (result == 1) {
                         auto name = w->getTextEditorContents("name");
                         if (name.isNotEmpty()) {
                           savePreset(name);
                         }
                       }
                       delete w;
                     }));
}

void PresetBrowserComponent::savePreset(const juce::String &name) {
  // 1. Update processor state to include current name?
  // Actually, saving happens from APVTS state + manual props.
  // PluginProcessor::getStateInformation does the work.
  juce::MemoryBlock mb;
  proc.getStateInformation(mb);

  proc.getPresetManager().savePreset(name, mb);
  proc.currentPresetName = name;
  updateDisplay();
}

void PresetBrowserComponent::loadPreset(const juce::String &name) {
  juce::MemoryBlock mb;
  proc.getPresetManager().loadPreset(name, mb);
  if (mb.getSize() > 0) {
    proc.setStateInformation(mb.getData(), (int)mb.getSize());
    proc.currentPresetName = name; // Update name manually as it might be
                                   // overwritten by setState if not in XML?
    // Actually setState reads it from XML, so if the preset has it, it's good.
    // But if we load an old preset without name, we might want to force it.
    if (proc.currentPresetName != name)
      proc.currentPresetName = name;
    updateDisplay();

    // Also update editor state? Editor listens to APVTS, so knobs move.
    // Slots? IRs load.
    // Slot Displays? We need to tell editor to refresh slot displays.
    // PresetBrowser can't easily reach editor components unless we have a
    // callback or dynamic cast parent. Or broadcasther? Let's add a
    // "onPresetLoaded" callback in PluginProcessor? Or just let Editor poll?
    // Editor is parent of PresetBrowser.
    // Maybe PresetBrowser has a "onPresetChange" callback.
    if (auto *editor =
            findParentComponentOfClass<juce::AudioProcessorEditor>()) {
      editor->repaint(); // Gross refresh
                         // We need to refresh slots specifically.
                         // We can assume Editor will repaint.
      // But SlotComponents update their text in 'updateSlotDisplay()'.
      // We need to trigger that.
    }
  }
}
