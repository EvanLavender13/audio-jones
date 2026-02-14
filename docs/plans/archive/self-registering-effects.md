# Self-Registering Effect Modules

Each effect `.cpp` registers its own lifecycle via a `REGISTER_EFFECT` macro. PostEffect discovers all registered effects at startup and loops over them — no central init/uninit/resize/register lists. The `GetTransformEffect()` 80-case switch is also absorbed. Adding a new effect means writing the `.cpp/.h`, placing one macro call, and adding the enum + config/struct fields.

**Research**: `docs/research/self-registering-effects.md`

## Design

### Overview

Each effect `.cpp` places a `REGISTER_EFFECT(...)` macro at the bottom of the file. The macro:
1. Generates static wrapper functions that adapt the effect's native Init/Uninit/RegisterParams/GetShader signatures to a uniform interface
2. Forward-declares the setup function (from `shader_setup_*.cpp`)
3. Populates a slot in the global `EFFECT_DESCRIPTORS[type]` array via a static-bool initializer that runs before `main()`

The `EFFECT_DESCRIPTORS` table changes from a `static constexpr` array in the header to an `extern` mutable array defined in `effect_descriptor.cpp`. Each slot carries both metadata (name, category, flags) and function pointers (lifecycle + dispatch). Registration order is irrelevant because each effect writes to its own enum-indexed slot.

PostEffect lifecycle functions (`Init`, `Uninit`, `Resize`, `RegisterParams`) become simple loops over the array. `GetTransformEffect()` becomes a 3-line table lookup.

### Scope

**In scope:** 73 effect modules, 7 sim boost dispatch entries, the 4 lifecycle functions, the `GetTransformEffect` switch.

**Out of scope:** Simulation lifecycle (heap-allocated, different API), infrastructure shaders, `EffectConfig` struct, UI panels, `render_pipeline.cpp` (unchanged — still calls `GetTransformEffect`).

**Migration strategy:** Option A from research — keep named struct fields on PostEffect. `pe->sineWarp.shader` access patterns unchanged. No opaque array.

### Types

**EffectDescriptor** (extended):

```
struct EffectDescriptor {
    // metadata (previously constexpr)
    TransformEffectType type
    const char *name
    const char *categoryBadge
    int categorySectionIndex
    size_t enabledOffset              // offsetof(EffectConfig, <name>.enabled)
    uint8_t flags

    // lifecycle function pointers
    bool (*init)(PostEffect *pe, int w, int h)
    void (*uninit)(PostEffect *pe)
    void (*resize)(PostEffect *pe, int w, int h)    // NULL unless NEEDS_RESIZE
    void (*registerParams)(EffectConfig *cfg)

    // dispatch (replaces GetTransformEffect)
    Shader *(*getShader)(PostEffect *pe)
    RenderPipelineShaderSetupFn setup               // void (*)(PostEffect *)
};
```

All function pointers nullable. Lifecycle loops skip NULL entries.

### Registration Mechanism

`effect_descriptor.h` declares `extern EffectDescriptor EFFECT_DESCRIPTORS[TRANSFORM_EFFECT_COUNT]` and a registration function:

```
bool EffectDescriptorRegister(TransformEffectType type, const EffectDescriptor &desc)
```

This writes `desc` into `EFFECT_DESCRIPTORS[type]`. Returns `true` (used as static-bool initializer).

`effect_descriptor.cpp` defines the zero-initialized array and the registration function. The existing accessor functions (`EffectDescriptorName`, `IsDescriptorEnabled`, etc.) continue to work — they read from the same array, now populated at startup.

### Macro Family

Each macro goes at the bottom of an effect's `.cpp` file. It generates:
- Static wrapper functions (adapting native signatures to uniform interface)
- A forward declaration of the setup function
- A `static bool reg_<field>` whose initializer calls `EffectDescriptorRegister()`

**`REGISTER_EFFECT(Type, Name, field, displayName, badge, section, flags, SetupFn, ResizeFn)`** — simple init: `Init(Effect*)`

Wrapper: `Init_field(pe, w, h)` → `NameEffectInit(&pe->field)` (ignores w, h)

**`REGISTER_EFFECT_CFG(Type, Name, field, displayName, badge, section, flags, SetupFn, ResizeFn)`** — config init: `Init(Effect*, Config*)`

Wrapper: `Init_field(pe, w, h)` → `NameEffectInit(&pe->field, &pe->effects.field)`

**`REGISTER_EFFECT_SIZED(Type, Name, field, displayName, badge, section, flags, SetupFn, ResizeFn)`** — sized init: `Init(Effect*, w, h)`

Wrapper: `Init_field(pe, w, h)` → `NameEffectInit(&pe->field, w, h)`

**`REGISTER_EFFECT_FULL(Type, Name, field, displayName, badge, section, flags, SetupFn, ResizeFn)`** — both: `Init(Effect*, Config*, w, h)`

Wrapper: `Init_field(pe, w, h)` → `NameEffectInit(&pe->field, &pe->effects.field, w, h)`

All four variants generate identical Uninit/RegisterParams/GetShader wrappers:
- `Uninit_field(pe)` → `NameEffectUninit(&pe->field)`
- `Register_field(cfg)` → `NameRegisterParams(&cfg->field)`
- `GetShader_field(pe)` → `&pe->field.shader`

**ResizeFn** is the last parameter: `NULL` for most effects. Effects with resize define a static wrapper above the macro call and pass it. Only 4 effects need resize (bloom, anamorphic, oil paint, attractor lines).

**`REGISTER_GENERATOR(Type, Name, field, displayName, SetupFn)`** — shortened form for generators. Bakes in `"GEN"` badge, section `10`, `EFFECT_FLAG_BLEND` flag. GetShader returns `&pe->blendCompositor->shader` instead of `&pe->field.shader`. Init variant is CFG (most generators need config for ColorLUT). Resize is NULL.

**`REGISTER_GENERATOR_FULL(Type, Name, field, displayName, SetupFn, ResizeFn)`** — generator with sized+config init and resize (attractor lines).

**`REGISTER_SIM_BOOST(Type, field, displayName, SetupFn, RegisterFn)`** — sim boosts. Init/uninit/resize are NULL. GetShader returns `&pe->blendCompositor->shader`. RegisterParams uses the provided `RegisterFn`.

**Multi-pass effects** (bloom, anamorphic, oil paint): Need custom GetShader returning composite shader (e.g., `&pe->bloom.compositeShader`). Define a manual `GetShader_` wrapper above the macro and use a variant that accepts a custom GetShader function, or write the registration call manually without a macro.

### Example Usage

```
// sine_warp.cpp (bottom of file)
REGISTER_EFFECT(
    TRANSFORM_SINE_WARP, SineWarp, sineWarp,
    "Sine Warp", "WARP", 1, EFFECT_FLAG_NONE,
    SetupSineWarp, NULL
)

// bloom.cpp (bottom of file)
static void ResizeBloom(PostEffect *pe, int w, int h) {
    BloomEffectResize(&pe->bloom, w, h);
}
static Shader *GetShaderBloom(PostEffect *pe) {
    return &pe->bloom.compositeShader;
}
// Manual registration or REGISTER_EFFECT_SIZED variant with custom getShader

// constellation.cpp (bottom of file)
REGISTER_GENERATOR(
    TRANSFORM_CONSTELLATION_BLEND, Constellation, constellation,
    "Constellation Blend", SetupConstellationBlend
)

// physarum.cpp (bottom of file)
REGISTER_SIM_BOOST(
    TRANSFORM_PHYSARUM_BOOST, physarum,
    "Physarum Boost", SetupTrailBoost, PhysarumRegisterParams
)
```

### Lifecycle Loops

**PostEffectInit** becomes:
```
for i in 0..TRANSFORM_EFFECT_COUNT:
    if EFFECT_DESCRIPTORS[i].init != NULL:
        if !EFFECT_DESCRIPTORS[i].init(pe, w, h):
            goto cleanup
```

**PostEffectUninit** becomes:
```
for i in 0..TRANSFORM_EFFECT_COUNT:
    if EFFECT_DESCRIPTORS[i].uninit != NULL:
        EFFECT_DESCRIPTORS[i].uninit(pe)
```

**PostEffectResize** becomes (after core texture resize):
```
for i in 0..TRANSFORM_EFFECT_COUNT:
    if EFFECT_DESCRIPTORS[i].resize != NULL:
        EFFECT_DESCRIPTORS[i].resize(pe, w, h)
```

**PostEffectRegisterParams** becomes:
```
for i in 0..TRANSFORM_EFFECT_COUNT:
    if EFFECT_DESCRIPTORS[i].registerParams != NULL:
        EFFECT_DESCRIPTORS[i].registerParams(&pe->effects)
```

### Dispatch Change

`GetTransformEffect()` (80-case switch) becomes a table lookup:
```
TransformEffectEntry GetTransformEffect(PostEffect *pe, TransformEffectType type) {
    const EffectDescriptor &d = EFFECT_DESCRIPTORS[type];
    bool *enabled = (bool *)((char *)&pe->effects + d.enabledOffset);
    return {d.getShader(pe), d.setup, enabled};
}
```

`render_pipeline.cpp` is unchanged.

### Init Variant Distribution

| Variant | Macro | Count | Examples |
|---------|-------|-------|----------|
| Simple | `REGISTER_EFFECT` | ~56 | SineWarp, Kaleidoscope, Glitch, Toon |
| Config | `REGISTER_EFFECT_CFG` | ~14 | FalseColor, HueRemap, Phyllotaxis |
| Sized | `REGISTER_EFFECT_SIZED` | 2 | Bloom, OilPaint |
| Full | `REGISTER_EFFECT_FULL` | 1 | AttractorLines |
| Generator | `REGISTER_GENERATOR` | ~16 | Constellation, Plasma, Nebula |
| Generator+resize | `REGISTER_GENERATOR_FULL` | 1 | AttractorLines blend |
| Sim boost | `REGISTER_SIM_BOOST` | 7 | PhysarumBoost, BoidsBoost |

### 6 Early Graphic Effects

Toon, NeonGlow, Halftone, Kuwahara, DiscoBall, LegoBricks are currently initialized inside `LoadPostEffectShaders()` (lines 86-91). Their Init functions already load their own shaders. Once they have `REGISTER_EFFECT` macros, the descriptor loop handles them. Remove their calls from `LoadPostEffectShaders` (and the shader.id checks on lines 97-99).

---

## Tasks

### Wave 1: Registry Infrastructure

#### Task 1.1: Create registration mechanism and macro definitions

**Files**: `src/config/effect_descriptor.h`, `src/config/effect_descriptor.cpp`

**Creates**: Extended EffectDescriptor struct, `EffectDescriptorRegister()` function, all macro definitions, zero-initialized extern array.

**Do**:

1. In `effect_descriptor.h`:
   - Forward-declare `struct PostEffect`; add `#include <raylib.h>` if needed for `Shader` type
   - Move `RenderPipelineShaderSetupFn` typedef here (or forward-declare it) so the struct can reference it
   - Add the 6 new function pointer fields to `EffectDescriptor` (see Design > Types)
   - Change declaration from `static constexpr EffectDescriptor EFFECT_DESCRIPTORS[...]` to `extern EffectDescriptor EFFECT_DESCRIPTORS[...]`
   - Declare `bool EffectDescriptorRegister(TransformEffectType type, const EffectDescriptor &desc)`
   - Define all `REGISTER_EFFECT*`, `REGISTER_GENERATOR*`, and `REGISTER_SIM_BOOST` macros (see Design > Macro Family). Each macro needs `#include "render/post_effect.h"` — guard this with a note that the macro expansion requires PostEffect to be a complete type
   - Keep existing accessor functions

2. In `effect_descriptor.cpp`:
   - Remove the old `static constexpr` table definition (it was in the header)
   - Define `EffectDescriptor EFFECT_DESCRIPTORS[TRANSFORM_EFFECT_COUNT] = {}` (zero-initialized)
   - Implement `EffectDescriptorRegister()`: writes the descriptor into `EFFECT_DESCRIPTORS[type]`, returns true
   - Keep existing accessor function implementations

**Verify**: `cmake.exe --build build` compiles. Existing behavior still works (table is zero-initialized, accessors return empty/false for unregistered slots — but nothing uses the new fields yet).

---

### Wave 2: Register All Effects

Each task adds `REGISTER_EFFECT` macros to effect `.cpp` files in one category. Tasks have no file overlap and run in parallel. Each file needs `#include "config/effect_descriptor.h"` (if not already included) and the macro call at the bottom.

The agent should read `post_effect.cpp` lines 184-517 to determine each effect's Init variant (simple/cfg/sized/full), then place the correct macro variant. Read `shader_setup.cpp` to find the setup function name for each effect.

#### Task 2.1: Register warp effects

**Files**: `src/effects/sine_warp.cpp`, `src/effects/texture_warp.cpp`, `src/effects/wave_ripple.cpp`, `src/effects/mobius.cpp`, `src/effects/gradient_flow.cpp`, `src/effects/chladni_warp.cpp`, `src/effects/circuit_board.cpp`, `src/effects/domain_warp.cpp`, `src/effects/surface_warp.cpp`, `src/effects/interference_warp.cpp`, `src/effects/corridor_warp.cpp`, `src/effects/radial_pulse.cpp`, `src/effects/tone_warp.cpp`

**Depends on**: Wave 1 complete

**Do**: Add appropriate `REGISTER_EFFECT` variant to each file. Look up each effect's Init signature in `post_effect.cpp`, display name and category info in `effect_descriptor.h` (current EFFECT_DESCRIPTORS), and setup function name in `shader_setup.cpp`/`shader_setup_warp.cpp`.

**Verify**: `cmake.exe --build build` compiles.

#### Task 2.2: Register symmetry effects

**Files**: `src/effects/kaleidoscope.cpp`, `src/effects/kifs.cpp`, `src/effects/poincare_disk.cpp`, `src/effects/mandelbox.cpp`, `src/effects/triangle_fold.cpp`, `src/effects/moire_interference.cpp`, `src/effects/radial_ifs.cpp`

**Depends on**: Wave 1 complete

**Do**: Same as 2.1 but for symmetry effects. Setup functions in `shader_setup_symmetry.cpp`.

**Verify**: Compiles.

#### Task 2.3: Register cellular + motion effects

**Files**: `src/effects/voronoi.cpp`, `src/effects/lattice_fold.cpp`, `src/effects/phyllotaxis.cpp`, `src/effects/multi_scale_grid.cpp`, `src/effects/dot_matrix.cpp`, `src/effects/infinite_zoom.cpp`, `src/effects/radial_streak.cpp`, `src/effects/droste_zoom.cpp`, `src/effects/density_wave_spiral.cpp`, `src/effects/shake.cpp`, `src/effects/relativistic_doppler.cpp`

**Depends on**: Wave 1 complete

**Do**: Same pattern. Setup functions in `shader_setup_cellular.cpp` and `shader_setup_motion.cpp`.

**Verify**: Compiles.

#### Task 2.4: Register artistic + graphic effects

**Files**: `src/effects/oil_paint.cpp`, `src/effects/watercolor.cpp`, `src/effects/impressionist.cpp`, `src/effects/ink_wash.cpp`, `src/effects/pencil_sketch.cpp`, `src/effects/cross_hatching.cpp`, `src/effects/toon.cpp`, `src/effects/neon_glow.cpp`, `src/effects/halftone.cpp`, `src/effects/kuwahara.cpp`, `src/effects/disco_ball.cpp`, `src/effects/lego_bricks.cpp`, `src/effects/synthwave.cpp`

**Depends on**: Wave 1 complete

**Do**: Same pattern. Note: oil paint needs `REGISTER_EFFECT_SIZED` with resize + custom GetShader (composite shader). Setup functions in `shader_setup_artistic.cpp` and `shader_setup_graphic.cpp`.

**Verify**: Compiles.

#### Task 2.5: Register retro + optical + color effects

**Files**: `src/effects/crt.cpp`, `src/effects/pixelation.cpp`, `src/effects/glitch.cpp`, `src/effects/ascii_art.cpp`, `src/effects/matrix_rain.cpp`, `src/effects/bloom.cpp`, `src/effects/bokeh.cpp`, `src/effects/heightfield_relief.cpp`, `src/effects/anamorphic_streak.cpp`, `src/effects/phi_blur.cpp`, `src/effects/color_grade.cpp`, `src/effects/false_color.cpp`, `src/effects/palette_quantization.cpp`, `src/effects/hue_remap.cpp`

**Depends on**: Wave 1 complete

**Do**: Same pattern. Note: bloom needs `REGISTER_EFFECT_SIZED` with resize + custom GetShader (compositeShader). Anamorphic streak needs resize + custom GetShader. Setup functions in `shader_setup_retro.cpp`, `shader_setup_optical.cpp`, `shader_setup_color.cpp`.

**Verify**: Compiles.

#### Task 2.6: Register generators + sim boosts

**Files**: `src/effects/constellation.cpp`, `src/effects/plasma.cpp`, `src/effects/interference.cpp`, `src/effects/solid_color.cpp`, `src/effects/scan_bars.cpp`, `src/effects/pitch_spiral.cpp`, `src/effects/moire_generator.cpp`, `src/effects/spectral_arcs.cpp`, `src/effects/muons.cpp`, `src/effects/filaments.cpp`, `src/effects/slashes.cpp`, `src/effects/glyph_field.cpp`, `src/effects/arc_strobe.cpp`, `src/effects/signal_frames.cpp`, `src/effects/nebula.cpp`, `src/effects/motherboard.cpp`, `src/effects/attractor_lines.cpp`, `src/simulation/physarum.cpp`, `src/simulation/boids.cpp`, `src/simulation/curl_flow.cpp`, `src/simulation/curl_advection.cpp`, `src/simulation/attractor_flow.cpp`, `src/simulation/particle_life.cpp`, `src/simulation/cymatics.cpp`

**Depends on**: Wave 1 complete

**Do**: Use `REGISTER_GENERATOR` for most generators. Use `REGISTER_GENERATOR_FULL` for attractor lines (config + sized + resize). Use `REGISTER_SIM_BOOST` for all 7 sim boosts. Setup functions in `shader_setup_generators.cpp`.

**Verify**: Compiles.

---

### Wave 3: Consume the Registry

#### Task 3.1: Replace lifecycle functions with loops

**Files**: `src/render/post_effect.cpp`

**Depends on**: Wave 2 complete (all effects registered)

**Do**:

1. Add `#include "config/effect_descriptor.h"`
2. In `LoadPostEffectShaders()`: remove the 6 early graphic effect Init calls (lines 86-91) and their shader.id checks (lines 97-99). Keep only the 8 infrastructure shader loads.
3. Replace the body of `PostEffectInit()`: remove all 69 sequential init calls (lines 228-491), replace with the lifecycle init loop. Keep simulation inits, core texture creation, uniform caching, resolution setup.
4. Replace the body of `PostEffectUninit()`: remove all 75 uninit calls, replace with lifecycle uninit loop. Keep sim uninit, texture unloads, infrastructure shader unloads, `free(pe)`.
5. Replace the body of `PostEffectResize()`: remove the 4 individual effect resize calls, replace with lifecycle resize loop. Keep core texture recreation, resolution uniforms, half-res/scratch recreation, sim resizes.
6. Replace the body of `PostEffectRegisterParams()`: remove all individual RegisterParams calls, replace with lifecycle registerParams loop.
7. Remove `#include` lines for individual effect headers that are no longer needed (effect-specific headers were only needed for the direct function calls).

**Verify**: `cmake.exe --build build` compiles. Run the app — all effects init, render, resize, and uninit correctly.

---

#### Task 3.2: Replace GetTransformEffect switch with table lookup

**Files**: `src/render/shader_setup.cpp`, `src/render/shader_setup.h`

**Depends on**: Wave 2 complete

**Do**:

1. In `shader_setup.cpp`:
   - Add `#include "config/effect_descriptor.h"`
   - Replace the entire `GetTransformEffect()` switch (80+ cases) with the 3-line table lookup (see Design > Dispatch Change)
   - Remove category-specific `#include "shader_setup_*.h"` that were only needed for the switch cases (the setup functions are now referenced by the macro expansions in the effect .cpp files)
   - Keep non-transform setup functions (`SetupFeedback`, `SetupBlur`, `SetupTrailBoost`, `SetupGamma`, `SetupClarity`, `SetupChromatic`)

2. In `shader_setup.h`:
   - Keep `TransformEffectEntry` and `RenderPipelineShaderSetupFn` typedef
   - Keep `GetTransformEffect()` declaration
   - If `RenderPipelineShaderSetupFn` was moved to `effect_descriptor.h`, add a note or re-export

**Verify**: `cmake.exe --build build` compiles. Run the app — transform chain renders identically.

---

## Final Verification

- [ ] Build succeeds with no new warnings
- [ ] All effects init correctly (no crashes on startup)
- [ ] Transform chain renders — enable several effects and verify they display
- [ ] Effect resize works (resize window with bloom/anamorphic/oil paint enabled)
- [ ] Modulation works (assign an LFO to a parameter, verify it modulates)
- [ ] Presets load correctly (load a preset with many effects)
- [ ] `post_effect.cpp` lifecycle functions are loops, not per-effect call lists
- [ ] `shader_setup.cpp` GetTransformEffect is a table lookup, not a switch
- [ ] `LoadPostEffectShaders` only loads infrastructure shaders
- [ ] Each effect `.cpp` has exactly one `REGISTER_*` macro call at the bottom

## What's Eliminated vs What Remains

**Eliminated:**
- 69 init calls in `PostEffectInit` → one loop
- 75 uninit calls in `PostEffectUninit` → one loop
- 73+ register calls in `PostEffectRegisterParams` → one loop
- 4 resize calls in `PostEffectResize` → one loop
- 80-case `GetTransformEffect` switch → table lookup
- 75 effect `#include` lines in `post_effect.cpp`
- 10+ `#include` lines in `shader_setup.cpp`
- `EFFECT_DESCRIPTORS` as a hand-maintained constexpr table

**Still present:**
- 73 named struct fields in `PostEffect` (one line each, Option A)
- 73 `#include` lines in `post_effect.h` for effect types
- Config fields in `EffectConfig` (unchanged — serialization/UI depend on names)
- `TransformEffectType` enum (unchanged)
- Simulation lifecycle in `post_effect.cpp` (out of scope)
