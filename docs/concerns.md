# Codebase Concerns

> Last sync: 2026-02-28 | Commit: 29551cf

## Tech Debt

**Duplicated GLSL Code:**
- Issue: 5 shaders define their own `hsv2rgb` function, 23 shaders duplicate inline luminance/luma calculations with inconsistent weights. Additional shared math (PI, noise, transforms) duplicated across shaders.
- Files with `hsv2rgb`: `shaders/color_grade.fs`, `shaders/physarum_agents.glsl`, `shaders/boids_agents.glsl`, `shaders/particle_life_agents.glsl`, `shaders/hue_remap.fs`
- Files with luminance: `shaders/hue_remap.fs`, `shaders/glitch.fs`, `shaders/watercolor.fs`, `shaders/halftone.fs`, `shaders/curl_flow_agents.glsl`, `shaders/boids_agents.glsl`, `shaders/physarum_agents.glsl`, `shaders/color_grade.fs`, `shaders/gradient_flow.fs`, `shaders/feedback.fs`, `shaders/false_color.fs`, `shaders/cross_hatching.fs`, `shaders/toon.fs`, `shaders/heightfield_relief.fs`, `shaders/effect_blend.fs`, `shaders/fxaa.fs`, `shaders/dot_matrix.fs`, `shaders/synthwave.fs`, `shaders/woodblock.fs`, `shaders/ink_wash.fs`, `shaders/texture_warp.fs`, `shaders/pencil_sketch.fs`, `shaders/ascii_art.fs`
- Impact: Inconsistent behavior (BT.601 `vec3(0.299, 0.587, 0.114)` vs BT.709 `vec3(0.2126, 0.7152, 0.0722)` luma weights), maintenance burden when fixing shared functions
- Fix approach: Shader include preprocessor per `docs/plans/shader-includes.md`

**PostEffect struct bloat (partially mitigated):**
- Issue: Effect modules own their shader handles and uniform locations. PostEffect struct holds 101 named effect struct fields as flat members plus 30+ feedback-related uniform location ints.
- Files: `src/render/post_effect.h` (301 lines), `src/render/post_effect.cpp` (366 lines)
- Impact: Each new effect adds one struct field to PostEffect plus init/uninit/register calls in post_effect.cpp.
- Fix approach: Store effects in an array indexed by type

**Static UI section state (largely resolved):**
- Issue: UI colocation replaced 90+ `static bool section*` variables across 16 category UI files with a single `g_effectSectionOpen[TRANSFORM_EFFECT_COUNT]` array in `src/ui/imgui_effects_dispatch.cpp`. 11 `static bool section*` variables remain across 3 files for non-effect UI sections (flow field, drawables, LFOs).
- Files: `src/ui/drawable_type_controls.cpp` (9 vars), `src/ui/imgui_effects.cpp` (1 var), `src/ui/imgui_lfo.cpp` (1 var)
- Impact: Remaining state resets on hot reload; cannot persist user preferences
- Fix approach: Move remaining into a UIState struct stored alongside app config

**Preset serialization split but still growing:**
- Issue: Preset serialization was split from a single 1132-line `preset.cpp` into `preset.cpp` (234 lines) + `effect_serialization.cpp` (607 lines, 103 NLOHMANN macros). The serialization file keeps growing with each new effect.
- Files: `src/config/preset.cpp`, `src/config/effect_serialization.cpp`
- Impact: Every new config struct requires a manual NLOHMANN_DEFINE macro and field listing. Missing fields silently load as defaults.
- Fix approach: Code generation or reflection-based serialization

## Known Bugs

None detected.

## Performance Bottlenecks

**param_registry string lookups:**
- Problem: Modulation engine uses std::unordered_map with string keys for every param access
- Files: `src/automation/modulation_engine.cpp`, `src/automation/param_registry.cpp`
- Cause: String hashing on every ModEngineUpdate call (multiple times per frame)
- Improvement path: Use integer IDs for hot paths; keep string lookup for UI/serialization only

## Fragile Areas

**PostEffect Init/Uninit Sequence:**
- Files: `src/render/post_effect.cpp`
- Why fragile: Sequential effect init calls with individual failure checks. Uninit order does not mirror init order. Adding an effect requires matching additions in init, uninit, resize, and register functions.
- Safe modification: Follow existing pattern exactly. Add new effect init in the same sequence block. Add matching uninit call.

**Transform effect dispatch table:**
- Files: `src/render/shader_setup.cpp` (446 lines), `src/config/effect_config.h` (97-entry enum), `src/config/effect_descriptor.h` (descriptor table)
- Why fragile: Adding an effect requires a new enum value, a descriptor table row, and a shader_setup.cpp dispatch case. The descriptor table consolidates name, category, enabled-check, and pipeline flags into one row, but the shader dispatch remains a separate switch.
- Safe modification: Follow `/add-effect` skill checklist; grep for an existing effect in the same category as template

**Preset Serialization:**
- Files: `src/config/effect_serialization.cpp` (607 lines), `src/config/preset.cpp` (234 lines)
- Why fragile: Every config struct requires a NLOHMANN_DEFINE macro and manual field listing. Missing fields silently load as defaults.
- Safe modification: Always test round-trip (save then load) when adding config fields

**Simulation init/uninit with goto cleanup:**
- Files: `src/simulation/physarum.cpp`, `src/simulation/boids.cpp`, `src/simulation/curl_flow.cpp`, `src/simulation/curl_advection.cpp`, `src/simulation/attractor_flow.cpp`, `src/simulation/particle_life.cpp`, `src/simulation/cymatics.cpp`, `src/simulation/trail_map.cpp`, `src/simulation/spatial_hash.cpp`
- Why fragile: Each simulation uses goto-based cleanup; missing a goto path on new allocation leaks resources
- Safe modification: Add new allocations immediately before the corresponding goto check; uninit in reverse order

**Drawable ID management:**
- Files: `src/automation/drawable_params.cpp`, `src/config/preset.cpp`, `src/ui/imgui_panels.h`
- Why fragile: Drawable IDs must stay synchronized between param registry, preset serialization, and UI state counter
- Safe modification: Always call `ImGuiDrawDrawablesSyncIdCounter` and `DrawableParamsSyncAll` after changing drawable array

**TransformEffectType enum ordering (mitigated):**
- Files: `src/config/effect_config.h`, `src/config/effect_serialization.cpp`
- Mitigated: String serialization (commit 74bd7d2) saves transform order as names, not integers. Loader handles both string names and legacy integer indices, and fills in missing effects from defaults. Enum reordering is safe.

**Modulation base value tracking:**
- Files: `src/automation/modulation_engine.cpp`
- Why fragile: LFO modulation applies offsets to stored base values; forgetting `ModEngineWriteBaseValues()` before preset save corrupts values
- Safe modification: Call `ModEngineWriteBaseValues()` before any operation that reads param values for persistence

## Large Files

| File | Lines | Concern |
|------|-------|---------|
| `src/ui/imgui_analysis.cpp` | 644 | Audio visualization UI |
| `src/config/effect_serialization.cpp` | 607 | 103 NLOHMANN macros for config structs |
| `src/config/effect_config.h` | 578 | 97-entry enum, 101 config fields |
| `src/simulation/particle_life.cpp` | 571 | GPU compute simulation |
| `src/simulation/physarum.cpp` | 536 | GPU compute simulation with colocated UI |
| `src/simulation/attractor_flow.cpp` | 536 | GPU compute simulation with colocated UI |
| `src/simulation/curl_flow.cpp` | 491 | GPU compute simulation |
| `src/simulation/boids.cpp` | 487 | GPU compute simulation |
| `src/ui/imgui_widgets.cpp` | 464 | Custom ImGui widgets |
| `src/ui/modulatable_slider.cpp` | 462 | LFO-modulatable slider widget |
| `src/effects/glitch.cpp` | 450 | Multi-sub-effect module with colocated UI |
| `src/render/shader_setup.cpp` | 446 | Switch-based transform effect dispatcher |
| `src/simulation/curl_advection.cpp` | 429 | GPU compute simulation |
| `src/effects/attractor_lines.cpp` | 377 | Generator with colocated UI |
| `src/render/post_effect.cpp` | 366 | Effect init/uninit/resize orchestration |
| `src/render/render_pipeline.cpp` | 350 | Frame rendering orchestration |
| `src/ui/gradient_editor.cpp` | 348 | Gradient editor widget |

## Complexity Hotspots

Functions with high cyclomatic complexity (measured by lizard):

| Function | Location | CCN | NLOC | Why |
|----------|----------|-----|------|-----|
| ImGuiDrawDrawablesPanel | `src/ui/imgui_drawables.cpp:20` | 42 | 199 | Drawable management with add/remove/reorder logic |
| from_json | `src/config/effect_serialization.cpp:148` | 25 | 76 | Deserializes 97+ config structs with fallback handling |
| ColorConfigEquals | `src/render/color_config.cpp:38` | 23 | 34 | Field-by-field equality comparison for 23 color config fields |
| AudioFeaturesProcess | `src/analysis/audio_features.cpp:19` | 19 | 102 | Multi-stage audio feature extraction pipeline |
| ModSourceGetName | `src/automation/mod_sources.cpp:46` | 19 | 42 | Switch over modulation source types |
| ModSourceGetColor | `src/automation/mod_sources.cpp:89` | 19 | 41 | Switch over modulation source types |
| ImGuiDrawEffectsPanel | `src/ui/imgui_effects.cpp:19` | 16 | 193 | Orchestrates effect category dispatch and simulation UI |

Note: CCN values carried forward from previous lizard run. Lizard was not available for re-measurement.

## Dependencies at Risk

**rlImGui (raylib-extras):**
- Risk: No native CMake support, manually compiled. Depends on ImGui docking branch (unstable).
- Impact: ImGui updates may break integration. Build failure if rlImGui API changes.
- Migration plan: Monitor for official raylib ImGui support. Pin to known-working commits.

**ImGui Docking Branch:**
- Risk: Not a stable release, tracks `docking` branch HEAD
- Impact: Breaking changes possible on `FetchContent` refresh
- Migration plan: Pin to specific commit hash instead of branch name

## Missing Features

**Hot Reload:**
- Problem: Shader changes require application restart
- Blocks: Rapid shader iteration during development

**~~Preset Versioning~~ (deferred):**
- No schema version in preset files. Not needed while solo-developing -- presets are manually updated alongside config changes. Revisit if the project gains users.

## TODO/FIXME Inventory

None detected. All lint suppressions have justification comments:

| Location | Type | Note |
|----------|------|------|
| `src/render/post_effect.cpp:59` | NOLINTNEXTLINE | readability-function-size - caches all shader uniform locations |
| `src/ui/imgui_effects.cpp:17` | NOLINTNEXTLINE | readability-function-size - immediate-mode UI requires sequential widget calls |
| `src/ui/imgui_panels.cpp:5` | NOLINTNEXTLINE | readability-function-size - theme setup requires setting all ImGui style colors |
| `src/ui/imgui_widgets.cpp:280` | NOLINTNEXTLINE | readability-function-size - UI widget with complex rendering and input handling |
| `src/ui/imgui_widgets.cpp:237` | NOLINTNEXTLINE | readability-isolate-declaration - output parameters for ImGui API |
| `src/ui/gradient_editor.cpp:138` | NOLINTNEXTLINE | readability-function-size - UI function with multiple input handling paths |
| `src/ui/modulatable_slider.cpp:166` | NOLINTNEXTLINE | readability-function-size - UI widget with detailed visual rendering |
| `src/ui/imgui_analysis.cpp:228,269,308,382` | NOLINTNEXTLINE | cert-err33-c - snprintf return value unused; buffer is fixed-size |
| `src/ui/imgui_analysis.cpp:395` | NOLINTNEXTLINE | readability-function-size - immediate-mode UI requires sequential widget calls |
| `src/ui/imgui_drawables.cpp:20` | NOLINTNEXTLINE | readability-function-size - immediate-mode UI requires sequential widget calls |
| `src/effects/oil_paint.cpp:40` | NOLINTBEGIN | concurrency-mt-unsafe - single-threaded init |
| `src/automation/lfo.cpp:45,47,64` | NOLINTNEXTLINE | concurrency-mt-unsafe - single-threaded visualizer, simple randomness sufficient |
| `src/analysis/analysis_pipeline.cpp:87` | NOLINTNEXTLINE | bugprone-integer-division - both operands explicitly cast to float |

---

*Run `/sync-docs` to regenerate.*
