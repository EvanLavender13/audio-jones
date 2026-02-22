# UI Colocation Refactor

Move each effect's DrawParams function from 14 monolithic `imgui_effects_<category>.cpp` files into each effect's own `.cpp` file. Extend the self-registration pattern so effects register DrawParams/DrawOutput alongside Init/Setup/Uninit. Replace 14 category files + 2 headers with a single framework dispatch function.

**Research**: `docs/research/ui-colocation-refactor.md`

## Design

### New Descriptor Fields

Add to `EffectDescriptor` after the `render` field:

| Field | Type | Default | Purpose |
|-------|------|---------|---------|
| `drawParams` | `DrawParamsFn` | `nullptr` | Effect-specific sliders |
| `drawOutput` | `DrawOutputFn` | `nullptr` | Generator output section (NULL for transforms) |

Typedefs:
```
typedef void (*DrawParamsFn)(EffectConfig *, const ModSources *, ImU32);
typedef void (*DrawOutputFn)(EffectConfig *, const ModSources *);
```

`drawParams` receives `categoryGlow` as `ImU32` so effects can pass it to `TreeNodeAccented()`.

Forward-declare in `effect_descriptor.h`: `struct ModSources;` and `typedef unsigned int ImU32;`

### Section Toggle State

Replace per-file `static bool sectionXxx` variables with a global array:

```
extern bool g_effectSectionOpen[TRANSFORM_EFFECT_COUNT];
```

Indexed by `TransformEffectType`. Owned by `imgui_effects_dispatch.cpp`.

### Category Info Table

Maps `categorySectionIndex` to display name + glow color index:

| Index | Category | Glow Index |
|-------|----------|------------|
| 0 | Symmetry | 0 |
| 1 | Warp | 1 |
| 2 | Cellular | 2 |
| 3 | Motion | 3 |
| 4 | Artistic | 4 |
| 5 | Graphic | 5 |
| 6 | Retro | 6 |
| 7 | Optical | 7 |
| 8 | Color | 8 |
| 9 | — | — |
| 10 | Geometric | 0 |
| 11 | Filament | 1 |
| 12 | Texture | 2 |
| 13 | Atmosphere | 3 |

### Macro Changes

**STANDARD_GENERATOR_OUTPUT(field)** — new macro in `effect_descriptor.h`. Generates a static `DrawOutput_##field(EffectConfig*, const ModSources*)` function that draws:
- `ImGui::SeparatorText("Output")`
- `ImGuiDrawColorMode(&e->field.gradient)`
- `ModulatableSlider("Blend Intensity##field", &e->field.blendIntensity, "field.blendIntensity", "%.2f", ms)`
- `ImGui::Combo("Blend Mode##field", ...)` with `BLEND_MODE_NAMES`

Uses `#field` for string literal concatenation in labels and param IDs.

**Updated REGISTER_EFFECT**: Add `DrawParamsFn` as last parameter. Wires `drawParams` in descriptor, `drawOutput = nullptr`.

New signature: `(Type, Name, field, displayName, badge, section, flags, SetupFn, ResizeFn, DrawParamsFn)`

**Updated REGISTER_EFFECT_CFG**: Same — add `DrawParamsFn` as last parameter.

**Updated REGISTER_GENERATOR**: Add `section` and `DrawParamsFn` and `DrawOutputFn` parameters. Replaces hardcoded `section = 10`.

New signature: `(Type, Name, field, displayName, SetupFn, ScratchSetupFn, section, DrawParamsFn, DrawOutputFn)`

**Updated REGISTER_GENERATOR_FULL**: Same — add `section`, `DrawParamsFn`, `DrawOutputFn`.

New signature: `(Type, Name, field, displayName, SetupFn, ScratchSetupFn, RenderFn, section, DrawParamsFn, DrawOutputFn)`

**Updated REGISTER_SIM_BOOST**: Add `DrawParamsFn` as last parameter (NULL — simulations out of scope).

All existing call sites updated to pass `NULL` for DrawParamsFn/DrawOutputFn (and `10` for generator section) in Wave 1.

### Framework Dispatch

New function in `imgui_effects_dispatch.cpp`:

```
void DrawEffectCategory(EffectConfig *e, const ModSources *modSources, int sectionIndex)
```

Logic:
1. Look up category name and glow index from `CATEGORY_INFO[sectionIndex]`
2. `DrawCategoryHeader(name, GetSectionGlow(glowIndex))`
3. Iterate `EFFECT_DESCRIPTORS[0..TRANSFORM_EFFECT_COUNT)` filtered by `categorySectionIndex == sectionIndex` and `drawParams != nullptr`
4. For each matching effect:
   - Resolve enabled: `bool *enabled = (bool *)((char *)e + desc.enabledOffset)`
   - `ImGui::PushID(desc.type)`
   - `DrawSectionBegin(desc.name, categoryGlow, &g_effectSectionOpen[desc.type], *enabled)`
   - Save `wasEnabled`, draw `ImGui::Checkbox("Enabled", enabled)`
   - If `!wasEnabled && *enabled`: `MoveTransformToEnd(&e->transformOrder, desc.type)`
   - If `*enabled`: call `desc.drawParams(e, modSources, categoryGlow)`
   - If `*enabled && desc.drawOutput`: call `desc.drawOutput(e, modSources)`
   - `DrawSectionEnd()`
   - `ImGui::PopID()`
   - `ImGui::Spacing()`

### Generator Field Normalization

All generators must have a `ColorConfig gradient` field for the standard output macro to work. Three generators deviate:

| Effect | Current Field | Rename To | Notes |
|--------|--------------|-----------|-------|
| Solid Color | `color` | `gradient` | Default stays `{.mode = COLOR_MODE_SOLID}` |
| Interference | `color` | `gradient` | Default stays `{.mode = COLOR_MODE_GRADIENT}` |
| Constellation | `pointGradient` | `gradient` | `lineGradient` stays, drawn in drawParams |

Each rename: update `.h` field + CONFIG_FIELDS macro, update `.cpp` references, rename key in preset JSON files. No migration code — just update the presets directly.

### Generator Sub-Category Indices

| Sub-Category | Old Index | New Index |
|---|---|---|
| Geometric | 10 | 10 |
| Filament | 10 | 11 |
| Texture | 10 | 12 |
| Atmosphere | 10 | 13 |

Set via the new `section` parameter in REGISTER_GENERATOR macros during migration.

### DrawParams Content

The `drawParams` function contains ONLY effect-specific sliders. The framework handles:
- SectionBegin / SectionEnd
- Enable checkbox
- MoveTransformToEnd on fresh enable
- Calling drawOutput (generators)

For generators with items that were in the old Output section but aren't standard (Nebula's Brightness, Moire's Color Mix + Brightness, Attractor Lines' transform controls): move those into drawParams.

### Effect File Layout (After Refactor)

```
// === Rendering ===
Init / Setup / Uninit / ConfigDefault / RegisterParams / bridge functions

// === UI ===
static void Draw<Name>Params(EffectConfig *e, const ModSources *ms, ImU32 glow) {
    // effect-specific sliders
}

// For generators only:
// clang-format off
STANDARD_GENERATOR_OUTPUT(field)
// clang-format on

// === Registration ===
// clang-format off
REGISTER_EFFECT(TYPE, Name, field, "Name", "BADGE", section,
                EFFECT_FLAG_NONE, SetupName, NULL, Draw<Name>Params)
// clang-format on
```

New includes added to effect `.cpp` files as needed: `"automation/mod_sources.h"`, `"imgui.h"`, `"ui/modulatable_slider.h"`, `"ui/ui_units.h"` (angle/speed sliders), `"ui/imgui_panels.h"` (TreeNodeAccented, ImGuiDrawColorMode), `"render/blend_mode.h"` (generators).

### imgui_effects.cpp Transition

Replace category function calls with `DrawEffectCategory`:
- `DrawSymmetryCategory(e, modSources)` → `DrawEffectCategory(e, modSources, 0)`
- `DrawWarpCategory(e, modSources)` → `DrawEffectCategory(e, modSources, 1)`
- through all 9 transform categories
- `DrawGeneratorsCategory(e, modSources)` → 4 calls: `DrawEffectCategory(e, modSources, 10)` through 13

Remove `#include "ui/imgui_effects_transforms.h"` and `#include "ui/imgui_effects_generators.h"`.

---

## Tasks

### Wave 1: Infrastructure

#### Task 1.1: Extend EffectDescriptor and registration macros

**Files**: `src/config/effect_descriptor.h`
**Creates**: DrawParamsFn/DrawOutputFn typedefs, updated macro signatures, STANDARD_GENERATOR_OUTPUT macro

**Do**:
- Add forward declarations: `struct ModSources;` and `typedef unsigned int ImU32;`
- Add DrawParamsFn and DrawOutputFn typedefs
- Add `drawParams` and `drawOutput` fields to EffectDescriptor (default nullptr, placed after `render`)
- Define `STANDARD_GENERATOR_OUTPUT(field)` macro — generates a static `DrawOutput_##field` function. Tokens like `ImGui::SeparatorText`, `ImGuiDrawColorMode`, `ModulatableSlider`, `BLEND_MODE_NAMES`, `EffectBlendMode` resolve at the expansion site (the effect .cpp file), not the definition site
- Update REGISTER_EFFECT: add `DrawParamsFn` as 10th parameter. Wire `drawParams` in descriptor, `drawOutput = nullptr`
- Update REGISTER_EFFECT_CFG: same
- Update REGISTER_GENERATOR: replace hardcoded `"GEN", 10` with `"GEN", section`. Add `section`, `DrawParamsFn`, `DrawOutputFn` parameters. Wire all three in descriptor
- Update REGISTER_GENERATOR_FULL: same changes
- Update REGISTER_SIM_BOOST: add `DrawParamsFn` as last parameter, wire in descriptor

**Verify**: Does NOT compile yet (call sites have wrong arg count). Task 1.2 fixes this.

#### Task 1.2: Update all registration call sites

**Files**: All 82 effect `.cpp` files using macros + 7 simulation `.cpp` files using REGISTER_SIM_BOOST (89 files total)
**Depends on**: Task 1.1

**Do**: Mechanical append to every registration macro call:
- 57 `REGISTER_EFFECT(...)` → append `, NULL` (DrawParamsFn)
- 2 `REGISTER_EFFECT_CFG(...)` → append `, NULL`
- 20 `REGISTER_GENERATOR(...)` → append `, 10, NULL, NULL` (section, DrawParamsFn, DrawOutputFn)
- 3 `REGISTER_GENERATOR_FULL(...)` → append `, 10, NULL, NULL`
- 7 `REGISTER_SIM_BOOST(...)` → append `, NULL`
- 5 manual `EffectDescriptorRegister` calls: no change needed (new fields default to nullptr)

**Verify**: `cmake.exe --build build` compiles.

#### Task 1.3: Create dispatch framework

**Files**: `src/ui/imgui_effects_dispatch.h` (new), `src/ui/imgui_effects_dispatch.cpp` (new), `CMakeLists.txt`

**Do**:
- Header: declare `DrawEffectCategory(EffectConfig*, const ModSources*, int sectionIndex)`, extern `g_effectSectionOpen[]`
- Implementation: define `g_effectSectionOpen[TRANSFORM_EFFECT_COUNT]` (zero-initialized), `CATEGORY_INFO[]` table (see Design section), `DrawEffectCategory` function (see Design section)
- Includes: `config/effect_config.h`, `config/effect_descriptor.h`, `automation/mod_sources.h`, `imgui.h`, `ui/imgui_panels.h`, `ui/theme.h`
- Add `src/ui/imgui_effects_dispatch.cpp` to CMakeLists.txt under IMGUI_UI_SOURCES

**Verify**: `cmake.exe --build build` compiles.

---

### Wave 2: Normalize generator fields

#### Task 2.1: Rename ColorConfig fields in 3 generators

**Files**: `src/effects/solid_color.h`, `src/effects/solid_color.cpp`, `src/effects/interference.h`, `src/effects/interference.cpp`, `src/effects/constellation.h`, `src/effects/constellation.cpp`, `src/config/effect_serialization.cpp`, `presets/*.json`

**Do**:
- **Solid Color**: In `solid_color.h`, rename `ColorConfig color` → `ColorConfig gradient` (keep default `{.mode = COLOR_MODE_SOLID}`). Update `SOLID_COLOR_CONFIG_FIELDS`: `color` → `gradient`. In `solid_color.cpp`, find-replace `cfg->color` → `cfg->gradient` (or however the field is accessed)
- **Interference**: In `interference.h`, rename `ColorConfig color` → `ColorConfig gradient`. Update `INTERFERENCE_CONFIG_FIELDS`: `color` → `gradient`. In `interference.cpp`, update references
- **Constellation**: In `constellation.h`, rename `ColorConfig pointGradient` → `ColorConfig gradient`. Update `CONSTELLATION_CONFIG_FIELDS`: `pointGradient` → `gradient`. In `constellation.cpp`, update references. `lineGradient` stays unchanged
- **Presets**: In every `presets/*.json` file, rename the old JSON key to the new key where it appears: `"color"` → `"gradient"` inside solidColor/interference objects, `"pointGradient"` → `"gradient"` inside constellation objects. No migration code in serialization — the auto-generated macros work as-is with the new field names.

**Verify**: `cmake.exe --build build` compiles. Presets load correctly.

---

### Wave 3: Pilot — Artistic category (6 effects)

Migrate 6 Artistic effects to validate the framework. STOP after this wave for manual testing.

#### Task 3.1: Oil Paint DrawParams

**Files**: `src/effects/oil_paint.cpp`

**Do**:
- Add includes: `"automation/mod_sources.h"`, `"imgui.h"`, `"ui/modulatable_slider.h"`
- Add `static void DrawOilPaintParams(EffectConfig *e, const ModSources *ms, ImU32 glow)` function containing the slider code from `imgui_effects_artistic.cpp` (the content inside the `if (e->oilPaint.enabled)` block — just the sliders, no checkbox, no SectionBegin/End)
- Oil paint uses manual `EffectDescriptorRegister`: add `.drawParams = DrawOilPaintParams` to the descriptor literal using designated initializer syntax

**Verify**: Compiles.

#### Task 3.2: Watercolor DrawParams

**Files**: `src/effects/watercolor.cpp`

**Do**: Same pattern as 3.1. Copy slider content from `imgui_effects_artistic.cpp` `DrawArtisticWatercolor` (the `if (enabled)` body). Change `REGISTER_EFFECT(...)` to pass `DrawWatercolorParams` as the last arg (replacing the NULL from Wave 1).

**Verify**: Compiles.

#### Task 3.3: Impressionist DrawParams

**Files**: `src/effects/impressionist.cpp`

**Do**: Same pattern. Copy from `DrawArtisticImpressionist`.

#### Task 3.4: Ink Wash DrawParams

**Files**: `src/effects/ink_wash.cpp`

**Do**: Same pattern. Copy from `DrawArtisticInkWash`.

#### Task 3.5: Pencil Sketch DrawParams

**Files**: `src/effects/pencil_sketch.cpp`

**Do**: Same pattern. Copy from `DrawArtisticPencilSketch`. Extra include: `"ui/imgui_panels.h"` for `TreeNodeAccented`. Pass the `glow` parameter to `TreeNodeAccented` calls.

#### Task 3.6: Cross-Hatching DrawParams

**Files**: `src/effects/cross_hatching.cpp`

**Do**: Same pattern. Copy from `DrawArtisticCrossHatching`.

#### Task 3.7: Switch Artistic dispatch and delete old file

**Files**: `src/ui/imgui_effects.cpp`, `src/ui/imgui_effects_artistic.cpp` (delete), `CMakeLists.txt`
**Depends on**: Tasks 3.1-3.6

**Do**:
- In `imgui_effects.cpp`: add `#include "ui/imgui_effects_dispatch.h"`. Replace `DrawArtisticCategory(e, modSources)` call with `DrawEffectCategory(e, modSources, 4)`
- Delete `src/ui/imgui_effects_artistic.cpp`
- Remove it from CMakeLists.txt IMGUI_UI_SOURCES

**Verify**: `cmake.exe --build build` compiles. **Run the app and confirm:** Artistic effects (Oil Paint, Watercolor, Impressionist, Ink Wash, Pencil Sketch, Cross-Hatching) show correct sliders, enable/disable works, section collapse works, TreeNodeAccented sub-sections in Pencil Sketch render correctly.

---

### Wave 4: Migrate remaining transforms (57 effects, 8 categories)

**Depends on**: Wave 3 verified

For each effect: add UI includes, create static `Draw<Name>Params` function (copy slider content from the old category file, strip boilerplate), update registration macro to pass DrawParams. Effects using manual `EffectDescriptorRegister` add `.drawParams` via designated initializer.

No file overlap between effects — all tasks in this wave run in parallel.

**Symmetry** (7 effects, copy from `imgui_effects_symmetry.cpp`):
Kaleidoscope, KIFS, Poincare Disk, Mandelbox, Triangle Fold, Moire Interference, Radial IFS.
Note: Mandelbox and Moire Interference use TreeNodeAccented — include `imgui_panels.h`.

**Warp** (15 effects, copy from `imgui_effects_warp.cpp`):
Sine Warp, Texture Warp, Gradient Flow, Wave Ripple, Mobius, Chladni Warp, Circuit Board, Domain Warp, Surface Warp, Interference Warp, Corridor Warp, Radial Pulse, Tone Warp, Flux Warp, Lens Space.
Note: Multiple use TreeNodeAccented — include `imgui_panels.h`. Several use `ui_units.h` for angle/speed sliders.

**Cellular** (6 effects, copy from `imgui_effects_cellular.cpp`):
Voronoi, Lattice Fold, Phyllotaxis, Multi-Scale Grid, Dot Matrix, Fracture Grid.

**Motion** (7 effects, copy from `imgui_effects_motion.cpp`):
Infinite Zoom, Radial Blur, Droste Zoom, Density Wave Spiral, Shake, Relativistic Doppler, Slit Scan Corridor.
Note: Slit Scan Corridor uses manual `EffectDescriptorRegister`. Multiple use TreeNodeAccented.

**Graphic** (6 effects, copy from `imgui_effects_graphic.cpp`):
Toon, Neon Glow, Kuwahara, Halftone, Disco Ball, LEGO Bricks.
Note: Toon, Neon Glow, Disco Ball use TreeNodeAccented.

**Retro** (7 effects, copy from `imgui_effects_retro.cpp`):
Pixelation, Glitch, CRT, ASCII Art, Matrix Rain, Synthwave, Lattice Crush.
Note: Glitch has many TreeNodeAccented sub-sections. CRT and Synthwave have several too.

**Optical** (5 effects, copy from `imgui_effects_optical.cpp`):
Bloom, Bokeh, Heightfield Relief, Anamorphic Streak, Phi Blur.
Note: Bloom and Anamorphic Streak use manual `EffectDescriptorRegister`.

**Color** (4 effects, copy from `imgui_effects_color.cpp`):
Color Grade, False Color, Palette Quantization, Hue Remap.
Note: Color Grade uses TreeNodeAccented.

**Verify**: `cmake.exe --build build` compiles after all tasks complete.

---

### Wave 5: Migrate all generators (24 effects, 4 sub-categories)

**Depends on**: Wave 4

For each generator: add UI includes, create static `Draw<Name>Params` function (copy ALL effect-specific sections EXCEPT the Output section), invoke `STANDARD_GENERATOR_OUTPUT(field)` before the registration macro, update `REGISTER_GENERATOR` / `REGISTER_GENERATOR_FULL` to pass the correct section index, DrawParams function, and `DrawOutput_##field`.

**DrawParams includes the Audio section** (Base Freq, Max Freq, Gain, Contrast, Base Bright) — this is effect-specific, not part of the standard Output.

**UI deviations to normalize** (move from old Output section into drawParams):
- **Nebula**: "Brightness" slider → last item in drawParams before Output
- **Moire Generator**: "Color Mix" and "Brightness" sliders → drawParams. Uses manual `EffectDescriptorRegister` — add `.drawParams` and `.drawOutput` via designated initializers
- **Attractor Lines**: Transform controls (X, Y, angles, speeds) → drawParams "Transform" section. Blend Mode/Intensity order normalized by standard output macro
- **Constellation**: `lineGradient` drawn via `ImGuiDrawColorMode` in drawParams "Lines" section. Standard output handles `gradient` (renamed from `pointGradient` in Wave 2)
- **Muons**: Color mode already in a separate section — keep in drawParams. Standard output handles the rest
- **Interference**: Color mode conditionals (colorMode-based visibility) stay in drawParams. Standard output handles gradient/blend

No file overlap — all tasks parallel.

**Geometric** (6 effects, section 10, copy from `imgui_effects_gen_geometric.cpp`):
Signal Frames, Arc Strobe, Hex Rush, Pitch Spiral, Spectral Arcs, Iris Rings.

**Filament** (5 effects, section 11, copy from `imgui_effects_gen_filament.cpp`):
Constellation, Filaments, Muons, Slashes, Attractor Lines.

**Texture** (10 effects, section 12, copy from `imgui_effects_gen_texture.cpp`):
Plasma, Interference, Moire Generator, Scan Bars, Glyph Field, Motherboard, Bit Crush, Data Traffic, Plaid, Scrawl.

**Atmosphere** (3 effects, section 13, copy from `imgui_effects_gen_atmosphere.cpp`):
Nebula, Fireworks, Solid Color.

**Verify**: `cmake.exe --build build` compiles.

---

### Wave 6: Switchover and cleanup

#### Task 6.1: Replace all category dispatches

**Files**: `src/ui/imgui_effects.cpp`
**Depends on**: Waves 4 and 5

**Do**:
- Replace remaining category calls with `DrawEffectCategory`:
  - `DrawSymmetryCategory(e, modSources)` → `DrawEffectCategory(e, modSources, 0)`
  - `DrawWarpCategory(e, modSources)` → `DrawEffectCategory(e, modSources, 1)`
  - `DrawCellularCategory(e, modSources)` → `DrawEffectCategory(e, modSources, 2)`
  - `DrawMotionCategory(e, modSources)` → `DrawEffectCategory(e, modSources, 3)`
  - `DrawGraphicCategory(e, modSources)` → `DrawEffectCategory(e, modSources, 5)`
  - `DrawRetroCategory(e, modSources)` → `DrawEffectCategory(e, modSources, 6)`
  - `DrawOpticalCategory(e, modSources)` → `DrawEffectCategory(e, modSources, 7)`
  - `DrawColorCategory(e, modSources)` → `DrawEffectCategory(e, modSources, 8)`
  - Replace `DrawGeneratorsCategory(e, modSources)` with 4 calls:
    ```
    DrawEffectCategory(e, modSources, 10);
    ImGui::Spacing();
    DrawEffectCategory(e, modSources, 11);
    ImGui::Spacing();
    DrawEffectCategory(e, modSources, 12);
    ImGui::Spacing();
    DrawEffectCategory(e, modSources, 13);
    ```
- Remove `#include "ui/imgui_effects_transforms.h"` and `#include "ui/imgui_effects_generators.h"`

#### Task 6.2: Delete old category files

**Files**: 15 files to delete, `CMakeLists.txt`

**Do**: Delete all old category UI files and headers:
- `src/ui/imgui_effects_artistic.cpp` (already deleted in Wave 3)
- `src/ui/imgui_effects_symmetry.cpp`
- `src/ui/imgui_effects_warp.cpp`
- `src/ui/imgui_effects_cellular.cpp`
- `src/ui/imgui_effects_motion.cpp`
- `src/ui/imgui_effects_graphic.cpp`
- `src/ui/imgui_effects_retro.cpp`
- `src/ui/imgui_effects_optical.cpp`
- `src/ui/imgui_effects_color.cpp`
- `src/ui/imgui_effects_generators.cpp`
- `src/ui/imgui_effects_gen_geometric.cpp`
- `src/ui/imgui_effects_gen_filament.cpp`
- `src/ui/imgui_effects_gen_texture.cpp`
- `src/ui/imgui_effects_gen_atmosphere.cpp`
- `src/ui/imgui_effects_transforms.h`
- `src/ui/imgui_effects_generators.h`

Remove all from CMakeLists.txt.

**Verify**: `cmake.exe --build build` compiles with zero warnings. Run app — all effects work.

---

## Final Verification

- [ ] Build succeeds with zero warnings
- [ ] All 63 transform effects show correct sliders
- [ ] All 24 generator effects show correct sliders + standard Output section
- [ ] Enable/disable toggles work (checkbox + MoveTransformToEnd)
- [ ] Section collapse/expand works via g_effectSectionOpen
- [ ] Category headers and glow colors match original layout
- [ ] TreeNodeAccented sub-sections render correctly (Glitch, Mandelbox, CRT, Synthwave, etc.)
- [ ] Generator sub-categories display under correct headings (Geometric/Filament/Texture/Atmosphere)
- [ ] Preset loading works including old presets with renamed fields
- [ ] No old category .cpp or .h files remain
- [ ] 14 category files deleted, 2 headers deleted, 2 new files created (dispatch .h/.cpp)
