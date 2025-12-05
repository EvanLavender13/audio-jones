# AudioJones

Real-time circular waveform audio visualizer. Captures system audio via loopback, renders reactive visuals with trail effects.

## Tech Stack

- **Graphics**: raylib 5.5
- **Audio**: miniaudio loopback capture
- **UI**: raygui 4.0
- **Platform**: Windows primary, Linux possible

## Build Commands

```bash
# Configure (Windows CMake from WSL for speed)
cmake.exe -B build -S .

# Build
cmake.exe --build build --config Release

# Run
./build/Release/AudioJones.exe
```

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

## Documentation Sync

Run `/sync-architecture` ONLY when:
- Adding/removing/renaming modules (new .c/.h files)
- Changing data flow between components
- Adding new pipeline stages (shaders, render passes)
- Modifying thread model or async boundaries

Do NOT run for:
- Adding parameters to existing functions
- UI changes (sliders, labels, layout)
- Bug fixes within existing functions
- Performance tweaks

Architecture: `docs/arch/architecture.md` (Mermaid diagrams).
Reference code with `file_path:line_number` format.

## Code Style

- C with minimal C++ (raylib is C)
- No exceptions, simple error handling
- raylib naming: `LoadTexture`, `BeginDrawing`, `EndShaderMode`
- miniaudio naming: `ma_device_init`, `ma_pcm_rb_acquire_read`

## Current State

Working prototype with audio loopback capture and circular waveform visualization.
Features: cubic interpolation, 30fps update rate, physarum-style trails via separable Gaussian blur.

## Key Files

**Architecture**: `docs/arch/architecture.md` - Full system architecture (run `/sync-architecture` to update)

**Research** (temporary planning docs):
- `docs/audiothing-existing-impl.md` - Analysis of original AudioThing
- `docs/graphics-framework-selection.md` - Why raylib
- `docs/miniaudio-loopback-capture.md` - Audio capture approach
- `docs/agent-driven-development.md` - Development philosophy
