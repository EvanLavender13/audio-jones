# Effect Modules Migration — Batch 4: Motion

Migrate 6 motion effects from monolithic PostEffect into self-contained modules. Each module owns its shader, uniform locations, time accumulators, and config struct. Follows the pattern established in Batches 0–3.

**Migration plan**: [docs/plans/effect-modules-migration.md](effect-modules-migration.md)

## Effects

| Effect | Config | Time Accumulators | Half-Res | Notes |
|--------|--------|-------------------|----------|-------|
| Infinite Zoom | `InfiniteZoomConfig` | `infiniteZoomTime` (speed-scaled) | No | Speed in RenderPipelineApplyFeedback |
| Radial Streak | `RadialStreakConfig` | None | Yes (`HALF_RES_EFFECTS[]`) | Listed in half-res array |
| Droste Zoom | `DrosteZoomConfig` | `drosteZoomTime` (speed-scaled) | No | Speed in RenderPipelineApplyFeedback |
| Density Wave Spiral | `DensityWaveSpiralConfig` | `densityWaveSpiralRotation`, `densityWaveSpiralGlobalRotation` | No | Two rotation accumulators in RenderPipelineApplyOutput |
| Shake | `ShakeConfig` | `shakeTime` (accumulated in Setup) | No | Time accumulation inside SetupShake, not render_pipeline |
| Relativistic Doppler | `RelativisticDopplerConfig` | None | No | Resolution uniform set in GetShaderUniformLocations |

## Design

### Module Pattern (same as Batch 0–3)

Each effect creates:
- **Header** (`src/effects/<name>.h`): Config struct (with member initializer defaults), Effect struct (shader + uniform locs + accumulators), Init/Setup/Uninit/ConfigDefault functions
- **Source** (`src/effects/<name>.cpp`): Load shader, cache uniform locations, accumulate time in Setup, set all uniforms

### Config Structs

Move verbatim from existing `src/config/*_config.h` into the module header. Use member initializer defaults. `ConfigDefault` returns `EffectConfig{}`.

### Shader Uniform Names (verified against .fs files)

**infinite_zoom.fs**: `time`, `zoomDepth`, `layers`, `spiralAngle`, `spiralTwist`
**radial_streak.fs**: `samples`, `streakLength`
**droste_zoom.fs**: `time`, `scale`, `spiralAngle`, `shearCoeff`, `innerRadius`, `branches`
**density_wave_spiral.fs**: `center`, `aspect`, `tightness`, `rotationAccum`, `globalRotationAccum`, `thickness`, `ringCount`, `falloff`
**shake.fs**: `resolution`, `time`, `intensity`, `samples`, `rate`, `gaussian`
**relativistic_doppler.fs**: `resolution`, `velocity`, `center`, `aberration`, `colorShift`, `headlight`

### Effect Structs

Each struct stores: `Shader shader`, one `int *Loc` per uniform, and any time accumulators (`float time`, `float rotation`, etc.).

**InfiniteZoomEffect**: shader, 5 uniform locs, `float time`
**RadialStreakEffect**: shader, 2 uniform locs, no accumulators
**DrosteZoomEffect**: shader, 6 uniform locs, `float time`
**DensityWaveSpiralEffect**: shader, 8 uniform locs + `resolutionLoc`, `float rotation`, `float globalRotation`
**ShakeEffect**: shader, 5 uniform locs + `resolutionLoc`, `float time`
**RelativisticDopplerEffect**: shader, 5 uniform locs + `resolutionLoc`

### Time Accumulation Migration

| Accumulator | Current Location | Migrates To |
|-------------|-----------------|-------------|
| `pe->infiniteZoomTime` | `RenderPipelineApplyFeedback` (line 229) | `InfiniteZoomEffect.time` in `InfiniteZoomEffectSetup` — `e->time += cfg->speed * deltaTime` |
| `pe->drosteZoomTime` | `RenderPipelineApplyFeedback` (line 230) | `DrosteZoomEffect.time` in `DrosteZoomEffectSetup` — `e->time += cfg->speed * deltaTime` |
| `pe->densityWaveSpiralRotation` | `RenderPipelineApplyOutput` (line 336) | `DensityWaveSpiralEffect.rotation` in Setup — `e->rotation += cfg->rotationSpeed * deltaTime` |
| `pe->densityWaveSpiralGlobalRotation` | `RenderPipelineApplyOutput` (line 338) | `DensityWaveSpiralEffect.globalRotation` in Setup — `e->globalRotation += cfg->globalRotationSpeed * deltaTime` |
| `pe->shakeTime` | `SetupShake` (shader_setup_motion.cpp line 70) | `ShakeEffect.time` in Setup — `e->time += deltaTime` |

### Resolution Uniforms

Some shaders declare `uniform vec2 resolution` that the current code sets outside Setup:
- **shake.fs**: Has `resolution` but NOT set in `SetupShake`. Check if bound via `GetShaderUniformLocations` general resolution section.
- **relativistic_doppler.fs**: Set in `GetShaderUniformLocations` (line 650–651) and bound in a resolution batch (line 801–802).
- **density_wave_spiral.fs**: No `resolution` uniform.

Module Setup functions that need resolution use `GetScreenWidth()`/`GetScreenHeight()` (same pattern as VoronoiEffectSetup).

---

## Tasks

### Wave 1: Create 6 Effect Module Headers + Sources

No file overlap between modules. All 6 run in parallel.

#### Task 1.1: Infinite Zoom Module

**Files (create)**: `src/effects/infinite_zoom.h`, `src/effects/infinite_zoom.cpp`
**Creates**: `InfiniteZoomEffect`, `InfiniteZoomConfig`, Init/Setup/Uninit/ConfigDefault

**Do**:
- Move `InfiniteZoomConfig` from `src/config/infinite_zoom_config.h` into header with member initializer defaults
- Effect struct: `shader`, `timeLoc`, `zoomDepthLoc`, `layersLoc`, `spiralAngleLoc`, `spiralTwistLoc`, `float time`
- Init: `LoadShader(NULL, "shaders/infinite_zoom.fs")`, cache 5 uniform locs, `e->time = 0.0f`
- Setup: `e->time += cfg->speed * deltaTime`, set all 5 uniforms
- Uninit: `UnloadShader`
- ConfigDefault: return `InfiniteZoomConfig{}`
- Follow `sine_warp.h`/`.cpp` as pattern

**Verify**: Compiles.

#### Task 1.2: Radial Streak Module

**Files (create)**: `src/effects/radial_streak.h`, `src/effects/radial_streak.cpp`
**Creates**: `RadialStreakEffect`, `RadialStreakConfig`, Init/Setup/Uninit/ConfigDefault

**Do**:
- Move `RadialStreakConfig` from `src/config/radial_streak_config.h` into header
- Effect struct: `shader`, `samplesLoc`, `streakLengthLoc` — no time accumulators
- Init: `LoadShader(NULL, "shaders/radial_streak.fs")`, cache 2 uniform locs
- Setup: set 2 uniforms (no deltaTime needed)
- Uninit: `UnloadShader`
- ConfigDefault: return `RadialStreakConfig{}`

**Verify**: Compiles.

#### Task 1.3: Droste Zoom Module

**Files (create)**: `src/effects/droste_zoom.h`, `src/effects/droste_zoom.cpp`
**Creates**: `DrosteZoomEffect`, `DrosteZoomConfig`, Init/Setup/Uninit/ConfigDefault

**Do**:
- Move `DrosteZoomConfig` from `src/config/droste_zoom_config.h` into header
- Effect struct: `shader`, `timeLoc`, `scaleLoc`, `spiralAngleLoc`, `shearCoeffLoc`, `innerRadiusLoc`, `branchesLoc`, `float time`
- Init: `LoadShader(NULL, "shaders/droste_zoom.fs")`, cache 6 uniform locs, `e->time = 0.0f`
- Setup: `e->time += cfg->speed * deltaTime`, set all 6 uniforms
- Uninit: `UnloadShader`
- ConfigDefault: return `DrosteZoomConfig{}`

**Verify**: Compiles.

#### Task 1.4: Density Wave Spiral Module

**Files (create)**: `src/effects/density_wave_spiral.h`, `src/effects/density_wave_spiral.cpp`
**Creates**: `DensityWaveSpiralEffect`, `DensityWaveSpiralConfig`, Init/Setup/Uninit/ConfigDefault

**Do**:
- Move `DensityWaveSpiralConfig` from `src/config/density_wave_spiral_config.h` into header. Drop the `typedef struct ... DensityWaveSpiralConfig` wrapper — use plain `struct DensityWaveSpiralConfig` with member initializer defaults (match other modules)
- Effect struct: `shader`, `centerLoc`, `aspectLoc`, `tightnessLoc`, `rotationAccumLoc`, `globalRotationAccumLoc`, `thicknessLoc`, `ringCountLoc`, `falloffLoc`, `float rotation`, `float globalRotation`
- Init: load shader, cache 8 uniform locs, zero both rotation accumulators
- Setup: accumulate both rotations (`e->rotation += cfg->rotationSpeed * deltaTime`, `e->globalRotation += cfg->globalRotationSpeed * deltaTime`), pack vec2 arrays for center/aspect, set all 8 uniforms
- Uninit: `UnloadShader`
- ConfigDefault: return `DensityWaveSpiralConfig{}`

**Verify**: Compiles.

#### Task 1.5: Shake Module

**Files (create)**: `src/effects/shake.h`, `src/effects/shake.cpp`
**Creates**: `ShakeEffect`, `ShakeConfig`, Init/Setup/Uninit/ConfigDefault

**Do**:
- Move `ShakeConfig` from `src/config/shake_config.h` into header
- Effect struct: `shader`, `resolutionLoc`, `timeLoc`, `intensityLoc`, `samplesLoc`, `rateLoc`, `gaussianLoc`, `float time`
- Init: `LoadShader(NULL, "shaders/shake.fs")`, cache 6 uniform locs, `e->time = 0.0f`
- Setup: `e->time += deltaTime`, set resolution via `GetScreenWidth()`/`GetScreenHeight()`, set all 5 config uniforms. Cast `samples` to int before sending. Convert `gaussian` bool to int.
- Uninit: `UnloadShader`
- ConfigDefault: return `ShakeConfig{}`

**Verify**: Compiles.

#### Task 1.6: Relativistic Doppler Module

**Files (create)**: `src/effects/relativistic_doppler.h`, `src/effects/relativistic_doppler.cpp`
**Creates**: `RelativisticDopplerEffect`, `RelativisticDopplerConfig`, Init/Setup/Uninit/ConfigDefault

**Do**:
- Move `RelativisticDopplerConfig` from `src/config/relativistic_doppler_config.h` into header
- Effect struct: `shader`, `resolutionLoc`, `velocityLoc`, `centerLoc`, `aberrationLoc`, `colorShiftLoc`, `headlightLoc` — no time accumulators
- Init: `LoadShader(NULL, "shaders/relativistic_doppler.fs")`, cache 6 uniform locs
- Setup: set resolution via `GetScreenWidth()`/`GetScreenHeight()`, pack vec2 for center, set all 5 config uniforms
- Uninit: `UnloadShader`
- ConfigDefault: return `RelativisticDopplerConfig{}`

**Verify**: Compiles.

---

### Wave 2: Integration (6 parallel tasks, no file overlap)

Depends on: Wave 1 complete

#### Task 2.1: Update post_effect.h

**Files (modify)**: `src/render/post_effect.h`

**Do**:
- Add 6 includes: `effects/infinite_zoom.h`, `effects/radial_streak.h`, `effects/droste_zoom.h`, `effects/density_wave_spiral.h`, `effects/shake.h`, `effects/relativistic_doppler.h`
- Add 6 effect struct fields: `InfiniteZoomEffect infiniteZoom_`, `RadialStreakEffect radialStreak_`, `DrosteZoomEffect drosteZoom_`, `DensityWaveSpiralEffect densityWaveSpiral_`, `ShakeEffect shake_`, `RelativisticDopplerEffect relativisticDoppler_`
  - Use trailing underscore to avoid name collision with existing config field names (`pe->effects.infiniteZoom` vs `pe->infiniteZoom_`)
  - OR check naming pattern from prior batches — existing modules use plain names like `pe->sineWarp`, `pe->voronoi`. Since config access is `pe->effects.infiniteZoom`, using `pe->infiniteZoom` for the effect struct is safe (no collision). **Use plain names: `pe->infiniteZoom`, `pe->radialStreak`, etc.** — WAIT, `pe->infiniteZoomShader` already exists. After migration, the old Shader field gets replaced by the module struct. Remove old fields.
- **Remove** from PostEffect struct:
  - Shader fields: `infiniteZoomShader`, `radialStreakShader`, `drosteZoomShader`, `densityWaveSpiralShader`, `shakeShader`, `relativisticDopplerShader`
  - All associated `int *Loc` fields (28 location ints for these 6 effects)
  - Time accumulators: `infiniteZoomTime`, `drosteZoomTime`, `densityWaveSpiralRotation`, `densityWaveSpiralGlobalRotation`, `shakeTime`
- **Add** 6 effect module fields: `InfiniteZoomEffect infiniteZoom`, `RadialStreakEffect radialStreak`, `DrosteZoomEffect drosteZoom`, `DensityWaveSpiralEffect densityWaveSpiral`, `ShakeEffect shake`, `RelativisticDopplerEffect relativisticDoppler`
  - WAIT: `shake` collides with nothing since config is `pe->effects.shake`. Safe.

**Verify**: Compiles after Wave 2 tasks complete together.

#### Task 2.2: Update post_effect.cpp

**Files (modify)**: `src/render/post_effect.cpp`

**Do**:
- In `LoadPostEffectShaders`: remove 6 `LoadShader` calls and their `.id != 0` validation checks
- In `GetShaderUniformLocations`: remove all `GetShaderLocation` calls for the 6 effects (approximately 28 lines)
- In `PostEffectInit` (after existing module inits): add 6 `EffectInit` calls with `if (!XxxEffectInit(&pe->xxx)) { free(pe); return NULL; }` — NO null-checks (Pitfall #7)
- In `PostEffectUninit`: replace 6 `UnloadShader(pe->xxxShader)` with `XxxEffectUninit(&pe->xxx)`
- In `PostEffectClearFeedback`: if any motion time resets exist, remove them (currently resets `infiniteZoomTime`, `drosteZoomTime`, `shakeTime` — no longer needed because modules own time)
  - Actually, check if ClearFeedback resets these. Looking at the code: the resets are in `render_pipeline.cpp` line ~831. If those resets exist, they need to move to the module or be handled there. See Task 2.3.

**Verify**: Compiles.

#### Task 2.3: Update render_pipeline.cpp

**Files (modify)**: `src/render/render_pipeline.cpp`

**Do**:
- In `RenderPipelineApplyFeedback`: remove time accumulation lines for `infiniteZoomTime` (line 229) and `drosteZoomTime` (line 230)
- In `RenderPipelineApplyOutput`: remove time accumulation lines for `densityWaveSpiralRotation` (lines 336–337) and `densityWaveSpiralGlobalRotation` (lines 338–339)
- Remove any time reset lines in the clear/reset section (around line 831): `pe->infiniteZoomTime = 0.0f`, `pe->drosteZoomTime = 0.0f`, `pe->shakeTime = 0.0f`
  - These resets happen in `RenderPipelineApplyFeedback` initial section. After migration, module Setup handles accumulation, so these resets are dead code.
- In `GetTransformEffect` dispatch: update 6 cases to use module struct:
  - `TRANSFORM_INFINITE_ZOOM`: `{&pe->infiniteZoom.shader, SetupInfiniteZoom, &pe->effects.infiniteZoom.enabled}`
  - `TRANSFORM_RADIAL_STREAK`: `{&pe->radialStreak.shader, SetupRadialStreak, &pe->effects.radialStreak.enabled}`
  - `TRANSFORM_DROSTE_ZOOM`: `{&pe->drosteZoom.shader, SetupDrosteZoom, &pe->effects.drosteZoom.enabled}`
  - `TRANSFORM_DENSITY_WAVE_SPIRAL`: `{&pe->densityWaveSpiral.shader, SetupDensityWaveSpiral, &pe->effects.densityWaveSpiral.enabled}`
  - `TRANSFORM_SHAKE`: `{&pe->shake.shader, SetupShake, &pe->effects.shake.enabled}`
  - `TRANSFORM_RELATIVISTIC_DOPPLER`: `{&pe->relativisticDoppler.shader, SetupRelativisticDoppler, &pe->effects.relativisticDoppler.enabled}`

**Verify**: Compiles.

#### Task 2.4: Update shader_setup_motion.cpp / .h

**Files (modify)**: `src/render/shader_setup_motion.cpp`, `src/render/shader_setup_motion.h`

**Do**:
- Replace all 6 Setup function bodies with one-line delegation to module Setup (same pattern as `shader_setup_warp.cpp`):
  ```
  void SetupInfiniteZoom(PostEffect *pe) {
    InfiniteZoomEffectSetup(&pe->infiniteZoom, &pe->effects.infiniteZoom, pe->currentDeltaTime);
  }
  ```
- Add `#include` for each effect module header in the `.cpp`
- Remove old `#include "post_effect.h"` if no longer needed (keep if Setup functions still take `PostEffect*`)
- Header unchanged (function signatures stay the same)

**Verify**: Compiles.

#### Task 2.5: Delete old config headers + update includes

**Files (delete)**: `src/config/infinite_zoom_config.h`, `src/config/radial_streak_config.h`, `src/config/droste_zoom_config.h`, `src/config/density_wave_spiral_config.h`, `src/config/shake_config.h`, `src/config/relativistic_doppler_config.h`

**Files (modify)**: `src/config/effect_config.h`

**Do**:
- Delete 6 config header files
- In `effect_config.h`: replace 6 `#include "*_config.h"` with `#include "effects/infinite_zoom.h"`, etc. (same pattern as Batch 1–3)
- Grep for any other files including the deleted headers. Known: `effect_config.h`. Check `preset.cpp`, `imgui_effects_motion.cpp`, `param_registry.cpp` for stale includes.

**Verify**: Compiles.

#### Task 2.6: Update CMakeLists.txt

**Files (modify)**: `CMakeLists.txt`

**Do**:
- Add 6 source files to the effects source group: `src/effects/infinite_zoom.cpp`, `src/effects/radial_streak.cpp`, `src/effects/droste_zoom.cpp`, `src/effects/density_wave_spiral.cpp`, `src/effects/shake.cpp`, `src/effects/relativistic_doppler.cpp`

**Verify**: Compiles.

---

## Wave 2 File Overlap Check

| File | Tasks |
|------|-------|
| `src/render/post_effect.h` | 2.1 only |
| `src/render/post_effect.cpp` | 2.2 only |
| `src/render/render_pipeline.cpp` | 2.3 only |
| `src/render/shader_setup_motion.cpp` | 2.4 only |
| `src/render/shader_setup_motion.h` | 2.4 only |
| `src/config/effect_config.h` | 2.5 only |
| `src/config/*_config.h` (6 deletes) | 2.5 only |
| `CMakeLists.txt` | 2.6 only |

No overlap. All 6 Wave 2 tasks can run in parallel.

---

## Pitfall Reminders (from Batches 1–3)

1. Do NOT include deleted `config/*_config.h` headers — modules define config inline
2. `PostEffectInit` returns `PostEffect*` not `bool` — use `free(pe); return NULL;` on failure
3. Grep for stale config includes in all touched files
4. Remove time accumulators from `render_pipeline.cpp` — module Setup handles them
5. Verify uniform names against actual `.fs` files (verified above)
6. Do NOT add null-checks to Init/Setup/Uninit (Pitfall #7)
7. Use `shader.id == 0` pattern for shader validation, not `IsShaderValid()` (Pitfall #8)
8. ConfigDefault returns `EffectConfig{}` — member initializers provide defaults (Pitfall #9)

---

## Final Verification

- [ ] `cmake.exe -G Ninja -B build -S . -DCMAKE_BUILD_TYPE=Release && cmake.exe --build build` succeeds with no warnings
- [ ] 6 old config headers deleted
- [ ] No stale `#include` of deleted headers (grep: `infinite_zoom_config\|radial_streak_config\|droste_zoom_config\|density_wave_spiral_config\|shake_config\|relativistic_doppler_config`)
- [ ] No stale `pe->infiniteZoomShader`, `pe->radialStreakShader`, etc. references
- [ ] No stale `pe->infiniteZoomTime`, `pe->drosteZoomTime`, `pe->shakeTime`, `pe->densityWaveSpiralRotation`, `pe->densityWaveSpiralGlobalRotation` references
- [ ] Transform dispatch returns module shader pointers
- [ ] HALF_RES_EFFECTS array still includes `TRANSFORM_RADIAL_STREAK` (no change needed — dispatch handles it)
- [ ] Update `docs/plans/effect-modules-migration.md` — mark Batch 4 complete
