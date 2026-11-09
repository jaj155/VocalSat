# VocalSat - vocal-first analog saturation
# UI framework: WebView2 (Design/v2-ui-map.json is the layout contract)
# Formats: VST3 + Standalone (CLAP not opted in; see status.json -> formats.clap)

if(WIN32)
    set(PLUGIN_FORMATS VST3 Standalone)
elseif(APPLE)
    set(PLUGIN_FORMATS VST3 Standalone)
elseif(UNIX)
    set(PLUGIN_FORMATS VST3 Standalone)
else()
    message(FATAL_ERROR "Unsupported platform")
endif()

message(STATUS "VocalSat: Building for ${PLUGIN_FORMATS}")

# ============================================
# PLUGIN DEFINITION
# ============================================
juce_add_plugin(VocalSat
    COMPANY_NAME "Jaj"
    PLUGIN_MANUFACTURER_CODE JajC
    PLUGIN_CODE VcSt
    FORMATS ${PLUGIN_FORMATS}
    PRODUCT_NAME "VocalSat"
    NEEDS_WEBVIEW2 TRUE
    VST3_CATEGORIES Fx
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
)

# ============================================
# CLAP FORMAT (opt-in: status.json -> formats.clap = false)
# ============================================
option(VocalSat_ENABLE_CLAP "Build VocalSat in CLAP format" OFF)
if(VocalSat_ENABLE_CLAP AND COMMAND clap_juce_extensions_plugin)
    clap_juce_extensions_plugin(TARGET VocalSat
        CLAP_ID "com.noizefield.vocalsat"
        CLAP_FEATURES audio-effect)
endif()

# ============================================
# EMBEDDED WEB UI (webview-008: ONE index.html, everything inline)
# ============================================
juce_add_binary_data(VocalSat_WebUI
    SOURCES
        Source/ui/public/index.html
)

# ============================================
# SOURCE FILES
# ============================================
target_sources(VocalSat
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/PluginProcessor.h
        Source/PluginEditor.h
        Source/ParameterIDs.hpp
)

# ============================================
# INCLUDE PATHS
# ============================================
target_include_directories(VocalSat
    PRIVATE
        Source
)

# ============================================
# LINK LIBRARIES
# ============================================
target_link_libraries(VocalSat
    PRIVATE
        VocalSat_WebUI
        juce::juce_audio_basics
        juce::juce_audio_devices
        juce::juce_audio_formats
        juce::juce_audio_plugin_client
        juce::juce_audio_processors
        juce::juce_audio_utils
        juce::juce_core
        juce::juce_data_structures
        juce::juce_dsp
        juce::juce_events
        juce::juce_graphics
        juce::juce_gui_basics
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# ============================================
# COMPILE DEFINITIONS (WebView2 on Windows)
# ============================================
target_compile_definitions(VocalSat
    PUBLIC
        JUCE_WEB_BROWSER=1
        JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        _USE_MATH_DEFINES=1
)

if(APPLE)
    set_target_properties(VocalSat PROPERTIES
        XCODE_ATTRIBUTE_ARCHS "x86_64;arm64"
        XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH NO
    )
endif()
