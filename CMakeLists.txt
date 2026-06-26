cmake_minimum_required(VERSION 3.22)
project(GenSound VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

#-------------------------------------------------------------------------------
# JUCE
#-------------------------------------------------------------------------------
include(FetchContent)

FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.7
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(JUCE)

#-------------------------------------------------------------------------------
# Bundle the SDK header as a binary resource so it travels with the installed
# plugin and does not depend on the source tree being present at runtime.
#-------------------------------------------------------------------------------
juce_add_binary_data(GenSoundSDKData SOURCES sdk/GenSoundSDK.h)

#-------------------------------------------------------------------------------
# Plugin target
#-------------------------------------------------------------------------------
juce_add_plugin(GenSound
    COMPANY_NAME            "GenSound"
    PLUGIN_MANUFACTURER_CODE GnSn
    PLUGIN_CODE             GnS1
    FORMATS                 AU VST3
    PRODUCT_NAME            "GenSound"
    IS_SYNTH                TRUE
    NEEDS_MIDI_INPUT        TRUE
    NEEDS_MIDI_OUTPUT       FALSE
    IS_MIDI_EFFECT          FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES         "Instrument" "Synth"
    AU_MAIN_TYPE            kAudioUnitType_MusicDevice
)

#-------------------------------------------------------------------------------
# Sources
#-------------------------------------------------------------------------------
target_sources(GenSound PRIVATE
    source/PluginProcessor.cpp
    source/PluginProcessor.h

    source/audio/Voice.h
    source/audio/Voice.cpp
    source/audio/VoiceManager.h
    source/audio/VoiceManager.cpp

    source/compile/CompileDiagnostics.h
    source/compile/CompileDiagnostics.cpp
    source/compile/ExternalCompiler.h
    source/compile/ExternalCompiler.cpp
    source/compile/FileWatcher.h
    source/compile/FileWatcher.cpp
    source/compile/ScriptManager.h
    source/compile/ScriptManager.cpp

    source/loader/ScriptVersion.h
    source/loader/DynamicLibraryLoader.h
    source/loader/DynamicLibraryLoader.cpp

    source/scripting/ScriptingTypes.h

    source/ui/PluginEditor.h
    source/ui/PluginEditor.cpp
)

#-------------------------------------------------------------------------------
# Include paths
#-------------------------------------------------------------------------------
target_include_directories(GenSound PRIVATE
    source
    sdk
)

#-------------------------------------------------------------------------------
# Compile definitions
#-------------------------------------------------------------------------------
target_compile_definitions(GenSound PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_REPORT_APP_USAGE=0
)

#-------------------------------------------------------------------------------
# Link libraries
#-------------------------------------------------------------------------------
juce_generate_juce_header(GenSound)

target_link_libraries(GenSound PRIVATE
    GenSoundSDKData
    juce::juce_audio_utils
    juce::juce_audio_plugin_client
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_processors
    juce::juce_gui_basics
    juce::juce_gui_extra
    juce::juce_core
    juce::juce_data_structures
    juce::juce_events
)

if(APPLE)
    target_link_libraries(GenSound PRIVATE "-framework CoreFoundation")
endif()

#-------------------------------------------------------------------------------
# Compiler warnings
#-------------------------------------------------------------------------------
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang|GNU")
    target_compile_options(GenSound PRIVATE
        -Wall -Wextra -Wshadow -Wnon-virtual-dtor
        -Wno-unused-parameter
    )
endif()
