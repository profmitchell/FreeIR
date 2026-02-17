#pragma once
#include "../PluginProcessor.h"
#include <JuceHeader.h>

class PresetBrowserComponent : public juce::Component {
public:
  PresetBrowserComponent(FreeIRAudioProcessor &p);
  ~PresetBrowserComponent() override;

  void paint(juce::Graphics &g) override;
  void resized() override;

  void updateDisplay();

private:
  FreeIRAudioProcessor &proc;

  juce::TextButton prevButton{"<"};
  juce::TextButton nextButton{">"};
  juce::Label presetNameLabel;
  juce::TextButton saveButton{"Save"};
  juce::TextButton menuButton{"..."};

  void loadNext();
  void loadPrev();
  void showMenu();
  void showSaveDialog();
  void savePreset(const juce::String &name);
  void loadPreset(const juce::String &name);

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetBrowserComponent)
};
