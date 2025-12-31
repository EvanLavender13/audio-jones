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

# Fetch Dear ImGui (docking branch for dockable windows)
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG docking
    EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(imgui)

# Fetch rlImGui (Dear ImGui + raylib integration)
FetchContent_Declare(
    rlimgui
    GIT_REPOSITORY https://github.com/raylib-extras/rlImGui.git
    GIT_TAG main
    EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(rlimgui)

# Build rlImGui as a static library (no native CMake support)
add_library(rlImGui STATIC
    ${rlimgui_SOURCE_DIR}/rlImGui.cpp
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
)
target_include_directories(rlImGui PUBLIC
    ${imgui_SOURCE_DIR}
    ${rlimgui_SOURCE_DIR}
)
target_link_libraries(rlImGui PUBLIC raylib)
target_compile_options(rlImGui PRIVATE -w)

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
    src/automation/mod_sources.cpp
    src/automation/modulation_engine.cpp
    src/automation/param_registry.cpp
    src/automation/drawable_params.cpp
)

set(RENDER_SOURCES
    src/render/waveform.cpp
    src/render/drawable.cpp
    src/render/spectrum_bars.cpp
    src/render/shape.cpp
    src/render/post_effect.cpp
    src/render/render_pipeline.cpp
    src/render/blend_compositor.cpp
    src/render/gradient.cpp
    src/render/render_utils.cpp
    src/render/draw_utils.cpp
    src/render/color_config.cpp
)

set(CONFIG_SOURCES
    src/config/preset.cpp
    src/config/modulation_config.cpp
)

set(SIMULATION_SOURCES
    src/simulation/shader_utils.cpp
    src/simulation/trail_map.cpp
    src/simulation/physarum.cpp
    src/simulation/curl_flow.cpp
)

set(IMGUI_UI_SOURCES
    src/ui/imgui_panels.cpp
    src/ui/imgui_widgets.cpp
    src/ui/imgui_effects.cpp
    src/ui/imgui_drawables.cpp
    src/ui/drawable_type_controls.cpp
    src/ui/imgui_audio.cpp
    src/ui/imgui_analysis.cpp
    src/ui/imgui_presets.cpp
    src/ui/imgui_lfo.cpp
    src/ui/gradient_editor.cpp
    src/ui/modulatable_slider.cpp
    src/ui/modulatable_drawable_slider.cpp
)

# Main executable
add_executable(AudioJones
    src/main.cpp
    ${AUDIO_SOURCES}
    ${ANALYSIS_SOURCES}
    ${AUTOMATION_SOURCES}
    ${RENDER_SOURCES}
    ${SIMULATION_SOURCES}
    ${CONFIG_SOURCES}
    ${IMGUI_UI_SOURCES}
)
target_link_libraries(AudioJones PRIVATE raylib rlImGui nlohmann_json::nlohmann_json kissfft)

# Include src/ for domain-based includes (e.g., #include "audio/audio.h")
target_include_directories(AudioJones PRIVATE ${CMAKE_SOURCE_DIR}/src)

# Header-only libs (SYSTEM suppresses warnings from external headers)
target_include_directories(AudioJones SYSTEM PRIVATE
    ${miniaudio_SOURCE_DIR}
)
