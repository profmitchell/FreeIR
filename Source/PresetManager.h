#pragma once
#include <JuceHeader.h>

class PresetManager {
public:
  PresetManager();

  // Global Settings (Folders, Playlist, Last Preset)
  void saveGlobalSettings(const juce::StringArray &folders,
                          const juce::StringArray &playlist,
                          const juce::String &lastPreset);
  void loadGlobalSettings(juce::StringArray &folders,
                          juce::StringArray &playlist,
                          juce::String &lastPreset);

  // Presets
  // We allow saving the state as MemoryBlock (from APVTS::copyState().toXml())
  // or passing APVTS directly. Let's pass APVTS and extra data (loaded IRs).
  // Actually, APVTS state usually includes parameters.
  // IR paths are handled in setStateInformation in Processor.
  // So we just need to save the XML state of the processor which includes
  // parameters and IR paths.

  void savePreset(const juce::String &name, const juce::MemoryBlock &stateData);
  void loadPreset(const juce::String &name, juce::MemoryBlock &destData);
  void deletePreset(const juce::String &name);

  juce::StringArray getPresetList();
  juce::String getNextPreset(const juce::String &current);
  juce::String getPrevPreset(const juce::String &current);

  juce::File getPresetsFolder() const;

private:
  juce::File rootFolder;
  juce::File presetsFolder;
  juce::File settingsFile;

  void ensureFoldersExist();
};
