# AudioJones

Real-time circular waveform audio visualizer. Captures system audio via loopback, renders reactive visuals with trail effects.

## Tech Stack

- **Graphics**: raylib 5.5
- **Audio**: miniaudio loopback capture
- **UI**: raygui 4.0
- **Platform**: Windows primary, Linux possible

## Build Commands

```bash
# Configure (Ninja generator for compile_commands.json)
cmake.exe -G Ninja -B build -S . -DCMAKE_BUILD_TYPE=Release

# Build
cmake.exe --build build

# Run
./build/AudioJones.exe
```

Requires Ninja (via MSYS2: `pacman -S mingw-w64-ucrt-x86_64-ninja`).
CMake copies shaders to build/ automatically.

## Workflow

When implementing features:
1. Use Explore agent to understand existing code patterns first
2. Plan before coding on non-trivial tasks
3. Follow skill conventions when working in their domains
4. Iterate based on visual/test feedback

When I ask about the codebase structure or "where is X":
- Use Explore agent, don't grep directly

When working on shaders, audio, or graphics:
- Relevant skills load automatically

## Code Style

C++20 compiled with C-style conventions (matches raylib/miniaudio APIs).

**Use:**
- `.h` headers, `.cpp` implementations
- Opaque types with Init/Uninit pairs (not RAII)
- Fixed-size arrays, raw pointers, NULL
- In-class defaults for struct fields
- C++ libraries only when they reduce boilerplate (JSON, filesystem)

**Avoid:**
- auto, nullptr, std::vector, std::string in public APIs
- Exceptions, templates, inheritance, virtual functions
- Smart pointers, RAII wrappers
- STL containers in headers (isolate to .cpp if needed)

**Naming:** raylib PascalCase (`LoadTexture`), miniaudio snake_case (`ma_device_init`)

**Comments:** Describe code, not changes. Put rationale in commits.

## Current State

Working visualizer with WASAPI loopback capture, circular/linear waveform modes (SPACE to toggle), physarum-style trails via separable Gaussian blur, beat-reactive bloom pulse and chromatic aberration effects.

Features: up to 8 concurrent waveforms with per-waveform config (radius, thickness, smoothness, rotation, color/rainbow), FFT-based spectral flux beat detection with configurable sensitivity, beat-reactive blur scaling for bloom effect, beat-reactive radial chromatic aberration, configurable stereo channel mixing modes (Left/Right/Max/Mix/Side/Interleaved), preset save/load (JSON), 60fps render with 20fps waveform updates.

Architecture: `AppContext` (`main.cpp`) holds all runtime state. Modules: audio.cpp (capture), audio_config.h (channel mode), beat.cpp (FFT beat detection), waveform.cpp (processing + render), visualizer.cpp (blur + chromatic pipeline), effects_config.h (consolidated effect parameters), ui.cpp (waveform/effects panel), ui_preset.cpp (preset panel), ui_layout.cpp (declarative layout), ui_widgets.cpp (custom controls), preset.cpp (JSON serialization).

## Key Files

**Architecture**: `docs/architecture.md` - Full system architecture

**Research** (temporary planning docs):
- `docs/research/audiothing-existing-impl.md` - Analysis of original AudioThing
- `docs/research/graphics-framework-selection.md` - Why raylib
- `docs/research/miniaudio-loopback-capture.md` - Audio capture approach
- `docs/research/agent-driven-development.md` - Development philosophy
