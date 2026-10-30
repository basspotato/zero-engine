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
        Source/SpectralCompressor.cpp
        Source/Arpeggiator.cpp
        Source/SoundPrint.cpp
        Source/ReferenceMatch.cpp
        Source/MatchNet.cpp
        Source/SoundTraits.cpp
        Source/TasteModel.cpp
        Source/GenerateNet.cpp
        Source/Moods.cpp
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
        ZeroEngineFonts
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# UI fonts (SIL Open Font License, see Resources/Fonts/OFL-*.txt), from Potato Kicker.
juce_add_binary_data(ZeroEngineFonts SOURCES
    Resources/Fonts/BarlowCondensed-SemiBold.ttf
    Resources/Fonts/BarlowCondensed-Bold.ttf
    Resources/Fonts/ShareTechMono-Regular.ttf)

# Space pictures for the Generate screen (the owner's photos, in git):
# whatever is in Resources/Space is built in; without them the screen draws its
# own planets.
file(GLOB ZERO_SPACE_IMAGES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/Resources/Space/*.jpg" "${CMAKE_CURRENT_SOURCE_DIR}/Resources/Space/*.png")
if (ZERO_SPACE_IMAGES)
    juce_add_binary_data(ZeroEngineSpace HEADER_NAME SpaceImages.h NAMESPACE SpaceImages SOURCES ${ZERO_SPACE_IMAGES})
endif()

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
        tools/analyze/MatchBench.cpp
        tools/analyze/MatchLibrary.cpp
        tools/analyze/MatchJudge.cpp
        tools/analyze/MatchNetTrain.cpp
        tools/analyze/GenerateNetTrain.cpp
        tools/analyze/Library.cpp
        tools/analyze/Tuner.cpp
        tools/analyze/Harmonics.cpp
        Source/PluginProcessor.cpp
        Source/FinishingChain.cpp
        Source/SpectralCompressor.cpp
        Source/Arpeggiator.cpp
        Source/SoundPrint.cpp
        Source/ReferenceMatch.cpp
        Source/MatchNet.cpp
        Source/SoundTraits.cpp
        Source/TasteModel.cpp
        Source/GenerateNet.cpp
        Source/Moods.cpp
        Source/Profiles.cpp
        Source/PluginEditor.cpp
        Source/SynthVoice.cpp
        Source/Wavetable.cpp)

target_compile_definitions(ZeroAnalyze
    PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JucePlugin_Name="Zero Engine"
        JUCE_MODAL_LOOPS_PERMITTED=1) # snapshot pumps the message loop so editor timers run

target_link_libraries(ZeroAnalyze
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
        ZeroEngineFonts
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)
# The tests and tools keep many engines (45 KB each) on the stack; inlined
# into one function they outgrow the default 1 MB.
if (MSVC)
    target_link_options(ZeroAnalyze PRIVATE /STACK:16777216)
endif()

if (ZERO_SPACE_IMAGES)
    foreach (target ZeroEngine ZeroAnalyze)
        target_link_libraries (${target} PRIVATE ZeroEngineSpace)
        target_compile_definitions (${target} PRIVATE ZERO_SPACE_IMAGES=1)
    endforeach()
endif()
