# Shader Setup Colocation

Move 10 functions from `shader_setup.cpp` to their natural homes in effect and simulation files, following the codebase's colocation convention. Also rename `SetupTrailBoost` → `SetupPhysarumTrailBoost` for consistency with the other sim trail boost names.

## Design

### What Moves Where

**Sim trail boosts** (7 functions → 7 sim `.cpp` files):
Each is a 3-4 line function calling `BlendCompositorApply` with the sim's trail map texture, boost intensity, and blend mode.

| Function | Destination |
|----------|-------------|
| `SetupTrailBoost` → `SetupPhysarumTrailBoost` | `src/simulation/physarum.cpp` |
| `SetupCurlFlowTrailBoost` | `src/simulation/curl_flow.cpp` |
| `SetupCurlAdvectionTrailBoost` | `src/simulation/curl_advection.cpp` |
| `SetupAttractorFlowTrailBoost` | `src/simulation/attractor_flow.cpp` |
| `SetupBoidsTrailBoost` | `src/simulation/boids.cpp` |
| `SetupParticleLifeTrailBoost` | `src/simulation/particle_life.cpp` |
| `SetupCymaticsTrailBoost` | `src/simulation/cymatics.cpp` |

**Multi-pass helpers** (3 function groups → 3 effect file pairs):

| Function(s) | Destination `.cpp` | Destination `.h` |
|-------------|-------------------|-----------------|
| `ApplyBloomPasses` + `BloomRenderPass` (static) | `src/effects/bloom.cpp` | `src/effects/bloom.h` |
| `ApplyAnamorphicStreakPasses` | `src/effects/anamorphic_streak.cpp` | `src/effects/anamorphic_streak.h` |
| `ApplyHalfResOilPaint` | `src/effects/oil_paint.cpp` | `src/effects/oil_paint.h` |

### Include Changes

**Sim files** (each `.cpp`):
- Remove: `#include "render/shader_setup.h"`
- Add: `#include "render/blend_compositor.h"` (for `BlendCompositorApply`)

Note: `BlendCompositorApply` is technically reachable via `effect_descriptor.h` → `blend_compositor.h`, but the direct include is correct since we call the function directly.

**Effect files** (each `.cpp`):
- No include changes needed. `bloom.cpp` and `anamorphic_streak.cpp` don't include `shader_setup.h`. `oil_paint.cpp` doesn't either. All already include `render/post_effect.h` which provides the `PostEffect` struct.

**`shader_setup.cpp`**:
- Remove includes that become unused after function removal: `simulation/physarum.h`, `simulation/curl_flow.h`, `simulation/curl_advection.h`, `simulation/attractor_flow.h`, `simulation/boids.h`, `simulation/particle_life.h`, `simulation/cymatics.h`, `simulation/trail_map.h`, `blend_compositor.h`

**`render_pipeline.cpp`**:
- No changes needed. It already includes `post_effect.h` which transitively includes `bloom.h`, `anamorphic_streak.h`, and `oil_paint.h`.

### Placement Rules

**Sim trail boost functions**: Place immediately before the `REGISTER_SIM_BOOST` macro at the bottom of each sim `.cpp` file. The function must be **non-static** (it's referenced by function pointer through the macro). The `REGISTER_SIM_BOOST` macro forward-declares the setup function, so no separate declaration is needed in the header.

**Multi-pass Apply functions**: Place after the existing module functions (Setup, Resize, Uninit) but before the `// === UI ===` section. Declare in the `.h` header alongside the other public function declarations.

**`BloomRenderPass`**: Stays `static` — it's a helper only used by `ApplyBloomPasses`. Place it immediately above `ApplyBloomPasses` in `bloom.cpp`.

---

## Tasks

### Wave 1: Move functions to destination files

All tasks in this wave touch different files — no overlap.

#### Task 1.1: Move sim trail boosts

**Files**: `src/simulation/physarum.cpp`, `src/simulation/curl_flow.cpp`, `src/simulation/curl_advection.cpp`, `src/simulation/attractor_flow.cpp`, `src/simulation/boids.cpp`, `src/simulation/particle_life.cpp`, `src/simulation/cymatics.cpp`

**Do**: For each of the 7 sim files:
1. Replace `#include "render/shader_setup.h"` with `#include "render/blend_compositor.h"`
2. Add the trail boost function body immediately before the `REGISTER_SIM_BOOST` macro. The function is non-static.
3. For physarum only: rename `SetupTrailBoost` → `SetupPhysarumTrailBoost` in both the function definition and the `REGISTER_SIM_BOOST` macro argument.

The function bodies to move (from `shader_setup.cpp`):

- **physarum**: `SetupTrailBoost` → renamed to `SetupPhysarumTrailBoost`
  ```cpp
  void SetupPhysarumTrailBoost(PostEffect *pe) {
    BlendCompositorApply(
        pe->blendCompositor, TrailMapGetTexture(pe->physarum->trailMap),
        pe->effects.physarum.boostIntensity, pe->effects.physarum.blendMode);
  }
  ```

- **curl_flow**: `SetupCurlFlowTrailBoost`
  ```cpp
  void SetupCurlFlowTrailBoost(PostEffect *pe) {
    BlendCompositorApply(
        pe->blendCompositor, TrailMapGetTexture(pe->curlFlow->trailMap),
        pe->effects.curlFlow.boostIntensity, pe->effects.curlFlow.blendMode);
  }
  ```

- **curl_advection**: `SetupCurlAdvectionTrailBoost`
  ```cpp
  void SetupCurlAdvectionTrailBoost(PostEffect *pe) {
    BlendCompositorApply(pe->blendCompositor,
                         TrailMapGetTexture(pe->curlAdvection->trailMap),
                         pe->effects.curlAdvection.boostIntensity,
                         pe->effects.curlAdvection.blendMode);
  }
  ```

- **attractor_flow**: `SetupAttractorFlowTrailBoost`
  ```cpp
  void SetupAttractorFlowTrailBoost(PostEffect *pe) {
    BlendCompositorApply(pe->blendCompositor,
                         TrailMapGetTexture(pe->attractorFlow->trailMap),
                         pe->effects.attractorFlow.boostIntensity,
                         pe->effects.attractorFlow.blendMode);
  }
  ```

- **boids**: `SetupBoidsTrailBoost`
  ```cpp
  void SetupBoidsTrailBoost(PostEffect *pe) {
    BlendCompositorApply(
        pe->blendCompositor, TrailMapGetTexture(pe->boids->trailMap),
        pe->effects.boids.boostIntensity, pe->effects.boids.blendMode);
  }
  ```

- **particle_life**: `SetupParticleLifeTrailBoost`
  ```cpp
  void SetupParticleLifeTrailBoost(PostEffect *pe) {
    BlendCompositorApply(pe->blendCompositor,
                         TrailMapGetTexture(pe->particleLife->trailMap),
                         pe->effects.particleLife.boostIntensity,
                         pe->effects.particleLife.blendMode);
  }
  ```

- **cymatics**: `SetupCymaticsTrailBoost`
  ```cpp
  void SetupCymaticsTrailBoost(PostEffect *pe) {
    BlendCompositorApply(
        pe->blendCompositor, TrailMapGetTexture(pe->cymatics->trailMap),
        pe->effects.cymatics.boostIntensity, pe->effects.cymatics.blendMode);
  }
  ```

**Verify**: Each file has the function before the REGISTER_SIM_BOOST macro and the include swap is correct.

#### Task 1.2: Move ApplyBloomPasses to bloom module

**Files**: `src/effects/bloom.h`, `src/effects/bloom.cpp`

**Do**:
1. In `bloom.h`: Add forward declarations before the `#endif`:
   ```cpp
   typedef struct PostEffect PostEffect;

   // Executes the multi-pass bloom pipeline (prefilter → downsample → upsample)
   void ApplyBloomPasses(PostEffect *pe, RenderTexture2D *source, int *writeIdx);
   ```
2. In `bloom.cpp`: Add `#include "render/shader_setup.h"` (for `PostEffect` definition — actually `render/post_effect.h` is already included, so no new include needed). Add these two functions after `BloomEffectUninit` and before `BloomConfigDefault`:
   - `static void BloomRenderPass(...)` — the static helper
   - `void ApplyBloomPasses(...)` — the public function

   Move the exact bodies from `shader_setup.cpp` lines 221-285.

**Verify**: Declarations in header, definitions in .cpp, static helper is above the public function.

#### Task 1.3: Move ApplyAnamorphicStreakPasses to anamorphic streak module

**Files**: `src/effects/anamorphic_streak.h`, `src/effects/anamorphic_streak.cpp`

**Do**:
1. In `anamorphic_streak.h`: Add forward declarations before the `#endif`:
   ```cpp
   typedef struct PostEffect PostEffect;

   // Executes the multi-pass anamorphic streak pipeline
   void ApplyAnamorphicStreakPasses(PostEffect *pe, RenderTexture2D *source);
   ```
2. In `anamorphic_streak.cpp`: Add the function after `AnamorphicStreakEffectUninit` and before `AnamorphicStreakConfigDefault`. Move the exact body from `shader_setup.cpp` lines 287-364.

**Verify**: Declaration in header, definition in .cpp.

#### Task 1.4: Move ApplyHalfResOilPaint to oil paint module

**Files**: `src/effects/oil_paint.h`, `src/effects/oil_paint.cpp`

**Do**:
1. In `oil_paint.h`: Add forward declarations before the `#endif`:
   ```cpp
   typedef struct PostEffect PostEffect;

   // Runs the oil paint 2-pass pipeline at half resolution
   void ApplyHalfResOilPaint(PostEffect *pe, RenderTexture2D *source,
                             const int *writeIdx);
   ```
2. In `oil_paint.cpp`: Add the function after `OilPaintEffectResize` and before `OilPaintConfigDefault`. Move the exact body from `shader_setup.cpp` lines 409-457.

**Verify**: Declaration in header, definition in .cpp.

---

### Wave 2: Clean up shader_setup

#### Task 2.1: Remove moved functions from shader_setup

**Files**: `src/render/shader_setup.h`, `src/render/shader_setup.cpp`
**Depends on**: Wave 1 complete

**Do**:
1. In `shader_setup.h`:
   - Remove all 7 trail boost declarations (lines 22-28: `SetupTrailBoost` through `SetupCymaticsTrailBoost`)
   - Remove `ApplyAnamorphicStreakPasses` declaration (line 34)
   - Remove `ApplyBloomPasses` declaration (line 35)
   - Remove `ApplyHalfResOilPaint` declaration (lines 39-40)

2. In `shader_setup.cpp`:
   - Remove all 7 trail boost function definitions (lines 143-186)
   - Remove `BloomRenderPass` static helper (lines 221-232)
   - Remove `ApplyBloomPasses` function (lines 234-285)
   - Remove `ApplyAnamorphicStreakPasses` function (lines 287-364)
   - Remove `ApplyHalfResOilPaint` function (lines 409-457)
   - Remove now-unused includes: `simulation/physarum.h`, `simulation/boids.h`, `simulation/curl_advection.h`, `simulation/curl_flow.h`, `simulation/cymatics.h`, `simulation/particle_life.h`, `simulation/attractor_flow.h`, `simulation/trail_map.h`, `blend_compositor.h`

**Verify**: `cmake.exe --build build` compiles with no errors. No duplicate symbol warnings.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] `shader_setup.cpp` no longer contains any sim-specific or effect-specific functions
- [ ] Each sim `.cpp` owns its trail boost function right above the `REGISTER_SIM_BOOST` macro
- [ ] Each multi-pass effect declares its Apply function in the `.h` and defines it in the `.cpp`
- [ ] `render_pipeline.cpp` required no changes
- [ ] Physarum uses `SetupPhysarumTrailBoost` consistently (function + macro arg)
