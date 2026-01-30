# Technology Stack

> Last sync: 2026-01-29 | Commit: 176b35f

## Languages

**Primary:**
- C++20 - Application logic, all `src/` modules

**Secondary:**
- GLSL 4.3 - Fragment shaders (`shaders/*.fs`), compute shaders (`shaders/*.glsl`)

## Build System

**Build Tool:**
- CMake 3.20+
- Config: `CMakeLists.txt`

**Package Manager:**
- CMake FetchContent (fetches dependencies at configure time)
- Manifest: `CMakeLists.txt` (FetchContent declarations)
- Lockfile: Missing (pinned via GIT_TAG)

## Frameworks

**Core:**
- raylib 5.5 - Window management, OpenGL rendering, input handling
- Dear ImGui (docking branch) - Parameter UI, dockable panels

**Build/Dev:**
- Ninja (recommended) - Fast parallel builds
- rlImGui - Dear ImGui + raylib integration layer

## Key Dependencies

**Critical:**
- miniaudio 0.11.21 - WASAPI loopback audio capture
- kiss_fft 131.1.0 - FFT for beat detection and spectral analysis
- nlohmann/json v3.11.3 - Preset serialization

**Infrastructure:**
- OpenGL 4.3 - Compute shader support for GPU simulations

## Configuration

**Build:**
- `CMakeLists.txt` - FetchContent declarations, source file groups
- `CMAKE_EXPORT_COMPILE_COMMANDS=ON` - Generates `compile_commands.json` for tooling

**Tooling:**
- `.clang-format` - LLVM style code formatting
- `.clang-tidy` - Static analysis (bugprone, clang-analyzer, performance, readability checks)

## Platform Requirements

**Development:**
- Windows primary, WSL2 development supported
- CMake 3.20+, Ninja build system
- C++20-compatible compiler (MSVC, GCC, Clang)

**Runtime:**
- Windows (WASAPI audio backend)
- OpenGL 4.3 capable GPU (compute shaders)

---

*Run `/sync-docs` to regenerate.*
