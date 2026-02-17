#include "../PluginProcessor.h"

//==============================================================================
class IRBrowserComponent : public juce::Component, public juce::ListBoxModel {
public:
  IRBrowserComponent(FreeIRAudioProcessor &p);
  ~IRBrowserComponent() override;

  void paint(juce::Graphics &g) override;
  void resized() override;

  // File List Model
  int getNumRows() override;
  void paintListBoxItem(int rowNumber, juce::Graphics &g, int width, int height,
                        bool rowIsSelected) override;
  void listBoxItemClicked(int row, const juce::MouseEvent &) override;
  void listBoxItemDoubleClicked(int row, const juce::MouseEvent &) override;
  juce::var
  getDragSourceDescription(const juce::SparseSet<int> &selectedRows) override;

  std::function<void(juce::File)> onLoadIR;
  std::function<void(juce::File, int)> onLoadIRToSlot;

private:
  FreeIRAudioProcessor &proc;

  // Data
  juce::StringArray
      favoriteFolders; // Change to StringArray for easier persistence
  juce::StringArray
      playlistFiles; // Change to StringArray for easier persistence

  std::vector<juce::File>
      currentFileList;         // Files in currently selected folder OR playlist
  juce::File currentDirectory; // If valid, we are browsing a folder. If
                               // invalid, maybe playlist?
  bool isShowingPlaylist = false;

  // UI
  juce::Label placesLabel{{}, "PLACES"};
  juce::TextButton addFolderButton{"+"};
  juce::ListBox placesList; // We might need a separate model for this or just
                            // use buttons?
  // Let's use a simpler approach for Sidebar: A specialized component or just
  // layout logic. For now, let's use a ListBox for simplicity of scrolling if
  // many folders.

  class SidebarModel : public juce::ListBoxModel {
  public:
    IRBrowserComponent &owner;
    SidebarModel(IRBrowserComponent &o) : owner(o) {}
    int getNumRows() override;
    void paintListBoxItem(int row, juce::Graphics &g, int w, int h,
                          bool sel) override;
    void listBoxItemClicked(int row, const juce::MouseEvent &) override;
  };

  SidebarModel sidebarModel;
  juce::ListBox sidebarList;

  juce::Label irListLabel{{}, "IMPULSE RESPONSES"};
  juce::ListBox fileList; // This component handles the Main List visuals

  // Helpers
  // Helpers
  void scanDirectory(const juce::File &dir);
  void showPlaylist();
  void addToPlaylist(const juce::File &file);
  void refreshFavorites(); // Updates listbox

  void savePersistentState();
  void loadPersistentState();

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(IRBrowserComponent)
};
