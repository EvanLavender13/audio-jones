# Codebase Concerns

> Last sync: 2026-02-01 | Commit: 996fbfd

## Tech Debt

**Effect Registration Boilerplate:**
- Issue: Adding a new transform effect requires coordinated changes across 11 files
- Files: `src/config/*_config.h`, `src/config/effect_config.h`, `shaders/*.fs`, `src/render/post_effect.h`, `src/render/post_effect.cpp`, `src/render/shader_setup*.cpp`, `src/render/render_pipeline.cpp`, `src/ui/imgui_effects*.cpp`, `src/automation/param_registry.cpp`, `src/config/preset.cpp`
- Impact: High friction for adding effects, risk of missing registration steps
- Fix approach: Implement Effect Descriptor System per `docs/plans/effect-descriptor-system.md` (reduces to 3-4 files)

**Duplicated GLSL Code:**
- Issue: ~455 lines of shared functions (PI, hsv2rgb, noise, transforms) copy-pasted across 40+ shaders
- Files: `shaders/*.fs`, `shaders/*.glsl` (10+ files contain duplicated `#define PI`, `hsv2rgb`, `luminance`)
- Impact: Inconsistent behavior (BT.601 vs BT.709 luma weights), maintenance burden
- Fix approach: Implement shader include preprocessor per `docs/plans/shader-includes.md`

**C-style Enums:**
- Issue: 10 enums use `typedef enum` instead of C++11 `enum class`
- Files: `src/audio/audio_config.h`, `src/automation/mod_sources.h`, `src/config/drawable_config.h`, `src/config/effect_config.h`, `src/config/lfo_config.h`, `src/render/blend_mode.h`, `src/render/color_config.h`, `src/render/profiler.h`, `src/simulation/attractor_flow.h`
- Impact: No type safety, pollutes global namespace
- Fix approach: Migrate to scoped enums per `docs/plans/enum-modernization.md`

**PostEffect struct bloat:**
- Issue: PostEffect struct holds 60+ shader handles and 400+ uniform location ints as flat fields
- Files: `src/render/post_effect.h` (639 lines), `src/render/post_effect.cpp` (1298 lines)
- Impact: Each new effect adds 5-15 fields; struct exceeds cache line efficiency
- Fix approach: Effect Descriptor System uses arrays indexed by effect type instead of individual fields

**Static UI section state:**
- Issue: ImGui section open/closed states stored as file-static bools scattered across UI files
- Files: `src/ui/imgui_effects.cpp:18-28`, `src/ui/imgui_effects_motion.cpp:11-14`
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

**Param registry linear search:**
- Problem: `ParamRegistryGet()` and `ParamRegistryGetDynamic()` scan 400+ table entries per lookup
- Files: `src/automation/param_registry.cpp:927-968`
- Cause: String comparison in O(n) loop for each modulation target
- Improvement path: Build hash map at init time; amortize O(1) lookups

**Half-resolution effect list linear scan:**
- Problem: `IsHalfResEffect()` performs linear scan of `HALF_RES_EFFECTS[]` array per transform pass
- Files: `src/render/render_pipeline.cpp:21-33`
- Cause: O(n) lookup on every enabled transform effect
- Improvement path: Use bitfield or lookup table indexed by `TransformEffectType`

## Fragile Areas

**PostEffect Struct:**
- Files: `src/render/post_effect.h` (639 lines), `src/render/post_effect.cpp` (1298 lines)
- Why fragile: Monolithic struct holds 60+ shader handles, 400+ uniform locations, 7 simulation pointers. Adding any effect requires modifications in multiple locations within these files.
- Safe modification: Follow existing patterns exactly. Add shader field, load call, uniform caching, and unload call in parallel sections.

**Transform effect dispatch table:**
- Files: `src/render/shader_setup.cpp:20-150`, `src/config/effect_config.h:64-140`
- Why fragile: Adding a new transform effect requires changes in 5+ files: enum in `effect_config.h`, switch case in `shader_setup.cpp`, UI category mapping in `imgui_effects.cpp`, and param registration
- Safe modification: Follow `/add-effect` skill checklist; grep for existing effect of same category as template

**Shader Uniform Binding:**
- Files: `src/render/shader_setup.cpp`, `src/render/post_effect.cpp:168-600`
- Why fragile: `GetShaderUniformLocations()` caches 400+ locations with no compile-time validation. Typos in uniform names fail silently at runtime.
- Safe modification: Test shader immediately after adding uniforms. Check for -1 return from GetShaderLocation.

**Preset Serialization:**
- Files: `src/config/preset.cpp` (885 lines)
- Why fragile: Every config struct requires a NLOHMANN_DEFINE macro and manual field listing. Missing fields silently load as defaults.
- Safe modification: Always test round-trip (save then load) when adding config fields.

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

**src/render/post_effect.cpp:**
- File: `src/render/post_effect.cpp`
- Lines: 1298
- Concern: Combines shader loading (66-164), uniform caching (168-600), resolution binding, init/uninit, and resize. Multiple NOLINTNEXTLINE suppressions for function size.
- Refactor approach: Effect Descriptor System extracts shader/uniform handling into data-driven registry.

**src/automation/param_registry.cpp:**
- File: `src/automation/param_registry.cpp`
- Lines: 968
- Concern: 850+ lines of parameter table definitions using offsetof pattern; adding entries requires careful offset calculation
- Refactor approach: Effect Descriptor System registers params from descriptor metadata.

**src/ui/imgui_effects.cpp:**
- File: `src/ui/imgui_effects.cpp`
- Lines: 890
- Concern: Sequential ImGui widget calls for simulation effects and transform category dispatch. NOLINTNEXTLINE for function size. Split into category files partially complete but main file still large.
- Refactor approach: Extract simulation panels to `imgui_effects_simulations.cpp`.

**src/config/preset.cpp:**
- File: `src/config/preset.cpp`
- Lines: 885
- Concern: NLOHMANN macros for 30+ config structs, manual to_json/from_json for complex types. NOLINTNEXTLINE for function size on EffectConfig serialization.
- Refactor approach: Effect Descriptor System auto-generates serialization from field metadata.

**src/render/post_effect.h:**
- File: `src/render/post_effect.h`
- Lines: 639
- Concern: Single struct with 400+ members including shader handles, uniform locations, simulation pointers, and animation state; changes require full recompile
- Refactor approach: Split into sub-structs by category; use forward declarations and pointer indirection

**src/render/shader_setup.cpp:**
- File: `src/render/shader_setup.cpp`
- Lines: 545
- Concern: Contains large switch statement mapping TransformEffectType enum to function pointers; setup functions split into category modules but dispatcher remains.
- Refactor approach: Effect Descriptor System replaces switch with array lookup from descriptors.

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
| `src/render/post_effect.cpp:166` | NOLINTNEXTLINE | readability-function-size - caches all shader uniform locations |
| `src/config/preset.cpp:410` | NOLINTNEXTLINE | readability-function-size - serializes all effect fields |
| `src/ui/imgui_effects.cpp:140` | NOLINTNEXTLINE | readability-function-size - immediate-mode UI requires sequential widget calls |
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
