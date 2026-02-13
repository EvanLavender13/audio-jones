# Coding Conventions

> Last sync: 2026-02-12 | Commit: e42039b

## Naming Patterns

**Files:**
- Use `snake_case.cpp` and `snake_case.h` for all source files
- Group related files by module directory (`src/analysis/`, `src/render/`, `src/config/`)
- Config headers follow `*_config.h` suffix pattern
- Effect modules live in `src/effects/<name>.cpp` and `src/effects/<name>.h` as paired files

**Functions:**
- Use `PascalCase` with module prefix: `FFTProcessorInit()`, `LFOProcess()`, `DrawableStateUninit()`
- Init/Uninit pairs for resource lifecycle: `PostEffectInit()` / `PostEffectUninit()`
- Setup functions for shader uniform binding: `SetupVoronoi()`, `SetupFeedback()`

**Effect Module Functions:**
- Each effect exposes a standard set of public functions with a `<Name>Effect` or `<Name>` prefix:
  - `<Name>EffectInit()`: Loads shaders, caches uniform locations, allocates GPU resources. Returns `bool`.
  - `<Name>EffectSetup()`: Accumulates animation state, binds all uniforms. Called per frame.
  - `<Name>EffectUninit()`: Unloads shaders and GPU resources.
  - `<Name>EffectResize()`: Reallocates resolution-dependent resources. Only present when the effect owns render textures (e.g., `BloomEffectResize()`).
  - `<Name>ConfigDefault()`: Returns a zero-initialized config struct with field defaults.
  - `<Name>RegisterParams()`: Registers modulatable config fields with the modulation engine.
- Internal helpers use `static` and short verb names: `CacheLocations()`, `InitMips()`, `SetupCrt()`.

**Variables:**
- Use `camelCase` for local variables and struct fields: `deltaTime`, `screenWidth`, `sampleCount`
- Pointer parameters use direct naming: `FFTProcessor* fft`, `PostEffect* pe`

**Types:**
- Use `PascalCase` for structs and enums: `FFTProcessor`, `DrawableType`, `LFOConfig`
- Effect modules define two structs per header: `<Name>Config` (user-facing params) and `<Name>Effect` (runtime state + shader handles)
- Shared embeddable config structs (`ColorConfig`, `DualLissajousConfig`) embed as member fields in effect configs with overridden defaults via designated initializers
- Enum values use `UPPER_SNAKE_CASE`: `DRAWABLE_WAVEFORM`, `LFO_WAVE_SINE`
- Typedef enums directly: `typedef enum { ... } EnumName;`

**Constants:**
- Use `UPPER_SNAKE_CASE` for all constants: `FFT_SIZE`, `ROTATION_SPEED_MAX`, `NUM_LFOS`
- Define in headers with `#define` or `static const`

## Effect Descriptor Pattern

**Central Effect Metadata:**
- All transform effects have metadata in `src/config/effect_descriptor.h`: `EFFECT_DESCRIPTORS[]` table
- Each entry maps `TransformEffectType` enum → struct with name, category badge, section index, enabled field offset, and flags
- Replaces: scattered `TRANSFORM_EFFECT_NAMES[]` switch statements, multiple enable/blend/resize tracking bools
- Flags: `EFFECT_FLAG_NONE`, `EFFECT_FLAG_BLEND`, `EFFECT_FLAG_HALF_RES`, `EFFECT_FLAG_SIM_BOOST`, `EFFECT_FLAG_NEEDS_RESIZE`
- Category badges (2-3 char): `"SYM"` (Symmetry), `"WARP"`, `"CELL"` (Cellular), `"MOT"` (Motion), `"ART"` (Artistic), `"GFX"` (Graphic), `"RET"` (Retro), `"OPT"` (Optical), `"COL"` (Color), `"SIM"` (Simulation), `"GEN"` (Generator)
- When adding a new transform effect: add one descriptor row instead of editing 5+ separate structures

## Generator UI Organization

**Generators (10 effects) split into category files:**
- `src/ui/imgui_effects_gen_geometric.cpp`: Signal Frames, Arc Strobe, Pitch Spiral, Spectral Arcs
- `src/ui/imgui_effects_gen_filament.cpp`: Constellation, Filaments, Muons, Slashes, Attractor Lines
- `src/ui/imgui_effects_gen_texture.cpp`: Plasma, Interference, Moire Generator, Scan Bars, Glyph Field, Motherboard
- `src/ui/imgui_effects_gen_atmosphere.cpp`: Nebula, Solid Color
- Each file defines static `bool section<Name>` toggles and `DrawXxxParams()` helper functions
- All generators share standard Audio section: Octaves (int slider), Base Freq, Gain, Contrast, Base Bright (all with standard FFT label names and ranges)
- Header includes: local effect headers, `imgui_effects_generators.h`, standard UI includes

## Code Style

**Formatting:**
- LLVM style via `.clang-format`
- Pre-commit hook auto-formats staged files
- Run manually: `clang-format.exe -i <file>`

**Linting:**
- clang-tidy available via `/lint` skill
- Use `// NOLINTNEXTLINE(rule)` with justification for intentional suppressions

## Error Handling

**Patterns:**
- Return `bool` for failable operations: `bool FFTProcessorInit(FFTProcessor* fft)`
- Return `NULL` for failed allocations
- Early return on null checks: `if (fft == NULL) { return false; }`
- Use macros for repetitive cleanup: `INIT_OR_FAIL(ptr, expr)`, `CHECK_OR_FAIL(expr)`

**Effect Module Init Cleanup:**
- `<Name>EffectInit()` unloads previously loaded shaders on failure before returning `false`
- Each shader load checks `shader.id == 0` and cascades cleanup of all prior resources

## Logging

**Framework:** TraceLog (raylib built-in)

**Patterns:**
- Reserve logging for initialization failures and critical errors
- Avoid per-frame logging in hot paths

## Comments

**When to Comment:**
- Explain WHY, never WHAT the code does
- Document non-obvious algorithm choices
- Add `// NOLINTNEXTLINE` with justification for intentional lint suppressions

**Documentation Style:**
- Inline comments for complex math or algorithm steps
- No Doxygen or formal doc comments
- Keep comments terse and actionable

**Effect Module Headers:**
- Top-of-file comment names the effect and describes its visual technique in one line
- Config struct fields include inline range comments: `float threshold = 0.8f; // Brightness cutoff (0.0-2.0)`
- Each public function has a one-line comment describing its purpose

## Function Design

**Size:** Keep functions under 50 lines. Extract setup/cleanup into helpers.

**Parameters:**
- Pass structs by pointer: `void SetupKaleido(PostEffect* pe)`
- Use `const` for read-only pointer parameters: `const LFOConfig* config`
- Output parameters last: `bool FFTProcessorUpdate(FFTProcessor* fft)`
- Effect Setup functions take `(Effect* e, const Config* cfg, float deltaTime)` for animated effects

**Return Values:**
- `bool` for success/failure
- Pointer for allocated resources (NULL on failure)
- `void` when no return needed

## Module Design

**Public Interface:**
- Declare in `.h` header with include guards (`#ifndef MODULE_H`)
- Expose Init/Uninit pairs for resource management
- Use opaque pointers for internal implementation details

**Effect Module Layout:**
- Header declares `<Name>Config` (defaults via `= value`), `<Name>Effect` (typedef struct), and 5-6 public functions
- Source includes its own header, `automation/modulation_engine.h`, and `<stddef.h>`
- File-local `static` helpers group related uniform binding (e.g., `SetupCrt()`, `SetupAnalog()` within `glitch.cpp`)

**Shared Config Structs:**
- Embeddable config structs live in `src/config/` (e.g., `DualLissajousConfig`) or `src/render/` (e.g., `ColorConfig`) depending on domain
- Headers may include inline update/evaluation functions when the logic is small and shared across many consumers
- Effects embed these structs as members with overridden defaults: `DualLissajousConfig lissajous = {.amplitude = 0.5f, ...};`
- Corresponding reusable UI widgets in `ui_units.h` render controls for shared configs (e.g., `DrawLissajousControls()`)

**Internal Implementation:**
- Isolate STL usage to `.cpp` files (exception: `preset.cpp` for JSON)
- Use `static` for file-local functions and variables
- Prefer C headers (`<stdlib.h>`) over C++ equivalents (`<cstdlib>`)

## Angular Field Conventions

**Suffix Rules:**
- `*Speed` for rotation rates (radians/second): accumulate with `accum += speed * deltaTime`
- `*Angle` for static angles (radians): apply directly without time scaling
- `*Twist` for per-unit rotation (per depth/octave)
- `*Freq` for oscillation frequency (Hz)

**UI Display:**
- Display degrees in UI, store radians internally
- Use `SliderAngleDeg()` or `ModulatableSliderAngleDeg()` from `ui_units.h`
- Use `SliderSpeedDeg()` or `ModulatableSliderSpeedDeg()` for rotation rate sliders
- Speed sliders show "/s" suffix, angle sliders show "deg" suffix
- `ModulatableSliderLog()` for logarithmic-scale parameters (e.g., small float ranges like 0.01-1.0)
- `ModulatableSliderInt()` for integer-valued parameters stored as float for modulation compatibility

---

*Run `/sync-docs` to regenerate.*
