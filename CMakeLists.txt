cmake_minimum_required(VERSION 3.20)
project(AudioJones LANGUAGES C)

set(CMAKE_C_STANDARD 17)
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

# Main executable
add_executable(AudioJones
    src/main.c
    src/audio.c
)
target_link_libraries(AudioJones PRIVATE raylib)

# Header-only libs
target_include_directories(AudioJones PRIVATE
    ${CMAKE_SOURCE_DIR}/vendor
    ${raygui_SOURCE_DIR}/src
)

# Copy shaders to build directory
file(GLOB SHADER_FILES ${CMAKE_SOURCE_DIR}/shaders/*.fs)
foreach(SHADER ${SHADER_FILES})
    get_filename_component(SHADER_NAME ${SHADER} NAME)
    configure_file(${SHADER} ${CMAKE_BINARY_DIR}/shaders/${SHADER_NAME} COPYONLY)
endforeach()
