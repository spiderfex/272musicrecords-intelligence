cmake_minimum_required(VERSION 3.22)

project(272MusicRecordsIntelligence VERSION 0.1.0 LANGUAGES CXX)

# JUCE is fetched at a pinned release tag. No framework source is vendored.
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 9.0.5
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(272MusicRecordsIntelligence
    COMPANY_NAME "272MusicRecords"
    BUNDLE_ID "com.272musicrecords.intelligence"
    PLUGIN_MANUFACTURER_CODE MRec
    PLUGIN_CODE MInt
    FORMATS VST3
    PRODUCT_NAME "272MusicRecords INTELLIGENCE"
    DESCRIPTION "Vocal and beat interaction analysis"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
)

target_sources(272MusicRecordsIntelligence
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginProcessor.h
        Source/PluginEditor.cpp
        Source/PluginEditor.h
)

target_compile_features(272MusicRecordsIntelligence PRIVATE cxx_std_17)

target_compile_definitions(272MusicRecordsIntelligence
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(272MusicRecordsIntelligence
    PRIVATE
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# The host sidechain is declared as an optional second input bus in the processor.
