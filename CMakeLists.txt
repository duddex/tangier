cmake_minimum_required(VERSION 3.22)

project(Tangier VERSION 0.1.1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# JUCE is fetched at configure time. Bump GIT_TAG to update the framework.
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        9.0.5
    GIT_SHALLOW    ON)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(Tangier
    COMPANY_NAME "duddex"
    COMPANY_WEBSITE "https://github.com/duddex"
    PLUGIN_MANUFACTURER_CODE Ddex
    PLUGIN_CODE Tngr
    FORMATS VST3 Standalone
    PRODUCT_NAME "Tangier"
    DESCRIPTION "Polyphonic virtual analogue synth voice, based on Valley Interzone"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES Instrument Synth
    NEEDS_WEB_BROWSER FALSE
    NEEDS_CURL FALSE)

target_sources(Tangier
    PRIVATE
        src/Parameters.cpp
        src/engine/SynthEngine.cpp
        src/PluginProcessor.cpp
        src/PluginEditor.cpp)

target_include_directories(Tangier
    PRIVATE
        src)

target_compile_definitions(Tangier
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(Tangier
    PRIVATE
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

enable_testing()

add_executable(TangierDspTests tests/DspTests.cpp src/engine/SynthEngine.cpp)
target_include_directories(TangierDspTests PRIVATE src)
add_test(NAME TangierDspTests COMMAND TangierDspTests)
