# Simulation UI Colocation & Rename

Move simulation effect UIs from `src/ui/imgui_effects.cpp` into each simulation's own `.cpp` file, matching the colocated pattern established for transforms and generators. Rename display names from "X Boost" to "X" and clean up enum names to match.

## Design

### Enum Renames

| Old | New |
|-----|-----|
| `TRANSFORM_PHYSARUM_BOOST` | `TRANSFORM_PHYSARUM` |
| `TRANSFORM_CURL_FLOW_BOOST` | `TRANSFORM_CURL_FLOW` |
| `TRANSFORM_CURL_ADVECTION_BOOST` | `TRANSFORM_CURL_ADVECTION` |
| `TRANSFORM_ATTRACTOR_FLOW_BOOST` | `TRANSFORM_ATTRACTOR_FLOW` |
| `TRANSFORM_BOIDS_BOOST` | `TRANSFORM_BOIDS` |
| `TRANSFORM_CYMATICS_BOOST` | `TRANSFORM_CYMATICS` |
| `TRANSFORM_PARTICLE_LIFE_BOOST` | `TRANSFORM_PARTICLE_LIFE` |

### Display Name Renames

| Old | New |
|-----|-----|
| `"Physarum Boost"` | `"Physarum"` |
| `"Curl Flow Boost"` | `"Curl Flow"` |
| `"Curl Advection Boost"` | `"Curl Advection"` |
| `"Attractor Flow Boost"` | `"Attractor Flow"` |
| `"Boids Boost"` | `"Boids"` |
| `"Cymatics Boost"` | `"Cymatics"` |
| `"Particle Life Boost"` | `"Particle Life"` |

### Section 9 Activation

Enable section index 9 in `CATEGORY_INFO` as `{"Simulation", 4}` (glowIndex 4, following Artistic's slot). All 7 sims share one glow color, consistent with other categories.

### Colocated UI Pattern

Each sim `.cpp` file gets a static `DrawParams` function matching the `DrawParamsFn` signature:

```
static void DrawPhysarumParams(EffectConfig *e, const ModSources *ms, ImU32 glow)
```

The function body contains the exact slider/checkbox/combo code currently in `imgui_effects.cpp` for that sim, but without the `DrawSectionBegin`/`Checkbox("Enabled")`/`DrawSectionEnd` wrapper (the dispatch framework handles that). The `DrawParams` function pointer is passed as the last argument to `REGISTER_SIM_BOOST`.

### New Includes for Sim `.cpp` Files

Each sim `.cpp` file needs these additional includes for UI drawing:

- `"automation/mod_sources.h"` — `ModSources` struct
- `"imgui.h"` — ImGui widgets
- `"ui/modulatable_slider.h"` — `ModulatableSlider`, `ModulatableSliderAngleDeg`, `ModulatableSliderSpeedDeg`, `ModulatableSliderLog`
- `"ui/imgui_panels.h"` — `ImGuiDrawColorMode`, `DrawLissajousControls`
- `"ui/ui_units.h"` — `DrawLissajousControls`
- `"render/blend_mode.h"` — `BLEND_MODE_NAMES`, `BLEND_MODE_NAME_COUNT`

Some sims already include a subset of these. Only add what's missing.

### Static String Arrays

Mode option arrays currently in `imgui_effects.cpp` move into the respective sim `.cpp` files:

- `PHYSARUM_BOUNDS_MODES[]` and `PHYSARUM_WALK_MODES[]` → `physarum.cpp`
- `BOIDS_BOUNDS_MODES[]` → `boids.cpp`
- `attractorTypes[]` (attractor type names) → `attractor_flow.cpp`

### imgui_effects.cpp Cleanup

Remove from `imgui_effects.cpp`:
- All `static bool section*` variables for simulations (7 vars)
- All `static const char*` mode arrays (4 arrays)
- The entire SIMULATIONS GROUP block (~510 lines, from `sectionPhysarum` through `DrawSectionEnd()` of Particle Life)
- The `#include "simulation/bounds_mode.h"` and `#include "simulation/physarum.h"` includes

Replace the removed SIMULATIONS GROUP with the dispatch call pattern:

```cpp
// SIMULATIONS GROUP
ImGui::Spacing();
ImGui::Spacing();
DrawGroupHeader("SIMULATIONS", Theme::GetSectionAccent(groupIdx++));
DrawEffectCategory(e, modSources, 9);
```

### Preset Updates

Update `transformOrder` arrays in all presets that reference simulation names:

| Preset | Old | New |
|--------|-----|-----|
| LINKY | `"Physarum Boost"` | `"Physarum"` |
| WINNY | `"Physarum Boost"` | `"Physarum"` |
| SMOOTHBOB | `"Physarum Boost"` | `"Physarum"` |
| SOUPB | `"Physarum Boost"` | `"Physarum"` |
| ZOOP | `"Physarum Boost"` | `"Physarum"` |
| WOBBYBOB | `"Physarum Boost"` | `"Physarum"` |
| GRAYBOB | `"Attractor Flow Boost"`, `"Curl Flow Boost"` | `"Attractor Flow"`, `"Curl Flow"` |
| SOUPA | `"Physarum Boost"` | `"Physarum"` |
| CYMATICBOB | `"Cymatics Boost"` | `"Cymatics"` |
| ICEY | `"Physarum Boost"` | `"Physarum"` |
| GLITCHYBOB | `"Physarum Boost"` | `"Physarum"` |
| BINGBANG | `"Physarum Boost"` | `"Physarum"` |
| OOPY | `"Curl Advection Boost"` | `"Curl Advection"` |

---

## Tasks

### Wave 1: Enum and Dispatch Infrastructure

#### Task 1.1: Rename enums and activate section 9

**Files**: `src/config/effect_config.h`, `src/ui/imgui_effects_dispatch.cpp`

**Do**:
- In `effect_config.h`: rename all 7 `TRANSFORM_*_BOOST` enum values to drop `_BOOST` suffix (see Enum Renames table)
- In `imgui_effects_dispatch.cpp`: change `CATEGORY_INFO[9]` from `{nullptr, -1}` to `{"Simulation", 4}`

**Verify**: `cmake.exe --build build` compiles (will fail until Wave 2 updates references).

---

### Wave 2: Colocate Sim UIs (7 parallel tasks)

All 7 tasks depend on Wave 1.

#### Task 2.1: Physarum UI colocation

**Files**: `src/simulation/physarum.cpp`
**Depends on**: Wave 1

**Do**:
- Update `REGISTER_SIM_BOOST` enum from `TRANSFORM_PHYSARUM_BOOST` to `TRANSFORM_PHYSARUM`, display name from `"Physarum Boost"` to `"Physarum"`, and `DrawParamsFnArg` from `NULL` to `DrawPhysarumParams`
- Add missing includes: `"automation/mod_sources.h"`, `"imgui.h"`, `"ui/modulatable_slider.h"`, `"ui/imgui_panels.h"`, `"ui/ui_units.h"` (already has `"render/blend_mode.h"` via header)
- Move `PHYSARUM_BOUNDS_MODES[]`, `PHYSARUM_BOUNDS_MODE_COUNT`, `PHYSARUM_WALK_MODES[]`, `PHYSARUM_WALK_MODE_COUNT` from `imgui_effects.cpp` into `physarum.cpp` as file-static arrays
- Add static `DrawPhysarumParams(EffectConfig*, const ModSources*, ImU32)` containing the physarum slider/checkbox code from `imgui_effects.cpp` lines 156-253 (everything between the Enabled checkbox and DrawSectionEnd, exclusive — the dispatch handles enabled/section)

**Verify**: Compiles.

#### Task 2.2: Curl Flow UI colocation

**Files**: `src/simulation/curl_flow.cpp`
**Depends on**: Wave 1

**Do**:
- Update `REGISTER_SIM_BOOST` enum from `TRANSFORM_CURL_FLOW_BOOST` to `TRANSFORM_CURL_FLOW`, display name from `"Curl Flow Boost"` to `"Curl Flow"`, `DrawParamsFnArg` from `NULL` to `DrawCurlFlowParams`
- Add missing includes: `"automation/mod_sources.h"`, `"imgui.h"`, `"ui/modulatable_slider.h"`, `"ui/imgui_panels.h"`, `"render/blend_mode.h"`
- Add static `DrawCurlFlowParams` with slider code from `imgui_effects.cpp` lines 264-304

**Verify**: Compiles.

#### Task 2.3: Curl Advection UI colocation

**Files**: `src/simulation/curl_advection.cpp`
**Depends on**: Wave 1

**Do**:
- Update `REGISTER_SIM_BOOST` enum from `TRANSFORM_CURL_ADVECTION_BOOST` to `TRANSFORM_CURL_ADVECTION`, display name from `"Curl Advection Boost"` to `"Curl Advection"`, `DrawParamsFnArg` from `NULL` to `DrawCurlAdvectionParams`
- Add missing includes: `"automation/mod_sources.h"`, `"imgui.h"`, `"ui/modulatable_slider.h"`, `"ui/imgui_panels.h"`, `"render/blend_mode.h"`
- Add static `DrawCurlAdvectionParams` with slider code from `imgui_effects.cpp` lines 473-523

**Verify**: Compiles.

#### Task 2.4: Attractor Flow UI colocation

**Files**: `src/simulation/attractor_flow.cpp`
**Depends on**: Wave 1

**Do**:
- Update `REGISTER_SIM_BOOST` enum from `TRANSFORM_ATTRACTOR_FLOW_BOOST` to `TRANSFORM_ATTRACTOR_FLOW`, display name from `"Attractor Flow Boost"` to `"Attractor Flow"`, `DrawParamsFnArg` from `NULL` to `DrawAttractorFlowParams`
- Add missing includes: `"automation/mod_sources.h"`, `"imgui.h"`, `"ui/modulatable_slider.h"`, `"ui/imgui_panels.h"`, `"render/blend_mode.h"`
- Move `attractorTypes[]` array from `imgui_effects.cpp` into `attractor_flow.cpp` as file-static
- Add static `DrawAttractorFlowParams` with slider code from `imgui_effects.cpp` lines 315-403

**Verify**: Compiles.

#### Task 2.5: Boids UI colocation

**Files**: `src/simulation/boids.cpp`
**Depends on**: Wave 1

**Do**:
- Update `REGISTER_SIM_BOOST` enum from `TRANSFORM_BOIDS_BOOST` to `TRANSFORM_BOIDS`, display name from `"Boids Boost"` to `"Boids"`, `DrawParamsFnArg` from `NULL` to `DrawBoidsParams`
- Add missing includes: `"automation/mod_sources.h"`, `"imgui.h"`, `"ui/modulatable_slider.h"`, `"ui/imgui_panels.h"`, `"render/blend_mode.h"`
- Move `BOIDS_BOUNDS_MODES[]` from `imgui_effects.cpp` into `boids.cpp` as file-static
- Add static `DrawBoidsParams` with slider code from `imgui_effects.cpp` lines 413-463

**Verify**: Compiles.

#### Task 2.6: Cymatics UI colocation

**Files**: `src/simulation/cymatics.cpp`
**Depends on**: Wave 1

**Do**:
- Update `REGISTER_SIM_BOOST` enum from `TRANSFORM_CYMATICS_BOOST` to `TRANSFORM_CYMATICS`, display name from `"Cymatics Boost"` to `"Cymatics"`, `DrawParamsFnArg` from `NULL` to `DrawCymaticsParams`
- Add missing includes: `"automation/mod_sources.h"`, `"imgui.h"`, `"ui/modulatable_slider.h"`, `"ui/imgui_panels.h"`, `"ui/ui_units.h"`, `"render/blend_mode.h"`
- Add static `DrawCymaticsParams` with slider code from `imgui_effects.cpp` lines 533-571

**Verify**: Compiles.

#### Task 2.7: Particle Life UI colocation

**Files**: `src/simulation/particle_life.cpp`
**Depends on**: Wave 1

**Do**:
- Update `REGISTER_SIM_BOOST` enum from `TRANSFORM_PARTICLE_LIFE_BOOST` to `TRANSFORM_PARTICLE_LIFE`, display name from `"Particle Life Boost"` to `"Particle Life"`, `DrawParamsFnArg` from `NULL` to `DrawParticleLifeParams`
- Add missing includes: `"automation/mod_sources.h"`, `"imgui.h"`, `"ui/modulatable_slider.h"`, `"ui/imgui_panels.h"`, `"render/blend_mode.h"`
- Add static `DrawParticleLifeParams` with slider code from `imgui_effects.cpp` lines 581-655

**Verify**: Compiles.

---

### Wave 3: Cleanup and Preset Updates

#### Task 3.1: Clean up imgui_effects.cpp

**Files**: `src/ui/imgui_effects.cpp`
**Depends on**: Wave 2

**Do**:
- Remove includes: `"simulation/bounds_mode.h"`, `"simulation/physarum.h"`
- Remove all 7 `static bool section*` variables for sims (`sectionPhysarum` through `sectionParticleLife`)
- Remove `sectionFlowField` only if it's unused after sim removal (it's for Flow Field in the FEEDBACK group — keep it)
- Remove the 4 static string arrays: `PHYSARUM_BOUNDS_MODES[]`, `PHYSARUM_BOUNDS_MODE_COUNT`, `BOIDS_BOUNDS_MODES[]`, `PHYSARUM_WALK_MODES[]`, `PHYSARUM_WALK_MODE_COUNT`
- Replace the entire SIMULATIONS GROUP block (lines 144-658) with:
  ```cpp
  // SIMULATIONS GROUP
  ImGui::Spacing();
  ImGui::Spacing();
  DrawGroupHeader("SIMULATIONS", Theme::GetSectionAccent(groupIdx++));
  DrawEffectCategory(e, modSources, 9);
  ```

**Verify**: `cmake.exe --build build` compiles.

#### Task 3.2: Update preset files

**Files**: 13 preset JSON files (see Preset Updates table)
**Depends on**: Wave 1

**Do**: For each preset in the table, replace old `"X Boost"` strings with `"X"` in the `transformOrder` array. Match exactly the strings in the Preset Updates table.

**Verify**: JSON is valid (no trailing commas, balanced brackets).

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] All 7 sims appear in the SIMULATIONS section of the Effects panel
- [ ] Sim enable/disable toggles work
- [ ] Sim sections expand/collapse to show params
- [ ] Transform order list shows "Physarum" (not "Physarum Boost") etc.
- [ ] Presets referencing sims load correctly
