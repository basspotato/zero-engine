cmake_minimum_required(VERSION 3.22)

project(ZeroEngine VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(ZeroEngine
    COMPANY_NAME "Zero Engine"
    PRODUCT_NAME "Zero Engine"
    PLUGIN_MANUFACTURER_CODE Zeng
    PLUGIN_CODE Zen1
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    FORMATS VST3 Standalone)

target_sources(ZeroEngine
    PRIVATE
        Source/PluginProcessor.cpp
        Source/FinishingChain.cpp
        Source/Profiles.cpp
        Source/PluginEditor.cpp
        Source/SynthVoice.cpp
        Source/Wavetable.cpp)

target_compile_definitions(ZeroEngine
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(ZeroEngine
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# Offline tool: measures reference sounds and engine renders (loudness,
# brightness, bass, width). Builds the engine sources directly so renders
# match the plugin exactly. Not shipped.
juce_add_console_app(ZeroAnalyze PRODUCT_NAME "ZeroAnalyze")

target_sources(ZeroAnalyze
    PRIVATE
        tools/analyze/Main.cpp
        tools/analyze/Descriptors.cpp
        tools/analyze/Freesound.cpp
        tools/analyze/Tests.cpp
        tools/analyze/Library.cpp
        tools/analyze/Tuner.cpp
        Source/PluginProcessor.cpp
        Source/FinishingChain.cpp
        Source/Profiles.cpp
        Source/PluginEditor.cpp
        Source/SynthVoice.cpp
        Source/Wavetable.cpp)

target_compile_definitions(ZeroAnalyze
    PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JucePlugin_Name="Zero Engine")

target_link_libraries(ZeroAnalyze
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)
