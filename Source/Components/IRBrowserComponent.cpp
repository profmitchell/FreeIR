#include "IRBrowserComponent.h"

//==============================================================================
// SidebarModel Implementation
int IRBrowserComponent::SidebarModel::getNumRows() {
  // 1 for "Playlist", then favorites
  return 1 + (int)owner.favoriteFolders.size();
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
    // Icon?
  } else {
    int idx = row - 1;
    if (idx < owner.favoriteFolders.size()) {
      g.drawText(owner.favoriteFolders[idx].getFileName(), 10, 0, w - 10, h,
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
      owner.scanDirectory(owner.favoriteFolders[idx]);
    }
  }
}

//==============================================================================
IRBrowserComponent::IRBrowserComponent() : sidebarModel(*this) {
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
                        favoriteFolders.push_back(result);
                        sidebarList.updateContent();
                        refreshFavorites();
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

  // Defaults
  favoriteFolders.push_back(
      juce::File::getSpecialLocation(juce::File::userHomeDirectory)
          .getChildFile("Music"));
  favoriteFolders.push_back(
      juce::File::getSpecialLocation(juce::File::userHomeDirectory)
          .getChildFile("Downloads"));

  // Allow right click on file list to add to playlist
  // Handled in list click? No, MouseEvent is passed.
}

IRBrowserComponent::~IRBrowserComponent() {}

void IRBrowserComponent::paint(juce::Graphics &g) {
  // Glass Background for entire browser
  g.setColour(juce::Colour(0x0affffff));
  g.fillRoundedRectangle(getLocalBounds().toFloat(), 8.0f);

  // Separator line
  int sidebarW = 100; // actually resized() defines this
                      // g.setColour(juce::Colour(0x1affffff));
  // g.drawVerticalLine(sidebarList.getRight() + 4, 0, (float)getHeight());
}

void IRBrowserComponent::resized() {
  auto area = getLocalBounds().reduced(8);

  // Sidebar: Top 40%? Or Left column?
  // User asked for "directories on the side". "Side" implies left column
  // usually. Let's do a split: Left 35% sidebar, Right 65% file list? Or
  // Top/Bottom? "on the side... and when selected display... within". Standard
  // 2-column layout.

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
// Main File List
int IRBrowserComponent::getNumRows() { return (int)currentFileList.size(); }

juce::var IRBrowserComponent::getDragSourceDescription(
    const juce::SparseSet<int> &selectedRows) {
  juce::Array<juce::var> files;
  for (int i = 0; i < selectedRows.size(); ++i) {
    if (isPositiveAndBelow(selectedRows[i], currentFileList.size())) {
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
    g.setColour(juce::Colour(0x3300ccff)); // Selection cyan tint
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
              if (isPositiveAndBelow(selected[i], currentFileList.size()))
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
}

void IRBrowserComponent::showPlaylist() {
  isShowingPlaylist = true;
  currentDirectory = juce::File();
  currentFileList = playlistFiles;

  irListLabel.setText("MY PLAYLIST", juce::dontSendNotification);
  fileList.updateContent();
}

void IRBrowserComponent::addToPlaylist(const juce::File &file) {
  // Avoid duplicates?
  for (auto &f : playlistFiles)
    if (f == file)
      return;
  playlistFiles.push_back(file);
  if (isShowingPlaylist) {
    currentFileList = playlistFiles;
    fileList.updateContent();
  }
}

void IRBrowserComponent::refreshFavorites() {
  // TODO: Save to properties
}
