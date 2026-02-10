# Effect Descriptor Table

Replace 5 hand-maintained parallel switch/array structures with a single compile-time descriptor table. Each transform effect declares its properties in one row. Adding a new effect reduces from 8+ file edits to 1 table row + the effect module itself.

**Addresses fragile areas**: Transform effect dispatch table, Generator Blend Pipeline, PostEffect Init/Uninit Sequence.

## Design

### Descriptor Entry Struct

```
src/config/effect_descriptor.h
```

Each entry captures everything the pipeline needs to know about one transform effect:

```
struct EffectDescriptor {
    TransformEffectType type;
    const char *name;           // Display name ("Sine Warp")
    const char *categoryBadge;  // UI badge ("WARP", "GEN", "SYM")
    int categorySectionIndex;   // UI section color index (0-10)
    size_t enabledOffset;       // offsetof(EffectConfig, xxx.enabled)
    uint8_t flags;              // bitmask: IS_BLEND | IS_HALF_RES | IS_SIM_BOOST | NEEDS_RESIZE
};
```

Flag constants:

| Flag | Value | Meaning |
|------|-------|---------|
| `EFFECT_FLAG_NONE` | 0 | Standard transform effect |
| `EFFECT_FLAG_BLEND` | 1 | Generator blend — routes through scratch+compositor |
| `EFFECT_FLAG_HALF_RES` | 2 | Renders at half resolution |
| `EFFECT_FLAG_SIM_BOOST` | 4 | Simulation trail boost composite |
| `EFFECT_FLAG_NEEDS_RESIZE` | 8 | Effect owns render textures — call Resize on window change |

### Descriptor Table

```
src/config/effect_descriptor.cpp
```

A `constexpr` array of `EffectDescriptor` indexed by `TransformEffectType`:

```
static constexpr EffectDescriptor EFFECT_DESCRIPTORS[TRANSFORM_EFFECT_COUNT] = {
    [TRANSFORM_SINE_WARP] = {
        TRANSFORM_SINE_WARP, "Sine Warp", "WARP", 1,
        offsetof(EffectConfig, sineWarp.enabled), EFFECT_FLAG_NONE
    },
    [TRANSFORM_CONSTELLATION_BLEND] = {
        TRANSFORM_CONSTELLATION_BLEND, "Constellation Blend", "GEN", 10,
        offsetof(EffectConfig, constellation.enabled), EFFECT_FLAG_BLEND
    },
    [TRANSFORM_BLOOM] = {
        TRANSFORM_BLOOM, "Bloom", "OPT", 7,
        offsetof(EffectConfig, bloom.enabled), EFFECT_FLAG_NEEDS_RESIZE
    },
    [TRANSFORM_IMPRESSIONIST] = {
        TRANSFORM_IMPRESSIONIST, "Impressionist", "ART", 4,
        offsetof(EffectConfig, impressionist.enabled), EFFECT_FLAG_HALF_RES
    },
    // ... one row per effect
};
```

Designated initializers ensure enum-to-index correspondence. Compile error if any index is out of range.

### What This Table Replaces

| Current code | Location | Replaced by |
|---|---|---|
| `TRANSFORM_EFFECT_NAMES[]` array | `effect_config.h:167-246` | `EFFECT_DESCRIPTORS[type].name` |
| `IsTransformEnabled()` 80-case switch | `effect_config.h:573-735` | `*(bool*)((char*)config + desc.enabledOffset)` |
| `GetTransformCategory()` 80-case switch | `imgui_effects.cpp:38-143` | `{desc.categoryBadge, desc.categorySectionIndex}` |
| `GENERATOR_BLEND_EFFECTS` lookup table | `render_pipeline.cpp:35-53` | `desc.flags & EFFECT_FLAG_BLEND` |
| `HALF_RES_EFFECTS` lookup table | `render_pipeline.cpp:27-33` | `desc.flags & EFFECT_FLAG_HALF_RES` |
| 15 `*BlendActive` bool fields | `post_effect.h:247-261` | Removed — pipeline checks `IsTransformEnabled()` directly |
| 7 `*BoostActive` bool fields | `post_effect.h:238-244` | Removed — pipeline checks `IsTransformEnabled()` directly |
| 15 blend flag assignments in `RenderPipelineApplyOutput` | `render_pipeline.cpp:353-384` | Removed |
| 7 boost flag assignments in `RenderPipelineApplyOutput` | `render_pipeline.cpp:332-351` | Removed |

### What This Table Does NOT Replace

These stay as-is (out of scope for this plan):

- `GetTransformEffect()` switch in `shader_setup.cpp` — returns `{shader*, setup_fn, enabled*}` which requires runtime PostEffect pointer dereference (not a compile-time property)
- `GetGeneratorScratchPass()` switch in `shader_setup_generators.cpp` — same reason
- Init/Uninit/Register call sequences in `post_effect.cpp` — these have 3 different init signatures and heterogeneous field types. A table-driven init loop requires function pointer casts and discriminated unions that add complexity without proportional benefit. The descriptor table already ensures the enabled-check and pipeline routing are correct; the init calls remain sequential but are low-frequency (startup/shutdown only).

**Rationale**: The fragility of init/uninit is that a missing call causes silent failure. The descriptor table doesn't directly fix that — but removing the `*BlendActive`/`*BoostActive` flag management eliminates the most error-prone part (the one that causes effects to silently never render). The init/uninit sequences are verbose but safe: `calloc` zeroes fields, uninit tolerates zero-initialized handles.

### IsTransformEnabled Replacement

The current 80-case switch becomes:

```c
inline bool IsTransformEnabled(const EffectConfig *e, TransformEffectType type) {
    const EffectDescriptor *desc = &EFFECT_DESCRIPTORS[type];
    return *(const bool *)((const char *)e + desc->enabledOffset);
}
```

This works because the user confirmed the intensity threshold checks (`blendIntensity > 0` and `boostIntensity > 0`) are unnecessary — those values are effectively never zero while the effect is enabled.

### Pipeline Changes

`RenderPipelineApplyOutput()` currently computes 22 `*Active` bools each frame and stores them in PostEffect fields. With the descriptor table:

1. Remove all `*BlendActive` and `*BoostActive` fields from `PostEffect`
2. Remove the 22 flag-assignment lines from `RenderPipelineApplyOutput()`
3. `GetTransformEffect()` in `shader_setup.cpp` already returns an `enabled*` pointer per effect — the blend/boost entries currently point at `pe->*BlendActive`. Change them to point at `pe->effects.xxx.enabled` directly (matching the simple effects)
4. `IsGeneratorBlendEffect()` and `IsHalfResEffect()` become: `EFFECT_DESCRIPTORS[type].flags & EFFECT_FLAG_BLEND` (etc.)

### Parameters

No new user-facing parameters. This is a structural refactor.

---

## Tasks

### Wave 1: Create descriptor table

#### Task 1.1: Create effect_descriptor.h and effect_descriptor.cpp

**Files**: `src/config/effect_descriptor.h` (create), `src/config/effect_descriptor.cpp` (create)

**Creates**: `EffectDescriptor` struct, flag constants, `EFFECT_DESCRIPTORS[]` array, accessor functions (`EffectDescriptorGet`, `EffectDescriptorName`, `EffectDescriptorCategory`, `EffectDescriptorIsEnabled`)

**Do**:
- Define `EffectDescriptor` struct and `EFFECT_FLAG_*` constants in the header
- Define `EFFECT_DESCRIPTORS[TRANSFORM_EFFECT_COUNT]` array using C++20 designated initializers in the .cpp
- Populate all 80 entries. Use `effect_config.h:85-164` for the enum values, `effect_config.h:167-246` for display names, `imgui_effects.cpp:38-143` for category badges/indices, `render_pipeline.cpp:27-53` for blend/halfres flags
- Add `EFFECT_FLAG_SIM_BOOST` for the 7 `*_BOOST` entries
- Add `EFFECT_FLAG_NEEDS_RESIZE` for `TRANSFORM_BLOOM`, `TRANSFORM_ANAMORPHIC_STREAK`, `TRANSFORM_OIL_PAINT`
- Provide inline accessor: `IsTransformEnabled(const EffectConfig*, TransformEffectType)` using `enabledOffset`
- Add to `CMakeLists.txt`

**Verify**: `cmake.exe --build build` compiles with the new file included.

---

### Wave 2: Replace consumers (parallel — no file overlap)

#### Task 2.1: Replace TRANSFORM_EFFECT_NAMES and IsTransformEnabled

**Files**: `src/config/effect_config.h` (modify)

**Depends on**: Wave 1

**Do**:
- Remove `TRANSFORM_EFFECT_NAMES[]` array (lines 167-246)
- Remove `TransformEffectName()` inline function (lines 248-253)
- Remove `IsTransformEnabled()` 80-case switch (lines 573-735)
- Add `#include "effect_descriptor.h"` and redirect `TransformEffectName` and `IsTransformEnabled` to the descriptor table accessors
- Alternatively, keep thin inline wrappers that delegate to the descriptor table

**Verify**: Build succeeds. All callers of `TransformEffectName()` and `IsTransformEnabled()` still compile.

#### Task 2.2: Replace GetTransformCategory

**Files**: `src/ui/imgui_effects.cpp` (modify)

**Depends on**: Wave 1

**Do**:
- Remove `GetTransformCategory()` 80-case switch (lines 38-143)
- Replace with a function body that reads `EFFECT_DESCRIPTORS[type].categoryBadge` and `.categorySectionIndex`
- Include `config/effect_descriptor.h`

**Verify**: Build succeeds.

#### Task 2.3: Replace blend/halfres/boost lookups and remove Active bools

**Files**: `src/render/render_pipeline.cpp` (modify), `src/render/post_effect.h` (modify)

**Depends on**: Wave 1

**Do**:
- In `post_effect.h`: Remove the 7 `*BoostActive` bool fields (lines 238-244) and 15 `*BlendActive` bool fields (lines 247-261)
- In `render_pipeline.cpp`: Remove `HALF_RES_EFFECTS` and `GENERATOR_BLEND_EFFECTS` constexpr lookup tables (lines 23-53)
- Remove `IsHalfResEffect()` and `IsGeneratorBlendEffect()` functions — replace with inline checks against descriptor flags
- Remove the 22 `*Active` flag assignments in `RenderPipelineApplyOutput()` (lines 332-384)
- Include `config/effect_descriptor.h`

**Verify**: Build succeeds.

#### Task 2.4: Update GetTransformEffect enabled pointers

**Files**: `src/render/shader_setup.cpp` (modify)

**Depends on**: Wave 1, Task 2.3

**Do**:
- The 7 sim boost entries currently return `&pe->*BoostActive` as the enabled pointer. Change to `&pe->effects.xxx.enabled` (e.g., `&pe->effects.physarum.enabled`)
- The 15 generator blend entries currently return `&pe->*BlendActive`. Change to `&pe->effects.xxx.enabled` (e.g., `&pe->effects.constellation.enabled`)
- This makes all 80 entries consistent: every `TransformEffectEntry.enabled` points into `pe->effects`

**Verify**: Build succeeds. Run the app and toggle a blend effect and a sim boost to confirm they still render.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] `IsTransformEnabled` returns correct results (test: enable/disable effects in UI, verify they toggle)
- [ ] Generator blend effects render (test: enable constellation blend, verify it appears)
- [ ] Simulation boost effects render (test: enable physarum + boost, verify trail compositing)
- [ ] Half-res effects render (test: enable impressionist, verify it renders)
- [ ] Effect ordering in pipeline list shows correct category badges
- [ ] No `*BlendActive` or `*BoostActive` fields remain in `post_effect.h`
- [ ] grep for `BlendActive` and `BoostActive` in `src/` returns zero hits outside of comments
