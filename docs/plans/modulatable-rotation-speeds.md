# Modulatable Rotation Speeds

Make rotation speed parameters modulatable across drawables, kaleidoscope, and attractor effects. Allows audio-reactive rotation: bass hits speed up spin, LFOs create oscillating rotation rates.

## Current State

Rotation speeds exist but aren't connected to the modulation system:
- `src/config/drawable_config.h:14` - `DrawableBase.rotationSpeed` (rad/frame)
- `src/config/kaleidoscope_config.h:12` - `KaleidoscopeConfig.rotationSpeed` (rad/tick)
- `src/simulation/attractor_flow.h:45-47` - only static `rotationX/Y/Z` angles, no speeds

UI uses plain `SliderAngleDeg()` instead of modulatable versions:
- `src/ui/drawable_type_controls.cpp:18` - drawable rotation slider
- `src/ui/imgui_effects.cpp:51` - kaleidoscope spin slider
- `src/ui/imgui_effects.cpp:175-177` - attractor rotation sliders (static only)

Modulation infrastructure exists:
- `src/automation/drawable_params.cpp:6-17` - dynamic param registration pattern
- `src/automation/param_registry.cpp:11-30` - static param table pattern
- `src/ui/modulatable_drawable_slider.cpp` - drawable-aware modulatable slider

---

## Phase 1: Attractor Rotation Speed Config

**Goal**: Add rotation speed fields to attractor config and wire accumulation logic.

**Build**:
- Add `rotationSpeedX`, `rotationSpeedY`, `rotationSpeedZ` float fields to `AttractorFlowConfig` in `src/simulation/attractor_flow.h` (default 0.0f)
- In `AttractorFlowUpdate()` at `src/simulation/attractor_flow.cpp`, accumulate speeds into angles before rotation matrix computation:
  ```
  config.rotationX += config.rotationSpeedX
  config.rotationY += config.rotationSpeedY
  config.rotationZ += config.rotationSpeedZ
  ```
- Extend `AttractorFlowConfig` JSON macro in `src/config/preset.cpp` to serialize new fields

**Done when**: Setting non-zero `rotationSpeedX` in code causes attractor to spin continuously.

---

## Phase 2: Static Param Registration

**Goal**: Register kaleidoscope and attractor rotation speeds with modulation engine.

**Build**:
- Add entries to `PARAM_TABLE[]` in `src/automation/param_registry.cpp`:
  - `{"kaleidoscope.rotationSpeed", {-0.1f, 0.1f}}`
  - `{"attractorFlow.rotationSpeedX", {-0.1f, 0.1f}}`
  - `{"attractorFlow.rotationSpeedY", {-0.1f, 0.1f}}`
  - `{"attractorFlow.rotationSpeedZ", {-0.1f, 0.1f}}`
- Add corresponding pointers to `targets[]` array pointing to `EffectConfig` fields

**Done when**: `ModEngineHasRoute("kaleidoscope.rotationSpeed")` returns true after setting a route via code.

---

## Phase 3: Drawable Rotation Speed Registration

**Goal**: Register drawable rotation speeds dynamically like x/y position.

**Build**:
- In `DrawableParamsRegister()` at `src/automation/drawable_params.cpp`, add registration for `rotationSpeed`:
  ```
  snprintf(paramId, ..., "drawable.%u.rotationSpeed", d->id)
  ModEngineRegisterParam(paramId, &d->base.rotationSpeed, -0.05f, 0.05f)
  ```
- Bounds: -0.05 to 0.05 rad/frame (~-2.87° to +2.87°/frame)

**Done when**: `ModEngineHasRoute("drawable.1.rotationSpeed")` works after drawable creation.

---

## Phase 4: Modulatable Drawable Angle Slider

**Goal**: Create UI helper for modulatable angle sliders on drawables.

**Build**:
- Add `ModulatableDrawableSliderAngleDeg()` declaration to `src/ui/modulatable_drawable_slider.h`
- Implement in `src/ui/modulatable_drawable_slider.cpp`:
  - Build paramId as `drawable.{id}.{field}`
  - Delegate to `ModulatableSlider()` with `RAD_TO_DEG` displayScale
  - Signature: `bool ModulatableDrawableSliderAngleDeg(const char* label, float* radians, uint32_t drawableId, const char* field, const ModSources* sources)`

**Done when**: Helper compiles and can be called from drawable controls.

---

## Phase 5: UI Integration

**Goal**: Replace plain sliders with modulatable versions.

**Build**:
- `src/ui/drawable_type_controls.cpp`:
  - Modify `DrawBaseAnimationControls()` to accept `uint32_t drawableId` and `const ModSources* sources` parameters
  - Replace `SliderAngleDeg("Rotation", ...)` with `ModulatableDrawableSliderAngleDeg("Rotation", &base->rotationSpeed, drawableId, "rotationSpeed", sources)`
  - Update callers (`DrawWaveformControls`, `DrawSpectrumControls`, `DrawShapeControls`) to pass drawable id and sources

- `src/ui/imgui_effects.cpp`:
  - Replace kaleidoscope `SliderAngleDeg("Spin", ...)` at line 51 with `ModulatableSliderAngleDeg("Spin", &k->rotationSpeed, "kaleidoscope.rotationSpeed", "%.3f °/f", sources)`
  - Add three attractor spin sliders after the static rotation sliders (after line 177):
    - `ModulatableSliderAngleDeg("Spin X", &af->rotationSpeedX, "attractorFlow.rotationSpeedX", "%.3f °/f", sources)`
    - `ModulatableSliderAngleDeg("Spin Y", &af->rotationSpeedY, "attractorFlow.rotationSpeedY", "%.3f °/f", sources)`
    - `ModulatableSliderAngleDeg("Spin Z", &af->rotationSpeedZ, "attractorFlow.rotationSpeedZ", "%.3f °/f", sources)`

**Done when**: All rotation speed sliders show modulation indicator diamond, popup works, and routing bass to any rotation speed causes visible speed changes.
