# Coding Conventions

> Last sync: 2026-01-29 | Commit: 176b35f

## Naming Patterns

**Files:**
- Use `snake_case.cpp` and `snake_case.h` for all source files
- Group related files by module directory (`src/analysis/`, `src/render/`, `src/config/`)
- Config headers follow `*_config.h` suffix pattern

**Functions:**
- Use `PascalCase` with module prefix: `FFTProcessorInit()`, `LFOProcess()`, `DrawableStateUninit()`
- Init/Uninit pairs for resource lifecycle: `PostEffectInit()` / `PostEffectUninit()`
- Setup functions for shader uniform binding: `SetupVoronoi()`, `SetupFeedback()`

**Variables:**
- Use `camelCase` for local variables and struct fields: `deltaTime`, `screenWidth`, `sampleCount`
- Pointer parameters use direct naming: `FFTProcessor* fft`, `PostEffect* pe`

**Types:**
- Use `PascalCase` for structs and enums: `FFTProcessor`, `DrawableType`, `LFOConfig`
- Enum values use `UPPER_SNAKE_CASE`: `DRAWABLE_WAVEFORM`, `LFO_WAVE_SINE`
- Typedef enums directly: `typedef enum { ... } EnumName;`

**Constants:**
- Use `UPPER_SNAKE_CASE` for all constants: `FFT_SIZE`, `ROTATION_SPEED_MAX`, `NUM_LFOS`
- Define in headers with `#define` or `static const`

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

## Function Design

**Size:** Keep functions under 50 lines. Extract setup/cleanup into helpers.

**Parameters:**
- Pass structs by pointer: `void SetupKaleido(PostEffect* pe)`
- Use `const` for read-only pointer parameters: `const LFOConfig* config`
- Output parameters last: `bool FFTProcessorUpdate(FFTProcessor* fft)`

**Return Values:**
- `bool` for success/failure
- Pointer for allocated resources (NULL on failure)
- `void` when no return needed

## Module Design

**Public Interface:**
- Declare in `.h` header with include guards (`#ifndef MODULE_H`)
- Expose Init/Uninit pairs for resource management
- Use opaque pointers for internal implementation details

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
- Speed sliders show "/s" suffix, angle sliders show "deg" suffix

---

*Run `/sync-docs` to regenerate.*
