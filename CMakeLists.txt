cmake_minimum_required(VERSION 3.20)
project(AudioJones LANGUAGES C CXX)

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)

# Fetch raylib
FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG 5.5
)
FetchContent_MakeAvailable(raylib)

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

# Main executable
add_executable(AudioJones
    src/main.c
    src/audio.c
    src/waveform.c
    src/visualizer.c
    src/preset.cpp
)
target_link_libraries(AudioJones PRIVATE raylib nlohmann_json::nlohmann_json)

# Header-only libs
target_include_directories(AudioJones PRIVATE
    ${CMAKE_SOURCE_DIR}/vendor
    ${raygui_SOURCE_DIR}/src
)
