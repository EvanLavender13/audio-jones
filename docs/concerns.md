# Codebase Concerns

> Last sync: 2026-04-04 | Commit: fcac2f99

## Tech Debt

**Duplicated GLSL Code:**
- Issue: 5 shaders define their own `hsv2rgb` function, 30 shaders duplicate inline luminance/luma calculations with inconsistent weights. Additional shared math (PI, noise, transforms) duplicated across shaders.
- Files with `hsv2rgb`: `shaders/color_grade.fs`, `shaders/physarum_agents.glsl`, `shaders/boids_agents.glsl`, `shaders/particle_life_agents.glsl`, `shaders/hue_remap.fs`
- Files with luminance: `shaders/hue_remap.fs`, `shaders/glitch.fs`, `shaders/gradient_flow.fs`, `shaders/feedback.fs`, `shaders/false_color.fs`, `shaders/cross_hatching.fs`, `shaders/effect_blend.fs`, `shaders/fxaa.fs`, `shaders/dot_matrix.fs`, `shaders/synthwave.fs`, `shaders/woodblock.fs`, `shaders/ink_wash.fs`, `shaders/texture_warp.fs`, `shaders/ascii_art.fs`, `shaders/curl_flow_agents.glsl`, `shaders/boids_agents.glsl`, `shaders/physarum_agents.glsl`, `shaders/curl_advection_state.fs`, `shaders/byzantine_display.fs`, `shaders/crt.fs`, `shaders/disco_ball.fs`, `shaders/kuwahara.fs`, `shaders/curl_gradient.glsl`, `shaders/subdivide.fs`, `shaders/watercolor.fs`, `shaders/surface_depth.fs`, `shaders/stripe_shift.fs`, `shaders/toon.fs`, `shaders/color_grade.fs`, `shaders/polygon_subdivide.fs`
- Impact: Inconsistent behavior (BT.601 `vec3(0.299, 0.587, 0.114)` vs BT.709 `vec3(0.2126, 0.7152, 0.0722)` luma weights), maintenance burden when fixing shared functions
- Fix approach: Shader include preprocessor per `docs/plans/shader-includes.md`

**PostEffect struct bloat (partially mitigated):**
- Issue: Effect modules own their shader handles and uniform locations. PostEffect struct holds 135 named effect struct fields as flat members plus 30+ feedback-related uniform location ints.
- Files: `src/render/post_effect.h` (380 lines), `src/render/post_effect.cpp` (381 lines)
- Impact: Each new effect adds one struct field to PostEffect plus init/uninit/register calls in post_effect.cpp.
- Fix approach: Store effects in an array indexed by type

**Static UI section state (nearly resolved):**
- Issue: UI colocation replaced 90+ `static bool section*` variables across 16 category UI files with a single `g_effectSectionOpen[TRANSFORM_EFFECT_COUNT]` array in `src/ui/imgui_effects_dispatch.cpp`. 1 `static bool section*` variable remains in 1 file for a non-effect UI section (flow field).
- Files: `src/ui/imgui_effects.cpp` (1 var)
- Impact: Remaining state resets on hot reload; cannot persist user preferences
- Fix approach: Move remaining into a UIState struct stored alongside app config

**Preset serialization split but still growing:**
- Issue: Preset serialization was split from a single 1132-line `preset.cpp` into `preset.cpp` (277 lines) + `effect_serialization.cpp` (778 lines, 140 NLOHMANN macros + 3 manual to_json/from_json pairs). The serialization file keeps growing with each new effect.
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

**Preset Serialization:**
- Files: `src/config/effect_serialization.cpp` (778 lines), `src/config/preset.cpp` (277 lines)
- Why fragile: Every config struct requires a NLOHMANN_DEFINE macro and manual field listing. Missing fields silently load as defaults.
- Safe modification: Always test round-trip (save then load) when adding config fields

**Simulation init/uninit with goto cleanup:**
- Files: `src/simulation/physarum.cpp`, `src/simulation/boids.cpp`, `src/simulation/curl_flow.cpp`, `src/simulation/attractor_flow.cpp`, `src/simulation/particle_life.cpp`, `src/simulation/trail_map.cpp`, `src/simulation/spatial_hash.cpp`, `src/simulation/maze_worms.cpp`
- Why fragile: Each simulation uses goto-based cleanup; missing a goto path on new allocation leaks resources
- Safe modification: Add new allocations immediately before the corresponding goto check; uninit in reverse order

**Drawable ID management:**
- Files: `src/automation/drawable_params.cpp`, `src/config/preset.cpp`, `src/ui/imgui_panels.h`
- Why fragile: Drawable IDs must stay synchronized between param registry, preset serialization, and UI state counter
- Safe modification: Always call `ImGuiDrawDrawablesSyncIdCounter` and `DrawableParamsSyncAll` after changing drawable array

**Playlist file-local state:**
- Files: `src/ui/imgui_playlist.cpp` (411 lines), `src/config/playlist.h`, `src/config/playlist.cpp`
- Why fragile: Playlist runtime state is file-local static variables (mirrors `imgui_presets.cpp` pattern). Playlist entries reference preset paths by string; renaming or deleting a preset file breaks playlist entries silently.
- Safe modification: Validate preset paths on playlist load; handle missing files gracefully in playback advance

## Large Files

| File | Lines | Concern |
|------|-------|---------|
| `src/config/effect_config.h` | 792 | 135-entry enum, 135 effect config fields |
| `src/config/effect_serialization.cpp` | 778 | 140 NLOHMANN macros + 3 manual serializers |
| `src/ui/imgui_analysis.cpp` | 614 | Audio visualization UI |
| `src/simulation/particle_life.cpp` | 579 | GPU compute simulation |
| `src/simulation/attractor_flow.cpp` | 530 | GPU compute simulation with colocated UI |
| `src/simulation/physarum.cpp` | 527 | GPU compute simulation with colocated UI |
| `src/ui/imgui_widgets.cpp` | 487 | Custom ImGui widgets |
| `src/simulation/curl_flow.cpp` | 482 | GPU compute simulation |
| `src/simulation/boids.cpp` | 482 | GPU compute simulation |
| `src/render/waveform.cpp` | 479 | Waveform rendering |
| `src/ui/modulatable_slider.cpp` | 472 | LFO-modulatable slider widget |
| `src/effects/glitch.cpp` | 425 | Multi-sub-effect module with colocated UI |
| `src/ui/imgui_playlist.cpp` | 411 | Playlist management UI |
| `src/effects/ripple_tank.cpp` | 397 | Cymatics simulation with colocated UI |
| `src/main.cpp` | 384 | Application entry point and main loop |
| `src/effects/polymorph.cpp` | 382 | Generator with colocated UI |
| `src/render/post_effect.cpp` | 381 | Effect init/uninit/resize orchestration |
| `src/effects/curl_advection.cpp` | 381 | GPU compute simulation with colocated UI |
| `src/render/post_effect.h` | 380 | PostEffect struct with 135 effect fields |
| `src/effects/attractor_lines.cpp` | 373 | Generator with colocated UI |
| `src/simulation/maze_worms.cpp` | 367 | GPU compute simulation with colocated UI |
| `src/ui/gradient_editor.cpp` | 350 | Gradient editor widget |
| `src/render/render_pipeline.cpp` | 331 | Frame rendering orchestration |
| `src/ui/imgui_bus.cpp` | 330 | Mod bus management UI |
| `src/ui/imgui_presets.cpp` | 328 | Preset management UI |
| `src/ui/imgui_effects.cpp` | 322 | Effects panel orchestration |
| `src/effects/data_traffic.cpp` | 321 | Generator with colocated UI |
| `src/simulation/spatial_hash.cpp` | 318 | Spatial hash grid implementation |
| `src/ui/imgui_drawables.cpp` | 317 | Drawable management UI |
| `src/render/drawable.cpp` | 312 | Drawable rendering |
| `src/effects/muons.cpp` | 299 | Generator with colocated UI |
| `src/effects/constellation.cpp` | 298 | Generator with colocated UI |

## Complexity Hotspots

Functions with cyclomatic complexity > 15 (measured via lizard):

| Function | Location | CCN | NLOC | Why |
|----------|----------|-----|------|-----|
| ImGuiDrawDrawablesPanel | `src/ui/imgui_drawables.cpp:22` | 37 | 228 | Drawable management with add/remove/reorder logic |
| ModSourceGetName | `src/automation/mod_sources.cpp:55` | 27 | 58 | Switch over modulation source types |
| ModSourceGetColor | `src/automation/mod_sources.cpp:114` | 27 | 50 | Switch over modulation source types |
| from_json | `src/config/effect_serialization.cpp:188` | 25 | 76 | Deserializes 135+ config structs with fallback handling |
| DrawPresetList | `src/ui/imgui_presets.cpp:134` | 23 | 93 | Preset list UI with rename/delete/drag-reorder |
| ColorConfigEquals | `src/render/color_config.cpp:38` | 23 | 34 | Field-by-field equality comparison for 23 color config fields |
| ImGuiDrawEffectsPanel | `src/ui/imgui_effects.cpp:22` | 21 | 232 | Orchestrates effect category dispatch and simulation UI |
| DrawManageBar | `src/ui/imgui_playlist.cpp:287` | 20 | 88 | Playlist management bar with add/remove/reorder |
| AudioFeaturesProcess | `src/analysis/audio_features.cpp:17` | 19 | 102 | Multi-stage audio feature extraction pipeline |
| DrawSetlist | `src/ui/imgui_playlist.cpp:151` | 16 | 102 | Playlist setlist UI with drag-reorder |

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
| `src/main.cpp:164,168,172,175,179,183,187` | NOLINTNEXTLINE | cert-err33-c - snprintf into fixed-size paramId buffer |
| `src/render/post_effect.cpp:59` | NOLINTNEXTLINE | readability-function-size - caches all shader uniform locations |
| `src/ui/imgui_effects.cpp:20` | NOLINTNEXTLINE | readability-function-size - immediate-mode UI requires sequential widget calls |
| `src/ui/imgui_panels.cpp:5` | NOLINTNEXTLINE | readability-function-size - theme setup requires setting all ImGui style colors |
| `src/ui/imgui_widgets.cpp:178,303` | NOLINTNEXTLINE | readability-function-size - widget with header drawing and complex rendering |
| `src/ui/imgui_drawables.cpp:20` | NOLINTNEXTLINE | readability-function-size - immediate-mode UI requires sequential widget calls |
| `src/ui/drawable_type_controls.cpp:131` | NOLINTNEXTLINE | readability-function-size - immediate-mode UI requires sequential widget calls |
| `src/ui/gradient_editor.cpp:138` | NOLINTNEXTLINE | readability-function-size - UI function with multiple input handling paths |
| `src/ui/modulatable_slider.cpp:166` | NOLINTNEXTLINE | readability-function-size - UI widget with detailed visual rendering |
| `src/ui/imgui_analysis.cpp:228,269,308,378` | NOLINTNEXTLINE | cert-err33-c - snprintf into fixed-size display buffer |
| `src/ui/imgui_analysis.cpp:389` | NOLINTNEXTLINE | readability-function-size - immediate-mode UI requires sequential widget calls |
| `src/automation/lfo.cpp:45,47,64` | NOLINTNEXTLINE | concurrency-mt-unsafe - single-threaded visualizer, simple randomness sufficient |
| `src/effects/curl_advection.cpp:50,52` | NOLINTNEXTLINE | concurrency-mt-unsafe - single-threaded init |

---

*Run `/sync-docs` to regenerate.*
