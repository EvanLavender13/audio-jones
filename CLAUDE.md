# AudioJones

Real-time audio visualizer: WASAPI loopback → FFT beat detection → reactive circular waveforms with bloom/trails.

## Stack

C++20, raylib 5.5, miniaudio, Dear ImGui (docking) via rlImGui (Windows primary, WSL2 dev OK)

## Build

```bash
cmake.exe -G Ninja -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake.exe --build build
./build/AudioJones.exe
```

## Writing Style

All text output—responses, comments, documentation, summaries—follows these rules:

**Voice & Tense:** Active voice, present tense. "The parser validates" not "is validated" or "will validate."

**Brevity:** 15-25 words per sentence max. One idea per paragraph. Cut words that don't teach something specific.

**Specificity:** Replace vague verbs with precise ones:
- "handles/manages/processes" → validates, parses, routes, stores, creates, deletes, filters
- "fast/slow/many" → concrete values with units (e.g., "<100ms", "up to 1000")

**Structure:**
- Lead with the action or finding
- Conditions before instructions: "To enable X, set Y" not "Set Y to enable X"
- Reference code as `file_path:line_number`

**Comments:** Explain WHY, not WHAT. Code shows what.

**Anti-patterns:** Never write "responsible for managing", "handles various operations", "main functionality", or "processes data as needed."

## Architecture

See [docs/architecture.md](docs/architecture.md) for layers, data flow, key abstractions, and entry points.

## Reference Documents

@docs/structure.md
@docs/conventions.md
