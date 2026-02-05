# Effect Modules Migration — Batch 6: Graphic

Migrate 6 graphic effects (Toon, Neon Glow, Kuwahara, Halftone, Disco Ball, LEGO Bricks) from monolithic PostEffect into self-contained modules under `src/effects/`.

**Migration plan**: [docs/plans/effect-modules-migration.md](effect-modules-migration.md)

## Effects Summary

All 6 are single-shader effects. None are half-res. Two have time accumulators that move into the module.

| Effect | Shader | Time Accumulator | Special |
|--------|--------|-----------------|---------|
| Toon | `toon.fs` | none | — |
| Neon Glow | `neon_glow.fs` | none | — |
| Kuwahara | `kuwahara.fs` | none | radius cast to int |
| Halftone | `halftone.fs` | `currentHalftoneRotation` (render_pipeline.cpp) | rotation = accumulated + static angle |
| Disco Ball | `disco_ball.fs` | `discoBallAngle` (shader_setup_graphic.cpp) | angle += speed * dt |
| LEGO Bricks | `lego_bricks.fs` | none | — |

## Design

### Module Pattern

Each effect module follows the established pattern (see `src/effects/voronoi.h` as reference):

**Header** (`src/effects/<name>.h`):
- Config struct (moved from `src/config/<name>_config.h`, identical fields)
- Effect struct with `Shader shader`, uniform location `int` fields, and time accumulator `float` if needed
- `bool <Name>EffectInit(<Name>Effect *e)` — loads shader, gets uniform locations
- `void <Name>EffectSetup(<Name>Effect *e, const <Name>Config *cfg, float deltaTime)` — sets uniforms (deltaTime only if effect has time accumulator)
- `void <Name>EffectUninit(<Name>Effect *e)` — unloads shader
- `<Name>Config <Name>ConfigDefault(void)` — returns `Config{}`

**Implementation** (`src/effects/<name>.cpp`):
- Init: `LoadShader(NULL, "shaders/<name>.fs")`, check `.id == 0`, get all uniform locations
- Setup: set resolution via `GetScreenWidth()/GetScreenHeight()`, set all config uniforms. Accumulate time if applicable.
- Uninit: `UnloadShader(e->shader)`
- ConfigDefault: `return <Name>Config{};`

### Effect-Specific Notes

**Halftone**: Module gains a `float rotation` accumulator. Setup accumulates `rotation += cfg->rotationSpeed * deltaTime`, then passes `rotation + cfg->rotationAngle` to shader. Remove `pe->currentHalftoneRotation` from render_pipeline.cpp.

**Disco Ball**: Module gains a `float angle` accumulator. Setup accumulates `angle += cfg->rotationSpeed * deltaTime`, then passes `angle` to shader. Remove `pe->discoBallAngle` from PostEffect struct.

**Kuwahara**: Setup casts `cfg->radius` to `int` before passing as `SHADER_UNIFORM_INT` (matches current `shader_setup_graphic.cpp:45`).

### Uniform Names (verified against shader_setup_graphic.cpp)

**Toon**: `resolution`, `levels`, `edgeThreshold`, `edgeSoftness`, `thicknessVariation`, `noiseScale`

**Neon Glow**: `resolution`, `glowColor` (vec3), `edgeThreshold`, `edgePower`, `glowIntensity`, `glowRadius`, `glowSamples`, `originalVisibility`, `colorMode`, `saturationBoost`, `brightnessBoost`

**Kuwahara**: `resolution`, `radius` (int)

**Halftone**: `resolution`, `dotScale`, `dotSize`, `rotation`

**Disco Ball**: `resolution`, `sphereRadius`, `tileSize`, `sphereAngle`, `bumpHeight`, `reflectIntensity`, `spotIntensity`, `spotFalloff`, `brightnessThreshold`

**LEGO Bricks**: `resolution`, `brickScale`, `studHeight`, `edgeShadow`, `colorThreshold`, `maxBrickSize` (int), `lightAngle`

---

## Tasks

### Wave 1: Create effect module headers and implementations

Six independent files, no overlap.

#### Task 1.1: Toon effect module

**Files** (create): `src/effects/toon.h`, `src/effects/toon.cpp`

**Do**:
- Move `ToonConfig` from `src/config/toon_config.h` (identical fields)
- `ToonEffect` struct: `Shader shader`, 6 uniform location ints (`resolutionLoc`, `levelsLoc`, `edgeThresholdLoc`, `edgeSoftnessLoc`, `thicknessVariationLoc`, `noiseScaleLoc`)
- No time accumulator, no deltaTime parameter on Setup
- Follow `VoronoiEffect` pattern for Init/Setup/Uninit/ConfigDefault

**Verify**: Files created with correct struct and function signatures.

#### Task 1.2: Neon Glow effect module

**Files** (create): `src/effects/neon_glow.h`, `src/effects/neon_glow.cpp`

**Do**:
- Move `NeonGlowConfig` from `src/config/neon_glow_config.h`
- `NeonGlowEffect` struct: `Shader shader`, 11 uniform location ints (see uniform list above)
- Setup builds `float glowColor[3] = {cfg->glowR, cfg->glowG, cfg->glowB}` and passes as VEC3
- No time accumulator

**Verify**: Files created, glowColor vec3 assembly matches `shader_setup_graphic.cpp:20`.

#### Task 1.3: Kuwahara effect module

**Files** (create): `src/effects/kuwahara.h`, `src/effects/kuwahara.cpp`

**Do**:
- Move `KuwaharaConfig` from `src/config/kuwahara_config.h`
- `KuwaharaEffect` struct: `Shader shader`, `resolutionLoc`, `radiusLoc`
- Setup casts `cfg->radius` to `int` before `SetShaderValue(..., SHADER_UNIFORM_INT)`
- No time accumulator

**Verify**: Files created, radius int-cast matches `shader_setup_graphic.cpp:45-47`.

#### Task 1.4: Halftone effect module

**Files** (create): `src/effects/halftone.h`, `src/effects/halftone.cpp`

**Do**:
- Move `HalftoneConfig` from `src/config/halftone_config.h`
- `HalftoneEffect` struct: `Shader shader`, 4 uniform location ints (`resolutionLoc`, `dotScaleLoc`, `dotSizeLoc`, `rotationLoc`), `float rotation` accumulator
- Setup: `e->rotation += cfg->rotationSpeed * deltaTime`, then `float finalRotation = e->rotation + cfg->rotationAngle`, pass `finalRotation` to shader
- Init: set `e->rotation = 0.0f`
- Signature includes `float deltaTime` parameter

**Verify**: rotation accumulation matches current split across render_pipeline.cpp:333 and shader_setup_graphic.cpp:52.

#### Task 1.5: Disco Ball effect module

**Files** (create): `src/effects/disco_ball.h`, `src/effects/disco_ball.cpp`

**Do**:
- Move `DiscoBallConfig` from `src/config/disco_ball_config.h`. Drop the `typedef struct ... DiscoBallConfig` C pattern — use plain `struct DiscoBallConfig` like all other migrated configs.
- `DiscoBallEffect` struct: `Shader shader`, 9 uniform location ints (see uniform list), `float angle` accumulator
- Setup: `e->angle += cfg->rotationSpeed * deltaTime`, pass `e->angle` as `sphereAngle`
- Init: set `e->angle = 0.0f`
- Signature includes `float deltaTime` parameter

**Verify**: angle accumulation matches `shader_setup_graphic.cpp:66-73`.

#### Task 1.6: LEGO Bricks effect module

**Files** (create): `src/effects/lego_bricks.h`, `src/effects/lego_bricks.cpp`

**Do**:
- Move `LegoBricksConfig` from `src/config/lego_bricks_config.h`
- `LegoBricksEffect` struct: `Shader shader`, 7 uniform location ints (see uniform list)
- `maxBrickSize` passed as `SHADER_UNIFORM_INT`
- No time accumulator

**Verify**: Files created, uniform types match `shader_setup_graphic.cpp:86-100`.

---

### Wave 2: Integration (all files are different — no overlap)

#### Task 2.1: Update effect_config.h includes

**Files** (modify): `src/config/effect_config.h`

**Do**:
- Replace 6 config includes with effect module includes:
  - `disco_ball_config.h` → `effects/disco_ball.h`
  - `halftone_config.h` → `effects/halftone.h`
  - `kuwahara_config.h` → `effects/kuwahara.h`
  - `lego_bricks_config.h` → `effects/lego_bricks.h`
  - `neon_glow_config.h` → `effects/neon_glow.h`
  - `toon_config.h` → `effects/toon.h`

**Verify**: `cmake.exe --build build` compiles (configs are identical, just relocated).

#### Task 2.2: Update post_effect.h — replace shader/loc fields with effect structs

**Files** (modify): `src/render/post_effect.h`

**Do**:
- Add 6 includes: `effects/toon.h`, `effects/neon_glow.h`, `effects/kuwahara.h`, `effects/halftone.h`, `effects/disco_ball.h`, `effects/lego_bricks.h`
- Remove from PostEffect struct:
  - Shader fields: `toonShader`, `neonGlowShader`, `kuwaharaShader`, `halftoneShader`, `discoBallShader`, `legoBricksShader`
  - All `toon*Loc` (6), `neonGlow*Loc` (11), `kuwahara*Loc` (2), `halftone*Loc` (4), `discoBall*Loc` (9), `legoBricks*Loc` (7) uniform location ints
  - Time accumulators: `currentHalftoneRotation`, `discoBallAngle`
- Add 6 effect struct fields: `ToonEffect toon`, `NeonGlowEffect neonGlow`, `KuwaharaEffect kuwahara`, `HalftoneEffect halftone`, `DiscoBallEffect discoBall`, `LegoBricksEffect legoBricks`

**Verify**: Header compiles (other files will break until tasks 2.3-2.6 complete).

#### Task 2.3: Update post_effect.cpp — shader loading, loc init, resolution, uninit

**Files** (modify): `src/render/post_effect.cpp`

**Do**:
- **LoadPostEffectShaders**: Remove 6 `LoadShader` calls (toon, neonGlow, kuwahara, halftone, discoBall, legoBricks). Replace with `<Name>EffectInit(&pe-><name>)` calls. Update the validation chain to check `pe-><name>.shader.id != 0` instead of `pe-><name>Shader.id != 0`.
  - CRITICAL: Do NOT remove adjacent non-graphic shader loads (e.g. `synthwaveShader`). Diff carefully — the load block mixes categories.
- **GetShaderLocation block**: Remove all `GetShaderLocation` calls for the 6 effects (toon: 6, neonGlow: 11, kuwahara: 2, halftone: 4, discoBall: 9, legoBricks: 7).
- **SetResolutionUniforms**: Remove 6 `SetShaderValue` resolution calls (toon, neonGlow, halftone, kuwahara, discoBall, legoBricks). Each module now sets resolution in its own Setup.
- **PostEffectUninit**: Replace 6 `UnloadShader(pe-><name>Shader)` with `<Name>EffectUninit(&pe-><name>)`.

**Verify**: Compiles after all Wave 2 tasks complete.

#### Task 2.4: Update shader_setup_graphic.cpp and .h — delegate to modules

**Files** (modify): `src/render/shader_setup_graphic.cpp`, `src/render/shader_setup_graphic.h`

**Do**:
- Each Setup function changes from directly calling `SetShaderValue` to calling the module's Setup:
  - `SetupToon(PostEffect *pe)` → calls `ToonEffectSetup(&pe->toon, &pe->effects.toon)`
  - `SetupNeonGlow(PostEffect *pe)` → calls `NeonGlowEffectSetup(&pe->neonGlow, &pe->effects.neonGlow)`
  - `SetupKuwahara(PostEffect *pe)` → calls `KuwaharaEffectSetup(&pe->kuwahara, &pe->effects.kuwahara)`
  - `SetupHalftone(PostEffect *pe)` → calls `HalftoneEffectSetup(&pe->halftone, &pe->effects.halftone, pe->currentDeltaTime)`
  - `SetupDiscoBall(PostEffect *pe)` → calls `DiscoBallEffectSetup(&pe->discoBall, &pe->effects.discoBall, pe->currentDeltaTime)`
  - `SetupLegoBricks(PostEffect *pe)` → calls `LegoBricksEffectSetup(&pe->legoBricks, &pe->effects.legoBricks)`
- Add includes for 6 effect headers in `.cpp`
- Header unchanged (function signatures stay the same since shader_setup.cpp calls them via PostEffect*)

**Verify**: Compiles.

#### Task 2.5: Update shader_setup.cpp dispatcher — use module shader refs

**Files** (modify): `src/render/shader_setup.cpp`

**Do**:
- Update 6 dispatch cases to reference module shader:
  - `TRANSFORM_TOON`: `&pe->toonShader` → `&pe->toon.shader`
  - `TRANSFORM_NEON_GLOW`: `&pe->neonGlowShader` → `&pe->neonGlow.shader`
  - `TRANSFORM_KUWAHARA`: `&pe->kuwaharaShader` → `&pe->kuwahara.shader`
  - `TRANSFORM_HALFTONE`: `&pe->halftoneShader` → `&pe->halftone.shader`
  - `TRANSFORM_DISCO_BALL`: `&pe->discoBallShader` → `&pe->discoBall.shader`
  - `TRANSFORM_LEGO_BRICKS`: `&pe->legoBricksShader` → `&pe->legoBricks.shader`

**Verify**: Compiles.

#### Task 2.6: Update render_pipeline.cpp — remove halftone time accumulator

**Files** (modify): `src/render/render_pipeline.cpp`

**Do**:
- Remove `pe->currentHalftoneRotation += pe->effects.halftone.rotationSpeed * dt;` (line 333). The module's Setup handles accumulation now.

**Verify**: Compiles.

#### Task 2.7: Update imgui_effects_graphic.cpp — remove stale config includes

**Files** (modify): `src/ui/imgui_effects_graphic.cpp`

**Do**:
- Remove `#include "config/disco_ball_config.h"` (line 2)
- Remove `#include "config/halftone_config.h"` (line 4)
- These types now come through `config/effect_config.h` → `effects/disco_ball.h` / `effects/halftone.h`

**Verify**: Compiles.

#### Task 2.8: Update CMakeLists.txt — add new source files

**Files** (modify): `CMakeLists.txt`

**Do**:
- Add 6 entries to the EFFECT_MODULE_SOURCES section (alphabetical):
  - `src/effects/disco_ball.cpp`
  - `src/effects/halftone.cpp`
  - `src/effects/kuwahara.cpp`
  - `src/effects/lego_bricks.cpp`
  - `src/effects/neon_glow.cpp`
  - `src/effects/toon.cpp`

**Verify**: `cmake.exe -G Ninja -B build -S .` regenerates successfully.

#### Task 2.9: Delete old config headers

**Files** (delete):
- `src/config/toon_config.h`
- `src/config/neon_glow_config.h`
- `src/config/kuwahara_config.h`
- `src/config/halftone_config.h`
- `src/config/disco_ball_config.h`
- `src/config/lego_bricks_config.h`

**Do**: Delete all 6 files. Grep for any remaining includes of these filenames — should find none in active source (only docs/plans/archive references).

**Verify**: `cmake.exe --build build` full build succeeds with zero warnings.

---

## Final Verification

- [ ] Build succeeds: `cmake.exe -G Ninja -B build -S . -DCMAKE_BUILD_TYPE=Release && cmake.exe --build build`
- [ ] No stale includes: `grep -r "toon_config.h\|neon_glow_config.h\|kuwahara_config.h\|halftone_config.h\|disco_ball_config.h\|lego_bricks_config.h" src/`
- [ ] No stale field refs: `grep -r "pe->toonShader\|pe->neonGlowShader\|pe->kuwaharaShader\|pe->halftoneShader\|pe->discoBallShader\|pe->legoBricksShader" src/`
- [ ] No stale time accumulators: `grep -r "currentHalftoneRotation\|discoBallAngle" src/` returns nothing
- [ ] Diff `LoadPostEffectShaders` to confirm ONLY graphic batch shaders removed (not synthwave, plasma, etc.)
- [ ] ConfigDefault returns ONLY `return Config{};` for all 6 modules
