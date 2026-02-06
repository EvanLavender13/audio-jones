# Codebase Concerns

> Last sync: 2026-02-06 | Commit: 957e250

## Tech Debt

**Effect Registration Boilerplate (partially mitigated):**
- Issue: The new `src/effects/` module directory (60 modules, 120 files) encapsulates shader loading, uniform caching, and param registration per effect. This eliminates scattered uniform location fields from PostEffect and centralizes init/uninit/setup logic.
- Remaining friction: Adding a new transform effect still requires coordinated changes across 8 files: `src/effects/<name>.h`, `src/effects/<name>.cpp`, `shaders/<name>.fs`, `src/render/post_effect.h` (effect struct field), `src/render/post_effect.cpp` (init/uninit/register calls), `src/render/shader_setup.cpp` (dispatch case), `src/config/effect_config.h` (enum + config field + IsTransformEnabled case), `src/ui/imgui_effects_<category>.cpp` (UI panel)
- Impact: Reduced from 11 files to 8, but still high friction. Three files require mechanical switch-case additions (`shader_setup.cpp`, `effect_config.h:IsTransformEnabled`, `effect_config.h:TRANSFORM_EFFECT_NAMES`).
- Fix approach: Effect Descriptor System per `docs/plans/effect-descriptor-system.md` would reduce to 3-4 files

**Duplicated GLSL Code:**
- Issue: 9 shaders define their own `hsv2rgb` or `luminance` functions. Additional shared math (PI, noise, transforms) appears across multiple shaders.
- Files: `shaders/watercolor.fs`, `shaders/neon_glow.fs`, `shaders/color_grade.fs`, `shaders/gradient_flow.fs`, `shaders/effect_blend.fs`, `shaders/physarum_agents.glsl`, `shaders/boids_agents.glsl`, `shaders/particle_life_agents.glsl`, `shaders/attractor_agents.glsl`
- Impact: Inconsistent behavior (BT.601 vs BT.709 luma weights), maintenance burden when fixing shared functions
- Fix approach: Shader include preprocessor per `docs/plans/shader-includes.md`

**C-style Enums:**
- Issue: 15 enums use `typedef enum` instead of C++11 `enum class`
- Files: `src/audio/audio_config.h`, `src/automation/mod_sources.h`, `src/automation/modulation_engine.h`, `src/config/drawable_config.h` (3 enums), `src/config/lfo_config.h`, `src/effects/texture_warp.h`, `src/render/blend_mode.h`, `src/render/color_config.h`, `src/render/profiler.h`, `src/simulation/physarum.h`, `src/simulation/bounds_mode.h` (2 enums), `src/simulation/attractor_flow.h`
- Impact: No type safety, pollutes global namespace
- Fix approach: Migrate to scoped enums per `docs/plans/enum-modernization.md`

**PostEffect struct bloat (partially mitigated):**
- Issue: Effect modules now own their shader handles and uniform locations. PostEffect struct dropped from 639 to 257 lines. However, it still holds 60 named effect struct fields as flat members and 30+ feedback-related uniform location ints.
- Files: `src/render/post_effect.h` (257 lines), `src/render/post_effect.cpp` (790 lines)
- Impact: Each new effect adds one struct field to PostEffect plus init/uninit/register calls in post_effect.cpp
- Fix approach: Effect Descriptor System would store effects in an array indexed by type

**PostEffectInit cascading leak on failure:**
- Issue: `PostEffectInit()` initializes 60+ effects sequentially. Early failure paths only `free(pe)` without uninitializing previously successful effects. Late failures (e.g., line 450+) leak all prior shader/texture resources.
- Files: `src/render/post_effect.cpp:182-507`
- Impact: Resource leak on partial initialization failure (rare at startup, but incorrect)
- Fix approach: Use a single goto-cleanup block or track init count and uninit in reverse

**Static UI section state:**
- Issue: 77 `static bool section*` variables scattered across 13 UI files store ImGui section open/closed states
- Files: `src/ui/imgui_effects_warp.cpp` (13 vars), `src/ui/imgui_effects.cpp` (8 vars), `src/ui/drawable_type_controls.cpp` (8 vars), `src/ui/imgui_effects_symmetry.cpp` (7 vars), and 9 other files
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

**Half-resolution effect list linear scan:**
- Problem: `IsHalfResEffect()` performs linear scan of `HALF_RES_EFFECTS[]` array per transform pass
- Files: `src/render/render_pipeline.cpp:22-34`
- Cause: O(n) lookup on every enabled transform effect (currently only 3 entries, so low practical impact)
- Improvement path: Use bitfield or lookup table indexed by `TransformEffectType`

## Fragile Areas

**PostEffect Init/Uninit Sequence:**
- Files: `src/render/post_effect.cpp:182-507` (init), `src/render/post_effect.cpp:607-698` (uninit)
- Why fragile: 60+ sequential effect init calls with individual failure checks. Uninit order does not mirror init order. Adding an effect requires matching additions in init, uninit, resize, and register functions.
- Safe modification: Add new effect init immediately before the texture/fft init block (line ~488). Add matching uninit call. Follow existing pattern exactly.

**Transform effect dispatch table:**
- Files: `src/render/shader_setup.cpp:23-632`, `src/config/effect_config.h:74-143` (enum), `src/config/effect_config.h:507-647` (IsTransformEnabled)
- Why fragile: Three parallel switch/array structures must stay synchronized: enum values, name strings, and IsTransformEnabled cases. The shader_setup.cpp dispatch adds a fourth.
- Safe modification: Follow `/add-effect` skill checklist; grep for an existing effect in the same category as template

**Preset Serialization:**
- Files: `src/config/preset.cpp` (961 lines)
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

Functions with high cyclomatic complexity (switch cases over 60+ enum values):

| Function | Location | Concern |
|----------|----------|---------|
| IsTransformEnabled | `src/config/effect_config.h:507` | 68-case switch, one case per effect |
| GetTransformEffect | `src/render/shader_setup.cpp:23` | 68-case switch mapping type to shader/setup |
| GetTransformCategory | `src/ui/imgui_effects.cpp:38` | 68-case switch mapping type to UI category |
| to_json / from_json | `src/config/preset.cpp:447` | Serializes 60+ config structs |
| ImGuiDrawEffectsPanel | `src/ui/imgui_effects.cpp:146` | Orchestrates all effect category panels |
| PostEffectInit | `src/render/post_effect.cpp:182` | 60+ sequential init calls with error paths |
| PostEffectUninit | `src/render/post_effect.cpp:607` | 60+ uninit calls |

## Large Files

| File | Lines | Concern |
|------|-------|---------|
| `src/config/preset.cpp` | 961 | NLOHMANN macros for 60+ config structs |
| `src/ui/imgui_effects.cpp` | 886 | Simulation panels + transform category dispatch |
| `src/render/post_effect.cpp` | 790 | Effect init/uninit, feedback uniform caching (down from 1452) |
| `src/config/effect_config.h` | 649 | 68-entry enum, 60+ config fields, IsTransformEnabled switch |
| `src/ui/imgui_analysis.cpp` | 648 | Audio visualization UI |
| `src/render/shader_setup.cpp` | 632 | Switch-based transform effect dispatcher |
| `src/ui/imgui_effects_warp.cpp` | 498 | 13 warp effect UI panels |
| `src/simulation/particle_life.cpp` | 489 | GPU compute simulation |
| `src/ui/imgui_effects_generators.cpp` | 475 | 6 generator effect UI panels |
| `src/ui/modulatable_slider.cpp` | 462 | LFO-modulatable slider widget |
| `src/render/render_pipeline.cpp` | 407 | Frame rendering orchestration (down from 501) |

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
| `src/render/post_effect.cpp:100` | NOLINTNEXTLINE | readability-function-size - caches all shader uniform locations |
| `src/config/preset.cpp:447` | NOLINTNEXTLINE | readability-function-size - serializes all effect fields |
| `src/ui/imgui_effects.cpp:146` | NOLINTNEXTLINE | readability-function-size - immediate-mode UI requires sequential widget calls |
| `src/ui/imgui_panels.cpp:5` | NOLINTNEXTLINE | readability-function-size - theme setup requires setting all ImGui style colors |
| `src/ui/imgui_widgets.cpp:271` | NOLINTNEXTLINE | readability-function-size - UI widget with complex rendering and input handling |
| `src/ui/imgui_widgets.cpp:228` | NOLINTNEXTLINE | readability-isolate-declaration - output parameters for ImGui API |
| `src/ui/gradient_editor.cpp:138` | NOLINTNEXTLINE | readability-function-size - UI function with multiple input handling paths |
| `src/ui/modulatable_slider.cpp:166` | NOLINTNEXTLINE | readability-function-size - UI widget with detailed visual rendering |
| `src/ui/imgui_analysis.cpp:228,270,310,385` | NOLINTNEXTLINE | cert-err33-c - snprintf return value unused; buffer is fixed-size |
| `src/ui/imgui_analysis.cpp:399` | NOLINTNEXTLINE | readability-function-size - immediate-mode UI requires sequential widget calls |
| `src/ui/imgui_drawables.cpp:20` | NOLINTNEXTLINE | readability-function-size - immediate-mode UI requires sequential widget calls |
| `src/render/waveform.cpp:171,237` | NOLINTNEXTLINE | misc-unused-parameters - globalTick reserved for future sync |
| `src/render/shape.cpp:20` | NOLINTNEXTLINE | misc-unused-parameters - globalTick reserved for future sync |
| `src/automation/lfo.cpp:45,48,66` | NOLINTNEXTLINE | concurrency-mt-unsafe - single-threaded visualizer, simple randomness sufficient |
| `src/analysis/analysis_pipeline.cpp:87` | NOLINTNEXTLINE | bugprone-integer-division - both operands explicitly cast to float |

---

*Run `/sync-docs` to regenerate.*
