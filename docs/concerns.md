# Codebase Concerns

> Last sync: 2026-02-08 | Commit: 88d66c3

## Tech Debt

**Duplicated GLSL Code:**
- Issue: 18 shaders define their own `hsv2rgb` or `luminance` functions. Additional shared math (PI, noise, transforms) appears across multiple shaders.
- Files with `hsv2rgb`: `shaders/neon_glow.fs`, `shaders/color_grade.fs`, `shaders/physarum_agents.glsl`, `shaders/boids_agents.glsl`, `shaders/particle_life_agents.glsl`, `shaders/attractor_agents.glsl`
- Files with `luminance`: `shaders/watercolor.fs`, `shaders/glitch.fs`, `shaders/halftone.fs`, `shaders/color_grade.fs`, `shaders/gradient_flow.fs`, `shaders/effect_blend.fs`, `shaders/feedback.fs`, `shaders/false_color.fs`, `shaders/cross_hatching.fs`, `shaders/toon.fs`, `shaders/heightfield_relief.fs`, `shaders/curl_flow_agents.glsl`, `shaders/boids_agents.glsl`, `shaders/physarum_agents.glsl`, `shaders/fxaa.fs`
- Impact: Inconsistent behavior (BT.601 vs BT.709 luma weights), maintenance burden when fixing shared functions
- Fix approach: Shader include preprocessor per `docs/plans/shader-includes.md`

**PostEffect struct bloat (partially mitigated):**
- Issue: Effect modules now own their shader handles and uniform locations. PostEffect struct dropped from 639 to 285 lines. However, it still holds 70 named effect struct fields as flat members, 30+ feedback-related uniform location ints, and 14 generator blend `*BlendActive` bools.
- Files: `src/render/post_effect.h` (285 lines), `src/render/post_effect.cpp` (863 lines)
- Impact: Each new effect adds one struct field to PostEffect plus init/uninit/register calls in post_effect.cpp. Generator blend effects add a `*BlendActive` bool.
- Fix approach: Effect Descriptor System would store effects in an array indexed by type

**Static UI section state:**
- Issue: 87 `static bool section*` variables scattered across 13 UI files store ImGui section open/closed states
- Files: `src/ui/imgui_effects_generators.cpp` (14 vars), `src/ui/imgui_effects_warp.cpp` (13 vars), `src/ui/imgui_effects.cpp` (8 vars), `src/ui/drawable_type_controls.cpp` (8 vars), `src/ui/imgui_effects_symmetry.cpp` (7 vars), and 8 other files
- Impact: UI state resets on hot reload; cannot persist user preferences
- Fix approach: Consolidate into a UIState struct stored alongside app config

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
- Files: `src/render/post_effect.cpp:183-559` (init), `src/render/post_effect.cpp:669-770` (uninit)
- Why fragile: 70+ sequential effect init calls with individual failure checks. Uninit order does not mirror init order. Adding an effect requires matching additions in init, uninit, resize, and register functions.
- Safe modification: Add new effect init immediately before the texture/fft init block (line ~540). Add matching uninit call. Follow existing pattern exactly.

**Transform effect dispatch table:**
- Files: `src/render/shader_setup.cpp:23-670`, `src/config/effect_config.h:84-162` (enum), `src/config/effect_config.h:567-727` (IsTransformEnabled)
- Why fragile: Three parallel switch/array structures must stay synchronized: enum values, name strings, and IsTransformEnabled cases. The shader_setup.cpp dispatch adds a fourth.
- Safe modification: Follow `/add-effect` skill checklist; grep for an existing effect in the same category as template

**Generator Blend Pipeline:**
- Files: `src/render/render_pipeline.cpp:36-48` (IsGeneratorBlendEffect), `src/render/render_pipeline.cpp:342-368` (BlendActive flags), `src/render/post_effect.h:245-258` (BlendActive fields)
- Why fragile: Three locations must stay synchronized for each generator blend effect. Missing any one causes silent render failure (effect compiles but never renders).
- Safe modification: Follow `/add-effect` skill Phase 5b checklist; grep for an existing `_BLEND` effect as template

**Preset Serialization:**
- Files: `src/config/preset.cpp` (1101 lines)
- Why fragile: Every config struct requires a NLOHMANN_DEFINE macro and manual field listing. Missing fields silently load as defaults. File keeps growing as effects accumulate.
- Safe modification: Always test round-trip (save then load) when adding config fields

**Simulation init/uninit with goto cleanup:**
- Files: `src/simulation/physarum.cpp`, `src/simulation/boids.cpp`, `src/simulation/curl_flow.cpp`, `src/simulation/particle_life.cpp`, `src/simulation/cymatics.cpp`, `src/simulation/attractor_flow.cpp`, `src/simulation/curl_advection.cpp`
- Why fragile: Each simulation uses goto-based cleanup; missing a goto path on new allocation leaks resources
- Safe modification: Add new allocations immediately before the corresponding goto check; uninit in reverse order

**Drawable ID management:**
- Files: `src/automation/drawable_params.cpp`, `src/config/preset.cpp`, `src/ui/imgui_panels.h`
- Why fragile: Drawable IDs must stay synchronized between param registry, preset serialization, and UI state counter
- Safe modification: Always call `ImGuiDrawDrawablesSyncIdCounter` and `DrawableParamsSyncAll` after changing drawable array

**TransformEffectType enum ordering:**
- Files: `src/config/effect_config.h`, `src/config/preset.cpp`
- Why fragile: Adding effects mid-enum breaks saved presets unless string serialization handles it
- Safe modification: String serialization added in commit 74bd7d2; add new effects at end of enum

**Modulation base value tracking:**
- Files: `src/automation/modulation_engine.cpp`
- Why fragile: LFO modulation applies offsets to stored base values; forgetting `ModEngineWriteBaseValues()` before preset save corrupts values
- Safe modification: Call `ModEngineWriteBaseValues()` before any operation that reads param values for persistence

## Complexity Hotspots

Functions with high cyclomatic complexity (switch cases over 77 enum values):

| Function | Location | Concern |
|----------|----------|---------|
| IsTransformEnabled | `src/config/effect_config.h:567` | 77-case switch, one case per effect |
| GetTransformEffect | `src/render/shader_setup.cpp:23` | 77-case switch mapping type to shader/setup |
| GetTransformCategory | `src/ui/imgui_effects.cpp:38` | 77-case switch mapping type to UI category |
| to_json / from_json | `src/config/preset.cpp:547` | Serializes 70+ config structs |
| ImGuiDrawEffectsPanel | `src/ui/imgui_effects.cpp:157` | Orchestrates all effect category panels |
| PostEffectInit | `src/render/post_effect.cpp:183` | 70+ sequential init calls with error paths |
| PostEffectUninit | `src/render/post_effect.cpp:669` | 70+ uninit calls |

## Large Files

| File | Lines | Concern |
|------|-------|---------|
| `src/config/preset.cpp` | 1101 | NLOHMANN macros for 70+ config structs |
| `src/ui/imgui_effects_generators.cpp` | 1064 | 14 generator effect UI panels |
| `src/ui/imgui_effects.cpp` | 895 | Simulation panels + transform category dispatch |
| `src/render/post_effect.cpp` | 863 | Effect init/uninit, feedback uniform caching (down from 1452) |
| `src/config/effect_config.h` | 729 | 77-entry enum, 70+ config fields, IsTransformEnabled switch |
| `src/render/shader_setup.cpp` | 670 | Switch-based transform effect dispatcher |
| `src/ui/imgui_analysis.cpp` | 644 | Audio visualization UI |
| `src/ui/imgui_effects_warp.cpp` | 496 | 13 warp effect UI panels |
| `src/simulation/particle_life.cpp` | 489 | GPU compute simulation |
| `src/ui/modulatable_slider.cpp` | 462 | LFO-modulatable slider widget |
| `src/ui/imgui_effects_retro.cpp` | 434 | 6 retro effect UI panels |
| `src/render/render_pipeline.cpp` | 429 | Frame rendering orchestration (down from 501) |

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
| `src/render/post_effect.cpp:101` | NOLINTNEXTLINE | readability-function-size - caches all shader uniform locations |
| `src/config/preset.cpp:547` | NOLINTNEXTLINE | readability-function-size - serializes all effect fields |
| `src/ui/imgui_effects.cpp:157` | NOLINTNEXTLINE | readability-function-size - immediate-mode UI requires sequential widget calls |
| `src/ui/imgui_panels.cpp:5` | NOLINTNEXTLINE | readability-function-size - theme setup requires setting all ImGui style colors |
| `src/ui/imgui_widgets.cpp:271` | NOLINTNEXTLINE | readability-function-size - UI widget with complex rendering and input handling |
| `src/ui/imgui_widgets.cpp:228` | NOLINTNEXTLINE | readability-isolate-declaration - output parameters for ImGui API |
| `src/ui/gradient_editor.cpp:138` | NOLINTNEXTLINE | readability-function-size - UI function with multiple input handling paths |
| `src/ui/modulatable_slider.cpp:166` | NOLINTNEXTLINE | readability-function-size - UI widget with detailed visual rendering |
| `src/ui/imgui_analysis.cpp:228,269,308,382` | NOLINTNEXTLINE | cert-err33-c - snprintf return value unused; buffer is fixed-size |
| `src/ui/imgui_analysis.cpp:395` | NOLINTNEXTLINE | readability-function-size - immediate-mode UI requires sequential widget calls |
| `src/ui/imgui_drawables.cpp:20` | NOLINTNEXTLINE | readability-function-size - immediate-mode UI requires sequential widget calls |
| `src/render/spectrum_bars.cpp:109,167` | NOLINT | misc-unused-parameters - globalTick reserved for future sync |
| `src/effects/oil_paint.cpp:35-41` | NOLINTBEGIN/END | concurrency-mt-unsafe - single-threaded init |
| `src/automation/lfo.cpp:45,47,64` | NOLINTNEXTLINE | concurrency-mt-unsafe - single-threaded visualizer, simple randomness sufficient |
| `src/analysis/analysis_pipeline.cpp:87` | NOLINTNEXTLINE | bugprone-integer-division - both operands explicitly cast to float |

---

*Run `/sync-docs` to regenerate.*
