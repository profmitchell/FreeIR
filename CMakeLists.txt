cmake_minimum_required(VERSION 3.15)

project(FreeIR VERSION 1.0.0)

# Set the path to the JUCE installation
set(JUCE_ROOT "/Users/mitchellcohen/Downloads/JUCE")

# Add the JUCE module directory to the module path
list(APPEND CMAKE_MODULE_PATH "${JUCE_ROOT}/modules")

# Add JUCE as a subdirectory
add_subdirectory("${JUCE_ROOT}" "${CMAKE_CURRENT_BINARY_DIR}/JUCE" EXCLUDE_FROM_ALL)

# Create the plugin target
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

juce_add_plugin(FreeIR
    COMPANY_NAME "CohenConcepts"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE "CoCo"
    PLUGIN_CODE "FrIr"
    FORMATS AU VST3 Standalone
    PRODUCT_NAME "FreeIR"
)

juce_generate_juce_header(FreeIR)

# Add source files
target_sources(FreeIR
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginProcessor.h
        Source/PluginEditor.cpp
        Source/PluginEditor.h
        Source/IRSlot.cpp
        Source/IRSlot.h
        Source/EQProcessor.cpp
        Source/EQProcessor.h
        Source/AutoAligner.cpp
        Source/AutoAligner.h
        Source/Components/IRSlotComponent.cpp
        Source/Components/IRSlotComponent.h
        Source/Components/IRBrowserComponent.cpp
        Source/Components/IRBrowserComponent.h
        Source/Components/WaveformDisplay.cpp
        Source/Components/WaveformDisplay.h
        Source/Components/EQSectionComponent.cpp
        Source/Components/EQSectionComponent.h
        Source/FreeIRLookAndFeel.h
)

# Link against JUCE modules
target_link_libraries(FreeIR
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_gui_basics
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# Compile definitions
target_compile_definitions(FreeIR
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)
