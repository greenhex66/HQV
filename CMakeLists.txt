cmake_minimum_required(VERSION 3.22)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(HexQVanta VERSION 1.0.0)

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(HexQVanta
    COMPANY_NAME "HEX-Q"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    PLUGIN_MANUFACTURER_CODE Hexq
    PLUGIN_CODE Vnta
    FORMATS VST3
    PRODUCT_NAME "HEX-Q VANTA"
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES Fx EQ)

target_sources(HexQVanta PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    Source/Parameters/VantaParams.cpp
    Source/Parameters/VantaParams.h
    Source/DSP/Biquad.h
    Source/DSP/EqEngine.h
    Source/HexQ/HexQTheme.h
    Source/HexQ/HexQLookAndFeel.cpp
    Source/HexQ/HexQLookAndFeel.h
    Source/HexQ/HexQHeader.h
    Source/HexQ/HexQFooter.h
    Source/Vanta/ResponseGraph.cpp
    Source/Vanta/ResponseGraph.h
    Source/Vanta/BandButton.h)

target_compile_definitions(HexQVanta PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(HexQVanta
    PRIVATE juce::juce_audio_utils
    PUBLIC juce::juce_recommended_config_flags)

# Offline DSP self-test (pure C++, no JUCE): run with `ctest -C Release`
enable_testing()
add_executable(HexQDspTest tests/dsp_test.cpp)
target_include_directories(HexQDspTest PRIVATE Source)
add_test(NAME dsp_self_test COMMAND HexQDspTest)
