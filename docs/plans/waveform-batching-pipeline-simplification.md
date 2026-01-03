# Waveform Batching and Pipeline Simplification

Batch waveform geometry into single draw calls and simplify the render pipeline to eliminate redundant passes. Reduces waveform draw calls from ~2047 to 1 and pipeline passes from 6 to 2.

## Current State

Waveform rendering bottleneck:
- `src/render/waveform.cpp:194-204` - DrawLineEx + DrawCircleV per segment (~2047 calls)
- `src/render/waveform.cpp:268-269` - Same pattern for circular (~4096 calls)

Pipeline complexity:
- `src/render/render_pipeline.cpp:459-485` - 6 drawable passes per frame (pre-feedback, 3 trail maps, post-feedback, output)
- `src/render/drawable.cpp:138-140` - feedbackPhase splits opacity between pre/post passes
- `src/config/drawable_config.h:16` - feedbackPhase field on DrawableBase

Working rlgl reference:
- `src/render/shape.cpp:79-122` - Triangle batching pattern that works

---

## Phase 1: ThickLine Module

**Goal**: Create a reusable thick polyline renderer with miter/bevel joins.

**Build**:
- Create `src/render/thick_line.h` with procedural API:
  - `ThickLineBegin(float thickness)` - start batch, store thickness
  - `ThickLineVertex(Vector2 pos, Color color)` - add point to polyline
  - `ThickLineEnd(bool closed)` - compute geometry and emit to GPU
- Create `src/render/thick_line.cpp`:
  - Static point buffer (4096 points max)
  - Miter join geometry computation with bevel fallback at sharp angles (>150 degrees)
  - Emit as `RL_TRIANGLES` (2 triangles = 6 vertices per segment)

**rlgl call order** (critical - reference `shape.cpp:83-84`):
```cpp
rlBegin(RL_TRIANGLES);  // NOT RL_TRIANGLE_STRIP - raylib doesn't support it!
rlColor4ub(r, g, b, a);
rlVertex2f(x1, y1);
rlVertex2f(x2, y2);
rlVertex2f(x3, y3);
// 6 vertices per segment (2 triangles forming a quad)
rlEnd();
```

**Geometry per segment** (quad from 2 triangles):
```
   left1 -------- left2
     |  \          |
     |    \  T2    |
     | T1   \      |
     |        \    |
  right1 ------- right2

T1: left1, right1, right2
T2: left1, right2, left2
```

**Done when**: ThickLine draws a visible colored polyline with correct thickness.

---

## Phase 2: Waveform Integration

**Goal**: Replace per-segment draw calls with ThickLine batch, remove unused interpolation.

**Build**:
- Modify `src/render/waveform.h`:
  - Remove `INTERPOLATION_MULT` constant
- Modify `src/render/waveform.cpp`:
  - Include `thick_line.h`
  - Remove `CubicInterp()` function (lines 169-177)
  - Rewrite `DrawWaveformLinear()` (lines 179-214):
    - `ThickLineBegin(thickness)`
    - Loop: compute position + color, call `ThickLineVertex()`
    - `ThickLineEnd(false)` for open polyline
  - Rewrite `DrawWaveformCircular()` (lines 216-271):
    - Remove cubic interpolation logic (was no-op with INTERPOLATION_MULT=1)
    - Direct sample access: `samples[i % count]`
    - Same ThickLine pattern, `ThickLineEnd(true)` for closed loop
  - Remove all `DrawLineEx()` and `DrawCircleV()` calls

**Done when**: Waveforms render visually identical to before, profiler shows 1 draw call per waveform.

---

## Phase 3: Pipeline Simplification

**Goal**: Single drawable pass after feedback, remove feedbackPhase complexity.

**Build**:

Modify `src/config/drawable_config.h`:
- Replace `feedbackPhase` with `opacity` field (same default 1.0f)

Modify `src/render/drawable.cpp`:
- Simplify `DrawableRenderCore()` (lines 103-168):
  - Remove `isPreFeedback` parameter
  - Use `d->base.opacity` directly instead of phase calculation
- Remove `DrawableRenderAll()` function (no longer needed)

Modify `src/render/render_pipeline.cpp`:
- Remove functions: `RenderPipelineDrawablesPreFeedback`, `RenderPipelineDrawablesPostFeedback`, `RenderPipelineDrawablesWithPhase`, `RenderPipelineDrawablesToPhysarum`, `RenderPipelineDrawablesToCurlFlow`, `RenderPipelineDrawablesToAttractor`
- Simplify `RenderPipelineExecute()` (lines 452-494):
  ```
  1. ProfilerBeginZone(ZONE_FEEDBACK)
  2. RenderPipelineApplyFeedback() - warp, blur, decay + simulation updates
  3. ProfilerEndZone(ZONE_FEEDBACK)
  4. ProfilerBeginZone(ZONE_DRAWABLES)
  5. PostEffectBeginDrawStage()
  6. RenderPipelineDrawablesFull() - single pass, all drawables
  7. PostEffectEndDrawStage()
  8. ProfilerEndZone(ZONE_DRAWABLES)
  9. Output chain...
  ```

Modify `src/render/render_pipeline.h`:
- Update `ProfileZoneId` enum: remove PRE_FEEDBACK, POST_FEEDBACK, PHYSARUM_TRAILS, CURL_TRAILS, ATTRACTOR_TRAILS; add DRAWABLES
- Remove deleted function declarations

Modify `src/render/drawable.h`:
- Remove `DrawableRenderAll` declaration
- Update `DrawableRenderCore` signature (remove isPreFeedback)

**Done when**: Profiler shows 2 zones (Feedback + Drawables), simulations still react to waveforms via accumTexture.

---

## Phase 4: Cleanup

**Goal**: Update UI and serialization.

**Build**:
- Modify UI to rename "Feedback Phase" slider to "Opacity" (find in drawable panel code)
- Modify `src/config/preset.cpp`:
  - Change JSON key from `feedbackPhase` to `opacity`
  - Add migration: if loading old preset with `feedbackPhase`, map to `opacity`
- Update profiler zone names array in `render_pipeline.cpp:16-24`
- Remove unused includes and dead code

**Done when**: Presets save/load correctly, UI shows "Opacity" slider.
