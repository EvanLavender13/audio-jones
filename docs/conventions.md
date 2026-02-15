# Coding Conventions

> Last sync: 2026-02-15 | Commit: 7949421

## Naming Patterns

**Files:**
- Use `snake_case.cpp` and `snake_case.h` for all source files
- Group related files by module directory (`src/analysis/`, `src/render/`, `src/config/`)
- Config headers follow `*_config.h` suffix pattern (in `src/config/`)
- Effect modules live in `src/effects/<name>.cpp` and `src/effects/<name>.h` as paired files

**Functions:**
- Use `PascalCase` with module prefix: `FFTProcessorInit()`, `LFOProcess()`, `DrawableStateUninit()`
- Init/Uninit pairs for resource lifecycle: `PostEffectInit()` / `PostEffectUninit()`
- Setup functions for shader uniform binding: `SetupVoronoi()`, `SetupFeedback()`

**Effect Module Functions:**
- Each effect exposes a standard set of public functions with a `<Name>Effect` or `<Name>` prefix:
  - `<Name>EffectInit()`: Load shaders, cache uniform locations, allocate GPU resources. Return `bool`.
  - `<Name>EffectSetup()`: Accumulate animation state, bind all uniforms. Called per frame.
  - `<Name>EffectUninit()`: Unload shaders and GPU resources.
  - `<Name>EffectResize()`: Reallocate resolution-dependent resources. Only present when the effect owns render textures (e.g., `BloomEffectResize()`).
  - `<Name>ConfigDefault()`: Return a zero-initialized config struct with field defaults via `return <Name>Config{};`.
  - `<Name>RegisterParams()`: Register modulatable config fields with the modulation engine.
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
- Angular bounds use constants from `src/config/constants.h`: `ROTATION_SPEED_MAX` (PI_F), `ROTATION_OFFSET_MAX` (PI_F)

## Effect Descriptor Pattern

**Central Effect Metadata:**
- All transform effects have metadata in `src/config/effect_descriptor.h`: `EFFECT_DESCRIPTORS[]` table
- Each entry maps `TransformEffectType` enum to an `EffectDescriptor` struct with name, category badge, section index, enabled field offset, flags, and lifecycle function pointers
- Flags: `EFFECT_FLAG_NONE`, `EFFECT_FLAG_BLEND`, `EFFECT_FLAG_HALF_RES`, `EFFECT_FLAG_SIM_BOOST`, `EFFECT_FLAG_NEEDS_RESIZE`
- Category badges (2-3 char): `"SYM"` (Symmetry), `"WARP"`, `"CELL"` (Cellular), `"MOT"` (Motion), `"ART"` (Artistic), `"GFX"` (Graphic), `"RET"` (Retro), `"OPT"` (Optical), `"COL"` (Color), `"SIM"` (Simulation), `"GEN"` (Generator)
- When adding a new transform effect: add one descriptor row instead of editing 5+ separate structures

**Self-Registration Macros:**
- Place one registration macro at the bottom of each effect `.cpp` file, wrapped in `// clang-format off` / `// clang-format on`
- `REGISTER_EFFECT(Type, Name, field, displayName, badge, section, flags, SetupFn, ResizeFn)`: Standard transform effect with `Init(Effect*)` signature
- `REGISTER_EFFECT_CFG(...)`: Transform effect whose Init takes `(Effect*, Config*)`
- `REGISTER_GENERATOR(Type, Name, field, displayName, SetupFn, ScratchSetupFn)`: Generator effect (auto-sets `"GEN"` badge, section 10, `EFFECT_FLAG_BLEND`)
- `REGISTER_GENERATOR_FULL(...)`: Generator with sized init and resize support
- `REGISTER_SIM_BOOST(Type, field, displayName, SetupFn, RegisterFn)`: Simulation boost (no init/uninit, uses blend compositor shader)
- Manual registration: Use `EffectDescriptorRegister()` directly when the effect needs custom `GetShader` or non-standard init patterns (e.g., `bloom.cpp` returns `compositeShader` instead of `shader`)

## Config Fields Macro

**Pattern:**
- Each config struct defines a `#define <NAME>_CONFIG_FIELDS` macro listing all serializable fields
- Use in `src/config/effect_serialization.cpp` with `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(<ConfigType>, <NAME>_CONFIG_FIELDS)`
- This macro provides the single source of truth for JSON serialization field lists

## Generator UI Organization

**Generators split into sub-category files:**
- `src/ui/imgui_effects_gen_geometric.cpp`: Signal Frames, Arc Strobe, Pitch Spiral, Spectral Arcs
- `src/ui/imgui_effects_gen_filament.cpp`: Constellation, Filaments, Muons, Slashes, Attractor Lines
- `src/ui/imgui_effects_gen_texture.cpp`: Plasma, Interference, Moire Generator, Scan Bars, Glyph Field, Motherboard
- `src/ui/imgui_effects_gen_atmosphere.cpp`: Nebula, Solid Color
- Each file defines static `bool section<Name>` toggles and `DrawXxxParams()` helper functions
- All generators share a standard Audio section (see FFT Audio UI Conventions below)
- Include: local effect headers, `imgui_effects_generators.h`, standard UI includes

## FFT Audio UI Conventions

**Standard Audio Section:**
- Section header: `ImGui::SeparatorText("Audio")`
- Slider order: Base Freq, Max Freq, Gain, Contrast, Base Bright
- Label names: `"Base Freq (Hz)"`, `"Max Freq (Hz)"`, `"Gain"`, `"Contrast"`, `"Base Bright"`
- `baseFreq` uses `ModulatableSlider` (not `ModulatableSliderLog`) with `"%.1f"` format
- `maxFreq` uses `ModulatableSlider` with `"%.0f"` format, range 1000-16000, default ~14000
- `gain` uses `"%.1f"` format; `curve`/`baseBright` use `"%.2f"`
- Standard ranges: baseFreq 27.5-440, maxFreq 1000-16000, gain 0.1-10, curve 0.1-3, baseBright 0-1
- `layers` is `int` in the Geometry section (not Audio) -- controls visual density, not frequency range
- Frequency spread: layers subdivide `baseFreq` to `maxFreq` in log space regardless of layer count

## Code Style

**Formatting:**
- LLVM style via `.clang-format` (`BasedOnStyle: LLVM`)
- Pre-commit hook auto-formats staged files
- Run manually: `clang-format.exe -i <file>`

**Linting:**
- clang-tidy available via `/lint` skill
- Use `// NOLINTNEXTLINE(rule)` with justification for intentional suppressions

## Include/Import Organization

**Order (effect `.cpp` files):**
1. Own header: `#include "<name>.h"`
2. Project headers: `"automation/modulation_engine.h"`, `"config/effect_descriptor.h"`, `"render/post_effect.h"`
3. System headers: `<stddef.h>`

**Effect `.h` headers:**
1. `"raylib.h"`
2. Shared config headers if embedding: `"render/color_config.h"`, `"config/dual_lissajous_config.h"`
3. `<stdbool.h>`

**UI `.cpp` files:**
1. Project UI headers: `"automation/mod_sources.h"`, `"config/effect_config.h"`
2. Effect headers: `"effects/<name>.h"`
3. ImGui: `"imgui.h"`
4. UI framework: `"ui/imgui_effects_transforms.h"` or `"ui/imgui_effects_generators.h"`, `"ui/imgui_panels.h"`, `"ui/modulatable_slider.h"`, `"ui/theme.h"`, `"ui/ui_units.h"`

## Error Handling

**Patterns:**
- Return `bool` for failable operations: `bool FFTProcessorInit(FFTProcessor* fft)`
- Return `NULL` for failed allocations
- Early return on null checks: `if (fft == NULL) { return false; }`
- Use macros for repetitive cleanup: `INIT_OR_FAIL(ptr, expr)`, `CHECK_OR_FAIL(expr)`

**Effect Module Init Cleanup:**
- `<Name>EffectInit()` unloads previously loaded shaders on failure before returning `false`
- Each shader load checks `shader.id == 0` and cascades cleanup of all prior resources
- Example cascade from `bloom.cpp`: load shader 1, check; load shader 2, check and unload 1 on fail; load shader 3, check and unload 1+2 on fail

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
- Setup functions needing resolution add `int screenWidth, int screenHeight` after `deltaTime`
- Setup functions needing FFT data add `Texture2D fftTexture` as the last parameter

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
- Source includes its own header, `automation/modulation_engine.h`, `config/effect_descriptor.h`, `render/post_effect.h`, and `<stddef.h>`
- File-local `static` helpers group related uniform binding (e.g., `SetupCrt()`, `SetupAnalog()` within `glitch.cpp`)
- Bottom of source file: `Setup<Name>(PostEffect* pe)` bridge function + registration macro

**Shared Config Structs:**
- Embeddable config structs live in `src/config/` (e.g., `DualLissajousConfig`) or `src/render/` (e.g., `ColorConfig`) depending on domain
- Headers may include inline update/evaluation functions when the logic is small and shared across many consumers
- Effects embed these structs as members with overridden defaults: `DualLissajousConfig lissajous = {.amplitude = 0.5f, ...};`
- Corresponding reusable UI widgets in `src/ui/ui_units.h` render controls for shared configs (e.g., `DrawLissajousControls()`)

**Internal Implementation:**
- Isolate STL usage to `.cpp` files (exception: `preset.cpp` and `effect_serialization.cpp` for JSON)
- Use `static` for file-local functions and variables
- Prefer C headers (`<stdlib.h>`) over C++ equivalents (`<cstdlib>`)

## Serialization Conventions

**JSON Preset System:**
- Use nlohmann/json with `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT` macro
- Define `<NAME>_CONFIG_FIELDS` macro in each config header listing all serializable fields
- Register serialization in `src/config/effect_serialization.cpp` using the config fields macro
- `ColorConfig` uses manual `to_json`/`from_json` due to variant-like mode handling
- `from_json` always starts with default initialization: `c = ConfigType{};`
- Preset I/O in `src/config/preset.cpp`; effect config serialization in `src/config/effect_serialization.cpp`

## UI Section Pattern

**Transform Effect UI:**
- Each category file (`src/ui/imgui_effects_<category>.cpp`) declares static `bool section<Name>` toggles
- Each effect gets a static `Draw<Category><Name>()` function taking `(EffectConfig*, const ModSources*, ImU32 categoryGlow)`
- Section begin/end: `DrawSectionBegin("Name", categoryGlow, &sectionFlag, enabledBool)` / `DrawSectionEnd()`
- Enable checkbox pattern: save `wasEnabled`, draw checkbox, call `MoveTransformToEnd()` on fresh enable
- Use `ModulatableSlider()` for float params with modulation support
- Use `ModulatableSliderAngleDeg()` and `ModulatableSliderSpeedDeg()` for angular params
- Use `ModulatableSliderLog()` for logarithmic-scale params (e.g., small float ranges like 0.01-1.0)
- Use `ModulatableSliderInt()` for integer-valued params stored as float for modulation compatibility
- Use `ImGui::SliderInt()` for true integer fields not needing modulation

## Angular Field Conventions

**Suffix Rules:**
- `*Speed` for rotation rates (radians/second): accumulate with `accum += speed * deltaTime`
- `*Angle` for static angles (radians): apply directly without time scaling
- `*Twist` for per-unit rotation (per depth/octave)
- `*Freq` for oscillation frequency (Hz)

**UI Display:**
- Display degrees in UI, store radians internally
- Use `SliderAngleDeg()` or `ModulatableSliderAngleDeg()` from `src/ui/ui_units.h`
- Use `SliderSpeedDeg()` or `ModulatableSliderSpeedDeg()` for rotation rate sliders
- Speed sliders show "/s" suffix, angle sliders show "deg" suffix
- Angle fields use `ROTATION_OFFSET_MAX` constant (PI_F) from `src/config/constants.h`, not hardcoded `6.28318f` or `TWO_PI_F`
- Range: `-ROTATION_OFFSET_MAX` to `+ROTATION_OFFSET_MAX` (plus/minus 180 degrees)

## Param Registration Conventions

**Effect Params:**
- Call `ModEngineRegisterParam("effectName.fieldName", &cfg->field, min, max)` in `<Name>RegisterParams()`
- Use dot-separated ID: `"bloom.threshold"`, `"fluxWarp.cellScale"`
- Bounds must match the range comments in the config struct header

**Param Registry Table:**
- Global params (feedback, flow field) use `PARAM_TABLE[]` in `src/automation/param_registry.cpp`
- Each entry: `{id, {min, max}, offsetof(EffectConfig, field)}`
- Rotation speeds use `ROTATION_SPEED_MAX` bounds; offsets use `ROTATION_OFFSET_MAX`

## Shader Coordinate Conventions

**Origin Rules:**
- Center coordinates relative to `center` uniform (or screen center if no center uniform exists)
- `fragTexCoord` is (0,0) at bottom-left -- never use it raw for scaling, noise, or distance fields
- Correct: `noise2D((fragTexCoord - center) * noiseScale + time)`
- Incorrect: `noise2D(fragTexCoord * noiseScale + time)`
- Apply this rule to all spatial operations: noise sampling, distance fields, radial effects, zoom origins

---

*Run `/sync-docs` to regenerate.*
