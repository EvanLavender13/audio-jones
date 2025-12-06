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

- C with minimal C++ (raylib is C)
- No exceptions, simple error handling
- raylib naming: `LoadTexture`, `BeginDrawing`, `EndShaderMode`
- miniaudio naming: `ma_device_init`, `ma_pcm_rb_acquire_read`
- Comments describe code, not changes. Put rationale in commits.

## Current State

Working prototype with audio loopback capture and circular waveform visualization.
Features: up to 8 concurrent waveforms with per-waveform configuration (radius, thickness, smoothness, rotation, color), cubic interpolation, 20fps update rate, physarum-style trails via separable Gaussian blur, preset save/load system.

Application state consolidated in `AppContext` struct (`main.c`) with centralized lifecycle management. UI extracted to dedicated module (`ui.c`/`ui.h`) with auto-stacking panel layout.

## Key Files

**Architecture**: `docs/architecture.md` - Full system architecture

**Research** (temporary planning docs):
- `docs/research/audiothing-existing-impl.md` - Analysis of original AudioThing
- `docs/research/graphics-framework-selection.md` - Why raylib
- `docs/research/miniaudio-loopback-capture.md` - Audio capture approach
- `docs/research/agent-driven-development.md` - Development philosophy
