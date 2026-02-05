# Effect Modules Batch 10: Generators

Migrate 3 generator effects (constellation, plasma, interference) from monolithic PostEffect into self-contained modules. Generators keep their hardcoded render passes in `render_pipeline.cpp` — this batch extracts shader, uniform, LUT, and time-accumulator state into module structs without changing the render pass structure.

**Research**: Generator-blend-modes refactor deferred to a separate feature.

## Design

### Unique Aspects (vs Batches 0–9)

Generators differ from transform effects:

- **No transform dispatch**: Generators have hardcoded `RenderPass()` calls in `render_pipeline.cpp` (lines 333–361), not `GetTransformEffect` entries in `shader_setup.cpp`
- **No `TransformEffectType` enum entries**: No `TRANSFORM_CONSTELLATION` etc.
- **No `IsTransformEnabled` / `GetTransformCategory` cases**: No presence in the reorder UI
- **ColorLUT ownership**: Each module owns its `ColorLUT*` pointers (allocated in Init, freed in Uninit). Follow `FalseColorEffect` pattern.
- **Time accumulators move into module**: `constellationAnimPhase`, `plasmaAnimPhase`, etc. become fields on the effect struct. Setup accumulates them.
- **Lissajous computation in Setup**: Interference's `DualLissajousUpdateCircular` call moves from `shader_setup_generators.cpp` into the module's Setup function.

### Module Structs

#### ConstellationEffect

```
Fields:
  shader            Shader
  pointLUT          ColorLUT*
  lineLUT           ColorLUT*
  animPhase         float         (time accumulator)
  radialPhase       float         (time accumulator)
  resolutionLoc     int           (for SetResolutionUniforms)
  gridScaleLoc      int
  wanderAmpLoc      int
  radialFreqLoc     int
  radialAmpLoc      int
  pointSizeLoc      int
  pointBrightnessLoc int
  lineThicknessLoc  int
  maxLineLenLoc     int
  lineOpacityLoc    int
  interpolateLineColorLoc int
  animPhaseLoc      int
  radialPhaseLoc    int
  pointLUTLoc       int
  lineLUTLoc        int
```

Init signature: `bool ConstellationEffectInit(ConstellationEffect* e, const ConstellationConfig* cfg)`
- Takes config pointer to initialize ColorLUT from gradient configs (same as FalseColor)

Setup signature: `void ConstellationEffectSetup(ConstellationEffect* e, const ConstellationConfig* cfg, float deltaTime)`
- Accumulates `animPhase` and `radialPhase`
- Updates LUTs, sets all uniforms

#### PlasmaEffect

```
Fields:
  shader            Shader
  gradientLUT       ColorLUT*
  animPhase         float         (time accumulator)
  driftPhase        float         (time accumulator)
  flickerTime       float         (time accumulator)
  resolutionLoc     int           (for SetResolutionUniforms)
  boltCountLoc      int
  layerCountLoc     int
  octavesLoc        int
  falloffTypeLoc    int
  driftAmountLoc    int
  displacementLoc   int
  glowRadiusLoc     int
  coreBrightnessLoc int
  flickerAmountLoc  int
  animPhaseLoc      int
  driftPhaseLoc     int
  flickerTimeLoc    int
  gradientLUTLoc    int
```

Init signature: `bool PlasmaEffectInit(PlasmaEffect* e, const PlasmaConfig* cfg)`

Setup signature: `void PlasmaEffectSetup(PlasmaEffect* e, const PlasmaConfig* cfg, float deltaTime)`
- Accumulates `animPhase += cfg->animSpeed * deltaTime`
- Accumulates `driftPhase += cfg->driftSpeed * deltaTime`
- Accumulates `flickerTime += deltaTime`

#### InterferenceEffect

```
Fields:
  shader            Shader
  colorLUT          ColorLUT*
  time              float         (time accumulator)
  resolutionLoc     int           (for SetResolutionUniforms)
  timeLoc           int
  sourcesLoc        int
  phasesLoc         int
  sourceCountLoc    int
  waveFreqLoc       int
  falloffTypeLoc    int
  falloffStrengthLoc int
  boundariesLoc     int
  reflectionGainLoc int
  visualModeLoc     int
  contourCountLoc   int
  visualGainLoc     int
  chromaSpreadLoc   int
  colorModeLoc      int
  colorLUTLoc       int
```

Init signature: `bool InterferenceEffectInit(InterferenceEffect* e, const InterferenceConfig* cfg)`

Setup signature: `void InterferenceEffectSetup(InterferenceEffect* e, InterferenceConfig* cfg, float deltaTime)`
- Non-const cfg: `DualLissajousUpdateCircular` mutates `cfg->lissajous.phase`
- Accumulates `time += cfg->waveSpeed * deltaTime`
- Computes Lissajous source positions and per-source phase offsets internally

### Files Changed

| File | Action |
|------|--------|
| `src/effects/constellation.h` | **Create** — Config stays in `src/config/constellation_config.h`; this defines `ConstellationEffect` struct + lifecycle |
| `src/effects/constellation.cpp` | **Create** — Init, Setup, Uninit, ConfigDefault |
| `src/effects/plasma.h` | **Create** |
| `src/effects/plasma.cpp` | **Create** |
| `src/effects/interference.h` | **Create** |
| `src/effects/interference.cpp` | **Create** |
| `src/render/post_effect.h` | **Modify** — Add includes, add 3 effect struct members, remove all generator-specific fields (shaders, uniform locs, LUTs, time accumulators) |
| `src/render/post_effect.cpp` | **Modify** — Replace shader loads/GetShaderLocation/LUT init with module Init calls; replace Uninit calls; remove generator lines from `SetResolutionUniforms`; update `LoadPostEffectShaders` and validation |
| `src/render/render_pipeline.cpp` | **Modify** — Replace `pe->constellationAnimPhase +=` with direct `RenderPass(pe, src, ..., pe->constellation.shader, SetupConstellation)` (Setup handles accumulation); remove time accumulator lines |
| `src/render/shader_setup_generators.h` | **Modify** — Keep declarations, update to delegate pattern |
| `src/render/shader_setup_generators.cpp` | **Modify** — Replace direct `pe->` field access with module Setup calls |
| `src/ui/imgui_effects_generators.cpp` | **Modify** — Replace `#include "config/constellation_config.h"` etc. with `#include "effects/constellation.h"` etc. |
| `src/config/preset.cpp` | **Modify** — Replace `#include "config/interference_config.h"` with `#include "effects/interference.h"` (NLOHMANN macros reference the config types, which are now reachable through the effect headers) |
| `CMakeLists.txt` | **Modify** — Add 3 new .cpp files to `EFFECTS_SOURCES` |

### Config Headers

The 3 config headers (`constellation_config.h`, `plasma_config.h`, `interference_config.h`) are **deleted** after migration. Config structs move into the effect module headers. This matches the established pattern — effect modules define config inline.

### What Stays Unchanged

- Generator shader files (`shaders/constellation.fs`, `shaders/plasma.fs`, `shaders/interference.fs`) — no shader modifications
- `param_registry.cpp` — PARAM_TABLE entries reference `EffectConfig.constellation.*` etc., which still exist in `effect_config.h`
- `effect_config.h` — EffectConfig members remain; includes change from config headers to effect headers
- Preset serialization logic (to_json/from_json lines) — stays, just the NLOHMANN macro source type definitions move

---

## Tasks

### Wave 1: Create Effect Module Headers

#### Task 1.1: Create constellation.h

**Files**: `src/effects/constellation.h` (create)
**Creates**: `ConstellationEffect` struct and lifecycle declarations

**Do**:
- Define `ConstellationConfig` struct (move from `constellation_config.h`, keep identical fields and defaults)
- Define `ConstellationEffect` struct per design above
- Declare `ConstellationEffectInit`, `ConstellationEffectSetup`, `ConstellationEffectUninit`, `ConstellationConfigDefault`
- Include `raylib.h`, `render/color_config.h`, `<stdbool.h>`
- Forward-declare `ColorLUT` as `typedef struct ColorLUT ColorLUT;`
- Init takes `(ConstellationEffect* e, const ConstellationConfig* cfg)` — needs cfg for LUT init
- Setup takes `(ConstellationEffect* e, const ConstellationConfig* cfg, float deltaTime)`

**Verify**: Header compiles when included from a .cpp.

#### Task 1.2: Create plasma.h

**Files**: `src/effects/plasma.h` (create)
**Creates**: `PlasmaEffect` struct and lifecycle declarations

**Do**:
- Move `PlasmaConfig` from `plasma_config.h`
- Define `PlasmaEffect` per design above
- Forward-declare `ColorLUT`
- Init takes `(PlasmaEffect* e, const PlasmaConfig* cfg)`
- Setup takes `(PlasmaEffect* e, const PlasmaConfig* cfg, float deltaTime)`

**Verify**: Header compiles.

#### Task 1.3: Create interference.h

**Files**: `src/effects/interference.h` (create)
**Creates**: `InterferenceEffect` struct and lifecycle declarations

**Do**:
- Move `InterferenceConfig` from `interference_config.h`
- Define `InterferenceEffect` per design above
- Forward-declare `ColorLUT`
- Include `config/dual_lissajous_config.h` (InterferenceConfig depends on DualLissajousConfig)
- Init takes `(InterferenceEffect* e, const InterferenceConfig* cfg)`
- Setup takes `(InterferenceEffect* e, InterferenceConfig* cfg, float deltaTime)` — non-const cfg because Lissajous mutates phase

**Verify**: Header compiles.

---

### Wave 2: Parallel Implementation

All tasks in this wave touch different files — no overlap.

#### Task 2.1: Implement constellation.cpp

**Files**: `src/effects/constellation.cpp` (create)
**Depends on**: Wave 1 complete

**Do**:
- Follow `false_color.cpp` pattern for ColorLUT ownership
- Init: LoadShader, get all uniform locations, allocate 2 ColorLUTs from cfg gradients, init time accumulators to 0
- Setup: accumulate `animPhase` and `radialPhase`, call `ColorLUTUpdate` on both LUTs, set all uniforms
- Uninit: UnloadShader, ColorLUTUninit both LUTs
- ConfigDefault: `return ConstellationConfig{};`
- Use `#include <stddef.h>` (not stdlib.h — pitfall #12)

**Verify**: `cmake.exe --build build` compiles.

#### Task 2.2: Implement plasma.cpp

**Files**: `src/effects/plasma.cpp` (create)
**Depends on**: Wave 1 complete

**Do**:
- Init: LoadShader, get uniform locations, allocate 1 ColorLUT, init 3 time accumulators to 0
- Setup: accumulate `animPhase`, `driftPhase`, `flickerTime`; update LUT; set all uniforms
- Uninit: UnloadShader, ColorLUTUninit
- ConfigDefault: `return PlasmaConfig{};`
- Use `#include <stddef.h>`

**Verify**: Compiles.

#### Task 2.3: Implement interference.cpp

**Files**: `src/effects/interference.cpp` (create)
**Depends on**: Wave 1 complete

**Do**:
- Init: LoadShader, get uniform locations, allocate 1 ColorLUT, init time to 0
- Setup: accumulate `time`, compute Lissajous source positions via `DualLissajousUpdateCircular` (move from `shader_setup_generators.cpp`), compute per-source phase offsets, update LUT, set all uniforms
- Uninit: UnloadShader, ColorLUTUninit
- ConfigDefault: `return InterferenceConfig{};`
- Use `#include <stddef.h>` and `#include <math.h>` (for TWO_PI constant used in phase computation)

**Verify**: Compiles.

#### Task 2.4: Update post_effect.h and post_effect.cpp

**Files**: `src/render/post_effect.h` (modify), `src/render/post_effect.cpp` (modify)
**Depends on**: Wave 1 complete

**Do** in `post_effect.h`:
- Add includes: `effects/constellation.h`, `effects/plasma.h`, `effects/interference.h`
- Remove all these fields from PostEffect struct:
  - Shaders: `constellationShader`, `plasmaShader`, `interferenceShader`
  - All `constellation*Loc`, `plasma*Loc`, `interference*Loc` uniform location ints (lines 131–175)
  - LUT pointers: `constellationPointLUT`, `constellationLineLUT`, `plasmaGradientLUT`, `interferenceColorLUT`
  - Time accumulators: `constellationAnimPhase`, `constellationRadialPhase`, `plasmaAnimPhase`, `plasmaDriftPhase`, `plasmaFlickerTime`, `interferenceTime`
- Add 3 effect struct members: `ConstellationEffect constellation;`, `PlasmaEffect plasma;`, `InterferenceEffect interference;`
- Remove the `ColorLUT` forward declaration if no other generators use it (check — blendCompositor still needs it, so keep it)

**Do** in `post_effect.cpp`:
- Remove `#include "config/interference_config.h"` (the include is no longer needed here; the effect headers provide the types)
- In `LoadPostEffectShaders`: remove the 3 `LoadShader` calls for generators and their validation checks
- In `GetShaderUniformLocations`: remove all `constellation*Loc`, `plasma*Loc`, `interference*Loc` GetShaderLocation lines
- In `SetResolutionUniforms`: replace the 3 generator resolution lines to use module fields:
  - `pe->constellationShader` → `pe->constellation.shader`, `pe->constellationResolutionLoc` → `pe->constellation.resolutionLoc`
  - Same pattern for plasma and interference
- In `PostEffectInit`: replace LUT init + time accumulator init block (lines 536–547) with 3 module Init calls:
  ```
  if (!ConstellationEffectInit(&pe->constellation, &pe->effects.constellation)) { ... free(pe); return NULL; }
  if (!PlasmaEffectInit(&pe->plasma, &pe->effects.plasma)) { ... free(pe); return NULL; }
  if (!InterferenceEffectInit(&pe->interference, &pe->effects.interference)) { ... free(pe); return NULL; }
  ```
- In `PostEffectUninit`: replace `UnloadShader(pe->constellationShader)`, `ColorLUTUninit(pe->constellationPointLUT)`, `ColorLUTUninit(pe->constellationLineLUT)`, `UnloadShader(pe->plasmaShader)`, `ColorLUTUninit(pe->plasmaGradientLUT)`, `UnloadShader(pe->interferenceShader)`, `ColorLUTUninit(pe->interferenceColorLUT)` with:
  ```
  ConstellationEffectUninit(&pe->constellation);
  PlasmaEffectUninit(&pe->plasma);
  InterferenceEffectUninit(&pe->interference);
  ```

**Verify**: Compiles.

#### Task 2.5: Update render_pipeline.cpp

**Files**: `src/render/render_pipeline.cpp` (modify)
**Depends on**: Wave 1 complete

**Do**:
- In the generator pass section (lines 333–361):
  - Remove time accumulator lines (`pe->constellationAnimPhase +=`, `pe->plasmaAnimPhase +=`, etc.) — Setup handles this now
  - Keep the `RenderPass` calls but update shader references:
    - `pe->constellationShader` → `pe->constellation.shader`
    - `pe->plasmaShader` → `pe->plasma.shader`
    - `pe->interferenceShader` → `pe->interference.shader`
  - Keep `SetupConstellation`, `SetupPlasma`, `SetupInterference` function names (they still exist in `shader_setup_generators.cpp`)

**Verify**: Compiles.

#### Task 2.6: Update shader_setup_generators.cpp and .h

**Files**: `src/render/shader_setup_generators.cpp` (modify), `src/render/shader_setup_generators.h` (modify)
**Depends on**: Wave 1 complete

**Do** in `shader_setup_generators.cpp`:
- Remove `#include "config/interference_config.h"` and `#include "color_lut.h"`
- Replace each Setup function body with a one-line delegate to the module:
  ```
  void SetupConstellation(PostEffect *pe) {
    ConstellationEffectSetup(&pe->constellation, &pe->effects.constellation, pe->currentDeltaTime);
  }
  ```
  Same pattern for Plasma and Interference (Interference uses non-const config: `&pe->effects.interference`)
- Keep `#include "post_effect.h"` — provides access to PostEffect and effects
- Remove `#include <math.h>` (no longer needed — Lissajous computation moved to module)

**Do** in `shader_setup_generators.h`:
- No changes needed (function declarations stay the same)

**Verify**: Compiles.

#### Task 2.7: Update imgui_effects_generators.cpp

**Files**: `src/ui/imgui_effects_generators.cpp` (modify)
**Depends on**: Wave 1 complete

**Do**:
- Replace includes:
  - `#include "config/constellation_config.h"` → `#include "effects/constellation.h"`
  - `#include "config/plasma_config.h"` → `#include "effects/plasma.h"`
  - `#include "config/interference_config.h"` → `#include "effects/interference.h"`

**Verify**: Compiles.

#### Task 2.8: Update effect_config.h

**Files**: `src/config/effect_config.h` (modify)
**Depends on**: Wave 1 complete

**Do**:
- Replace includes:
  - `#include "config/constellation_config.h"` → `#include "effects/constellation.h"`
  - `#include "config/plasma_config.h"` → `#include "effects/plasma.h"`
  - `#include "config/interference_config.h"` → `#include "effects/interference.h"`
- Verify EffectConfig members (`constellation`, `plasma`, `interference`) still reference the same types

**Verify**: Compiles.

#### Task 2.9: Update preset.cpp

**Files**: `src/config/preset.cpp` (modify)
**Depends on**: Wave 1 complete

**Do**:
- Remove `#include "config/interference_config.h"` — `InterferenceConfig` is now reachable through `effect_config.h` → `effects/interference.h`
- Remove `#include "config/dual_lissajous_config.h"` if it's only included for InterferenceConfig (check — `DualLissajousConfig` has its own NLOHMANN macro and is included by `effects/interference.h`)
- NLOHMANN macros and to_json/from_json lines stay unchanged

**Verify**: Compiles.

#### Task 2.10: Update CMakeLists.txt

**Files**: `CMakeLists.txt` (modify)
**Depends on**: Wave 1 complete

**Do**:
- Add to `EFFECTS_SOURCES`:
  ```
  src/effects/constellation.cpp
  src/effects/plasma.cpp
  src/effects/interference.cpp
  ```

**Verify**: Compiles.

#### Task 2.11: Delete old config headers

**Files**: `src/config/constellation_config.h` (delete), `src/config/plasma_config.h` (delete), `src/config/interference_config.h` (delete)
**Depends on**: All other Wave 2 tasks complete

**Do**:
- Delete the 3 config headers
- Grep for any remaining includes of deleted headers across the codebase
- Fix any stale includes found

**Verify**: Full clean build succeeds with no warnings.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] Grep confirms zero includes of `config/constellation_config.h`, `config/plasma_config.h`, `config/interference_config.h`
- [ ] Generator effects render identically to pre-migration (visual check)
- [ ] Preset save/load preserves generator settings
- [ ] Diff `LoadPostEffectShaders` to confirm only generator shaders removed, no collateral (pitfall #10)
- [ ] ConfigDefault returns only `return Config{};` for all 3 modules (pitfall #9)
- [ ] No null-checks in Init/Setup/Uninit (pitfall #7)
- [ ] `shader.id == 0` pattern for shader validation (pitfall #8)
- [ ] `#include <stddef.h>` not `<stdlib.h>` in .cpp files (pitfall #12)

---

## Migration Notes (Batch 10–specific)

These generators do NOT follow the standard add-effect checklist because they lack `TransformEffectType` entries, `GetTransformCategory` cases, and `IsTransformEnabled` cases. The generator-blend-modes feature (a later refactor) will add those.

Key differences from transform effect migrations:
1. **No enum/name/order changes** in `effect_config.h`
2. **No `shader_setup.cpp` dispatch case** — generators use dedicated render passes
3. **No `imgui_effects.cpp` category mapping** change
4. **ColorLUT ownership transfers** to modules (unique to generators and FalseColor)
5. **Time accumulator ownership transfers** to modules (was on PostEffect struct)
6. **Resolution uniforms**: Each module caches `resolutionLoc`. `SetResolutionUniforms` references `pe->constellation.shader` + `pe->constellation.resolutionLoc` etc. (same pattern as `pe->voronoi.shader` + `pe->voronoi.resolutionLoc`).
