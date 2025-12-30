# Textured Shapes Sample Final Output

Textured shapes currently sample from `shapeSampleTex` (a copy of accumTexture before output effects). This change makes them sample from the previous frame's output after trail boost and kaleidoscope effects, enabling recursive visual patterns.

**Captured effects** (compound recursively): trail boost, kaleidoscope
**Excluded effects** (apply once at display): chromatic aberration, FXAA, gamma

## Current State

Texture flow and where to hook in:

- `src/render/post_effect.h:13` - `shapeSampleTex` field stores accumTexture copy
- `src/render/post_effect.cpp:115,152,178,182` - shapeSampleTex init/uninit/resize
- `src/render/render_pipeline.cpp:190-199` - `RenderPipelineUpdateShapeSample` copies accumTexture before output effects
- `src/render/render_pipeline.cpp:227-230` - Gamma shader renders directly to screen (no texture target)
- `src/render/render_pipeline.h:12-14` - `RenderPipelineUpdateShapeSample` declaration
- `src/main.cpp:173` - Calls `RenderPipelineUpdateShapeSample`
- `src/main.cpp:253` - RenderContext points `accumTexture` to `shapeSampleTex.texture`
- `src/render/render_context.h:14` - `accumTexture` field documentation

**Key insight:** Shapes sample `shapeSampleTex` which is copied BEFORE output effects. The "final output" with physarum/kaleidoscope/chromatic/FXAA/gamma only exists during screen rendering and is never stored.

---

## Phase 1: Rename Texture Field

**Goal**: Rename `shapeSampleTex` to `outputTexture` to reflect new semantics.

**Build**:
- `src/render/post_effect.h:13` - Rename field, update comment to document 1-frame delay behavior
- `src/render/post_effect.cpp` - Find-replace `shapeSampleTex` → `outputTexture` at lines 115, 118, 152, 178, 182
- `src/render/render_context.h:14` - Update `accumTexture` comment to note 1-frame delay

**Done when**: Project builds with renamed field, behavior unchanged.

---

## Phase 2: Move Texture Update to Output Pipeline

**Goal**: Update `outputTexture` after trail boost + kaleidoscope, before chromatic/FXAA/gamma.

**Build**:
- `src/render/render_pipeline.cpp:190-199` - Delete `RenderPipelineUpdateShapeSample` function
- `src/render/render_pipeline.h:12-14` - Delete `RenderPipelineUpdateShapeSample` declaration and comment
- `src/render/render_pipeline.cpp` - Modify `RenderPipelineApplyOutput`:
  - Copy to `outputTexture` after kaleidoscope pass, before chromatic pass
  - Continue with chromatic → FXAA → gamma → screen blit

**Done when**: `outputTexture` contains trail boost + kaleidoscope but not chromatic/FXAA/gamma.

---

## Phase 3: Update Main Loop References

**Goal**: Wire shapes to sample from `outputTexture` and remove obsolete call.

**Build**:
- `src/main.cpp:173` - Delete `RenderPipelineUpdateShapeSample(ctx->postEffect)` call
- `src/main.cpp:253` - Change `.accumTexture = ctx->postEffect->shapeSampleTex.texture` to `.accumTexture = ctx->postEffect->outputTexture.texture`

**Done when**: Textured shapes sample from previous frame's final output including all effects.

---

## Phase 4: Verify and Test

**Goal**: Confirm feature works correctly.

**Build**:
- Create test preset with textured shape over kaleidoscope effect
- Verify textured shapes show physarum trails
- Verify `texZoom` slider still zooms texture correctly
- Verify `texAngle` slider still rotates texture correctly
- Test window resize recreates outputTexture correctly
- Run `/sync-architecture` to update generated docs

**Done when**: All visual effects visible in textured shapes, controls work, no regressions.

---

## Phase 5: Add Texture Brightness Attenuation

**Goal**: Prevent brightness accumulation in recursive texture sampling.

**Problem**: Textured shapes re-inject sampled content each frame. Without attenuation, brightness accumulates faster than feedback decay can remove it, causing white blowout over time.

**Solution**: Add `texBrightness` parameter (0.0-1.0, default 0.9) that attenuates sampled RGB values before drawing. Each recursion loses 10% brightness at default, allowing decay to win.

**Build**:
- `src/config/shape_config.h` - Add `texBrightness` field to ShapeConfig
- `src/render/post_effect.h` - Add `shapeTexBrightnessLoc` uniform location
- `src/render/post_effect.cpp` - Get uniform location in `GetShaderUniformLocations`
- `src/render/shape.cpp` - Set `texBrightness` uniform before drawing textured shapes
- `shaders/shape_texture.fs` - Add `texBrightness` uniform, multiply sampled RGB
- `src/ui/imgui_drawables.cpp` - Add slider in shape UI panel

**Done when**: Slider controls recursion brightness, no more white blowout with default settings.

---

## Notes

**1-frame delay**: Textured shapes sample `outputTexture` containing the previous frame's final output. At 60fps this is ~16.7ms delay, imperceptible to human vision.

**Memory**: No net change. `outputTexture` replaces `shapeSampleTex` (same size, same HDR format).

**Performance**: Near-zero impact. One blit removed (pre-output copy), one blit added (post-gamma copy). Gamma now renders to texture instead of screen, plus one extra blit to screen.
