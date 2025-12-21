cmake_minimum_required(VERSION 3.20)
project(AudioJones LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)

# Fetch raylib with OpenGL 4.3 for compute shader support
set(OPENGL_VERSION "4.3" CACHE STRING "OpenGL version for raylib")
FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG 5.5
)
FetchContent_MakeAvailable(raylib)
target_compile_options(raylib PRIVATE -w)

# Fetch raygui (header-only)
FetchContent_Declare(
    raygui
    GIT_REPOSITORY https://github.com/raysan5/raygui.git
    GIT_TAG 4.0
)
FetchContent_MakeAvailable(raygui)

# Fetch nlohmann/json for preset serialization
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(nlohmann_json)

# Fetch miniaudio (header-only)
FetchContent_Declare(
    miniaudio
    GIT_REPOSITORY https://github.com/mackron/miniaudio.git
    GIT_TAG 0.11.21
)
FetchContent_MakeAvailable(miniaudio)

# Fetch kiss_fft for beat detection
FetchContent_Declare(
    kissfft
    GIT_REPOSITORY https://github.com/mborgerding/kissfft.git
    GIT_TAG 131.1.0
)
set(KISSFFT_PKGCONFIG OFF CACHE BOOL "" FORCE)
set(KISSFFT_STATIC ON CACHE BOOL "" FORCE)
set(KISSFFT_TEST OFF CACHE BOOL "" FORCE)
set(KISSFFT_TOOLS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(kissfft)

# Source file groups by domain
set(AUDIO_SOURCES
    src/audio/audio.cpp
)

set(ANALYSIS_SOURCES
    src/analysis/fft.cpp
    src/analysis/beat.cpp
    src/analysis/bands.cpp
    src/analysis/analysis_pipeline.cpp
)

set(AUTOMATION_SOURCES
    src/automation/lfo.cpp
)

set(RENDER_SOURCES
    src/render/waveform.cpp
    src/render/waveform_pipeline.cpp
    src/render/spectrum_bars.cpp
    src/render/post_effect.cpp
    src/render/physarum.cpp
)

set(CONFIG_SOURCES
    src/config/preset.cpp
)

set(UI_SOURCES
    src/ui/ui_main.cpp
    src/ui/ui_layout.cpp
    src/ui/ui_widgets.cpp
    src/ui/ui_common.cpp
    src/ui/ui_color.cpp
    src/ui/ui_window.cpp
    src/ui/ui_panel_preset.cpp
    src/ui/ui_panel_effects.cpp
    src/ui/ui_panel_audio.cpp
    src/ui/ui_panel_spectrum.cpp
    src/ui/ui_panel_waveform.cpp
    src/ui/ui_panel_analysis.cpp
)

# Main executable
add_executable(AudioJones
    src/main.cpp
    ${AUDIO_SOURCES}
    ${ANALYSIS_SOURCES}
    ${AUTOMATION_SOURCES}
    ${RENDER_SOURCES}
    ${CONFIG_SOURCES}
    ${UI_SOURCES}
)
target_link_libraries(AudioJones PRIVATE raylib nlohmann_json::nlohmann_json kissfft)

# Include src/ for domain-based includes (e.g., #include "audio/audio.h")
target_include_directories(AudioJones PRIVATE ${CMAKE_SOURCE_DIR}/src)

# Header-only libs (SYSTEM suppresses warnings from external headers)
target_include_directories(AudioJones SYSTEM PRIVATE
    ${raygui_SOURCE_DIR}/src
    ${miniaudio_SOURCE_DIR}
)
