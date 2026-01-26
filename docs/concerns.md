# Codebase Concerns

> Last sync: 2026-01-25 | Commit: 990f2e7

## Tech Debt

**shader_setup.cpp monolith:**
- Issue: Single 1268-line file handles all shader uniform binding for 40+ effects
- Files: `src/render/shader_setup.cpp`
- Impact: Adding new effects requires modifying one massive file; high merge conflict risk
- Fix approach: Split into per-effect setup files or group by effect category (transforms, simulations, artistic)

**post_effect.h struct bloat:**
- Issue: PostEffect struct holds 70+ shader handles and 150+ uniform locations as flat fields
- Files: `src/render/post_effect.h`, `src/render/post_effect.cpp`
- Impact: Each new effect adds 5-10 fields; struct exceeds cache line efficiency
- Fix approach: Group shaders/uniforms into per-effect sub-structs

**preset.cpp STL exception:**
- Issue: Uses STL containers and nlohmann/json while rest of codebase avoids STL
- Files: `src/config/preset.cpp`
- Impact: Inconsistent patterns; JSON serialization requires exception handling
- Fix approach: Acceptable deviation documented in CLAUDE.md; no action needed

## Known Bugs

None detected.

## Performance Bottlenecks

**param_registry string lookups:**
- Problem: Modulation engine uses std::unordered_map with string keys for every param access
- Files: `src/automation/modulation_engine.cpp`, `src/automation/param_registry.cpp`
- Cause: String hashing on every ModEngineUpdate call (multiple times per frame)
- Improvement path: Use integer IDs for hot paths; keep string lookup for UI/serialization only

**half-res effect overhead:**
- Problem: Some effects (bokeh, watercolor, impressionist, radial streak) run at half resolution for performance
- Files: `src/render/render_pipeline.cpp`, `src/render/shader_setup.cpp`
- Cause: GPU-heavy algorithms require downscaling to maintain framerate
- Improvement path: Current approach is acceptable; document per-effect GPU cost

## Fragile Areas

**Drawable ID management:**
- Files: `src/automation/drawable_params.cpp`, `src/config/preset.cpp`, `src/ui/imgui_panels.h`
- Why fragile: Drawable IDs must stay synchronized between param registry, preset serialization, and UI state counter
- Safe modification: Always call `ImGuiDrawDrawablesSyncIdCounter` and `DrawableParamsSyncAll` after changing drawable array

**TransformEffectType enum ordering:**
- Files: `src/config/effect_config.h`, `src/config/preset.cpp`
- Why fragile: Adding effects mid-enum breaks saved presets unless string serialization handles it
- Safe modification: Commit 74bd7d2 added string serialization for transform order; add new effects at end of enum

**Modulation base value tracking:**
- Files: `src/automation/modulation_engine.cpp`
- Why fragile: LFO modulation applies offsets to stored base values; forgetting `ModEngineWriteBaseValues()` before preset save corrupts values
- Safe modification: Call `ModEngineWriteBaseValues()` before any operation that reads param values for persistence

## Complexity Hotspots

**shader_setup.cpp:**
- File: `src/render/shader_setup.cpp`
- Lines: 1268
- Concern: Contains setup functions for every post-processing effect; switch statement maps enum to function pointers
- Refactor approach: Extract into shader_setup_transforms.cpp, shader_setup_simulations.cpp, shader_setup_artistic.cpp

**post_effect.cpp:**
- File: `src/render/post_effect.cpp`
- Lines: 804
- Concern: Shader loading, texture initialization, and multi-pass rendering all in one file
- Refactor approach: Split shader loading into post_effect_shaders.cpp; keep render logic separate

**imgui_analysis.cpp:**
- File: `src/ui/imgui_analysis.cpp`
- Lines: 681
- Concern: Custom ImGui drawing for beat graphs, band meters, and feature displays
- Refactor approach: Extract meter widgets into imgui_meters.cpp

**imgui_effects.cpp:**
- File: `src/ui/imgui_effects.cpp`
- Lines: 637
- Concern: UI panels for 40+ effects in one file
- Refactor approach: Group by effect category or use code generation for repetitive slider blocks

**preset.cpp:**
- File: `src/config/preset.cpp`
- Lines: 608
- Concern: JSON serialization macros for every config struct; adding new fields requires multiple edits
- Refactor approach: Current nlohmann macros minimize boilerplate; acceptable as-is

**param_registry.cpp:**
- File: `src/automation/param_registry.cpp`
- Lines: 525
- Concern: Giant PARAM_TABLE and DRAWABLE_FIELD_TABLE arrays with all modulatable parameter bounds
- Refactor approach: Consider code generation or constexpr tables with static_assert validation

## Dependencies at Risk

None detected. raylib, miniaudio, Dear ImGui, and nlohmann/json are stable and actively maintained.

## Missing Features

**Hot shader reloading:**
- Problem: Shader changes require application restart
- Blocks: Rapid shader iteration during development

**Preset versioning:**
- Problem: No schema version in preset files
- Blocks: Graceful migration when config structs change

## TODO/FIXME Inventory

| Location | Type | Note |
|----------|------|------|
| None detected | - | Codebase has no TODO/FIXME/HACK/XXX comments |

---

*Run `/sync-docs` to regenerate.*
