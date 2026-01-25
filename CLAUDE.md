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

## Key Paths

- `shaders/` — Fragment shaders (`.fs`) and compute shaders (`.glsl`)
- `src/config/*_config.h` — Per-effect config structs
- `src/render/shader_setup.cpp` — Shader uniform binding

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

## Code Style

C++20 with C-style conventions (matches raylib/miniaudio APIs).

**Structures:** Public fields with direct access. In-class defaults for config structs.

**Functions:** Init/Uninit pairs for resources. PascalCase with module prefix (e.g., `FFTProcessorInit`).

**Types:** Explicit types, NULL, raw pointers, fixed arrays, C-style casts, `const` for unmodified values. Use C++ brace initialization `Type{ ... }`.

**Formatting:** Braces `{}` on all control flow, even single statements.

**Comments:** Only when logic isn't self-evident. Explain "why", never "what". No comments on unchanged code.

**Headers:** `.h`/`.cpp` split. C headers only (`stdbool.h`, `stdint.h`). Isolate STL to `.cpp` files.

**Naming:** PascalCase functions, camelCase locals, UPPER_SNAKE_CASE constants.

**Known deviations:** `preset.cpp` uses STL for JSON serialization.

**Angles:** Display degrees in UI, store radians internally. Use `SliderAngleDeg` or `ModulatableSliderAngleDeg` from `ui_units.h`.

**Angular Field Naming:** All rotation/angular config fields follow strict suffixes:
- `*Speed` → rate (radians/second), time-scaled accumulation (`accum += speed * deltaTime`), UI shows "°/s", bounded by `ROTATION_SPEED_MAX`
- `*Angle` → static angle (radians), instant application, UI shows "°", bounded by `ROTATION_OFFSET_MAX`
- `*Twist` → per-unit rotation (per depth/octave), UI shows "°", bounded by `ROTATION_OFFSET_MAX`
- `*Freq` → oscillation frequency (Hz), UI shows "Hz"

Speed fields MUST use CPU accumulation with deltaTime. Never pass speed to shaders for `time * value` computation—this causes jumps when the value changes mid-animation. Register all angular fields in `param_registry.cpp` with appropriate `ROTATION_*` constants.

**UI Styling:** Use colors, spacing, and dimensions from `src/ui/theme.h`.

## Architecture

See [docs/architecture.md](docs/architecture.md) for system diagram, module index, and data flow.
