# Half-Resolution Effect Rendering

Run sample-heavy effects at quarter pixel count (half width × half height), then bilinear upsample. Validates with Bokeh (64 samples), generalizes for Impressionist (~200), Kuwahara (~324), etc.

## Current State

- `src/render/post_effect.h:75` - `bloomMips[5]` demonstrates multi-resolution textures
- `src/render/shader_setup.cpp:1130-1190` - `ApplyBloomPasses()` shows downsample/upsample pattern
- `src/render/render_pipeline.cpp:359-361` - Bloom special-cased in transform loop
- `shaders/bokeh.fs` - 64-sample golden-angle disc blur, pure soft output

## Technical Implementation

**Downsample**: Bilinear via `DrawTexturePro()` scaling source to half-res texture. GPU handles filtering.

**Upsample**: Same `DrawTexturePro()` scaling half-res result back to full-res. Bilinear sufficient for blur-heavy effects.

**No special shaders needed** - raylib's `DrawTexturePro` with different source/dest sizes uses hardware bilinear filtering.

## Phase 1: Half-Res Textures

**Goal**: Add shared half-resolution render textures to PostEffect.
**Depends on**: —
**Files**: `src/render/post_effect.h`, `src/render/post_effect.cpp`

**Build**:
- Add `RenderTexture2D halfResA` and `halfResB` to PostEffect struct (after bloomMips)
- In `PostEffectInit()`: create both at `screenWidth/2 × screenHeight/2` using `RenderUtilsInitTextureHDR()`
- In `PostEffectResize()`: unload and recreate at new half dimensions
- In `PostEffectUninit()`: unload both textures

**Verify**: `cmake.exe --build build && ./build/AudioJones.exe` → no crash, textures created (check log for "POST_EFFECT: Initialized" without errors)

**Done when**: Half-res textures exist and resize correctly.

---

## Phase 2: Half-Res Helper Function

**Goal**: Create reusable function that wraps any effect in downsample → run → upsample.
**Depends on**: Phase 1
**Files**: `src/render/shader_setup.h`, `src/render/shader_setup.cpp`

**Build**:
- Declare `void ApplyHalfResEffect(PostEffect* pe, RenderTexture2D* source, int* writeIdx, Shader shader, RenderPipelineShaderSetupFn setup)`
- Implementation:
  1. Downsample: `DrawTexturePro(source->texture, srcRect, halfResA rect)` with shader disabled
  2. Run effect: `BeginTextureMode(halfResB)`, `BeginShaderMode(shader)`, draw `halfResA`, end modes
  3. Upsample: `DrawTexturePro(halfResB.texture, halfRect, pingPong[writeIdx] rect)`
  4. Flip writeIdx

**Verify**: Function compiles. Not yet called.

**Done when**: Helper function exists and compiles.

---

## Phase 3: Half-Res Effect Routing

**Goal**: Route Bokeh through half-res path in transform loop.
**Depends on**: Phase 2
**Files**: `src/render/render_pipeline.cpp`

**Build**:
- Add static array: `static const TransformEffectType HALF_RES_EFFECTS[] = { TRANSFORM_BOKEH };`
- Add helper: `static bool IsHalfResEffect(TransformEffectType type)` that checks membership
- In transform loop (around line 355-371), before the normal `RenderPass`:
  ```cpp
  if (IsHalfResEffect(effectType)) {
      ApplyHalfResEffect(pe, src, &writeIdx, *entry.shader, entry.setup);
      continue;  // skip normal RenderPass
  }
  ```

**Verify**: `cmake.exe --build build && ./build/AudioJones.exe` → enable Bokeh effect → visual output appears (may look different due to half-res)

**Done when**: Bokeh runs through half-res path.

---

## Phase 4: Resolution Uniform Fix

**Goal**: Bokeh shader receives correct (half) resolution for aspect ratio calculation.
**Depends on**: Phase 3
**Files**: `src/render/shader_setup.cpp`

**Build**:
- In `ApplyHalfResEffect()`, before running the effect shader:
  - Set resolution uniform to half-res dimensions: `vec2(screenWidth/2, screenHeight/2)`
- After upsampling, restore original resolution uniform if needed by subsequent effects

**Verify**: Bokeh aspect ratio looks correct (circles not ellipses).

**Done when**: Bokeh output matches full-res quality at reduced cost.

---

## Phase 5: Add More Effects

**Goal**: Extend half-res support to other sample-heavy effects.
**Depends on**: Phase 4
**Files**: `src/render/render_pipeline.cpp`

**Build**:
- Add to `HALF_RES_EFFECTS[]`: `TRANSFORM_IMPRESSIONIST`, `TRANSFORM_KUWAHARA`, `TRANSFORM_RADIAL_STREAK`, `TRANSFORM_WATERCOLOR`
- Test each effect visually

**Verify**: Each effect renders correctly at half-res without obvious artifacts.

**Done when**: All listed effects work through half-res path.

---

## Execution Schedule

| Wave | Phases | Depends on |
|------|--------|------------|
| 1 | Phase 1 | — |
| 2 | Phase 2 | Wave 1 |
| 3 | Phase 3 | Wave 2 |
| 4 | Phase 4 | Wave 3 |
| 5 | Phase 5 | Wave 4 |

Sequential execution required - each phase builds on the previous.

---

## Post-Implementation Notes

Changes made after testing that extend beyond the original plan:

### Added: Oil Paint half-res support (2026-01-25)

**Reason**: Oil Paint is sample-heavy with 2 passes (stroke + lighting). The softening from half-res fits its smudgey aesthetic.

**Changes**:
- `shader_setup.h`: Added `ApplyHalfResOilPaint()` declaration
- `shader_setup.cpp`: Implemented 2-pass half-res pipeline (downsample → stroke → lighting → upsample)
- `render_pipeline.cpp`: Route Oil Paint through `ApplyHalfResOilPaint()`

### Rejected: Pencil Sketch half-res

**Reason**: Line detection at half resolution doubled apparent stroke width when upsampled, producing unacceptably dark/thick lines.

**Decision**: Keep Pencil Sketch at full resolution.
