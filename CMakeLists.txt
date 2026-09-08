cmake_minimum_required(VERSION 3.20)
project(MidiVisualizerBassEngineStudio CXX)

if(NOT WIN32)
    message(FATAL_ERROR "This project targets Windows 11 x64 only (see CLAUDE.md).")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

enable_testing()

add_library(timeline STATIC
    src/timeline/MeasuresMap.h
    src/timeline/MeasuresMap.cpp
    src/timeline/ChordEvent.h
    src/timeline/Timeline.h
    src/timeline/Timeline.cpp
    src/timeline/Types.h
)
target_include_directories(timeline PUBLIC src)

add_library(ui STATIC
    src/app/PianoLayout.h
    src/app/PianoLayout.cpp
    src/app/KeyboardState.h
    src/app/KeyboardState.cpp
    src/app/TextRenderer.h
    src/app/TextRenderer.cpp
)
target_include_directories(ui PUBLIC src)
target_link_libraries(ui PUBLIC midi trainer)

add_executable(MidiVisualizerApp WIN32
    src/app/main.cpp
    src/app/Window.h
    src/app/Window.cpp
    src/app/RendererDX11.h
    src/app/RendererDX11.cpp
)
target_include_directories(MidiVisualizerApp PRIVATE src)
target_compile_definitions(MidiVisualizerApp PRIVATE UNICODE _UNICODE)
target_link_libraries(MidiVisualizerApp PRIVATE timeline midi trainer ui d3d11 dxgi d3dcompiler)

add_library(trainer STATIC
    src/trainer/RootlessTrainer.h
    src/trainer/RootlessTrainer.cpp
)
target_include_directories(trainer PUBLIC src)

add_library(bass STATIC
    src/bass/BassPatterns.h
    src/bass/BassPatterns.cpp
)
target_include_directories(bass PUBLIC src)
target_link_libraries(bass PUBLIC timeline)

add_library(midi STATIC
    src/midi/MidiTypes.h
    src/midi/MidiInput.h
    src/midi/MidiInput.cpp
    src/midi/MidiOutput.h
    src/midi/MidiOutput.cpp
    src/midi/MidiEventProcessor.h
    src/midi/MidiEventProcessor.cpp
)
target_include_directories(midi PUBLIC src)
target_link_libraries(midi PRIVATE winmm)

add_executable(timeline_tests
    tests/test_main.cpp
    tests/TestFramework.h
    tests/measures_map_tests.cpp
    tests/timeline_tests.cpp
    tests/midi_decode_tests.cpp
    tests/midi_event_processor_tests.cpp
    tests/rootless_trainer_tests.cpp
    tests/bass_patterns_tests.cpp
    tests/piano_layout_tests.cpp
    tests/keyboard_state_tests.cpp
    tests/text_renderer_tests.cpp
)
target_include_directories(timeline_tests PRIVATE src tests)
target_link_libraries(timeline_tests PRIVATE timeline trainer midi bass ui)

add_test(NAME timeline_tests COMMAND timeline_tests)

add_executable(midi_bringup tools/midi_bringup/main.cpp)
target_include_directories(midi_bringup PRIVATE src)
target_compile_definitions(midi_bringup PRIVATE UNICODE _UNICODE)
target_link_libraries(midi_bringup PRIVATE midi)
