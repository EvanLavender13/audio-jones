# Radial Pulse

Polar-native sine distortion creating concentric rings, petal symmetry, and animated spirals. Applies sine displacement in both radial (rings) and angular (petals) directions with spiral coupling for rotating patterns.

## Current State

Transform effect infrastructure exists with 22 effects. Radial Pulse follows the same pattern:
- `src/config/sine_warp_config.h:1-16` - Reference config structure
- `src/render/shader_setup.cpp:227-235` - Reference setup function
- `src/ui/imgui_effects_transforms.cpp:38-133` - Symmetry category UI
- `src/automation/param_registry.cpp:53-54` - SineWarp param registration example
- `docs/research/radial-pulse.md` - Algorithm source

## Technical Implementation

**Source**: `docs/research/radial-pulse.md` (lines 97-111)

**Core algorithm** (displacement vector approach):
```glsl
vec2 radialPulse(vec2 uv) {
    vec2 delta = uv - vec2(0.5);
    float radius = length(delta);
    float angle = atan(delta.y, delta.x);

    // Direction vectors
    vec2 radialDir = normalize(delta);
    vec2 tangentDir = vec2(-radialDir.y, radialDir.x);

    // Radial displacement (concentric rings)
    float radialDisp = sin(radius * radialFreq + phase) * radialAmp;

    // Angular displacement (petals) with spiral twist
    float spiralPhase = phase + radius * spiralTwist;
    float tangentDisp = sin(angle * float(segments) + spiralPhase) * angularAmp * radius;

    return uv + radialDir * radialDisp + tangentDir * tangentDisp;
}
```

**UV transformation**: Input UV displaced by weighted radial + tangent vectors.

**Parameters**:
| Uniform | Type | Range | Purpose |
|---------|------|-------|---------|
| radialFreq | float | 1.0-30.0 | Ring density |
| radialAmp | float | 0.0-0.3 | Radial displacement strength |
| segments | int | 2-16 | Petal count (angular symmetry) |
| angularAmp | float | 0.0-0.5 | Angular displacement strength |
| phase | float | accumulated | Animation phase (CPU accumulated) |
| spiralTwist | float | -10.0-10.0 | Angular phase shift per radius |

**Rose curve behavior**: Odd segments produce N petals, even segments produce 2N petals.

## Phase 1: Config and Shader

**Goal**: Create config structure and working shader.

**Build**:
- `src/config/radial_pulse_config.h` - Config struct with fields: enabled, radialFreq, radialAmp, segments, angularAmp, phaseSpeed, spiralTwist. Use in-class defaults per CLAUDE.md.
- `shaders/radial_pulse.fs` - GLSL 330 fragment shader implementing displacement vector algorithm. Uniforms: radialFreq, radialAmp, segments, angularAmp, phase, spiralTwist.

**Done when**: Shader compiles with hardcoded test values.

---

## Phase 2: Integration

**Goal**: Wire shader into render pipeline.

**Build**:
- `src/config/effect_config.h`:
  - Add `#include "radial_pulse_config.h"`
  - Add `TRANSFORM_RADIAL_PULSE` to enum before `TRANSFORM_EFFECT_COUNT`
  - Add display name to `TransformEffectName()` switch
  - Add `RadialPulseConfig radialPulse;` field to EffectConfig
  - Add case to `IsTransformEnabled()`
  - Add to default TransformOrderConfig array

- `src/render/post_effect.h`:
  - Add `Shader radialPulseShader;`
  - Add uniform location ints: radialPulseRadialFreqLoc, radialPulseRadialAmpLoc, radialPulseSegmentsLoc, radialPulseAngularAmpLoc, radialPulsePhaseLoc, radialPulseSpiralTwistLoc
  - Add `float radialPulseTime;` for CPU accumulation

- `src/render/post_effect.cpp`:
  - Add `LoadShader(0, "shaders/radial_pulse.fs")` call
  - Add to shader load validation
  - Add `GetShaderLocation` calls for all uniforms

- `src/render/shader_setup.h`:
  - Declare `void SetupRadialPulse(PostEffect* pe);`

- `src/render/shader_setup.cpp`:
  - Add case to `GetTransformEffect()` returning radialPulseShader, SetupRadialPulse, &effects.radialPulse.enabled
  - Implement `SetupRadialPulse()` - set all uniforms from config

- `src/render/render_pipeline.cpp`:
  - Add `pe->radialPulseTime += deltaTime * pe->effects.radialPulse.phaseSpeed;` in `RenderPipelineApplyFeedback()`

**Done when**: Effect renders with default config values.

---

## Phase 3: UI and Modulation

**Goal**: Add UI controls and audio-reactive parameters.

**Build**:
- `src/ui/imgui_effects_transforms.cpp`:
  - Add `static bool sectionRadialPulse = false;`
  - In `DrawSymmetryCategory()`, add Radial Pulse section after Poincare Disk
  - Controls: enabled checkbox, ModulatableSlider for radialFreq/radialAmp/angularAmp/spiralTwist, SliderInt for segments, SliderFloat for phaseSpeed

- `src/automation/param_registry.cpp`:
  - Add PARAM_TABLE entries: radialPulse.radialFreq, radialPulse.radialAmp, radialPulse.angularAmp, radialPulse.spiralTwist
  - Add targets array entries pointing to effects.radialPulse fields

**Done when**: All sliders appear in UI and modulation routes can target parameters.

---

## Phase 4: Serialization

**Goal**: Enable preset save/load.

**Build**:
- `src/config/preset.cpp`:
  - Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE` macro for RadialPulseConfig (or inline in header if pattern allows)
  - Add serialize line: `if (e.radialPulse.enabled) { j["radialPulse"] = e.radialPulse; }`
  - Add deserialize line: `e.radialPulse = j.value("radialPulse", e.radialPulse);`

**Done when**: Effect persists across preset save/load cycle.

---

## Implementation Notes

**Shader algorithm revised**: Added `petalAmp` uniform for radial petal modulation. Angular wave now serves dual purpose: scales radial displacement (petal shapes) and drives tangent swirl separately.

**Missing from plan**: `GetTransformCategory()` case in `src/ui/imgui_effects.cpp` - added to add-effect skill checklist as commonly missed step.
