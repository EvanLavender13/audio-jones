# Angular Slider Standardization

Standardize all angle sliders to display degrees while storing radians internally. Currently some sliders show radians (Physarum, Flow Field) while others show degrees (Drawable rotation).

## Current State

Sliders needing conversion:
- `src/ui/imgui_effects.cpp:55-56` - Physarum Sensor Angle (shows "rad")
- `src/ui/imgui_effects.cpp:57-58` - Physarum Turn Angle (shows "rad")
- `src/ui/imgui_effects.cpp:30` - Kaleido Spin (raw float, no units)
- `src/ui/imgui_effects.cpp:85-86` - Flow Field Rot Base (raw float)
- `src/ui/imgui_effects.cpp:87-88` - Flow Field Rot Radial (raw float)

Existing infrastructure:
- `src/ui/ui_units.h:9-17` - `SliderAngleDeg()` for non-modulatable sliders
- `src/ui/modulatable_slider.cpp:267-330` - `ModulatableSlider()` implementation
- `src/automation/param_registry.cpp:11-26` - Parameter definitions (min/max in radians)

## Phase 1: Add displayScale to ModulatableSlider

**Goal**: Enable unit conversion in modulatable sliders without duplicating min/max definitions.

**Modify**:
- `src/ui/modulatable_slider.h` - Add `displayScale` parameter (default 1.0f)
- `src/ui/modulatable_slider.cpp` - Apply scale to value and bounds for display, inverse on input

**Done when**: Existing sliders work unchanged (scale=1.0f default).

---

## Phase 2: Add ModulatableSliderAngleDeg wrapper

**Goal**: Provide convenient angle slider with degree display.

**Modify**:
- `src/ui/ui_units.h` - Add `ModulatableSliderAngleDeg()` wrapper that calls `ModulatableSlider()` with `RAD_TO_DEG` scale and "%.1f °" format

**Done when**: Wrapper compiles and can be called.

---

## Phase 3: Convert Physarum angle sliders

**Goal**: Physarum sensor/turn angles display degrees.

**Modify**:
- `src/ui/imgui_effects.cpp:55-58` - Replace `ModulatableSlider` calls with `ModulatableSliderAngleDeg` for sensorAngle and turningAngle

**Done when**: Physarum sliders show "°" and display 0-360 range.

---

## Phase 4: Convert rotation speed sliders

**Goal**: Kaleido Spin and Flow Field rotation display degrees/frame.

**Modify**:
- `src/ui/imgui_effects.cpp:30` - Convert Kaleido Spin to use `SliderAngleDeg()` with "%.2f °/f" format
- `src/ui/imgui_effects.cpp:85-88` - Convert Flow Field rotBase/rotRadial to `ModulatableSliderAngleDeg` with "%.2f °/f" format

**Done when**: All rotation sliders show degree units.

---

## Phase 5: Document convention in CLAUDE.md

**Goal**: Prevent future inconsistency.

**Modify**:
- `CLAUDE.md` - Add bullet under Code Style about angle display convention

**Done when**: CLAUDE.md contains the convention.
