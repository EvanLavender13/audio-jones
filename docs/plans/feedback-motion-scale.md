# Feedback Motion Scale

Global time-dilation control for the feedback pipeline. Reduces per-frame displacement of all feedback transforms while auto-coupling decay compensation to preserve trail length.

**Note**: The research document describes two parameters: `motionScale` (feedback transforms) and `waveformMotionScale` (waveform shape smoothing). The `waveformMotionScale` feature is already implemented in `src/config/drawable_config.h:26`, `src/render/drawable.cpp:88-108`, and registered for modulation. This plan covers only the remaining `motionScale` parameter for feedback transforms.

## Current State

- `src/config/effect_config.h:268-275` - `EffectConfig` contains `halfLife`, `feedbackDesaturate`, `flowField`, `feedbackFlow`, `proceduralWarp`
- `src/render/shader_setup.cpp:153-218` - `SetupFeedback` computes `rotBase = rotationSpeed * deltaTime`, passes zoom/dx/dy/warp uniforms
- `src/render/shader_setup.cpp:226-234` - `SetupBlurV` passes `halfLife` and `deltaTime` to blur shader
- `src/render/render_pipeline.cpp:221-231` - `RenderPipelineApplyFeedback` accumulates `warpTime += deltaTime * warpSpeed`
- `shaders/blur_v.fs:38-42` - decay formula: `exp(-0.693147 * deltaTime / halfLife)`
- `src/ui/imgui_effects.cpp:125-130` - FEEDBACK group UI with Blur, Half-life, Desat sliders

## Technical Implementation

**Source**: `docs/research/feedback-motion-scale.md`

### Feedback Transform Scaling

Apply `motionScale` to all per-frame motion parameters:

**Speed/displacement values** — direct multiplication:
```cpp
float rotBase = rotationSpeed * deltaTime * motionScale;
float rotRadial = rotationSpeedRadial * deltaTime * motionScale;
warpTime += deltaTime * warpSpeed * motionScale;
float dxEff = dxBase * motionScale;
float dyEff = dyBase * motionScale;
float flowEff = feedbackFlowStrength * motionScale;
```

**Identity-centered values** — scale deviation from 1.0:
```cpp
float zoomEff = 1.0f + (zoomBase - 1.0f) * motionScale;
float sxEff = 1.0f + (sx - 1.0f) * motionScale;
float syEff = 1.0f + (sy - 1.0f) * motionScale;
```

At `motionScale = 0`: all motion stops (zoom=1, rotation=0, translation=0).
At `motionScale = 1`: behavior unchanged.

**Radial and angular components** follow the same pattern:
- Radial speed offsets (rotRadial, dxRadial, dyRadial): direct multiplication
- Radial zoom offsets (zoomRadial, zoomAngular): direct multiplication (they're additive modifiers, not identity-centered)

### Decay Compensation

The research specifies `effectiveDecay = decay^motionScale` where `decay` is a per-frame multiplier (0-1). The codebase uses `halfLife` (seconds) with shader formula `decayMultiplier = exp(-0.693147 * deltaTime / halfLife)`.

To preserve trail length when motion slows, increase effective halfLife proportionally to motion slowdown:

```cpp
float effectiveHalfLife = halfLife / motionScale;
```

**Why this works**: If motion slows to 0.25x speed, content takes 4x longer to travel the same distance. Trails need to persist 4x longer in time, so effective halfLife = halfLife / 0.25 = 4x original.

At `motionScale = 1`: `effectiveHalfLife = halfLife` (unchanged).
At `motionScale = 0.25`: `effectiveHalfLife = halfLife / 0.25 = 4 * halfLife` (trails last 4x longer).
At `motionScale → 0`: `effectiveHalfLife → ∞` (clamp to prevent division by zero, e.g., `max(motionScale, 0.01f)`).

<!-- Intentional deviation from research: Research uses decay^motionScale formula. Codebase uses halfLife-based exponential decay. The equivalent halfLife transformation is division, not power. Both achieve the same goal: proportionally slowing fade to match motion slowdown. -->

---

## Phase 1: Config + Registration

**Goal**: Add `motionScale` field and register for modulation.
**Depends on**: —
**Files**: `src/config/effect_config.h`, `src/automation/param_registry.cpp`

**Build**:
- Add `float motionScale = 1.0f;` to `EffectConfig` after `feedbackDesaturate`
- Add `{"effects.motionScale", {0.01f, 1.0f}}` to `PARAM_RANGES` in param_registry.cpp

**Verify**: `cmake.exe --build build` → compiles without error.

**Done when**: Build succeeds, new field exists.

---

## Phase 2: Transform Scaling in SetupFeedback

**Goal**: Apply motionScale to all feedback transform uniforms.
**Depends on**: Phase 1
**Files**: `src/render/shader_setup.cpp`

**Build**:
In `SetupFeedback`:
- Retrieve `float ms = pe->effects.motionScale;`
- Modify rotBase/rotRadial: multiply by `ms`
- Modify zoom: `float zoomEff = 1.0f + (pe->effects.flowField.zoomBase - 1.0f) * ms;`
- Modify sx/sy: same deviation pattern
- Modify dx/dy: multiply base values by `ms`
- Modify feedbackFlowStrength: multiply by `ms`

**Verify**: `cmake.exe --build build && ./build/AudioJones.exe` → app runs (no visual change yet since motionScale defaults to 1.0).

**Done when**: All transform uniforms incorporate motionScale.

---

## Phase 3: WarpTime Accumulation Scaling

**Goal**: Scale warpTime accumulation by motionScale.
**Depends on**: Phase 1
**Files**: `src/render/render_pipeline.cpp`

**Build**:
- In `RenderPipelineApplyFeedback`, change:
  `pe->warpTime += deltaTime * pe->effects.proceduralWarp.warpSpeed;`
  to:
  `pe->warpTime += deltaTime * pe->effects.proceduralWarp.warpSpeed * pe->effects.motionScale;`

**Verify**: `cmake.exe --build build && ./build/AudioJones.exe` → warp animation still works at default motionScale=1.0.

**Done when**: WarpTime scales with motionScale.

---

## Phase 4: Decay Compensation in SetupBlurV

**Goal**: Apply decay coupling formula.
**Depends on**: Phase 1
**Files**: `src/render/shader_setup.cpp`

**Build**:
- In `SetupBlurV`, before passing halfLife:
  ```cpp
  float safeMotionScale = fmaxf(pe->effects.motionScale, 0.01f);
  float effectiveHalfLife = pe->effects.halfLife / safeMotionScale;
  ```
- Pass `effectiveHalfLife` to shader instead of `pe->effects.halfLife`

**Verify**: `cmake.exe --build build && ./build/AudioJones.exe` → trails still decay normally at motionScale=1.0.

**Done when**: Decay compensation formula applied.

---

## Phase 5: UI + Serialization

**Goal**: Add slider and preset persistence.
**Depends on**: Phase 1
**Files**: `src/ui/imgui_effects.cpp`, `src/config/preset.cpp`

**Build**:
- In `imgui_effects.cpp`, in FEEDBACK group after Blur slider:
  `ModulatableSliderLog("Motion", &e->motionScale, "effects.motionScale", "%.3f", modSources);`
- In `preset.cpp`, add `motionScale` to the `EffectConfig` JSON macro (in the existing NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT for EffectConfig... if it exists, otherwise add to whatever serialization pattern is used)

**Verify**: `cmake.exe --build build && ./build/AudioJones.exe` → Motion slider appears below Blur. Drag to 0.1: all feedback motion slows dramatically. Warp slows. Trails remain consistent length. Save/load preset preserves value.

**Done when**: UI works, serialization works, motion visibly scales.

---

## Execution Schedule

| Wave | Phases | Depends on |
|------|--------|------------|
| 1 | Phase 1 | — |
| 2 | Phase 2, Phase 3, Phase 4 | Wave 1 |
| 3 | Phase 5 | Wave 1 |

---

## Post-Implementation Notes

### Fixed: Warp displacement not scaling (2026-01-24)

**Reason**: Original implementation only scaled warpTime accumulation (animation speed), not the warp displacement intensity. Content was still pushed at full strength, just with a slower-changing pattern.

**Changes**:
- `src/render/shader_setup.cpp`: Scale `proceduralWarp.warp` by motionScale before passing to shader
