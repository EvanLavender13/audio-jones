# Spatial UV Flow Fields

Add position-dependent UV displacement to the experimental feedback shader, enabling MilkDrop-style flow fields where different screen regions exhibit different motion characteristics (tunnels, vortices, directional flow).

## Current State

- `src/config/experimental_config.h:4-8` - Config with 3 fields (halfLife, zoomFactor, injectionOpacity)
- `src/render/experimental_effect.h:13-18` - 6 uniform location fields
- `src/render/experimental_effect.cpp:22-30` - GetShaderUniformLocations with 6 calls
- `src/render/experimental_effect.cpp:127-132` - SetShaderValue for zoomFactor
- `shaders/experimental/feedback_exp.fs:17-24` - Simple center-zoom transform
- `src/ui/imgui_experimental.cpp:28-31` - Single "Zoom" slider

## Design Decisions

- **Clean break**: Rename `zoomFactor` to `zoomBase` (no backward compatibility)
- **Sub-struct**: Group 8 flow params into `FlowFieldConfig` (matches LFOConfig pattern)
- **Technical names**: Keep DX Base, Rotation Radial, etc. (matches MilkDrop docs)
- **Transform order**: zoom -> rotate -> translate (MilkDrop order)
- **Aspect correction**: Normalize radius for non-square windows (rad=1 is a circle)

---

## Phase 1: Config and Header

**Goal**: Define the data structures for flow field parameters.

**Build**:
- Create `FlowFieldConfig` struct in `experimental_config.h` with 8 float fields:
  - `zoomBase` (default 0.995f), `zoomRadial` (default 0.0f)
  - `rotBase`, `rotRadial` (defaults 0.0f)
  - `dxBase`, `dxRadial`, `dyBase`, `dyRadial` (defaults 0.0f)
- Replace `zoomFactor` field with `FlowFieldConfig flowField` member
- Add 8 uniform location fields to `ExperimentalEffect` struct in header:
  - `feedbackZoomBaseLoc`, `feedbackZoomRadialLoc`
  - `feedbackRotBaseLoc`, `feedbackRotRadialLoc`
  - `feedbackDxBaseLoc`, `feedbackDxRadialLoc`
  - `feedbackDyBaseLoc`, `feedbackDyRadialLoc`
- Remove old `feedbackZoomFactorLoc` field

**Done when**: Project compiles (will have linker/runtime errors until Phase 2).

---

## Phase 2: Shader Implementation

**Goal**: Implement spatially-varying UV transforms in the feedback shader.

**Build**:
- Add 8 float uniforms to `feedback_exp.fs` (replace single `zoomFactor` uniform):
  - `zoomBase`, `zoomRadial`, `rotBase`, `rotRadial`
  - `dxBase`, `dxRadial`, `dyBase`, `dyRadial`
- Replace UV transform section (lines 17-24) with:
  - Compute aspect-corrected radius: normalize against smaller dimension
  - Compute 4 spatially-varying params: `zoom = zoomBase + rad * zoomRadial`, etc.
  - Apply transforms in order: zoom (scale), rotate (2D matrix), translate (offset)
  - Keep final `clamp(uv, 0.0, 1.0)` to prevent edge artifacts
- Leave blur kernel (lines 26-48) and decay (lines 50-53) unchanged

**Done when**: Shader compiles (will crash at runtime until uniform binding in Phase 3).

---

## Phase 3: Uniform Binding

**Goal**: Wire config values through to shader uniforms.

**Build**:
- Update `GetShaderUniformLocations()` in `experimental_effect.cpp`:
  - Remove `feedbackZoomFactorLoc` lookup
  - Add 8 `GetShaderLocation()` calls for new uniforms
- Update `ExperimentalEffectBeginAccum()`:
  - Remove old `zoomFactor` SetShaderValue call
  - Add 8 `SetShaderValue()` calls using `exp->config.flowField.*` paths

**Done when**: Application runs with default values. Visual output should match previous behavior (all radial coefficients = 0).

---

## Phase 4: UI Panel

**Goal**: Add controls for the 8 flow field parameters.

**Build**:
- Add `#include "ui/imgui_widgets.h"` for DrawSectionBegin/End
- Add static `bool sectionFlowField = false` for collapse state
- Move "Zoom" slider from Feedback section into new Flow Field section
- Add collapsible "Flow Field" section using `DrawSectionBegin("Flow Field", Theme::GLOW_CYAN, &sectionFlowField)`
- Add 8 sliders with ranges from spec:
  - Zoom Base: 0.98 to 1.02 (format "%.4f")
  - Zoom Radial: -0.02 to 0.02
  - Rotation Base/Radial: -0.01 to 0.01 (format "%.4f rad")
  - DX/DY Base/Radial: -0.02 to 0.02
- Add tooltips explaining each parameter
- Update field paths from `cfg->zoomFactor` to `cfg->flowField.zoomBase`, etc.

**Done when**: UI shows collapsible Flow Field section with 8 working sliders.

---

## Phase 5: Validation

**Goal**: Verify implementation correctness and visual quality.

**Build**:
- Test default values: all radial = 0, zoomBase = 0.995 should match previous behavior
- Test tunnel effect: zoomRadial = 0.01 (center zooms faster than edges)
- Test vortex: rotRadial = -0.005 (rotation increases toward center)
- Test horizontal flow: dxRadial = 0.01 (edges push outward)
- Test aspect correction: resize window to 16:9, verify flow field is circular not elliptical
- Test extreme values: verify clamp prevents texture sampling outside bounds

**Done when**: All test cases produce expected visual results without artifacts.
