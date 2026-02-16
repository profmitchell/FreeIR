#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
FreeIRAudioProcessor::FreeIRAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor(
          BusesProperties()
              .withInput("Input", juce::AudioChannelSet::stereo(), true)
              .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
#endif
      apvts(*this, nullptr, "PARAMETERS", createParameterLayout()),
      eqProcessor(apvts), autoAligner(slots) {
  for (int i = 0; i < numSlots; ++i)
    slots[i].init(i, &apvts);
}

FreeIRAudioProcessor::~FreeIRAudioProcessor() {}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout
FreeIRAudioProcessor::createParameterLayout() {
  juce::AudioProcessorValueTreeState::ParameterLayout layout;

  for (int i = 1; i <= numSlots; ++i) {
    auto prefix = "Slot" + juce::String(i) + "_";

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID(prefix + "Mute", 1), prefix + "Mute", false));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID(prefix + "Solo", 1), prefix + "Solo", false));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(prefix + "Level", 1), prefix + "Level",
        juce::NormalisableRange<float>(-60.0f, 6.0f, 0.1f), -6.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(prefix + "Pan", 1), prefix + "Pan",
        juce::NormalisableRange<float>(-100.0f, 100.0f, 1.0f), 0.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(prefix + "DelayMs", 1), prefix + "Delay",
        juce::NormalisableRange<float>(0.0f, 10.0f, 0.01f), 0.0f));
  }

  // EQ Parameters
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID("LoCutHz", 1), "Lo Cut",
      juce::NormalisableRange<float>(20.0f, 500.0f, 1.0f, 0.5f), 80.0f));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID("HiCutHz", 1), "Hi Cut",
      juce::NormalisableRange<float>(2000.0f, 20000.0f, 1.0f, 0.5f), 12000.0f));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID("BassGainDb", 1), "Bass",
      juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f), 0.0f));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID("MidGainDb", 1), "Mid Gain",
      juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f), 0.0f));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID("MidFreqHz", 1), "Mid Freq",
      juce::NormalisableRange<float>(200.0f, 6000.0f, 1.0f, 0.5f), 1000.0f));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID("MidQ", 1), "Mid Q",
      juce::NormalisableRange<float>(0.3f, 10.0f, 0.01f, 0.5f), 1.0f));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID("TrebleGainDb", 1), "Treble",
      juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f), 0.0f));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID("AirGainDb", 1), "Air",
      juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f), 0.0f));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID("OutputGainDb", 1), "Output",
      juce::NormalisableRange<float>(-24.0f, 6.0f, 0.1f), 0.0f));

  return layout;
}

//==============================================================================
void FreeIRAudioProcessor::prepareToPlay(double sampleRate,
                                         int samplesPerBlock) {
  currentSampleRate = sampleRate;
  currentBlockSize = samplesPerBlock;

  juce::dsp::ProcessSpec spec;
  spec.sampleRate = sampleRate;
  spec.maximumBlockSize = (juce::uint32)samplesPerBlock;
  spec.numChannels = 2;

  for (auto &slot : slots)
    slot.prepare(spec);

  eqProcessor.prepare(spec);

  mixBuffer.setSize(2, samplesPerBlock);
}

void FreeIRAudioProcessor::releaseResources() {
  for (auto &slot : slots)
    slot.reset();
  eqProcessor.reset();
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool FreeIRAudioProcessor::isBusesLayoutSupported(
    const BusesLayout &layouts) const {
  if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
    return false;

  if (layouts.getMainInputChannelSet() != juce::AudioChannelSet::mono() &&
      layouts.getMainInputChannelSet() != juce::AudioChannelSet::stereo())
    return false;

  return true;
}
#endif

void FreeIRAudioProcessor::processBlock(juce::AudioBuffer<float> &buffer,
                                        juce::MidiBuffer &) {
  juce::ScopedNoDenormals noDenormals;
  int totalNumInputChannels = getTotalNumInputChannels();
  int totalNumOutputChannels = getTotalNumOutputChannels();
  int numSamples = buffer.getNumSamples();

  // Clear unused output channels
  for (int i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
    buffer.clear(i, 0, numSamples);

  // Check if any slot is soloed
  bool anySoloed = false;
  for (int i = 0; i < numSlots; ++i) {
    if (slots[i].isLoaded() && slots[i].isSoloed()) {
      anySoloed = true;
      break;
    }
  }

  // Clear mix bus
  mixBuffer.setSize(2, numSamples, false, false, true);
  mixBuffer.clear();

  // Process each slot
  for (int i = 0; i < numSlots; ++i) {
    if (!slots[i].isLoaded())
      continue;

    // Solo logic: if any slot is soloed, skip non-soloed slots
    if (anySoloed && !slots[i].isSoloed())
      continue;

    slots[i].process(buffer, mixBuffer);
  }

  // Copy mix result back to main buffer (ensure stereo)
  buffer.setSize(2, numSamples, true, false, true);
  for (int ch = 0; ch < 2; ++ch)
    buffer.copyFrom(ch, 0, mixBuffer, ch, 0, numSamples);

  // Apply EQ chain
  eqProcessor.process(buffer);

  // Apply output gain
  float outGainDb = apvts.getRawParameterValue("OutputGainDb")->load();
  float outGain = juce::Decibels::decibelsToGain(outGainDb, -60.0f);
  buffer.applyGain(outGain);
}

bool FreeIRAudioProcessor::exportMixedIR(const juce::File &outputFile) {
  // Renders a single impulse through all loaded/enabled slots with current
  // settings (alignment, level, pan, EQ) and writes the result as a WAV file.

  double sampleRate = exportSampleRate;
  // Increase length to ~2 seconds to capture full tail and delay
  int lengthSamples = (int)(sampleRate * 2.0);

  int blockSize = 512;
  juce::dsp::ProcessSpec renderSpec;
  renderSpec.sampleRate = sampleRate;
  renderSpec.maximumBlockSize = (juce::uint32)blockSize;
  renderSpec.numChannels = 2; // Internal processing is stereo

  // Prepare input: Dirac delta
  juce::AudioBuffer<float> impulseInput(2, lengthSamples);
  impulseInput.clear();
  impulseInput.setSample(0, 0, 1.0f);
  impulseInput.setSample(1, 0, 1.0f);

  // Buffer for the mixed result
  juce::AudioBuffer<float> exportMix(2, lengthSamples);
  exportMix.clear();

  // Create temporary copies of slots for offline rendering
  // We need to re-create the whole chain to process offline safely
  bool anySoloed = false;
  for (int i = 0; i < numSlots; ++i) {
    if (slots[i].isLoaded() && slots[i].isSoloed()) {
      anySoloed = true;
      break;
    }
  }

  // Prepare temp slots
  std::array<IRSlot, numSlots> exportSlots;
  for (int i = 0; i < numSlots; ++i) {
    exportSlots[i].init(i, &apvts);
    exportSlots[i].prepare(renderSpec);
    if (slots[i].isLoaded()) {
      exportSlots[i].loadImpulseResponse(slots[i].getCurrentFile());
      exportSlots[i].setAlignmentDelay(slots[i].getAlignmentDelay());
    }
  }

  // Temp EQ
  EQProcessor exportEQ(apvts);
  exportEQ.prepare(renderSpec);

  // Process in blocks
  int totalBlocks = (lengthSamples + blockSize - 1) / blockSize;

  for (int block = 0; block < totalBlocks; ++block) {
    int startSample = block * blockSize;
    int samplesThisBlock = juce::jmin(blockSize, lengthSamples - startSample);

    // Input block (impulse only in first block)
    juce::AudioBuffer<float> inputBlock(2, samplesThisBlock);
    inputBlock.clear();

    if (block == 0) {
      inputBlock.setSample(0, 0, 1.0f);
      inputBlock.setSample(1, 0, 1.0f);
    }

    // Mix bus for this block
    juce::AudioBuffer<float> blockMix(2, samplesThisBlock);
    blockMix.clear();

    // Process slots
    for (int i = 0; i < numSlots; ++i) {
      if (!exportSlots[i].isLoaded())
        continue;
      if (anySoloed && !exportSlots[i].isSoloed())
        continue;
      if (!anySoloed && exportSlots[i].isMuted())
        continue;

      exportSlots[i].process(inputBlock, blockMix);
    }

    // Apply EQ
    exportEQ.process(blockMix);

    // Apply output gain
    float outGainDb = *apvts.getRawParameterValue("OutputGainDb");
    float outGain = juce::Decibels::decibelsToGain(outGainDb);
    blockMix.applyGain(outGain);

    // Copy to export buffer
    for (int ch = 0; ch < 2; ++ch)
      exportMix.copyFrom(ch, startSample, blockMix, ch, 0, samplesThisBlock);
  }

  // Apply Mono if requested
  int numOutputChannels = exportMono ? 1 : 2;
  if (exportMono) {
    // Sum Stereo to Mono (L+R)/2
    exportMix.addFrom(0, 0, exportMix, 1, 0, lengthSamples);
    exportMix.applyGain(0, 0, lengthSamples, 0.5f);
  }

  // Write to WAV
  outputFile.deleteFile();
  std::unique_ptr<juce::FileOutputStream> fileStream(
      outputFile.createOutputStream());
  if (fileStream == nullptr)
    return false;

  juce::WavAudioFormat wavFormat;
  std::unique_ptr<juce::AudioFormatWriter> writer(wavFormat.createWriterFor(
      fileStream.get(), sampleRate, numOutputChannels, 24, {}, 0));

  if (writer == nullptr)
    return false;

  fileStream.release(); // writer takes ownership
  writer->writeFromAudioSampleBuffer(exportMix, 0, lengthSamples);

  return true;
}

//==============================================================================
juce::AudioProcessorEditor *FreeIRAudioProcessor::createEditor() {
  return new FreeIREditor(*this);
}

bool FreeIRAudioProcessor::hasEditor() const { return true; }
const juce::String FreeIRAudioProcessor::getName() const {
  return JucePlugin_Name;
}
bool FreeIRAudioProcessor::acceptsMidi() const { return false; }
bool FreeIRAudioProcessor::producesMidi() const { return false; }
bool FreeIRAudioProcessor::isMidiEffect() const { return false; }
double FreeIRAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int FreeIRAudioProcessor::getNumPrograms() { return 1; }
int FreeIRAudioProcessor::getCurrentProgram() { return 0; }
void FreeIRAudioProcessor::setCurrentProgram(int) {}
const juce::String FreeIRAudioProcessor::getProgramName(int) { return {}; }
void FreeIRAudioProcessor::changeProgramName(int, const juce::String &) {}

//==============================================================================
void FreeIRAudioProcessor::getStateInformation(juce::MemoryBlock &destData) {
  auto state = apvts.copyState();

  // Store IR file paths in the state tree
  for (int i = 0; i < numSlots; ++i) {
    auto path = slots[i].getCurrentFile().getFullPathName();
    state.setProperty("irFilePath" + juce::String(i), path, nullptr);
    state.setProperty("irAlignDelay" + juce::String(i),
                      slots[i].getAlignmentDelay(), nullptr);
  }

  std::unique_ptr<juce::XmlElement> xml(state.createXml());
  copyXmlToBinary(*xml, destData);
}

void FreeIRAudioProcessor::setStateInformation(const void *data,
                                               int sizeInBytes) {
  std::unique_ptr<juce::XmlElement> xmlState(
      getXmlFromBinary(data, sizeInBytes));

  if (xmlState != nullptr) {
    if (xmlState->hasTagName(apvts.state.getType())) {
      auto state = juce::ValueTree::fromXml(*xmlState);
      apvts.replaceState(state);

      // Restore IR file paths
      for (int i = 0; i < numSlots; ++i) {
        auto path =
            state.getProperty("irFilePath" + juce::String(i), "").toString();
        if (path.isNotEmpty()) {
          juce::File file(path);
          if (file.existsAsFile())
            slots[i].loadImpulseResponse(file);
        }
        auto delay =
            (double)state.getProperty("irAlignDelay" + juce::String(i), 0.0);
        slots[i].setAlignmentDelay(delay);
      }
    }
  }
}

//==============================================================================
juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter() {
  return new FreeIRAudioProcessor();
}
//==============================================================================
void FreeIRAudioProcessor::cacheManualDelays() {
  for (int i = 0; i < 4; ++i) {
    auto *param = apvts.getParameter("Slot" + juce::String(i + 1) + "_DelayMs");
    if (param)
      slots[i].manualDelayMs = param->convertFrom0to1(param->getValue());
  }
}

void FreeIRAudioProcessor::applyAlignmentResults() {
  auto results = autoAligner.results;
  for (int i = 0; i < 4; ++i) {
    auto *param = apvts.getParameter("Slot" + juce::String(i + 1) + "_DelayMs");
    if (param) {
      if (auto *p = dynamic_cast<juce::AudioParameterFloat *>(param)) {
        p->setValueNotifyingHost(p->convertTo0to1((float)results[i]));
      }
    }
  }
}

void FreeIRAudioProcessor::revertAutoAlignment() {
  for (int i = 0; i < 4; ++i) {
    auto *param = apvts.getParameter("Slot" + juce::String(i + 1) + "_DelayMs");
    if (param) {
      if (auto *p = dynamic_cast<juce::AudioParameterFloat *>(param)) {
        p->setValueNotifyingHost(
            p->convertTo0to1((float)slots[i].manualDelayMs));
      }
    }
  }
}
