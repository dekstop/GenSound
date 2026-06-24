cmake_minimum_required(VERSION 3.22)
project(GenSynth VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

#-------------------------------------------------------------------------------
# JUCE — fetched automatically if not already available on CMAKE_PREFIX_PATH
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
# Plugin target
#-------------------------------------------------------------------------------
juce_add_plugin(GenSynth
    COMPANY_NAME            "GenSynth"
    PLUGIN_MANUFACTURER_CODE GnSy
    PLUGIN_CODE             GnS1
    FORMATS                 AU VST3
    PRODUCT_NAME            "GenSynth"
    IS_SYNTH                TRUE
    NEEDS_MIDI_INPUT        TRUE
    NEEDS_MIDI_OUTPUT       FALSE
    IS_MIDI_EFFECT          FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE   # set TRUE and configure paths when ready
    VST3_CATEGORIES         "Instrument" "Synth"
    AU_MAIN_TYPE            kAudioUnitType_MusicDevice
)

#-------------------------------------------------------------------------------
# Sources
#-------------------------------------------------------------------------------
target_sources(GenSynth PRIVATE
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
target_include_directories(GenSynth PRIVATE
    source
    sdk
)

#-------------------------------------------------------------------------------
# Compile definitions
#-------------------------------------------------------------------------------
target_compile_definitions(GenSynth PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_REPORT_APP_USAGE=0
)

#-------------------------------------------------------------------------------
# Link libraries
#-------------------------------------------------------------------------------
juce_generate_juce_header(GenSynth)

target_link_libraries(GenSynth PRIVATE
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

# dl is needed for dlopen / dlsym on macOS (linked automatically, but explicit
# is cleaner in case of unusual toolchain setups)
if(APPLE)
    target_link_libraries(GenSynth PRIVATE "-framework CoreFoundation")
endif()

#-------------------------------------------------------------------------------
# Compiler warnings
#-------------------------------------------------------------------------------
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang|GNU")
    target_compile_options(GenSynth PRIVATE
        -Wall -Wextra -Wshadow -Wnon-virtual-dtor
        -Wno-unused-parameter   # JUCE has a lot of these
    )
endif()

#-------------------------------------------------------------------------------
# Install SDK header alongside built plugin (optional convenience)
#-------------------------------------------------------------------------------
install(FILES sdk/GenSynthSDK.h
        DESTINATION "${CMAKE_INSTALL_PREFIX}/sdk")
