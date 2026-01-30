# Codebase Concerns

> Last sync: 2026-01-29 | Commit: 176b35f

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

**post_effect.h struct bloat:**
- Issue: PostEffect struct holds 57 shader handles and 300+ uniform locations as flat fields
- Files: `src/render/post_effect.h` (545 lines), `src/render/post_effect.cpp` (1137 lines)
- Impact: Each new effect adds 5-10 fields; struct exceeds cache line efficiency
- Fix approach: Effect Descriptor System uses arrays indexed by effect type instead of individual fields

## Known Bugs

None detected.

## Performance Bottlenecks

**param_registry string lookups:**
- Problem: Modulation engine uses std::unordered_map with string keys for every param access
- Files: `src/automation/modulation_engine.cpp`, `src/automation/param_registry.cpp`
- Cause: String hashing on every ModEngineUpdate call (multiple times per frame)
- Improvement path: Use integer IDs for hot paths; keep string lookup for UI/serialization only

## Fragile Areas

**PostEffect Struct:**
- Files: `src/render/post_effect.h` (545 lines), `src/render/post_effect.cpp` (1137 lines)
- Why fragile: Monolithic struct holds 57 shader handles, 300+ uniform locations, 7 simulation pointers. Adding any effect requires modifications in multiple locations within these files.
- Safe modification: Follow existing patterns exactly. Add shader field, load call, uniform caching, and unload call in parallel sections.

**Shader Uniform Binding:**
- Files: `src/render/shader_setup.cpp`, `src/render/post_effect.cpp:158-793`
- Why fragile: `GetShaderUniformLocations()` caches 300+ locations with no compile-time validation. Typos in uniform names fail silently at runtime.
- Safe modification: Test shader immediately after adding uniforms. Check for -1 return from GetShaderLocation.

**Preset Serialization:**
- Files: `src/config/preset.cpp` (806 lines)
- Why fragile: Every config struct requires a NLOHMANN_DEFINE macro and manual field listing. Missing fields silently load as defaults.
- Safe modification: Always test round-trip (save then load) when adding config fields.

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
- Lines: 1137
- Concern: Combines shader loading (66-154), uniform caching (158-793), resolution binding (795-852), init/uninit (854-1045), and resize (1047-1085). Multiple NOLINTNEXTLINE suppressions for function size.
- Refactor approach: Effect Descriptor System extracts shader/uniform handling into data-driven registry.

**src/ui/imgui_effects.cpp:**
- File: `src/ui/imgui_effects.cpp`
- Lines: 849
- Concern: Sequential ImGui widget calls for all transform effects. NOLINTNEXTLINE for function size. Split into category files (style, warp, symmetry) partially complete but main file still large.
- Refactor approach: Continue modularization into category-specific files (imgui_effects_*.cpp).

**src/config/preset.cpp:**
- File: `src/config/preset.cpp`
- Lines: 806
- Concern: NLOHMANN macros for 30+ config structs, manual to_json/from_json for complex types. NOLINTNEXTLINE for function size on EffectConfig serialization.
- Refactor approach: Effect Descriptor System auto-generates serialization from field metadata.

**src/automation/param_registry.cpp:**
- File: `src/automation/param_registry.cpp`
- Lines: 632
- Concern: Static PARAM_TABLE with 200+ entries, parallel target pointer arrays. Adding modulatable params requires entries in two places.
- Refactor approach: Effect Descriptor System registers params from descriptor metadata.

**src/render/shader_setup.cpp:**
- File: `src/render/shader_setup.cpp`
- Lines: 521 (recently modularized from 1268)
- Concern: Contains switch statement mapping enum to function pointers; setup functions split into category modules but dispatcher remains.
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
| `src/render/post_effect.cpp:156` | NOLINTNEXTLINE | readability-function-size - caches all shader uniform locations |
| `src/config/preset.cpp:373` | NOLINTNEXTLINE | readability-function-size - serializes all effect fields |
| `src/ui/imgui_effects.cpp:125` | NOLINTNEXTLINE | readability-function-size - immediate-mode UI requires sequential widget calls |
| `src/ui/imgui_panels.cpp:5` | NOLINTNEXTLINE | readability-function-size - theme setup requires setting all ImGui style colors |
| `src/ui/imgui_widgets.cpp:271` | NOLINTNEXTLINE | readability-function-size - UI widget with complex rendering and input handling |
| `src/ui/gradient_editor.cpp:138` | NOLINTNEXTLINE | readability-function-size - UI function with multiple input handling paths |
| `src/ui/modulatable_slider.cpp:166` | NOLINTNEXTLINE | readability-function-size - UI widget with detailed visual rendering |
| `src/ui/imgui_analysis.cpp:399` | NOLINTNEXTLINE | readability-function-size - immediate-mode UI requires sequential widget calls |
| `src/ui/imgui_drawables.cpp:20` | NOLINTNEXTLINE | readability-function-size - immediate-mode UI requires sequential widget calls |
| `src/render/waveform.cpp:171,199` | NOLINTNEXTLINE | misc-unused-parameters - globalTick reserved for future sync |
| `src/render/shape.cpp:20` | NOLINTNEXTLINE | misc-unused-parameters - globalTick reserved for future sync |
| `src/automation/lfo.cpp:45,48,66` | NOLINTNEXTLINE | concurrency-mt-unsafe - single-threaded visualizer, simple randomness sufficient |
| `src/analysis/analysis_pipeline.cpp:87` | NOLINTNEXTLINE | bugprone-integer-division - both operands explicitly cast to float |
| `src/ui/imgui_analysis.cpp:228,270,310,385` | NOLINTNEXTLINE | cert-err33-c - snprintf return value unused; buffer is fixed-size |
| `src/ui/imgui_widgets.cpp:228` | NOLINTNEXTLINE | readability-isolate-declaration - output parameters for ImGui API |

---

*Run `/sync-docs` to regenerate.*
