# Feedback Flow

UV displacement driven by luminance gradients in the current frame. Creates organic flowing distortion that follows edges and contours. Unlike position-based flow field (zoom/rotation/translation), this displaces based on image content — where edges exist.

**Research doc:** `docs/research/feedback-flow.md`

## Current State

- `shaders/feedback.fs:1-68` — Position-based flow field shader (zoom, rotation, translation by radial distance)
- `src/config/effect_config.h:149-158` — `FlowFieldConfig` struct with 8 position-based parameters
- `src/render/shader_setup.cpp:100-120` — `SetupFeedback()` sets 9 uniforms
- `src/render/post_effect.h:97-106` — Feedback uniform location members
- `src/ui/imgui_effects.cpp:97-114` — Flow Field UI section
- `src/config/preset.cpp:106-107` — FlowFieldConfig JSON serialization
- `src/automation/param_registry.cpp:19-26` — flowField.* parameter entries

## Technical Implementation

### Core Algorithm

Compute luminance gradient at each pixel using a 4-sample cross pattern, then displace UV perpendicular or parallel to gradient direction:

```glsl
// Sample luminance at neighboring pixels
vec2 texelSize = 1.0 / resolution;
vec2 offset = texelSize * gradientScale;

float lumL = getLuminance(texture(inputTex, uv - vec2(offset.x, 0)).rgb);
float lumR = getLuminance(texture(inputTex, uv + vec2(offset.x, 0)).rgb);
float lumD = getLuminance(texture(inputTex, uv - vec2(0, offset.y)).rgb);
float lumU = getLuminance(texture(inputTex, uv + vec2(0, offset.y)).rgb);

// Gradient points toward brighter areas
vec2 gradient = vec2(lumR - lumL, lumU - lumD);
float gradMag = length(gradient);

if (gradMag < edgeThreshold) {
    // No edge, no displacement
    return;
}

// Normalize and rotate by flowAngle
vec2 gradNorm = gradient / gradMag;
float c = cos(flowAngle);
float s = sin(flowAngle);
vec2 flowDir = vec2(
    gradNorm.x * c - gradNorm.y * s,
    gradNorm.x * s + gradNorm.y * c
);

// Displacement scales with gradient magnitude and strength
vec2 displacement = flowDir * gradMag * flowStrength * texelSize;
uv += displacement;
```

### Integration Point

Add gradient displacement **after** position-based flow (zoom/rotate/translate) but **before** UV mirroring in `feedback.fs`. The position-based flow transforms UV space; gradient flow then displaces within that transformed space based on content.

### Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| flowStrength | float | 0–20 | 0.0 | Displacement magnitude in pixels. 0 = disabled. |
| flowAngle | float | 0–2π | 0.0 | Direction relative to gradient (0 = perpendicular, π/2 = parallel) |
| gradientScale | float | 1–5 | 1.0 | Sampling distance for gradient. Larger = smoother response. |
| edgeThreshold | float | 0–0.1 | 0.001 | Minimum gradient magnitude to trigger flow |

### Luminance Function

Use standard Rec. 709 coefficients (same as `feedback.fs:66`):
```glsl
float getLuminance(vec3 c) {
    return dot(c, vec3(0.299, 0.587, 0.114));
}
```

---

## Phase 1: Config and Shader

**Goal**: Create config struct and extend feedback shader with gradient displacement.

**Build**:
- Create `src/config/feedback_flow_config.h` with `FeedbackFlowConfig` struct containing 4 float fields (strength, angle, scale, threshold)
- Add `#include "feedback_flow_config.h"` to `effect_config.h`
- Add `FeedbackFlowConfig feedbackFlow` member to `EffectConfig` struct
- Extend `shaders/feedback.fs`:
  - Add 4 uniforms: `feedbackFlowStrength`, `feedbackFlowAngle`, `feedbackFlowScale`, `feedbackFlowThreshold`
  - Add `getLuminance()` helper function
  - Add gradient flow displacement logic after position-based UV transform, before mirroring

**Done when**: Shader compiles. Config struct exists with defaults.

---

## Phase 2: Shader Setup

**Goal**: Wire uniforms from config to shader.

**Build**:
- Add 4 uniform location members to `PostEffect` struct in `post_effect.h`: `feedbackFlowStrengthLoc`, `feedbackFlowAngleLoc`, `feedbackFlowScaleLoc`, `feedbackFlowThresholdLoc`
- Get uniform locations in `GetShaderUniformLocations()` in `post_effect.cpp`
- Extend `SetupFeedback()` in `shader_setup.cpp` to set the 4 new uniforms from `pe->effects.feedbackFlow.*`

**Done when**: Changing config values in code affects shader output.

---

## Phase 3: Preset Serialization

**Goal**: Save and load feedbackFlow config in presets.

**Build**:
- Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT` macro for `FeedbackFlowConfig` in `preset.cpp`
- Add `j["feedbackFlow"] = e.feedbackFlow` in `to_json` for EffectConfig
- Add `e.feedbackFlow = j.value("feedbackFlow", e.feedbackFlow)` in `from_json`

**Done when**: Preset save/load preserves feedbackFlow settings.

---

## Phase 4: UI Controls

**Goal**: Add sliders to Flow Field section.

**Build**:
- Add UI controls in `imgui_effects.cpp` after existing Flow Field sliders:
  - `ImGui::SeparatorText("Gradient Flow")` divider
  - `ModulatableSlider` for strength with param ID `feedbackFlow.strength`
  - `ModulatableSliderAngleDeg` for angle with param ID `feedbackFlow.angle`
  - `ModulatableSlider` for scale with param ID `feedbackFlow.scale`
  - `ModulatableSlider` for threshold with param ID `feedbackFlow.threshold`

**Done when**: UI sliders appear and modify effect in real-time.

---

## Phase 5: Parameter Registration

**Goal**: Enable modulation routing to feedbackFlow parameters.

**Build**:
- Add 4 entries to `PARAM_TABLE` in `param_registry.cpp`:
  - `{"feedbackFlow.strength", {0.0f, 20.0f}}`
  - `{"feedbackFlow.angle", {-ROTATION_OFFSET_MAX, ROTATION_OFFSET_MAX}}`
  - `{"feedbackFlow.scale", {1.0f, 5.0f}}`
  - `{"feedbackFlow.threshold", {0.0f, 0.1f}}`
- Add 4 corresponding entries to `targets[]` array in `ParamRegistryInit()`

**Done when**: LFOs and audio sources can modulate feedbackFlow parameters.

---

## Verification

- [ ] Gradient flow displaces UVs when strength > 0
- [ ] flowAngle=0 creates spreading/smearing across edges
- [ ] flowAngle=π/2 creates swirling/tracing along edges
- [ ] Works in combination with position-based flow field
- [ ] Preset save/load preserves settings
- [ ] Modulation routes correctly to all 4 parameters
- [ ] Build succeeds with no warnings

## File Summary

| File | Changes |
|------|---------|
| `src/config/feedback_flow_config.h` | Create config struct (new file) |
| `src/config/effect_config.h` | Include and add member |
| `shaders/feedback.fs` | Add uniforms and gradient flow logic |
| `src/render/post_effect.h` | Add 4 uniform location members |
| `src/render/post_effect.cpp` | Get uniform locations |
| `src/render/shader_setup.cpp` | Set uniforms in SetupFeedback() |
| `src/config/preset.cpp` | JSON serialization |
| `src/ui/imgui_effects.cpp` | UI controls |
| `src/automation/param_registry.cpp` | Parameter registration |
