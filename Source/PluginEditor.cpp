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
    if (proc.getIRSlot(0).isLoaded()) {
      for (int i = 0; i < 4; ++i) {
        if (!proc.getIRSlot(i).isLoaded()) {
          proc.getIRSlot(i).loadImpulseResponse(f);
          return;
        }
      }
      proc.getIRSlot(0).loadImpulseResponse(f);
    } else {
      proc.getIRSlot(0).loadImpulseResponse(f);
    }
    refreshWaveform();
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
                             proc.exportMixedIR(file);
                           }
                         });
  };
  exportButton.setColour(juce::TextButton::buttonColourId,
                         juce::Colour(0x15228822));
  exportButton.setColour(juce::TextButton::buttonOnColourId,
                         juce::Colour(0x3344aa44));
  addAndMakeVisible(exportButton);

  // Plugin button
  pluginButton.onClick = [this]() { showPluginMenu(); };
  pluginButton.setColour(juce::TextButton::buttonColourId,
                         juce::Colour(0x15884422));
  pluginButton.setColour(juce::TextButton::buttonOnColourId,
                         juce::Colour(0x33aa6644));
  addAndMakeVisible(pluginButton);
  updatePluginButtonText();

  // EQ Section
  addAndMakeVisible(eqSection);

  // Register for auto-align callbacks & init
  proc.getAutoAligner().addListener(this);

  startTimerHz(15);
  refreshWaveform();
  setSize(defaultWidth, defaultHeight);
}

FreeIREditor::~FreeIREditor() {
  hostedPluginWindow.reset();
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
void FreeIREditor::rebuildNoiseTexture(int w, int h) {
  if (w <= 0 || h <= 0)
    return;

  noiseTexture = juce::Image(juce::Image::ARGB, w, h, true);
  juce::Image::BitmapData bitmap(noiseTexture,
                                 juce::Image::BitmapData::writeOnly);

  juce::Random rng(99);
  auto noiseColour = juce::Colour(0x06ffffff);

  for (int i = 0; i < 4000; ++i) {
    int nx = rng.nextInt(w);
    int ny = rng.nextInt(h);
    bitmap.setPixelColour(nx, ny, noiseColour);
  }
}

void FreeIREditor::paint(juce::Graphics &g) {
  juce::ColourGradient bgGrad(juce::Colour(0xff000000), 0, 0,
                              juce::Colour(0xff121212), 0, (float)getHeight(),
                              false);
  g.setGradientFill(bgGrad);
  g.fillAll();

  if (noiseTexture.isValid())
    g.drawImageAt(noiseTexture, 0, 0);
}

//==============================================================================
void FreeIREditor::resized() {
  rebuildNoiseTexture(getWidth(), getHeight());

  auto mapRect = [&](int x, int y, int w, int h) -> juce::Rectangle<int> {
    float sx = (float)getWidth() / 1100.0f;
    float sy = (float)getHeight() / 720.0f;
    return juce::Rectangle<float>(x * sx, y * sy, w * sx, h * sy)
        .toNearestInt();
  };

  // Header
  titleLabel.setBounds(mapRect(24, 14, 200, 40));
  subtitleLabel.setBounds(mapRect(26, 50, 200, 20));

  // Preset Browser (Top Center)
  presetBrowser.setBounds(mapRect(400, 20, 300, 30));

  // Plugin button (Top Right)
  pluginButton.setBounds(mapRect(860, 20, 220, 30));

  // Main Content Area
  int contentY = 80;
  int contentH = 720 - contentY - 12;

  int browserW = 240;
  browser.setBounds(mapRect(12, contentY, browserW, contentH));

  int mixerX = 12 + browserW + 12;
  int mixerW = 1100 - mixerX - 12;

  int waveH = 200;
  waveformDisplay.setBounds(mapRect(mixerX, contentY, mixerW, waveH));

  int slotsY = contentY + waveH + 12;
  int eqH = 120;
  int slotH = contentH - waveH - 12 - eqH - 12;

  int slotW = mixerW / 4;
  for (size_t i = 0; i < 4; ++i) {
    slotComponents[i]->setBounds(
        mapRect(mixerX + ((int)i * slotW), slotsY, slotW - 8, slotH));
  }

  int eqY = slotsY + slotH + 12;
  int eqReqW = (int)(mixerW * 0.70f);
  eqSection.setBounds(mapRect(mixerX, eqY, eqReqW, eqH));

  int btnAreaX = mixerX + eqReqW + 12;
  int btnAreaW = mixerW - eqReqW - 12;

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
void FreeIREditor::timerCallback() { updatePluginButtonText(); }

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

void FreeIREditor::loadIRForSlot(int /*slotIndex*/) {}

//==============================================================================
// Plugin Hosting
//==============================================================================
void FreeIREditor::showPluginMenu() {
  juce::PopupMenu menu;

  // --- Loaded plugin controls ---
  if (proc.hasHostedPlugin()) {
    auto name = proc.getHostedPluginName();
    menu.addSectionHeader("Loaded: " + name);

    bool editorVisible = hostedPluginWindow != nullptr &&
                         hostedPluginWindow->isVisible();

    if (editorVisible)
      menu.addItem("Hide Plugin Editor", [this]() { hideHostedPluginEditor(); });
    else
      menu.addItem("Show Plugin Editor", [this]() { showHostedPluginEditor(); });

    menu.addItem("Clear Plugin", [this]() { clearHostedPlugin(); });
    menu.addSeparator();
  }

  // --- Scan ---
  if (pluginScanInProgress) {
    menu.addItem("Scanning...", false, false, nullptr);
  } else if (proc.getKnownPluginList().getNumTypes() == 0 &&
             !pluginScanComplete) {
    menu.addItem("Scan for Plugins...", [this]() { scanForPluginsAsync(); });
  } else {
    menu.addItem("Rescan Plugins...", [this]() { scanForPluginsAsync(); });
  }

  // --- Search ---
  if (proc.getKnownPluginList().getNumTypes() > 0) {
    menu.addItem("Search Plugins...", [this]() { searchAndLoadPlugin(); });
    menu.addSeparator();

    // Full plugin list (effects only), grouped by manufacturer
    auto fullList = buildPluginListMenu("");
    menu.addSubMenu(
        "All Plugins (" +
            juce::String(proc.getKnownPluginList().getNumTypes()) + ")",
        fullList);
  }

  menu.showMenuAsync(
      juce::PopupMenu::Options().withTargetComponent(pluginButton));
}

juce::PopupMenu
FreeIREditor::buildPluginListMenu(const juce::String &filter) {
  juce::PopupMenu result;

  auto &knownList = proc.getKnownPluginList();
  auto types = knownList.getTypes();

  // Sort by manufacturer then name
  std::sort(types.begin(), types.end(),
            [](const juce::PluginDescription &a,
               const juce::PluginDescription &b) {
              if (a.manufacturerName == b.manufacturerName)
                return a.name.compareIgnoreCase(b.name) < 0;
              return a.manufacturerName.compareIgnoreCase(
                         b.manufacturerName) < 0;
            });

  std::map<juce::String, juce::PopupMenu> byManufacturer;
  int count = 0;

  for (auto &desc : types) {
    // Skip instruments (synths) -- only show effects
    if (desc.isInstrument)
      continue;

    // Apply search filter
    if (filter.isNotEmpty()) {
      bool matches =
          desc.name.containsIgnoreCase(filter) ||
          desc.manufacturerName.containsIgnoreCase(filter) ||
          desc.category.containsIgnoreCase(filter);
      if (!matches)
        continue;
    }

    // Format tag: (VST3) or (AU)
    juce::String tag;
    if (desc.pluginFormatName == "VST3")
      tag = " [VST3]";
    else if (desc.pluginFormatName == "AudioUnit")
      tag = " [AU]";

    byManufacturer[desc.manufacturerName].addItem(
        desc.name + tag, [this, desc]() {
          proc.loadHostedPlugin(desc, [this](bool success) {
            juce::MessageManager::callAsync([this, success]() {
              if (success) {
                updatePluginButtonText();
                showHostedPluginEditor();
              }
            });
          });
        });
    ++count;
  }

  if (count == 0) {
    result.addItem("No matching plugins", false, false, nullptr);
    return result;
  }

  for (auto &[manufacturer, subMenu] : byManufacturer)
    result.addSubMenu(manufacturer, subMenu);

  return result;
}

void FreeIREditor::showFilteredPluginMenu(const juce::String &filter) {
  auto menu = buildPluginListMenu(filter);
  menu.showMenuAsync(
      juce::PopupMenu::Options().withTargetComponent(pluginButton));
}

void FreeIREditor::scanForPluginsAsync() {
  if (pluginScanInProgress)
    return;

  pluginScanInProgress = true;

  // Run scan on a background thread
  juce::Thread::launch([this]() {
    auto &formatManager = proc.getPluginFormatManager();
    auto &knownList = proc.getKnownPluginList();

    for (auto *format : formatManager.getFormats()) {
      auto searchPaths = format->getDefaultLocationsToSearch();
      juce::PluginDirectoryScanner scanner(knownList, *format, searchPaths,
                                           true, juce::File());

      juce::String pluginName;
      while (scanner.scanNextFile(true, pluginName)) {
        // scanning
      }
    }

    juce::MessageManager::callAsync([this]() {
      pluginScanComplete = true;
      pluginScanInProgress = false;

      auto numFound = proc.getKnownPluginList().getNumTypes();
      juce::NativeMessageBox::showMessageBoxAsync(
          juce::MessageBoxIconType::InfoIcon, "Plugin Scan Complete",
          "Found " + juce::String(numFound) + " plugins.");
    });
  });
}

void FreeIREditor::searchAndLoadPlugin() {
  auto *aw = new juce::AlertWindow("Search Plugins", "Type a plugin name:",
                                   juce::MessageBoxIconType::QuestionIcon);
  aw->addTextEditor("search", "", "Plugin name:");
  aw->addButton("Search", 1);
  aw->addButton("Cancel", 0);

  aw->enterModalState(true, juce::ModalCallbackFunction::create(
                                [this, aw](int result) {
                                  if (result == 1) {
                                    auto query =
                                        aw->getTextEditorContents("search")
                                            .trim();
                                    if (query.isNotEmpty())
                                      showFilteredPluginMenu(query);
                                  }
                                  delete aw;
                                }));
}

void FreeIREditor::showHostedPluginEditor() {
  auto *plugin = proc.getHostedPlugin();
  if (plugin == nullptr)
    return;

  // If window exists but hidden, just show it
  if (hostedPluginWindow != nullptr) {
    hostedPluginWindow->setVisible(true);
    hostedPluginWindow->toFront(true);
    return;
  }

  if (auto *editor = plugin->createEditorIfNeeded()) {
    hostedPluginWindow =
        std::make_unique<HostedPluginWindow>(editor, plugin->getName());
  }
}

void FreeIREditor::hideHostedPluginEditor() {
  if (hostedPluginWindow != nullptr)
    hostedPluginWindow->setVisible(false);
}

void FreeIREditor::toggleHostedPluginEditor() {
  if (hostedPluginWindow != nullptr && hostedPluginWindow->isVisible())
    hideHostedPluginEditor();
  else
    showHostedPluginEditor();
}

void FreeIREditor::clearHostedPlugin() {
  hostedPluginWindow.reset();
  proc.unloadHostedPlugin();
  updatePluginButtonText();
}

void FreeIREditor::updatePluginButtonText() {
  if (proc.hasHostedPlugin()) {
    auto name = proc.getHostedPluginName();
    // Truncate long names
    if (name.length() > 22)
      name = name.substring(0, 20) + "..";
    pluginButton.setButtonText("FX: " + name);
    pluginButton.setColour(juce::TextButton::buttonColourId,
                           juce::Colour(0x33ff8800));
  } else {
    pluginButton.setButtonText("Load Plugin");
    pluginButton.setColour(juce::TextButton::buttonColourId,
                           juce::Colour(0x15884422));
  }
}
