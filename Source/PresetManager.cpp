#include "PresetManager.h"

PresetManager::PresetManager() {
  auto appData =
      juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory);
  rootFolder = appData.getChildFile("CohenConcepts").getChildFile("FreeIR");
  presetsFolder = rootFolder.getChildFile("Presets");
  settingsFile = rootFolder.getChildFile("settings.xml");

  ensureFoldersExist();
}

void PresetManager::ensureFoldersExist() {
  if (!rootFolder.exists())
    rootFolder.createDirectory();
  if (!presetsFolder.exists())
    presetsFolder.createDirectory();
}

void PresetManager::saveGlobalSettings(const juce::StringArray &folders,
                                       const juce::StringArray &playlist,
                                       const juce::String &lastPreset) {
  juce::XmlElement root("FreeIRSettings");

  // Favorite Folders
  auto *foldersXml = new juce::XmlElement("FavoriteFolders");
  for (const auto &f : folders) {
    auto *e = new juce::XmlElement("Folder");
    e->setAttribute("path", f);
    foldersXml->addChildElement(e);
  }
  root.addChildElement(foldersXml);

  // Playlist
  auto *playlistXml = new juce::XmlElement("Playlist");
  for (const auto &f : playlist) {
    auto *e = new juce::XmlElement("Item");
    e->setAttribute("path", f);
    playlistXml->addChildElement(e);
  }
  root.addChildElement(playlistXml);

  // Last Preset
  root.setAttribute("lastPreset", lastPreset);

  root.writeTo(settingsFile);
}

void PresetManager::loadGlobalSettings(juce::StringArray &folders,
                                       juce::StringArray &playlist,
                                       juce::String &lastPreset) {
  if (!settingsFile.existsAsFile())
    return;

  auto root = juce::XmlDocument::parse(settingsFile);
  if (!root || !root->hasTagName("FreeIRSettings"))
    return;

  // Folders
  folders.clear();
  if (auto *foldersXml = root->getChildByName("FavoriteFolders")) {
    for (auto *e : foldersXml->getChildIterator()) {
      folders.add(e->getStringAttribute("path"));
    }
  }

  // Playlist
  playlist.clear();
  if (auto *playlistXml = root->getChildByName("Playlist")) {
    for (auto *e : playlistXml->getChildIterator()) {
      playlist.add(e->getStringAttribute("path"));
    }
  }

  // Last Preset
  lastPreset = root->getStringAttribute("lastPreset");
}

void PresetManager::savePreset(const juce::String &name,
                               const juce::MemoryBlock &stateData) {
  ensureFoldersExist();
  auto file = presetsFolder.getChildFile(
      name + ".freeirpreset"); // custom extension or .xml
  // We can just save it as XML if we convert first, but MemoryBlock is binary.
  // Let's save as binary or wrap in XML. Wrapping in XML is safer for
  // versioning. Actually, standard setStateInformation uses binary or XML.
  // Let's just write the memory block to file.

  file.replaceWithData(stateData.getData(), stateData.getSize());
}

void PresetManager::loadPreset(const juce::String &name,
                               juce::MemoryBlock &destData) {
  auto file = presetsFolder.getChildFile(name + ".freeirpreset");
  if (file.existsAsFile()) {
    file.loadFileAsData(destData);
  }
}

void PresetManager::deletePreset(const juce::String &name) {
  auto file = presetsFolder.getChildFile(name + ".freeirpreset");
  if (file.existsAsFile())
    file.deleteFile();
}

juce::StringArray PresetManager::getPresetList() {
  ensureFoldersExist();
  juce::StringArray results;
  auto files = presetsFolder.findChildFiles(juce::File::findFiles, false,
                                            "*.freeirpreset");
  for (const auto &f : files) {
    results.add(f.getFileNameWithoutExtension());
  }
  results.sort(true);
  return results;
}

juce::String PresetManager::getNextPreset(const juce::String &current) {
  auto list = getPresetList();
  if (list.isEmpty())
    return {};

  int idx = list.indexOf(current);
  if (idx == -1)
    return list[0]; // if unknown, go to first

  return list[(idx + 1) % list.size()];
}

juce::String PresetManager::getPrevPreset(const juce::String &current) {
  auto list = getPresetList();
  if (list.isEmpty())
    return {};

  int idx = list.indexOf(current);
  if (idx == -1)
    return list[list.size() - 1]; // if unknown, go to last

  int prev = idx - 1;
  if (prev < 0)
    prev = list.size() - 1;
  return list[prev];
}

juce::File PresetManager::getPresetsFolder() const { return presetsFolder; }
