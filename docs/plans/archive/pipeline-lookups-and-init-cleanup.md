# Pipeline Lookups and Init Cleanup

Replace two linear-scan functions in `render_pipeline.cpp` with O(1) lookup tables, fix the cascading resource leak in `PostEffectInit`, and update the add-effect skill to match the new lookup table pattern.

---

## Design

### Lookup Tables (render_pipeline.cpp)

Replace `IsHalfResEffect()` (linear scan of 3-element array) and `IsGeneratorBlendEffect()` (14-way `||` chain) with `static const bool` arrays indexed by `TransformEffectType`.

Each array has `TRANSFORM_EFFECT_COUNT` entries. Set `true` for the relevant enum values, `false` for everything else. The function bodies become a bounds check + array index.

### PostEffectInit Leak Fix (post_effect.cpp)

Current behavior: every `EffectInit` failure path calls `free(pe)` without uniniting previously successful effects. Late failures leak all prior shader/texture/simulation resources.

Fix: replace all `free(pe); return NULL;` blocks (after the render-texture allocation at line ~213) with `goto cleanup;`. The `cleanup:` label calls `PostEffectUninit(pe)` — which already tears down every resource type and calls `free(pe)`.

This works because:
- `calloc` zero-initializes the struct — all shader `.id` fields start at 0
- raylib's `UnloadShader`/`UnloadRenderTexture`/`UnloadTexture` skip id=0
- All simulation `Uninit` functions (Physarum, Boids, etc.) guard against NULL pointers
- `BlendCompositorUninit` guards against NULL
- All effect `Uninit` functions call `UnloadShader` which is safe on zero-initialized shaders

The early failure path (shaders fail at line 193) stays as-is — `PostEffectUninit` expects shaders to exist for its manual `UnloadShader` calls, and at that point nothing else has been allocated. The render-texture failure path (line 213) changes to `goto cleanup;` since `PostEffectUninit` handles partial state.

### Add-Effect Skill Update

Phase 5b of the add-effect skill currently shows the `||`-chain pattern for `IsGeneratorBlendEffect()`. Update to reference the new `GENERATOR_BLEND_EFFECTS` lookup table instead.

---

## Tasks

### Wave 1: All tasks (no file overlap)

#### Task 1.1: Replace linear scans with lookup tables

**Files**: `src/render/render_pipeline.cpp` (modify)

**Do**:
1. Remove the `HALF_RES_EFFECTS[]` array and `HALF_RES_EFFECTS_COUNT` constant
2. Replace with `static const bool HALF_RES_EFFECTS[TRANSFORM_EFFECT_COUNT]` — set `[TRANSFORM_IMPRESSIONIST] = true`, `[TRANSFORM_RADIAL_STREAK] = true`, `[TRANSFORM_WATERCOLOR] = true`, all others default to `false`
3. Rewrite `IsHalfResEffect()`: `return type >= 0 && type < TRANSFORM_EFFECT_COUNT && HALF_RES_EFFECTS[type];`
4. Replace `IsGeneratorBlendEffect()` body with a similar `static const bool GENERATOR_BLEND_EFFECTS[TRANSFORM_EFFECT_COUNT]` array. Set `true` for all 14 generator blend types: `TRANSFORM_CONSTELLATION_BLEND`, `TRANSFORM_PLASMA_BLEND`, `TRANSFORM_INTERFERENCE_BLEND`, `TRANSFORM_SOLID_COLOR`, `TRANSFORM_SCAN_BARS_BLEND`, `TRANSFORM_PITCH_SPIRAL_BLEND`, `TRANSFORM_MOIRE_GENERATOR_BLEND`, `TRANSFORM_SPECTRAL_ARCS_BLEND`, `TRANSFORM_MUONS_BLEND`, `TRANSFORM_FILAMENTS_BLEND`, `TRANSFORM_SLASHES_BLEND`, `TRANSFORM_GLYPH_FIELD_BLEND`, `TRANSFORM_ARC_STROBE_BLEND`, `TRANSFORM_SIGNAL_FRAMES_BLEND`
5. Rewrite `IsGeneratorBlendEffect()`: same pattern as `IsHalfResEffect()`

**Verify**: `cmake.exe --build build` compiles with no warnings.

#### Task 1.2: Fix PostEffectInit cascading leak

**Files**: `src/render/post_effect.cpp` (modify)

**Do**:
1. Add a `cleanup:` label before the final `return pe;` at line ~559: `cleanup: PostEffectUninit(pe); return NULL;`
2. Replace the render-texture failure block (lines 213-223) — remove the manual `UnloadShader` calls and `free(pe)`, replace with `goto cleanup;`
3. Replace every subsequent `free(pe); return NULL;` block (lines 234-538, approximately 50 occurrences) with `goto cleanup;`
4. Keep the `TraceLog` calls before each `goto cleanup;` — they provide diagnostic context
5. Leave the first failure path (line 194-197, shader load failure) unchanged — at that point no other resources exist, and `PostEffectUninit` expects shaders to be loaded

**Verify**: `cmake.exe --build build` compiles with no warnings.

#### Task 1.3: Update add-effect skill for lookup table pattern

**Files**: `.claude/skills/add-effect/SKILL.md` (modify)

**Do**:
1. In Phase 5b (line ~221-230), replace the `IsGeneratorBlendEffect()` `||`-chain example with the new lookup table pattern: "Add `[TRANSFORM_{EFFECT_NAME}_BLEND] = true` entry to the `GENERATOR_BLEND_EFFECTS` array"
2. Keep the `*BlendActive` flag assignment instruction (step 3) unchanged — that part is unaffected

**Verify**: Read the updated skill and confirm the instruction matches the new code pattern.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] `IsHalfResEffect` and `IsGeneratorBlendEffect` return identical results for all enum values (compare mentally against the old implementations)
- [ ] Every `PostEffectInit` failure path after line ~200 routes through `goto cleanup`
- [ ] The first failure path (shader load, line ~194) still uses `free(pe); return NULL;` directly
- [ ] Add-effect skill Phase 5b references lookup table, not `||` chain
