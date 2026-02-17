#include "IRBrowserComponent.h"

//==============================================================================
// SidebarModel Implementation
int IRBrowserComponent::SidebarModel::getNumRows() {
  // 1 for "Playlist", then favorites
  return 1 + owner.favoriteFolders.size();
}

void IRBrowserComponent::SidebarModel::paintListBoxItem(int row,
                                                        juce::Graphics &g,
                                                        int w, int h,
                                                        bool sel) {
  if (sel) {
    g.setColour(juce::Colour(0x33ffffff));
    g.fillRect(0, 0, w, h);
  }

  g.setColour(juce::Colours::white);
  g.setFont(14.0f);

  if (row == 0) {
    g.drawText("My Playlist", 10, 0, w - 10, h,
               juce::Justification::centredLeft);
  } else {
    int idx = row - 1;
    if (idx < owner.favoriteFolders.size()) {
      juce::File f(owner.favoriteFolders[idx]);
      g.drawText(f.getFileName(), 10, 0, w - 10, h,
                 juce::Justification::centredLeft);
    }
  }
}

void IRBrowserComponent::SidebarModel::listBoxItemClicked(
    int row, const juce::MouseEvent &) {
  if (row == 0) {
    owner.showPlaylist();
  } else {
    int idx = row - 1;
    if (idx >= 0 && idx < owner.favoriteFolders.size()) {
      owner.scanDirectory(juce::File(owner.favoriteFolders[idx]));
    }
  }
}

//==============================================================================
IRBrowserComponent::IRBrowserComponent(FreeIRAudioProcessor &p)
    : proc(p), sidebarModel(*this) {
  // Setup Sidebar
  placesLabel.setFont(juce::Font(11.0f, juce::Font::bold));
  placesLabel.setColour(juce::Label::textColourId, juce::Colour(0xffaaaaaa));
  addAndMakeVisible(placesLabel);

  addFolderButton.setButtonText("+");
  addFolderButton.onClick = [this] {
    auto fc = std::make_shared<juce::FileChooser>(
        "Select Folder to Add",
        juce::File::getSpecialLocation(juce::File::userHomeDirectory));
    fc->launchAsync(juce::FileBrowserComponent::openMode |
                        juce::FileBrowserComponent::canSelectDirectories,
                    [this, fc](const juce::FileChooser &chooser) {
                      auto result = chooser.getResult();
                      if (result.isDirectory()) {
                        favoriteFolders.add(result.getFullPathName());
                        sidebarList.updateContent();
                        refreshFavorites();
                        savePersistentState();
                      }
                    });
  };
  addAndMakeVisible(addFolderButton);

  sidebarList.setModel(&sidebarModel);
  sidebarList.setRowHeight(24);
  sidebarList.setColour(juce::ListBox::backgroundColourId,
                        juce::Colours::transparentBlack);
  addAndMakeVisible(sidebarList);

  // Setup File List
  irListLabel.setFont(juce::Font(11.0f, juce::Font::bold));
  irListLabel.setColour(juce::Label::textColourId, juce::Colour(0xffaaaaaa));
  addAndMakeVisible(irListLabel);

  fileList.setModel(this);
  fileList.setRowHeight(20);
  fileList.setColour(juce::ListBox::backgroundColourId,
                     juce::Colour(0x1affffff));
  fileList.setMultipleSelectionEnabled(true);
  addAndMakeVisible(fileList);

  // Load persistence
  loadPersistentState();

  // No default folders unless empty?
  // User said "remove Music, downloads too".
  // So if empty, stay empty or maybe just keep blank.
  if (favoriteFolders.size() == 0) {
    // Optional: Add default documents? No, user explicitly dislikes autos.
  }
}

IRBrowserComponent::~IRBrowserComponent() {}

void IRBrowserComponent::paint(juce::Graphics &g) {
  g.setColour(juce::Colour(0x0affffff));
  g.fillRoundedRectangle(getLocalBounds().toFloat(), 8.0f);
}

void IRBrowserComponent::resized() {
  auto area = getLocalBounds().reduced(8);

  int sidebarW = area.getWidth() * 0.35f;
  auto sidebarArea = area.removeFromLeft(sidebarW);
  area.removeFromLeft(8); // Gap

  // Sidebar Layout
  auto headerArea = sidebarArea.removeFromTop(24);
  addFolderButton.setBounds(headerArea.removeFromRight(24));
  placesLabel.setBounds(headerArea);

  sidebarList.setBounds(sidebarArea);

  // File List Layout
  irListLabel.setBounds(area.removeFromTop(24));
  fileList.setBounds(area);
}

//==============================================================================
int IRBrowserComponent::getNumRows() { return (int)currentFileList.size(); }

juce::var IRBrowserComponent::getDragSourceDescription(
    const juce::SparseSet<int> &selectedRows) {
  juce::Array<juce::var> files;
  for (int i = 0; i < selectedRows.size(); ++i) {
    if (isPositiveAndBelow(selectedRows[i], (int)currentFileList.size())) {
      auto file = currentFileList[selectedRows[i]];
      if (file.existsAsFile())
        files.add(file.getFullPathName());
    }
  }
  return files;
}

void IRBrowserComponent::paintListBoxItem(int row, juce::Graphics &g, int width,
                                          int height, bool rowIsSelected) {
  if (rowIsSelected) {
    g.setColour(juce::Colour(0x3300ccff));
    g.fillRect(0, 0, width, height);
  }

  if (row >= currentFileList.size())
    return;

  g.setColour(juce::Colours::white);
  g.setFont(12.0f);
  g.drawText(currentFileList[row].getFileName(), 4, 0, width - 4, height,
             juce::Justification::centredLeft);
}

void IRBrowserComponent::listBoxItemClicked(int row,
                                            const juce::MouseEvent &e) {
  if (e.mods.isPopupMenu()) {
    if (row >= 0 && row < currentFileList.size()) {
      juce::PopupMenu m;
      m.addItem(1, "Add to Playlist");
      m.addSeparator();
      m.addItem(2, "Load into Slot A");
      m.addItem(3, "Load into Slot B");
      m.addItem(4, "Load into Slot C");
      m.addItem(5, "Load into Slot D");

      m.showMenuAsync(juce::PopupMenu::Options(), [this, row](int id) {
        if (id == 1) {
          auto selected = fileList.getSelectedRows();
          if (selected.size() > 0) {
            for (int i = 0; i < selected.size(); ++i) {
              if (isPositiveAndBelow(selected[i], (int)currentFileList.size()))
                addToPlaylist(currentFileList[selected[i]]);
            }
          } else {
            addToPlaylist(currentFileList[row]);
          }
        } else if (id >= 2 && id <= 5) {
          int slotIdx = id - 2;
          if (onLoadIRToSlot)
            onLoadIRToSlot(currentFileList[row], slotIdx);
        }
      });
    }
  }
}

void IRBrowserComponent::listBoxItemDoubleClicked(int row,
                                                  const juce::MouseEvent &) {
  if (row >= 0 && row < currentFileList.size()) {
    if (onLoadIR)
      onLoadIR(currentFileList[row]);
  }
}

//==============================================================================
void IRBrowserComponent::scanDirectory(const juce::File &dir) {
  currentDirectory = dir;
  isShowingPlaylist = false;
  currentFileList.clear();

  if (dir.isDirectory()) {
    auto files =
        dir.findChildFiles(juce::File::findFiles, false, "*.wav;*.aif;*.aiff");
    for (auto &f : files)
      currentFileList.push_back(f);
  }

  irListLabel.setText(dir.getFileName().toUpperCase(),
                      juce::dontSendNotification);
  fileList.updateContent();

  // Note: we don't save selection, just the favorite folders
}

void IRBrowserComponent::showPlaylist() {
  isShowingPlaylist = true;
  currentDirectory = juce::File();
  // refresh playlist from paths
  currentFileList.clear();
  for (auto &path : playlistFiles) {
    juce::File f(path);
    if (f.existsAsFile())
      currentFileList.push_back(f);
  }

  irListLabel.setText("MY PLAYLIST", juce::dontSendNotification);
  fileList.updateContent();
}

void IRBrowserComponent::addToPlaylist(const juce::File &file) {
  if (playlistFiles.contains(file.getFullPathName()))
    return;

  playlistFiles.add(file.getFullPathName());
  savePersistentState();

  if (isShowingPlaylist) {
    showPlaylist();
  }
}

void IRBrowserComponent::refreshFavorites() { sidebarList.updateContent(); }

void IRBrowserComponent::savePersistentState() {
  // Current Preset? We don't track it here.
  // Just folders and playlist.
  juce::String lastPreset = ""; // We don't control this yet.
  proc.getPresetManager().saveGlobalSettings(favoriteFolders, playlistFiles,
                                             lastPreset);
}

void IRBrowserComponent::loadPersistentState() {
  juce::String lastPreset;
  proc.getPresetManager().loadGlobalSettings(favoriteFolders, playlistFiles,
                                             lastPreset);
  sidebarList.updateContent();
}
