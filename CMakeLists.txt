cmake_minimum_required(VERSION 3.20)
project(VST_Synth_V1 VERSION 1.0.0)

# Fetch JUCE from GitHub
include(FetchContent)
FetchContent_Declare(
    juce
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.0
)
FetchContent_MakeAvailable(juce)

# Declare the JUCE plugin target
juce_add_plugin(VSTSynthV1
    COMPANY_NAME "Maayan Biller"
    BUNDLE_ID "com.maayan.vstsynthv1"
    PLUGIN_MANUFACTURER_CODE "Maay"
    PLUGIN_CODE "Vs02"
    FORMATS VST3 Standalone
    PRODUCT_NAME "VST Synth V1"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
)

# Add source files
target_sources(VSTSynthV1
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginProcessor.h
        Source/DSP/DAHDSREnvelope.cpp
        Source/DSP/DAHDSREnvelope.h
        Source/DSP/SynthVoice.cpp
        Source/DSP/SynthVoice.h
        Source/UI/PluginEditor.cpp
        Source/UI/PluginEditor.h
)

# Set compile definitions
target_compile_definitions(VSTSynthV1
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

# Link against JUCE modules
target_link_libraries(VSTSynthV1
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_dsp
        juce::juce_gui_basics
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# Generate JUCE header
juce_generate_juce_header(VSTSynthV1)


# Test Target
juce_add_console_app(SynthTest)
target_sources(SynthTest
    PRIVATE
        Tests/GuiTest.cpp
        Source/PluginProcessor.cpp
        Source/DSP/DAHDSREnvelope.cpp
        Source/DSP/SynthVoice.cpp
        Source/UI/PluginEditor.cpp
)
target_link_libraries(SynthTest
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_dsp
        juce::juce_gui_basics
        juce::juce_gui_extra
)
target_compile_definitions(SynthTest
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
)
juce_generate_juce_header(SynthTest)
