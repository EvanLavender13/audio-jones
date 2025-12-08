# AudioJones

Real-time audio visualizer: WASAPI loopback → FFT beat detection → reactive circular waveforms with bloom/trails.

## Stack

- C++20, raylib 5.5, miniaudio, raygui 4.0
- Windows primary (WSL2 dev OK)

## Commands

```bash
cmake.exe -G Ninja -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake.exe --build build
./build/AudioJones.exe
```

## Slash Commands

- `/lint` - Run clang-tidy, triage warnings
- `/complexity` - Analyze with lizard, find refactor targets
- `/modularize` - Extract modules from large files
- `/sync-architecture` - Regenerate docs/architecture.md

## Skills (invoke explicitly)

- `raylib` - API reference before implementing graphics/input/audio
- `architecture-diagrams` - Mermaid diagrams for system docs
- `technical-writing` - Prose standards for docs/comments
- `planning` - Update ROADMAP.md with consistent structure

## Workflow

- Use Explore agent for "where is X" questions
- Plan before coding on non-trivial tasks
- Invoke skills explicitly when working in their domains

## Code Style

C++20 with C-style conventions (matches raylib/miniaudio APIs).

**Use:** `.h`/`.cpp` split, Init/Uninit pairs, fixed arrays, raw pointers, NULL, in-class defaults

**Avoid:** auto, nullptr, STL in headers, exceptions, templates, RAII wrappers, smart pointers

**Naming:** raylib PascalCase, miniaudio snake_case

## Commit Messages

**Subject line:** Imperative mood, 50 chars max, capitalized, no period

- "Add spectrum analyzer" NOT "Added spectrum analyzer"
- Test: "If applied, this commit will [your subject]"

**Body:** Explain WHY, not HOW. Code shows what changed; only the message explains the reasoning.

**Scope:** One logical change per commit. Atomic commits simplify review and revert.

## Key Files

- `docs/architecture.md` - System architecture (authoritative)
- `main.cpp:AppContext` - All runtime state
- `ROADMAP.md` - Feature planning and priorities
