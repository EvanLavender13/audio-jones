# Effect Modules Batch 9: Color

Migrate 3 color effects (Color Grade, False Color, Palette Quantization) from monolithic PostEffect into self-contained modules under `src/effects/`.

**Reference**: [docs/plans/effect-modules-migration.md](effect-modules-migration.md)

## Design

### Module Pattern

Each effect becomes a self-contained module with:
- **Header** (`src/effects/<name>.h`): Config struct (moved from `src/config/<name>_config.h`), Effect struct (shader + uniform locations + resources), lifecycle declarations
- **Implementation** (`src/effects/<name>.cpp`): Init (load shader, cache locations, allocate resources), Setup (bind uniforms), Uninit (unload shader, free resources), ConfigDefault (return `Config{}`)

### Effect Struct Layouts

**ColorGradeEffect** (8 uniform locations):
- `Shader shader`
- `int hueShiftLoc, saturationLoc, brightnessLoc, contrastLoc, temperatureLoc, shadowsOffsetLoc, midtonesOffsetLoc, highlightsOffsetLoc`

**FalseColorEffect** (2 uniform locations + 1 ColorLUT):
- `Shader shader`
- `int intensityLoc, gradientLUTLoc`
- `ColorLUT *lut`

**PaletteQuantizationEffect** (3 uniform locations):
- `Shader shader`
- `int colorLevelsLoc, ditherStrengthLoc, bayerSizeLoc`

### FalseColor Resource Ownership

FalseColor owns a `ColorLUT*` for its gradient LUT texture. Currently `pe->falseColorLUT` in PostEffect. The module takes ownership:
- `FalseColorEffectInit` allocates the LUT via `ColorLUTInit`
- `FalseColorEffectSetup` calls `ColorLUTUpdate` then binds the texture
- `FalseColorEffectUninit` calls `ColorLUTUninit`

The `FalseColorConfig` includes `ColorConfig gradient` which requires `#include "render/color_config.h"` — this stays in the header since it defines a field type.

### Setup Function Signatures

- `ColorGradeEffectSetup(ColorGradeEffect* e, const ColorGradeConfig* cfg)` — binds 8 float uniforms
- `FalseColorEffectSetup(FalseColorEffect* e, const FalseColorConfig* cfg)` — calls `ColorLUTUpdate`, binds intensity + LUT texture
- `PaletteQuantizationEffectSetup(PaletteQuantizationEffect* e, const PaletteQuantizationConfig* cfg)` — binds 2 floats + 1 int

### Resolution Handling

None of the 3 color effects use a `resolution` uniform. No resolution handling needed.

### Integration Changes

**`effect_config.h`**: Replace 3 config includes (`color_grade_config.h`, `false_color_config.h`, `palette_quantization_config.h`) with 3 effect includes (`effects/color_grade.h`, `effects/false_color.h`, `effects/palette_quantization.h`)

**`post_effect.h`**: Remove 3 shader fields (`colorGradeShader`, `falseColorShader`, `paletteQuantizationShader`), 13 uniform location fields, and `falseColorLUT` pointer. Add 3 effect struct members (`ColorGradeEffect colorGrade`, `FalseColorEffect falseColor`, `PaletteQuantizationEffect paletteQuantization`). Remove `#include "effects/..."` lines that become transitively included via `effect_config.h` — not applicable here since color effects weren't previously in `src/effects/`.

**`post_effect.cpp`**: Replace LoadShader/GetShaderLocation/UnloadShader calls with module Init/Uninit calls. Remove `falseColorLUT` init/uninit (now owned by module). Remove color entries from `LoadPostEffectShaders`, `GetShaderUniformLocations`, `PostEffectUninit`.

**`shader_setup_color.cpp`**: Each Setup function becomes a one-line delegate to the module's EffectSetup. Remove `#include "color_lut.h"` (no longer needed — module handles LUT internally).

**`shader_setup.cpp`**: Update 3 dispatch cases to use module shader fields. Remove `#include "shader_setup_color.h"` — wait, this include IS still needed since the Setup functions are declared there and called from the dispatch table. Keep it.

### Pitfall Reminders

From migration plan:
- `#include <stddef.h>` (not `<stdlib.h>`) in .cpp files
- ConfigDefault returns ONLY `return Config{};`
- No null-checks in Init/Setup/Uninit
- Use `shader.id == 0` pattern for shader validation
- Verify ONLY batch 9 shaders removed from `LoadPostEffectShaders` — don't touch plasma, constellation, interference shader loads
- None of these effects are in HALF_RES_EFFECTS

---

## Tasks

### Wave 1: Effect Modules

Create 3 module file pairs. No file overlap between tasks — all parallel.

#### Task 1.1: Color Grade Module

**Files**: `src/effects/color_grade.h` (create), `src/effects/color_grade.cpp` (create)

**Do**:
- Move `ColorGradeConfig` from `src/config/color_grade_config.h` into the header
- Define `ColorGradeEffect` struct with 8 uniform locations per layout above
- Init: load `shaders/color_grade.fs`, cache 8 locations (`hueShift`, `saturation`, `brightness`, `contrast`, `temperature`, `shadowsOffset`, `midtonesOffset`, `highlightsOffset`)
- Setup: bind 8 float uniforms from config. No resolution, no time state.
- ConfigDefault: `return ColorGradeConfig{};`
- Follow `src/effects/toon.h`/`toon.cpp` as reference (simple single-shader, no resolution)

**Verify**: Files compile in isolation.

#### Task 1.2: False Color Module

**Files**: `src/effects/false_color.h` (create), `src/effects/false_color.cpp` (create)

**Do**:
- Move `FalseColorConfig` from `src/config/false_color_config.h` into the header. Keep `#include "render/color_config.h"` since `ColorConfig` is a field type.
- Define `FalseColorEffect` struct with 2 uniform locations + `ColorLUT *lut`
- Init: load `shaders/false_color.fs`, cache 2 locations (`intensity`, `texture1`). Allocate LUT — signature: `bool FalseColorEffectInit(FalseColorEffect* e, const FalseColorConfig* cfg)`. Calls `ColorLUTInit(&cfg->gradient)`. Include `"render/color_lut.h"`.
- Setup: call `ColorLUTUpdate(e->lut, &cfg->gradient)`, then bind intensity float + LUT texture via `SetShaderValueTexture(e->shader, e->gradientLUTLoc, ColorLUTGetTexture(e->lut))`
- Uninit: unload shader, call `ColorLUTUninit(e->lut)`
- ConfigDefault: `return FalseColorConfig{};`
- Follow `src/effects/toon.h`/`toon.cpp` for structure, but note the Init takes a config pointer (for LUT initialization) unlike most modules

**Verify**: Files compile in isolation.

#### Task 1.3: Palette Quantization Module

**Files**: `src/effects/palette_quantization.h` (create), `src/effects/palette_quantization.cpp` (create)

**Do**:
- Move `PaletteQuantizationConfig` from `src/config/palette_quantization_config.h` into the header
- Define `PaletteQuantizationEffect` struct with 3 uniform locations per layout above
- Init: load `shaders/palette_quantization.fs`, cache 3 locations (`colorLevels`, `ditherStrength`, `bayerSize`)
- Setup: bind 2 floats + 1 int from config. No resolution, no time state.
- ConfigDefault: `return PaletteQuantizationConfig{};`
- Follow `src/effects/toon.h`/`toon.cpp` as reference

**Verify**: Files compile in isolation.

---

### Wave 2: Integration

Modify existing files to use modules. No file overlap between tasks — all parallel.

#### Task 2.1: Effect Config Integration

**Files**: `src/config/effect_config.h` (modify)

**Depends on**: Wave 1 complete

**Do**:
- Replace 3 config includes with effect includes:
  - `#include "color_grade_config.h"` -> `#include "effects/color_grade.h"`
  - `#include "false_color_config.h"` -> `#include "effects/false_color.h"`
  - `#include "palette_quantization_config.h"` -> `#include "effects/palette_quantization.h"`
- Everything else unchanged (enum values, name array, EffectConfig members, IsTransformEnabled)

**Verify**: `cmake.exe --build build` compiles.

#### Task 2.2: PostEffect Integration

**Files**: `src/render/post_effect.h` (modify), `src/render/post_effect.cpp` (modify)

**Depends on**: Wave 1 complete

**Do** in `post_effect.h`:
- Remove bare shader fields: `colorGradeShader`, `falseColorShader`, `paletteQuantizationShader`
- Remove all 13 uniform location fields: `colorGradeHueShiftLoc` through `colorGradeHighlightsOffsetLoc`, `falseColorIntensityLoc`, `falseColorGradientLUTLoc`, `paletteQuantizationColorLevelsLoc`, `paletteQuantizationDitherStrengthLoc`, `paletteQuantizationBayerSizeLoc`
- Remove `ColorLUT *falseColorLUT` pointer (the `typedef struct ColorLUT ColorLUT;` forward declaration STAYS — other LUTs still use it)
- Add 3 effect struct members: `ColorGradeEffect colorGrade;`, `FalseColorEffect falseColor;`, `PaletteQuantizationEffect paletteQuantization;`
- Effect module includes come transitively through `config/effect_config.h` — no new includes needed in post_effect.h

**Do** in `post_effect.cpp`:
- In `LoadPostEffectShaders`: remove 3 LoadShader calls (`colorGradeShader`, `falseColorShader`, `paletteQuantizationShader`). Remove their `.id != 0` checks from validation. **CRITICAL**: Keep plasma, constellation, interference shader loads untouched.
- In `GetShaderUniformLocations`: remove all GetShaderLocation calls for the 13 color uniform locations.
- In `PostEffectInit`: add 3 Init calls after existing effect inits:
  - `ColorGradeEffectInit(&pe->colorGrade)` — with error handling (`free(pe); return NULL;`)
  - `FalseColorEffectInit(&pe->falseColor, &pe->effects.falseColor)` — with error handling
  - `PaletteQuantizationEffectInit(&pe->paletteQuantization)` — with error handling
- Remove `pe->falseColorLUT = ColorLUTInit(...)` line (now owned by FalseColorEffect)
- In `PostEffectUninit`: replace 3 UnloadShader calls with module Uninit calls. Remove `ColorLUTUninit(pe->falseColorLUT)`.
- Remove `#include "color_lut.h"` only if no other code in post_effect.cpp uses it — check first (other LUTs like constellationPointLUT, constellationLineLUT, plasmaGradientLUT, interferenceColorLUT still exist in PostEffect, so color_lut.h include STAYS)

**Verify**: `cmake.exe --build build` compiles.

#### Task 2.3: Shader Setup Color

**Files**: `src/render/shader_setup_color.cpp` (modify)

**Depends on**: Wave 1 complete

**Do**:
- Remove `#include "color_lut.h"` (module handles LUT internally)
- Replace each Setup function body with a one-line delegate:
  - `SetupColorGrade`: `ColorGradeEffectSetup(&pe->colorGrade, &pe->effects.colorGrade);`
  - `SetupFalseColor`: `FalseColorEffectSetup(&pe->falseColor, &pe->effects.falseColor);`
  - `SetupPaletteQuantization`: `PaletteQuantizationEffectSetup(&pe->paletteQuantization, &pe->effects.paletteQuantization);`

**Verify**: `cmake.exe --build build` compiles.

#### Task 2.4: Shader Setup Dispatch

**Files**: `src/render/shader_setup.cpp` (modify)

**Depends on**: Wave 1 complete

**Do**:
- Update 3 dispatch cases in `GetTransformEffect()`:
  - `TRANSFORM_COLOR_GRADE`: `&pe->colorGradeShader` -> `&pe->colorGrade.shader`
  - `TRANSFORM_FALSE_COLOR`: `&pe->falseColorShader` -> `&pe->falseColor.shader`
  - `TRANSFORM_PALETTE_QUANTIZATION`: `&pe->paletteQuantizationShader` -> `&pe->paletteQuantization.shader`

**Verify**: `cmake.exe --build build` compiles.

#### Task 2.5: Build System

**Files**: `CMakeLists.txt` (modify)

**Depends on**: Wave 1 complete

**Do**: Add 3 source files to `EFFECTS_SOURCES`:
- `src/effects/color_grade.cpp`
- `src/effects/false_color.cpp`
- `src/effects/palette_quantization.cpp`

**Verify**: `cmake.exe --build build` compiles.

---

### Wave 3: Cleanup

#### Task 3.1: Delete Old Config Headers + Stale Includes

**Files**: Delete `src/config/color_grade_config.h`, `src/config/false_color_config.h`, `src/config/palette_quantization_config.h`

**Depends on**: Wave 2 complete

**Do**:
- Delete the 3 config headers
- Grep for stale includes. Expected locations:
  - `src/ui/imgui_effects_color.cpp`: `#include "config/false_color_config.h"`, `#include "config/palette_quantization_config.h"` — remove both (types come through `config/effect_config.h`)
  - `color_grade_config.h` is NOT directly included by imgui_effects_color.cpp or preset.cpp — it comes through effect_config.h

**Verify**: `cmake.exe --build build` compiles with no warnings. No stale includes remain.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] All 3 effects appear in transform order pipeline
- [ ] Enabling each effect renders correctly
- [ ] False Color gradient LUT updates in real-time
- [ ] UI controls modify each effect in real-time
- [ ] Preset save/load preserves all settings (including FalseColor gradient stops)
- [ ] Modulation routes to all registered parameters
- [ ] No stale config includes remain
- [ ] `LoadPostEffectShaders` diff shows ONLY 3 color shaders removed (plasma, constellation, interference untouched)
- [ ] `HALF_RES_EFFECTS` array unchanged
