# Technology Stack

> Last sync: 2026-02-06 | Commit: 957e250

## Languages

**Primary:**
- C++20 - Application logic (`src/**/*.cpp`, 131 source files, 135 headers)

**Secondary:**
- GLSL 4.30 - Fragment shaders (`shaders/*.fs`, 76 files) and compute shaders (`shaders/*.glsl`, 10 files)

## Build System

**Build Tool:**
- CMake 3.20+
- Config: `CMakeLists.txt`

**Package Manager:**
- CMake FetchContent (no external package manager)
- Manifest: Dependencies declared inline in `CMakeLists.txt`
- Lockfile: Not present (uses Git tags for version pinning)

## Frameworks

**Core:**
- raylib 5.5 - Window management, OpenGL context, input handling, rendering primitives
- Dear ImGui (docking branch) - Immediate-mode GUI for control panels
- rlImGui (main) - raylib-to-ImGui integration layer

**Build/Dev:**
- Ninja - Build generator (recommended)
- FetchContent - Dependency fetching at configure time

## Key Dependencies

**Critical:**
- raylib 5.5 - OpenGL 4.3 context for compute shaders and rendering
- miniaudio 0.11.21 - WASAPI loopback audio capture on Windows
- kissfft 131.1.0 - FFT processing for beat detection and spectral analysis

**Infrastructure:**
- nlohmann/json 3.11.3 - Preset serialization/deserialization
- rlImGui - Bridges raylib rendering with Dear ImGui

## Configuration

**Build:**
- `CMakeLists.txt` - Project definition, FetchContent dependencies, source groups

**Tooling:**
- `.clang-format` - LLVM-based code formatting
- `.clang-tidy` - Static analysis with bugprone, performance, and readability checks

## Platform Requirements

**Development:**
- Windows 10/11 (primary), WSL2 (development OK)
- CMake 3.20+
- C++20 compiler (MSVC, Clang, or GCC)
- OpenGL 4.3 capable GPU (compute shader support)

**Runtime:**
- Windows 10/11 (WASAPI loopback requires Windows audio stack)
- OpenGL 4.3 capable GPU
- Audio output device (loopback captures system audio)

---

*Run `/sync-docs` to regenerate.*
