# Codebase Concerns

> Last sync: 2026-02-20 | Commit: 6b8481f

## Tech Debt

**Duplicated GLSL Code:**
- Issue: 6 shaders define their own `hsv2rgb` function, 19 shaders duplicate inline luminance/luma calculations with inconsistent weights. Additional shared math (PI, noise, transforms) duplicated across shaders.
- Files with `hsv2rgb`: `shaders/neon_glow.fs`, `shaders/color_grade.fs`, `shaders/physarum_agents.glsl`, `shaders/boids_agents.glsl`, `shaders/particle_life_agents.glsl`, `shaders/hue_remap.fs`
- Files with luminance: `shaders/hue_remap.fs`, `shaders/glitch.fs`, `shaders/watercolor.fs`, `shaders/halftone.fs`, `shaders/curl_flow_agents.glsl`, `shaders/boids_agents.glsl`, `shaders/physarum_agents.glsl`, `shaders/color_grade.fs`, `shaders/gradient_flow.fs`, `shaders/feedback.fs`, `shaders/false_color.fs`, `shaders/cross_hatching.fs`, `shaders/toon.fs`, `shaders/heightfield_relief.fs`, `shaders/effect_blend.fs`, `shaders/fxaa.fs`, `shaders/dot_matrix.fs`, `shaders/synthwave.fs`, `shaders/crt.fs`, `shaders/ink_wash.fs`, `shaders/kuwahara.fs`, `shaders/ascii_art.fs`, `shaders/disco_ball.fs`, `shaders/texture_warp.fs`, `shaders/curl_advection.glsl`
- Impact: Inconsistent behavior (BT.601 `vec3(0.299, 0.587, 0.114)` vs BT.709 `vec3(0.2126, 0.7152, 0.0722)` luma weights), maintenance burden when fixing shared functions
- Fix approach: Shader include preprocessor per `docs/plans/shader-includes.md`

**PostEffect struct bloat (partially mitigated):**
- Issue: Effect modules own their shader handles and uniform locations. PostEffect struct holds 83 named effect struct fields as flat members plus 30+ feedback-related uniform location ints.
- Files: `src/render/post_effect.h` (290 lines), `src/render/post_effect.cpp` (374 lines)
- Impact: Each new effect adds one struct field to PostEffect plus init/uninit/register calls in post_effect.cpp.
- Fix approach: Store effects in an array indexed by type

**Static UI section state:**
- Issue: 101 `static bool section*` variables scattered across 16 UI files store ImGui section open/closed states
- Files: `src/ui/imgui_effects_warp.cpp` (14 vars), `src/ui/imgui_effects_gen_texture.cpp` (9 vars), `src/ui/drawable_type_controls.cpp` (9 vars), `src/ui/imgui_effects.cpp` (8 vars), `src/ui/imgui_effects_symmetry.cpp` (7 vars), `src/ui/imgui_effects_retro.cpp` (7 vars), `src/ui/imgui_effects_motion.cpp` (7 vars), `src/ui/imgui_effects_artistic.cpp` (6 vars), `src/ui/imgui_effects_graphic.cpp` (6 vars), `src/ui/imgui_effects_cellular.cpp` (5 vars), `src/ui/imgui_effects_optical.cpp` (5 vars), `src/ui/imgui_effects_gen_filament.cpp` (5 vars), `src/ui/imgui_effects_gen_geometric.cpp` (5 vars), `src/ui/imgui_effects_color.cpp` (4 vars), `src/ui/imgui_effects_gen_atmosphere.cpp` (3 vars), `src/ui/imgui_lfo.cpp` (1 var)
- Impact: UI state resets on hot reload; cannot persist user preferences
- Fix approach: Consolidate into a UIState struct stored alongside app config

**Preset serialization split but still growing:**
- Issue: Preset serialization was split from a single 1132-line `preset.cpp` into `preset.cpp` (234 lines) + `effect_serialization.cpp` (546 lines, 96 NLOHMANN macros). The serialization file keeps growing with each new effect.
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
- Files: `src/render/shader_setup.cpp` (451 lines), `src/config/effect_config.h` (90-entry enum), `src/config/effect_descriptor.h` (descriptor table)
- Why fragile: Adding an effect requires a new enum value, a descriptor table row, and a shader_setup.cpp dispatch case. The descriptor table consolidates name, category, enabled-check, and pipeline flags into one row, but the shader dispatch remains a separate switch.
- Safe modification: Follow `/add-effect` skill checklist; grep for an existing effect in the same category as template

**Preset Serialization:**
- Files: `src/config/effect_serialization.cpp` (546 lines), `src/config/preset.cpp` (234 lines)
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

**TransformEffectType enum ordering:**
- Files: `src/config/effect_config.h`, `src/config/effect_serialization.cpp`
- Why fragile: Adding effects mid-enum breaks saved presets unless string serialization handles it
- Safe modification: String serialization added in commit 74bd7d2; add new effects at end of enum

**Modulation base value tracking:**
- Files: `src/automation/modulation_engine.cpp`
- Why fragile: LFO modulation applies offsets to stored base values; forgetting `ModEngineWriteBaseValues()` before preset save corrupts values
- Safe modification: Call `ModEngineWriteBaseValues()` before any operation that reads param values for persistence

## Complexity Hotspots

Functions with high cyclomatic complexity (measured by lizard):

| Function | Location | CCN | NLOC | Why |
|----------|----------|-----|------|-----|
| ImGuiDrawEffectsPanel | `src/ui/imgui_effects.cpp:43` | 60 | 648 | Orchestrates all effect category panels and simulation UI |
| ImGuiDrawDrawablesPanel | `src/ui/imgui_drawables.cpp:22` | 42 | 199 | Drawable management with add/remove/reorder logic |
| from_json | `src/config/effect_serialization.cpp:144` | 25 | 76 | Deserializes 90+ config structs with fallback handling |
| DrawCellularVoronoi | `src/ui/imgui_effects_cellular.cpp:16` | 25 | 116 | Voronoi effect UI with many mode-dependent controls |
| DrawCellularPhyllotaxis | `src/ui/imgui_effects_cellular.cpp:173` | 25 | 125 | Phyllotaxis effect UI with many mode-dependent controls |
| ColorConfigEquals | `src/render/color_config.cpp:38` | 23 | 34 | Field-by-field equality comparison for 23 color config fields |
| DrawRetroGlitch | `src/ui/imgui_effects_retro.cpp:47` | 23 | 146 | Glitch effect UI with many sub-effect toggles |
| AudioFeaturesProcess | `src/analysis/audio_features.cpp:19` | 19 | 102 | Multi-stage audio feature extraction pipeline |
| ModSourceGetName | `src/automation/mod_sources.cpp:46` | 19 | 42 | Switch over modulation source types |
| ModSourceGetColor | `src/automation/mod_sources.cpp:89` | 19 | 41 | Switch over modulation source types |

## Large Files

| File | Lines | Concern |
|------|-------|---------|
| `src/ui/imgui_effects_gen_texture.cpp` | 806 | 9 texture generator UI panels |
| `src/ui/imgui_effects.cpp` | 802 | Simulation panels + effect ordering UI |
| `src/ui/imgui_analysis.cpp` | 644 | Audio visualization UI |
| `src/config/effect_serialization.cpp` | 546 | 96 NLOHMANN macros for config structs |
| `src/config/effect_config.h` | 546 | 90-entry enum, 83 config fields |
| `src/ui/imgui_effects_warp.cpp` | 536 | 14 warp effect UI panels |
| `src/simulation/particle_life.cpp` | 497 | GPU compute simulation |
| `src/ui/imgui_effects_retro.cpp` | 475 | 7 retro effect UI panels |
| `src/ui/imgui_widgets.cpp` | 464 | Custom ImGui widgets |
| `src/ui/modulatable_slider.cpp` | 462 | LFO-modulatable slider widget |
| `src/render/shader_setup.cpp` | 451 | Switch-based transform effect dispatcher |
| `src/simulation/attractor_flow.cpp` | 447 | GPU compute simulation |
| `src/simulation/curl_flow.cpp` | 444 | GPU compute simulation |
| `src/simulation/boids.cpp` | 428 | GPU compute simulation |
| `src/ui/imgui_effects_gen_geometric.cpp` | 426 | 5 geometric generator UI panels |
| `src/ui/imgui_effects_gen_filament.cpp` | 423 | 5 filament generator UI panels |
| `src/simulation/physarum.cpp` | 421 | GPU compute simulation |

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

**Preset Versioning:**
- Problem: No schema version in preset files
- Blocks: Graceful migration when config structs change

## TODO/FIXME Inventory

None detected. All lint suppressions have justification comments:

| Location | Type | Note |
|----------|------|------|
| `src/render/post_effect.cpp:60` | NOLINTNEXTLINE | readability-function-size - caches all shader uniform locations |
| `src/ui/imgui_effects.cpp:41` | NOLINTNEXTLINE | readability-function-size - immediate-mode UI requires sequential widget calls |
| `src/ui/imgui_panels.cpp:5` | NOLINTNEXTLINE | readability-function-size - theme setup requires setting all ImGui style colors |
| `src/ui/imgui_widgets.cpp:280` | NOLINTNEXTLINE | readability-function-size - UI widget with complex rendering and input handling |
| `src/ui/imgui_widgets.cpp:237` | NOLINTNEXTLINE | readability-isolate-declaration - output parameters for ImGui API |
| `src/ui/gradient_editor.cpp:138` | NOLINTNEXTLINE | readability-function-size - UI function with multiple input handling paths |
| `src/ui/modulatable_slider.cpp:166` | NOLINTNEXTLINE | readability-function-size - UI widget with detailed visual rendering |
| `src/ui/imgui_analysis.cpp:228,269,308,382` | NOLINTNEXTLINE | cert-err33-c - snprintf return value unused; buffer is fixed-size |
| `src/ui/imgui_analysis.cpp:395` | NOLINTNEXTLINE | readability-function-size - immediate-mode UI requires sequential widget calls |
| `src/ui/imgui_drawables.cpp:20` | NOLINTNEXTLINE | readability-function-size - immediate-mode UI requires sequential widget calls |
| `src/effects/oil_paint.cpp:37` | NOLINTBEGIN | concurrency-mt-unsafe - single-threaded init |
| `src/automation/lfo.cpp:45,47,64` | NOLINTNEXTLINE | concurrency-mt-unsafe - single-threaded visualizer, simple randomness sufficient |
| `src/analysis/analysis_pipeline.cpp:87` | NOLINTNEXTLINE | bugprone-integer-division - both operands explicitly cast to float |

---

*Run `/sync-docs` to regenerate.*
