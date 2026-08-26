cmake_minimum_required(VERSION 3.22)

project(Sliceomat VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "" FORCE)
set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "" FORCE)

include(FetchContent)
FetchContent_Declare(
    juce
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.8
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(juce)

juce_add_plugin(Sliceomat
    COMPANY_NAME "Blindsmyth"
    COMPANY_WEBSITE "https://github.com/Blindsmyth/Sliceomat"
    BUNDLE_ID "com.blindsmyth.sliceomat"
    PLUGIN_MANUFACTURER_CODE Blnd
    PLUGIN_CODE Slic
    FORMATS AU VST3 Standalone
    PRODUCT_NAME "Slice-O-Mat"
    PLUGIN_NAME "Slice-O-Mat"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    AU_MAIN_TYPE kAudioUnitType_MusicEffect
    VST3_CATEGORIES Fx Filter
    AU_SANDBOX_SAFE TRUE
)

target_sources(Sliceomat
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginProcessor.h
        Source/PluginEditor.cpp
        Source/PluginEditor.h
        Source/Dsp.h
        Source/Filters.h
        Source/faust/DiodeLadder.cpp
        Source/faust/Korg35HPF.cpp
        Source/faust/Korg35LPF.cpp
        Source/faust/MoogHalfLadder.cpp
        Source/faust/MoogLadder.cpp
        Source/faust/Oberheim.cpp
)

target_include_directories(Sliceomat PRIVATE Source Source/faust)

set_source_files_properties(
    Source/faust/DiodeLadder.cpp
    Source/faust/Korg35HPF.cpp
    Source/faust/Korg35LPF.cpp
    Source/faust/MoogHalfLadder.cpp
    Source/faust/MoogLadder.cpp
    Source/faust/Oberheim.cpp
    PROPERTIES COMPILE_FLAGS "-w"
)

target_compile_definitions(Sliceomat
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(Sliceomat
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
