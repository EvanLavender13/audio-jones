# PostEffect Descriptor Dispatch

Replace the 145 named `<Name>Effect` member fields on `PostEffect` with a descriptor-driven `void *effectStates[TRANSFORM_EFFECT_COUNT]` array. Each effect module owns a single file-local `static <Name>Effect g_<field>State;` instance and registers its address in the existing `EFFECT_DESCRIPTORS[]` table via a new `state` slot. Adding a new transform effect stops requiring any edit to `post_effect.h`.

**Research**: `docs/research/post_effect_descriptor_dispatch.md`

**Out of scope**: Step 6 of the research doc (relocating the 29 `pe->feedback*Loc` ints into a `FeedbackEffect` struct). Feedback is currently inline post-pass code with no `FeedbackEffect` module; relocating it is a separate refactor and not done here. The 29 feedback locs stay on `PostEffect` alongside fxaa/clarity/gamma/blur/halfLife/shapeTexture utility loc fields.

---

## Design

### Types

#### `EffectDescriptor` extension (`src/config/effect_descriptor.h`)

Add one trailing field with a default initializer so existing literal constructions remain valid:

```cpp
struct EffectDescriptor {
  // ... existing fields unchanged ...
  DrawParamsFn drawParams = nullptr;
  DrawOutputFn drawOutput = nullptr;
  void *state = nullptr;  // points to file-local static <Name>Effect instance, or null for sim boosts
};
```

Set at registration time by macros to `&g_<field>State`. Lifetime is program lifetime — no allocation, no construct/destruct callback. `REGISTER_SIM_BOOST` leaves it null because sim-boost effects do not own a `<Name>Effect` member.

#### `PostEffect` slot array (`src/render/post_effect.h`)

Add one field:

```cpp
struct PostEffect {
  // ... non-effect state stays (utility shaders, halfRes textures, BlendCompositor*,
  // EffectConfig effects, simulation pointers, screen dimensions, FFT/waveform textures,
  // 29 feedback*Loc fields, fxaa/clarity/gamma/blur/halfLife/shapeTex loc fields,
  // per-frame temporaries) ...

  void *effectStates[TRANSFORM_EFFECT_COUNT];
};
```

Populated once at the start of `PostEffectInit` from the already-static-initialized `EFFECT_DESCRIPTORS[]` table. The slot array is a per-instance convenience indirection so call sites with a `PostEffect *` resolve state without reaching into the global descriptor table.

Removed from `PostEffect`:
- All 145 `<Name>Effect <field>;` members (lines 172-376 of current `post_effect.h`).
- All 143 `#include "effects/<name>.h"` lines (lines 5-147 of current `post_effect.h`).

Stays on `PostEffect` (out of scope):
- `feedbackShader`, `blurHShader`, `blurVShader`, `fxaaShader`, `clarityShader`, `gammaShader`, `shapeTextureShader`.
- All 29 `feedback*Loc` ints (post_effect.h:187-217).
- `blurH/VResolutionLoc`, `blurH/VScaleLoc`, `halfLifeLoc`, `deltaTimeLoc`, `fxaaResolutionLoc`, `clarityResolutionLoc`, `clarityAmountLoc`, `gammaGammaLoc`, `shapeTexZoomLoc`, `shapeTexAngleLoc`, `shapeTexBrightnessLoc`.
- `accumTexture`, `pingPong[2]`, `outputTexture`, `halfResA`, `halfResB`, `generatorScratch`.
- `BlendCompositor *blendCompositor`, simulation pointers (`physarum`, `curlFlow`, `attractorFlow`, `particleLife`, `boids`, `mazeWorms`).
- `EffectConfig effects`.
- `screenWidth`, `screenHeight`.
- `fftTexture`, `fftMaxMagnitude`, `waveformTexture`, `waveformWriteIndex`.
- `currentDeltaTime`, `currentBlurScale`, `transformTime`, `warpTime`, `currentSceneTexture`, `currentRenderDest`.

#### Per-effect typed accessor

Added inline to each effect's header (e.g. `src/effects/bloom.h`):

```cpp
inline BloomEffect *GetBloomEffect(PostEffect *pe) {
  return (BloomEffect *)pe->effectStates[TRANSFORM_BLOOM];
}
```

Naming convention: `Get<Name>Effect(PostEffect *)`, where `<Name>` matches the `Name` argument passed to the registration macro (e.g. `Bloom`, `Kaleidoscope`, `ChromaticAberration`). Cast is `(<Name>Effect *)pe->effectStates[<TransformEffectType enum value>]`.

For the accessor to compile, each effect's `.h` must include `render/post_effect.h` (for `PostEffect`'s layout) and `config/effect_config.h` (for the enum value — most effect headers already include this). After this refactor `post_effect.h` no longer includes any effect header, so no circular dependency forms.

### Algorithm

#### Registration macro rewrite

The four state-bearing macros (`REGISTER_EFFECT`, `REGISTER_EFFECT_CFG`, `REGISTER_GENERATOR`, `REGISTER_GENERATOR_FULL`) gain a file-local static state declaration, change every `&pe->field` thunk to cast through `pe->effectStates[Type]`, and set the descriptor's new `state` slot to `&g_##field##State`. `REGISTER_SIM_BOOST` is unchanged (sim boosts do not own a `PostEffect` state member; they share the simulation pointers).

`REGISTER_EFFECT` becomes (full body):

```cpp
#define REGISTER_EFFECT(Type, Name, field, displayName, badge, section,        \
                        flags, SetupFn, ResizeFn, DrawParamsFnArg)             \
  static Name##Effect g_##field##State;                                        \
  void SetupFn(PostEffect *);                                                  \
  static bool Init_##field(PostEffect *pe, int, int) {                         \
    return Name##EffectInit((Name##Effect *)pe->effectStates[Type]);           \
  }                                                                            \
  static void Uninit_##field(PostEffect *pe) {                                 \
    Name##EffectUninit((Name##Effect *)pe->effectStates[Type]);                \
  }                                                                            \
  static void Register_##field(EffectConfig *cfg) {                            \
    Name##RegisterParams(&cfg->field);                                         \
  }                                                                            \
  static Shader *GetShader_##field(PostEffect *pe) {                           \
    return &((Name##Effect *)pe->effectStates[Type])->shader;                  \
  }                                                                            \
  static bool reg_##field = EffectDescriptorRegister(                          \
      Type,                                                                    \
      EffectDescriptor{Type, displayName, badge, section,                      \
       offsetof(EffectConfig, field.enabled), #field ".",                       \
       (uint8_t)(flags),                                                       \
       Init_##field, Uninit_##field, ResizeFn, Register_##field,               \
       GetShader_##field, SetupFn,                                             \
       nullptr, nullptr, nullptr,                                              \
       DrawParamsFnArg, nullptr,                                               \
       &g_##field##State});
```

`REGISTER_EFFECT_CFG` is the same shape but its `Init_##field` keeps the second-argument config pointer:

```cpp
  static bool Init_##field(PostEffect *pe, int, int) {                         \
    return Name##EffectInit((Name##Effect *)pe->effectStates[Type],            \
                            &pe->effects.field);                               \
  }
```

`REGISTER_GENERATOR` is the same shape but its `GetShader_##field` returns the blend compositor shader (not the per-effect shader) and its `GetScratchShader_##field` returns the per-effect shader. Replace `&pe->field.shader` (line 213 of current header) with `&((Name##Effect *)pe->effectStates[Type])->shader`. The `&pe->blendCompositor->shader` line is unchanged.

`REGISTER_GENERATOR_FULL` matches `REGISTER_GENERATOR` and additionally declares a `Resize_##field` thunk; replace `&pe->field` (current line 239) and `&pe->field.shader` (current line 248) with the cast form.

`REGISTER_SIM_BOOST` is unchanged. Its descriptor literal can rely on the default-initialized `state = nullptr` from the struct.

#### Init order

In `PostEffectInit` (`src/render/post_effect.cpp`), populate `pe->effectStates[]` from the descriptor table BEFORE the existing descriptor init loop runs, so the `Init_##field` thunks find their state pointer:

```cpp
// After: pe->effects = EffectConfig{}; (current line 144)
// Before: LoadPostEffectShaders(pe); (current line 146)
for (int i = 0; i < TRANSFORM_EFFECT_COUNT; i++) {
  pe->effectStates[i] = EFFECT_DESCRIPTORS[i].state;
}
```

Slots whose descriptor has `state == nullptr` (sim boosts, `TRANSFORM_ACCUM_COMPOSITE`, any zero-initialized slot) keep `nullptr` — typed accessors for those types are not generated and not called.

The existing init loop at lines 190-196, the uninit loop at 253-257, the resize loop at 299-303, and the register-params loop at 232-237 all stay unchanged. They are already descriptor-driven.

#### Bridge function migration

Every effect's bridge `Setup<Name>(PostEffect *pe)` currently dereferences `&pe->field` directly. After the refactor it goes through the typed accessor:

Before:
```cpp
void SetupKaleido(PostEffect *pe) {
  KaleidoscopeEffectSetup(&pe->kaleidoscope, &pe->effects.kaleidoscope,
                          pe->currentDeltaTime);
}
```

After:
```cpp
void SetupKaleido(PostEffect *pe) {
  KaleidoscopeEffectSetup(GetKaleidoscopeEffect(pe), &pe->effects.kaleidoscope,
                          pe->currentDeltaTime);
}
```

Mechanical substitution: `&pe-><field>` → `Get<Name>Effect(pe)`. The `&pe->effects.<field>` (config side) is unchanged.

#### Multi-pass effect Apply helpers

Five effects have Apply helpers that read `pe-><field>.<member>` repeatedly: `bloom.cpp`, `anamorphic_streak.cpp`, `curl_advection.cpp`, `attractor_lines.cpp`, `light_medley.cpp`. Convert by hoisting the typed pointer to a local at the top of each helper, then dereferencing through the local:

Before (`bloom.cpp::ApplyBloomPasses`):
```cpp
SetShaderValue(pe->bloom.prefilterShader, pe->bloom.thresholdLoc, ...);
BloomRenderPass(source, &pe->bloom.mips[0], pe->bloom.prefilterShader);
// ... ~24 more pe->bloom.X references ...
```

After:
```cpp
BloomEffect *e = GetBloomEffect(pe);
SetShaderValue(e->prefilterShader, e->thresholdLoc, ...);
BloomRenderPass(source, &e->mips[0], e->prefilterShader);
// ... e->X throughout ...
```

`anamorphic_streak.cpp` already follows this pattern (`AnamorphicStreakEffect *e = &pe->anamorphicStreak;` at line 114) — change the assignment to `GetAnamorphicStreakEffect(pe)` and the rest of the helper is untouched.

#### `bloom.cpp` manual registration

`bloom.cpp` does not use a registration macro. Its hand-written thunks (lines 175-187) and `EffectDescriptorRegister` call (lines 208-216) must be migrated by hand to match the new pattern:

```cpp
static BloomEffect g_bloomState;

static bool Init_bloom(PostEffect *pe, int w, int h) {
  return BloomEffectInit((BloomEffect *)pe->effectStates[TRANSFORM_BLOOM], w, h);
}
static void Uninit_bloom(PostEffect *pe) {
  BloomEffectUninit((BloomEffect *)pe->effectStates[TRANSFORM_BLOOM]);
}
static void Resize_bloom(PostEffect *pe, int w, int h) {
  BloomEffectResize((BloomEffect *)pe->effectStates[TRANSFORM_BLOOM], w, h);
}
static void Register_bloom(EffectConfig *cfg) {
  BloomRegisterParams(&cfg->bloom);
}
static Shader *GetShader_bloom(PostEffect *pe) {
  return &((BloomEffect *)pe->effectStates[TRANSFORM_BLOOM])->compositeShader;
}

void SetupBloom(PostEffect *pe) {
  BloomEffectSetup(GetBloomEffect(pe), &pe->effects.bloom);
}

static bool reg_bloom = EffectDescriptorRegister(
    TRANSFORM_BLOOM,
    EffectDescriptor{TRANSFORM_BLOOM, "Bloom", "OPT", 7,
     offsetof(EffectConfig, bloom.enabled), "bloom.",
     (uint8_t)(EFFECT_FLAG_NEEDS_RESIZE),
     Init_bloom, Uninit_bloom, Resize_bloom, Register_bloom,
     GetShader_bloom, SetupBloom,
     nullptr, nullptr, nullptr,
     DrawBloomParams, nullptr,
     &g_bloomState});
```

`GetShader_bloom` keeps returning `compositeShader` (not `shader`) — that is the deviation that forced manual registration in the first place.

#### `PostEffectClearFeedback` cross-cutting access

The function at `post_effect.cpp:321-375` reaches directly into `pe->attractorLines.{pingPong, readIdx}` (lines 340-346) and `pe->curlAdvection.statePingPong[0]` (line 357). After the refactor, route those through the typed accessors:

```cpp
AttractorLinesEffect *al = GetAttractorLinesEffect(pe);
BeginTextureMode(al->pingPong[0]);
ClearBackground(BLACK);
EndTextureMode();
BeginTextureMode(al->pingPong[1]);
ClearBackground(BLACK);
EndTextureMode();
al->readIdx = 0;
// ...
CurlAdvectionEffect *ca = GetCurlAdvectionEffect(pe);
if (ca->statePingPong[0].id > 0) {
  CurlAdvectionEffectReset(ca, GetScreenWidth(), GetScreenHeight());
}
```

`post_effect.cpp` must add `#include "effects/attractor_lines.h"` and `#include "effects/curl_advection.h"` for these accessors.

### Constants

Not applicable (architecture refactor — no new enums, no new effect types).

### Parameters

Not applicable (no user-facing parameters).

---

## Tasks

The build does not compile cleanly between Wave 1 and Wave 3 — once Wave 1 lands, the macros reference `pe->effectStates[]` (which exists only after Wave 1 Task 1.2) and the bridge functions still reference removed `pe->field` members. Wave 2 fixes 145 effect translation units in parallel; Wave 3 wires the last cross-cutting access in `post_effect.cpp`. Build verification runs at the end of Wave 3.

### Wave 1: Foundation (build broken at end of wave)

Two parallel tasks. No file overlap.

#### Task 1.1: Extend `EffectDescriptor` and rewrite four registration macros

**Files**: `src/config/effect_descriptor.h`
**Creates**: `void *state` slot on `EffectDescriptor`. Macro shapes that declare `static <Name>Effect g_<field>State;` and route thunks through `pe->effectStates[Type]`.

**Do**:
1. Add `void *state = nullptr;` as the last field of `EffectDescriptor` (after `DrawOutputFn drawOutput = nullptr;`).
2. Rewrite `REGISTER_EFFECT` (current lines 142-165) per the Algorithm section: declare static state instance, change Init/Uninit/GetShader thunks to cast through `pe->effectStates[Type]`, append `&g_##field##State` as the last field of the descriptor literal.
3. Rewrite `REGISTER_EFFECT_CFG` (current lines 168-191) — same shape; Init signature passes `&pe->effects.field` as second arg.
4. Rewrite `REGISTER_GENERATOR` (current lines 195-223) — same shape; `GetScratchShader_##field` casts through `pe->effectStates[Type]` for `->shader`; `GetShader_##field` keeps `&pe->blendCompositor->shader` unchanged.
5. Rewrite `REGISTER_GENERATOR_FULL` (current lines 226-258) — same shape with `Resize_##field` thunk also casting through `pe->effectStates[Type]`.
6. **Do NOT modify `REGISTER_SIM_BOOST`** (current lines 261-278). Sim boosts have no `<Name>Effect` member; their descriptor's `state` slot stays `nullptr` via the struct default.

**Verify**: File compiles in isolation when included by the descriptor table TU. (Full build will fail until Wave 2 lands.)

---

#### Task 1.2: Add `effectStates[]` to `PostEffect` and strip effect fields/includes

**Files**: `src/render/post_effect.h`
**Creates**: `void *effectStates[TRANSFORM_EFFECT_COUNT]` slot on `PostEffect`. Header is no longer the index of every effect type.

**Do**:
1. Remove the 143 `#include "effects/<name>.h"` lines (current lines 5-147). Keep `"config/effect_config.h"`, `"raylib.h"`, `<stdint.h>`, simulation forward decls, `BlendCompositor`/`ColorLUT` forward decls.
2. Remove all 145 named `<Name>Effect <field>;` members from `PostEffect`. Specifically: `chromaticAberration` (172), `vignette` (173), and the contiguous block at lines 227-376.
3. Add `void *effectStates[TRANSFORM_EFFECT_COUNT];` as a new field on `PostEffect`. Place it where the named effect-state fields used to live (logical grouping is fine; alphabetical adjacent to `effects` works).
4. Leave every other field on `PostEffect` untouched (utility shaders, halfRes textures, halfRes loc fields, all 29 `feedback*Loc` ints at lines 187-217, all simulation pointers, `EffectConfig effects`, screen dimensions, FFT/waveform/generatorScratch, per-frame temporaries).
5. Public function declarations at the bottom of the header are unchanged.

**Verify**: File parses (`post_effect.h` does not need to compile standalone, but should be syntactically valid). Full build will fail until Wave 2 lands.

---

### Wave 2: Per-effect migration (parallel; build still broken at end of wave)

After Wave 1, every effect's bridge `Setup<Name>` references a removed `pe->field` member, and every macro expansion references a missing `pe->effectStates[]` slot — until Wave 1 Task 1.2 lands. Wave 2 fixes all 145 effect translation units. The five tasks below have no file overlap with each other.

#### Task 2.1: Bloom (manual registration + Apply helpers + accessor)

**Files**: `src/effects/bloom.h`, `src/effects/bloom.cpp`
**Depends on**: Wave 1 complete.

**Do**:
1. In `bloom.h`: add `#include "render/post_effect.h"` and an `inline BloomEffect *GetBloomEffect(PostEffect *pe) { return (BloomEffect *)pe->effectStates[TRANSFORM_BLOOM]; }` near the bottom of the header (after the `BloomEffect` struct and the public function declarations).
2. In `bloom.cpp`: rewrite the manual registration block per the Algorithm section's `bloom.cpp` example. Declare `static BloomEffect g_bloomState;`, change every `Init_bloom`/`Uninit_bloom`/`Resize_bloom`/`GetShader_bloom` thunk to cast through `pe->effectStates[TRANSFORM_BLOOM]` (preserving `compositeShader` in `GetShader_bloom`), update `SetupBloom` to use `GetBloomEffect(pe)`, append `&g_bloomState` as the last field of the `EffectDescriptor` literal.
3. In `bloom.cpp::ApplyBloomPasses` (lines 115-167): hoist `BloomEffect *e = GetBloomEffect(pe);` at the top of the helper and replace every `pe->bloom.<member>` with `e-><member>` (~24 sites).

**Verify**: Build succeeds for `bloom.cpp` once all of Wave 2 + Wave 3 land. Compile errors specific to `pe->bloom.X` in this file are gone.

---

#### Task 2.2: Multi-pass effects (Apply helper hoist + accessors)

**Files**:
- `src/effects/anamorphic_streak.h`, `src/effects/anamorphic_streak.cpp`
- `src/effects/curl_advection.h`, `src/effects/curl_advection.cpp`
- `src/effects/attractor_lines.h`, `src/effects/attractor_lines.cpp`
- `src/effects/light_medley.h`, `src/effects/light_medley.cpp`

**Depends on**: Wave 1 complete.

**Do** (for each of the four effects):
1. In `<name>.h`: add `#include "render/post_effect.h"` and an `inline <Name>Effect *Get<Name>Effect(PostEffect *pe) { return (<Name>Effect *)pe->effectStates[<TRANSFORM_TYPE>]; }`. Match `<Name>` to the macro's `Name` argument (`AnamorphicStreak`, `CurlAdvection`, `AttractorLines`, `LightMedley`); match `<TRANSFORM_TYPE>` to the registration macro's `Type` argument.
2. In `<name>.cpp`: update the bridge `Setup<Name>(PostEffect *pe)` to use `Get<Name>Effect(pe)` instead of `&pe-><field>`.
3. In any `Apply<Name>...` helper inside `<name>.cpp`: hoist `<Name>Effect *e = Get<Name>Effect(pe);` at the top, then replace every `pe-><field>.<member>` with `e-><member>`. `anamorphic_streak.cpp` already has `AnamorphicStreakEffect *e = &pe->anamorphicStreak;` at line 114 — change the right-hand side to `GetAnamorphicStreakEffect(pe)`.

**Verify**: Compile errors specific to `pe-><field>.X` in these four files are gone after Wave 2 + Wave 3 land.

---

#### Task 2.3: Simple effects batch A (alphabetical first half, ~70 effects)

**Files**: `src/effects/<name>.h` and `src/effects/<name>.cpp` for each effect from `anamorphic_streak` (excluded — done by 2.2) onward through the alphabetical midpoint, **excluding** the 5 effects handled in 2.1 and 2.2 (`bloom`, `anamorphic_streak`, `curl_advection`, `attractor_lines`, `light_medley`).

A representative concrete batch (use as a guide; the implementer should split the 145 total effect files alphabetically and match this size):

`apollonian_tunnel`, `arc_strobe`, `ascii_art`, `attractor_flow`, `bilateral`, `bit_crush`, `bokeh`, `butterflies`, `byzantine`, `chladni`, `chladni_warp`, `chromatic_aberration`, `circuit_board`, `color_grade`, `color_stretch`, `constellation`, `corridor_warp`, `cross_hatching`, `crt`, `cyber_march`, `dancing_lines`, `data_traffic`, `density_wave_spiral`, `digital_shard`, `disco_ball`, `dog_filter`, `domain_warp`, `dot_matrix`, `drekker_paint`, `dream_fractal`, `droste_zoom`, `escher_droste`, `false_color`, `faraday`, `filaments`, `film_grain`, `fireworks`, `flip_book`, `fractal_tree`, `fracture_grid`, `flux_warp`, `galaxy`, `geode`, `glitch`, `glyph_field`, `gradient_flow`, `halftone`, `hex_rush`, `hue_remap`, `impressionist`, `infinite_zoom`, `infinity_matrix`, `ink_wash`, `interference_warp`, `iris_rings`, `isoflow`, `kaleidoscope`, `kifs`, `kuwahara`, `laser_dance`, `lattice_crush`, `lattice_fold`, `led_cube`, `lego_bricks`, `lens_space`, `lichen`, `lotus_warp`, `marble`, `matrix_rain`.

**Depends on**: Wave 1 complete.

**Do** (for each effect file in the batch):
1. In `<name>.h`: add `#include "render/post_effect.h"` and an `inline <Name>Effect *Get<Name>Effect(PostEffect *pe) { return (<Name>Effect *)pe->effectStates[<TRANSFORM_TYPE>]; }`. Match `<Name>` to the registration macro's `Name` argument and `<TRANSFORM_TYPE>` to its `Type` argument.
2. In `<name>.cpp::Setup<Name>(PostEffect *pe)`: replace `&pe-><field>` with `Get<Name>Effect(pe)`. The `&pe->effects.<field>` config-side argument (when present) stays unchanged.

**Skip**: Files registered with `REGISTER_SIM_BOOST` (in `src/simulation/`, e.g. `physarum.cpp`, `curl_flow.cpp`). Sim-boost effects do not own a `<Name>Effect` member and need no accessor or bridge change.

**Verify**: After all of Wave 2 + Wave 3 land, no `pe-><field>.X` references remain in this batch.

---

#### Task 2.4: Simple effects batch B (alphabetical second half, ~70 effects)

**Files**: `src/effects/<name>.h` and `src/effects/<name>.cpp` for each effect from the alphabetical midpoint through `woodblock`, excluding the 5 effects handled in 2.1 and 2.2.

A representative concrete batch (companion to 2.3):

`mobius`, `moire_generator`, `moire_interference`, `motherboard`, `multi_scale_grid`, `muons`, `nebula`, `neon_lattice`, `oil_paint`, `orrery`, `palette_quantization`, `pencil_sketch`, `perspective_tilt`, `phi_blur`, `phyllotaxis`, `pitch_spiral`, `pixelation`, `plaid`, `plasma`, `poincare_disk`, `polygon_subdivide`, `polyhedral_mirror`, `polymorph`, `prism_shatter`, `protean_clouds`, `radial_ifs`, `radial_pulse`, `radial_streak`, `rainbow_road`, `random_volumetric`, `relativistic_doppler`, `ripple_tank`, `risograph`, `scan_bars`, `scrawl`, `shake`, `shard_crush`, `shell`, `signal_frames`, `slashes`, `slit_scan`, `snake_skin`, `solarize`, `solid_color`, `spark_flash`, `spectral_arcs`, `spectral_rings`, `spin_cage`, `spiral_march`, `spiral_nest`, `spiral_walk`, `star_trail`, `stripe_shift`, `subdivide`, `surface_depth`, `surface_warp`, `synapse_tree`, `synthwave`, `texture_warp`, `tone_warp`, `toon`, `triangle_fold`, `triskelion`, `twist_cage`, `voronoi`, `vortex`, `voxel_march`, `watercolor`, `wave_drift`, `wave_ripple`, `wave_warp`, `mandelbox`, `woodblock`.

**Depends on**: Wave 1 complete.

**Do**: Same as Task 2.3 — accessor in `<name>.h`, bridge update in `<name>.cpp::Setup<Name>`. Mechanical substitution.

**Skip**: Files registered with `REGISTER_SIM_BOOST`.

**Verify**: After all of Wave 2 + Wave 3 land, no `pe-><field>.X` references remain in this batch.

---

### Wave 3: Wire `PostEffectClearFeedback` and populate `effectStates[]`

#### Task 3.1: `post_effect.cpp` — populate slots in init, route ClearFeedback through accessors

**Files**: `src/render/post_effect.cpp`
**Depends on**: Wave 1 and Wave 2 complete (specifically requires `GetAttractorLinesEffect` and `GetCurlAdvectionEffect` accessors to exist).

**Do**:
1. At the top of `post_effect.cpp`, add `#include "effects/attractor_lines.h"` and `#include "effects/curl_advection.h"` (alphabetical group with other includes). Other effect headers are not needed here because `post_effect.cpp` does not reference any other effect's state directly.
2. In `PostEffectInit` (current line 135), insert the slot-population loop after `pe->effects = EffectConfig{};` (line 144) and before `LoadPostEffectShaders(pe)` (line 146):
   ```cpp
   for (int i = 0; i < TRANSFORM_EFFECT_COUNT; i++) {
     pe->effectStates[i] = EFFECT_DESCRIPTORS[i].state;
   }
   ```
   This must run before the descriptor init loop at lines 190-196 — placing it immediately after `pe->effects = EffectConfig{};` satisfies the ordering and matches the natural "set up bookkeeping fields first" cadence.
3. In `PostEffectClearFeedback` (current lines 321-375), rewrite the cross-cutting accesses per the Algorithm section:
   - Replace lines 340-346 (the `pe->attractorLines` block) with a hoisted `AttractorLinesEffect *al = GetAttractorLinesEffect(pe);` and use `al->pingPong[0]`, `al->pingPong[1]`, `al->readIdx`.
   - Replace lines 357-359 (the `pe->curlAdvection` block) with a hoisted `CurlAdvectionEffect *ca = GetCurlAdvectionEffect(pe);` and use `ca->statePingPong[0].id` and `CurlAdvectionEffectReset(ca, ...)`.
4. Leave every other line of `post_effect.cpp` unchanged. The four descriptor loops (init at 190-196, register-params at 232-237, uninit at 253-257, resize at 299-303) stay untouched — they are already descriptor-driven.

**Verify**: `cmake.exe --build build` succeeds with no warnings. Full build is now valid for the first time since Wave 1 began.

---

## Final Verification

- [ ] `cmake.exe --build build` succeeds with no warnings.
- [ ] Launch `./build/AudioJones.exe` and load a saved preset that exercises bloom, attractor_lines, and curl_advection. Visuals match pre-refactor (no garbage state, no crashes).
- [ ] Toggle each of the 145 effects on/off via the UI panels — descriptor `init`/`uninit` loops still drive lifecycle correctly.
- [ ] Resize the window — descriptor `resize` loop still dispatches to multi-pass effects (bloom, anamorphic_streak, byzantine, curl_advection, attractor_lines, light_medley).
- [ ] `PostEffectClearFeedback` (called from preset reload / panel button) clears `attractor_lines` and `curl_advection` state correctly.
- [ ] `grep -rn "pe->[a-z][a-zA-Z]*\." src/` returns zero matches against `<Name>Effect`-typed fields. Remaining matches are utility-state fields (`pe->accumTexture`, `pe->feedback*Loc`, `pe->blendCompositor`, simulation pointers, etc.) — these are out of scope.
- [ ] `src/render/post_effect.h` has zero `#include "effects/<name>.h"` lines.
- [ ] Adding a 146th effect requires zero edits to `post_effect.h` or `post_effect.cpp`.
