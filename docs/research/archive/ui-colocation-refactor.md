# UI Colocation Refactor

Move each effect's UI (DrawParams function) from monolithic `imgui_effects_<category>.cpp` files into each effect's own `.cpp` file. Extend the existing self-registration pattern so effects register their DrawUI alongside Init/Setup/Uninit.

## Classification

- **Category**: General (architecture refactor)
- **Scope**: Transforms (63 effects in 9 category files) and Generators (24 effects in 5 category files)
- **Out of scope**: Simulations (7 in imgui_effects.cpp), Feedback/Output (small, stay inline)

## Current State

**Monolithic category files** (14 files, ~4,686 lines total):
- `imgui_effects_artistic.cpp` (6 effects), `imgui_effects_warp.cpp` (15 effects), etc.
- Each file has static `bool sectionXxx` toggles, static `DrawCategoryXxx()` functions per effect
- `imgui_effects_generators.cpp` routes to 4 sub-category files (`gen_geometric`, `gen_filament`, `gen_texture`, `gen_atmosphere`)

**Every transform effect Draw function has identical boilerplate**:
- `DrawSectionBegin(name, glow, &sectionToggle, enabled)`
- Enable checkbox + `MoveTransformToEnd()` on fresh enable
- Draw params only if enabled
- `DrawSectionEnd()`

**Every generator additionally has identical output boilerplate**:
- `ImGuiDrawColorMode(&cfg->gradient)`
- `ModulatableSlider("Blend Intensity##...", ...)`
- `ImGui::Combo("Blend Mode##...", ...)`

**Effect .cpp files** currently have zero UI code — pure rendering logic.

## Design

### 1. New EffectDescriptor Fields

Add two function pointers to `EffectDescriptor`:

```
DrawParamsFn drawParams   — draws effect-specific parameter sliders
DrawParamsFn drawOutput   — draws output section (generators only, NULL for transforms)
```

Signature: `void (*)(EffectConfig*, const ModSources*)`

### 2. Section Toggle State

Replace per-file `static bool sectionXxx` variables with a single mutable array:

```
bool g_effectSectionOpen[TRANSFORM_EFFECT_COUNT]
```

Indexed by `TransformEffectType`. Owned by the framework (in a new `imgui_effects_dispatch.cpp` or in `imgui_effects.cpp`).

### 3. Framework Wrapper

A single dispatch function replaces all 9 transform category functions:

**For each effect in category:**
1. `DrawSectionBegin(desc.name, categoryGlow, &g_sectionOpen[type], *enabled)`
2. Enable checkbox (label derived from type index)
3. `MoveTransformToEnd()` on fresh enable (transforms only)
4. Call `desc.drawParams(e, modSources)` if enabled
5. Call `desc.drawOutput(e, modSources)` if non-NULL and enabled
6. `DrawSectionEnd()`

This eliminates the identical boilerplate from every effect.

### 4. Generator Output Macro

A macro auto-generates the standard output function for generators:

```
STANDARD_GENERATOR_OUTPUT(field, paramPrefix)
```

Expands to a static function that draws: color mode (gradient), blend intensity slider, blend mode combo. The field name is stringified to derive the param ID prefix (e.g., `"nebula.blendIntensity"`).

Effects with non-standard output (e.g., Solid Color uses `color` not `gradient`) register a custom drawOutput function instead.

### 5. Registration Macro Extensions

Extend existing macros to accept the DrawParams function:

- `REGISTER_EFFECT(...)` adds a `DrawParamsFn` parameter, `drawOutput = NULL`
- `REGISTER_GENERATOR(...)` adds a `DrawParamsFn` parameter, auto-generates drawOutput via `STANDARD_GENERATOR_OUTPUT`
- All other macro variants updated similarly

### 6. Generator Sub-Category Indices

Currently all generators share `categorySectionIndex = 10`. Split into:

| Sub-Category | Index |
|---|---|
| Geometric | 10 |
| Filament | 11 |
| Texture | 12 |
| Atmosphere | 13 |

This lets the framework group generators by sub-category using the same mechanism as transform categories.

### 7. Category Dispatch

Replace the 9 transform category functions and 4 generator sub-category functions with a single parametric function:

```
DrawEffectCategory(EffectConfig* e, const ModSources* modSources, int sectionIndex)
```

This loops over `EFFECT_DESCRIPTORS[]`, filters by `categorySectionIndex`, and calls the wrapper pattern from step 3.

### 8. imgui_effects.cpp Changes

Current manual dispatch:
```
DrawSymmetryCategory(e, modSources);
DrawWarpCategory(e, modSources);
...
DrawGeneratorsGeometric(e, modSources);
...
```

Becomes:
```
DrawEffectCategory(e, modSources, 0);  // Symmetry
DrawEffectCategory(e, modSources, 1);  // Warp
...
DrawEffectCategory(e, modSources, 10); // Generators: Geometric
DrawEffectCategory(e, modSources, 11); // Generators: Filament
...
```

### 9. Effect .cpp File Layout (After Refactor)

Each effect file gains a DrawParams function at the bottom, above the registration macro:

```
// === Rendering ===
Init / Setup / Uninit / ConfigDefault / RegisterParams

// === UI ===
static void DrawXxxParams(EffectConfig* e, const ModSources* ms) { ... }

// === Registration ===
REGISTER_EFFECT(TYPE, Name, field, "Display Name", ..., DrawXxxParams)
```

New includes added to effect files: `"imgui.h"`, `"ui/modulatable_slider.h"`, `"ui/ui_units.h"`, `"ui/imgui_panels.h"`, `"ui/theme.h"` (as needed).

## Migration Strategy

This is a large refactor touching 87+ files. Recommended approach:

### Wave 1: Infrastructure
- Add `drawParams` and `drawOutput` to `EffectDescriptor`
- Add `g_effectSectionOpen[]` array
- Add `STANDARD_GENERATOR_OUTPUT` macro
- Create `DrawEffectCategory()` dispatch function
- Update all `REGISTER_*` macros to accept DrawParams (NULL initially for all effects)

### Wave 2: Migrate One Category (Pilot)
- Pick a small category (e.g., Artistic — 6 effects)
- Move each effect's Draw function into its .cpp file as DrawParams
- Update registration macros to pass the function
- Verify the framework wrapper renders identically
- Delete `imgui_effects_artistic.cpp`

### Wave 3-10: Migrate Remaining Categories
- One category per wave (or batch small ones together)
- Each wave: move DrawParams into effect files, delete category file
- Build and verify after each wave

### Wave 11: Cleanup
- Delete `imgui_effects_transforms.h`, `imgui_effects_generators.h`
- Remove category dispatch functions from imgui_effects.cpp
- Update CMakeLists.txt to remove deleted files

## Files Affected

**New/Modified (infrastructure):**
- `src/config/effect_descriptor.h` — new fields + macro updates
- `src/ui/imgui_effects.cpp` (or new `imgui_effects_dispatch.cpp`) — framework dispatch

**Modified (per effect, 87 files):**
- `src/effects/<name>.cpp` — add DrawParams function + update registration

**Deleted (14 files):**
- `src/ui/imgui_effects_artistic.cpp`
- `src/ui/imgui_effects_cellular.cpp`
- `src/ui/imgui_effects_color.cpp`
- `src/ui/imgui_effects_graphic.cpp`
- `src/ui/imgui_effects_motion.cpp`
- `src/ui/imgui_effects_optical.cpp`
- `src/ui/imgui_effects_retro.cpp`
- `src/ui/imgui_effects_symmetry.cpp`
- `src/ui/imgui_effects_warp.cpp`
- `src/ui/imgui_effects_gen_geometric.cpp`
- `src/ui/imgui_effects_gen_filament.cpp`
- `src/ui/imgui_effects_gen_texture.cpp`
- `src/ui/imgui_effects_gen_atmosphere.cpp`
- `src/ui/imgui_effects_generators.cpp`

**Deleted (2 headers):**
- `src/ui/imgui_effects_transforms.h`
- `src/ui/imgui_effects_generators.h`

## Notes

- Simulations (physarum, boids, etc.) are out of scope — their UI stays in imgui_effects.cpp. A follow-up refactor could bring them into the descriptor system.
- The framework wrapper needs to handle the ImGui ID suffix (##name) to prevent label collisions. The effect's TransformEffectType index or the display name can serve as the suffix.
- `MoveTransformToEnd` is only called for effects using the transform order pipeline. Generators use `TRANSFORM_*_BLEND` types but the same mechanism applies.
- Category names and glow color indices are currently hardcoded in category functions. The framework needs a small lookup table mapping section index to category display name.
