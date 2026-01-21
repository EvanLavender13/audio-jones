# Moiré Interference

Multi-sample UV transform that overlays rotated/scaled copies of the input texture with configurable blending to produce visible interference fringes. The moiré acts as a "beat frequency" in 2D space—small rotation or scale differences produce large-scale wave patterns.

## Current State

Transform effect system files to modify:
- `src/config/effect_config.h:49-93` - TransformEffectType enum, TransformOrderConfig
- `src/render/post_effect.h:17-409` - PostEffect struct with shaders and uniform locations
- `src/render/post_effect.cpp:96-154` - Shader loading and uniform location caching
- `src/render/shader_setup.cpp:1-103` - GetTransformEffect dispatch table
- `src/ui/imgui_effects_transforms.cpp:63-81` - DrawSymmetryKaleidoscope pattern to follow
- `src/config/preset.cpp:316-374` - JSON serialization
- `src/automation/param_registry.cpp:59-61` - Kaleidoscope params pattern to follow

Reference research: `docs/research/moire-interference.md`

## Technical Implementation

### Source
- Wikipedia Moiré pattern mathematics
- Observable Graphene Moiré fragment shader

### Core Algorithm

```glsl
// Rotation matrix
mat2 rotate2d(float angle) {
    float c = cos(angle);
    float s = sin(angle);
    return mat2(c, -s, s, c);
}

vec4 moireEffect(sampler2D tex, vec2 uv, float angle, float scale, int layers, int blendMode) {
    vec2 center = vec2(centerX, centerY);
    vec2 centered = uv - center;

    vec4 result = texture(tex, uv);

    for (int i = 1; i < layers; i++) {
        float layerAngle = angle * float(i);
        float layerScale = 1.0 + (scale - 1.0) * float(i);

        vec2 rotated = rotate2d(layerAngle) * centered;
        vec2 scaled = rotated * layerScale + center;

        // Mirror repeat for edge handling
        scaled = 1.0 - abs(mod(scaled, 2.0) - 1.0);

        vec4 sample = texture(tex, scaled);

        // Blend based on mode
        if (blendMode == 0) { // Multiply
            result *= sample;
        } else if (blendMode == 1) { // Min
            result = min(result, sample);
        } else if (blendMode == 2) { // Average
            result = (result + sample) / 2.0;
        } else { // Difference
            result = abs(result - sample);
        }
    }

    // Normalize multiply mode to prevent excessive darkening
    if (blendMode == 0) {
        result = pow(result, vec4(1.0 / float(layers)));
    }

    return result;
}
```

### Parameters

| Uniform | Type | Range | Default | Purpose |
|---------|------|-------|---------|---------|
| rotationAngle | float | 0 - 0.52 rad (0-30°) | 0.087 rad (5°) | Angle between layers; smaller = wider fringes |
| scaleDiff | float | 0.95 - 1.05 | 1.02 | Scale ratio between layers |
| layers | int | 2 - 4 | 2 | Number of overlaid samples |
| blendMode | int | 0-3 | 0 | 0=multiply, 1=min, 2=average, 3=difference |
| centerX | float | 0 - 1 | 0.5 | Rotation/scale center X |
| centerY | float | 0 - 1 | 0.5 | Rotation/scale center Y |
| animationSpeed | float | 0 - ROTATION_SPEED_MAX | 0.017 rad/s (1°/s) | Animation rate, CPU accumulated |
| rotationAccum | float | - | - | CPU-accumulated rotation passed to shader |

### Modulation Candidates

Register in param_registry.cpp:
- `moireInterference.rotationAngle` - {0.0f, 0.52f} - Flowing fringes
- `moireInterference.scaleDiff` - {0.95f, 1.05f} - Breathing/pulsing effect
- `moireInterference.animationSpeed` - {-ROTATION_SPEED_MAX, ROTATION_SPEED_MAX}

---

## Phase 1: Config and Effect Registration

**Goal**: Create config struct and register transform effect type.

**Build**:
- Create `src/config/moire_interference_config.h` with MoireInterferenceConfig struct:
  - `bool enabled`
  - `float rotationAngle` (radians, default 0.087)
  - `float scaleDiff` (default 1.02)
  - `int layers` (default 2)
  - `int blendMode` (default 0)
  - `float centerX`, `float centerY` (default 0.5)
  - `float animationSpeed` (radians/second, default 0.017)
- Modify `src/config/effect_config.h`:
  - Add include for moire_interference_config.h
  - Add `TRANSFORM_MOIRE_INTERFERENCE` to enum
  - Add case in `TransformEffectName()` returning "Moiré Interference"
  - Add to `TransformOrderConfig::order` array
  - Add `MoireInterferenceConfig moireInterference` member to EffectConfig
  - Add case in `IsTransformEnabled()`

**Done when**: Project compiles with new config types defined.

---

## Phase 2: Shader Implementation

**Goal**: Create the fragment shader with multi-sample UV transform and blend modes.

**Build**:
- Create `shaders/moire_interference.fs`:
  - GLSL 330 with standard raylib input/output
  - Uniforms: rotationAngle, scaleDiff, layers, blendMode, centerX, centerY, rotationAccum
  - rotate2d() helper function
  - Main loop: iterate layers 1 to layers-1, apply rotation and scale
  - Mirror repeat UV: `uv = 1.0 - abs(mod(uv, 2.0) - 1.0)`
  - Blend mode switch with multiply/min/average/difference
  - Normalize multiply result with pow(result, 1/layers)

**Done when**: Shader file exists with complete algorithm from research doc.

---

## Phase 3: PostEffect Integration

**Goal**: Load shader and cache uniform locations.

**Build**:
- Modify `src/render/post_effect.h`:
  - Add `Shader moireInterferenceShader` member
  - Add uniform location ints: `moireInterferenceRotationAngleLoc`, `moireInterferenceScaleDiffLoc`, `moireInterferenceLayersLoc`, `moireInterferenceBlendModeLoc`, `moireInterferenceCenterXLoc`, `moireInterferenceCenterYLoc`, `moireInterferenceRotationAccumLoc`
  - Add `float moireInterferenceRotationAccum` for CPU accumulation
- Modify `src/render/post_effect.cpp`:
  - Load shader in `LoadPostEffectShaders()`
  - Add to success check
  - Get uniform locations in `GetShaderUniformLocations()`
  - Unload in `PostEffectUninit()`

**Done when**: Shader loads at startup and uniform locations are cached.

---

## Phase 4: Shader Setup and Dispatch

**Goal**: Wire shader into transform pipeline with CPU rotation accumulation.

**Build**:
- Modify `src/render/shader_setup.h`:
  - Declare `void SetupMoireInterference(PostEffect* pe)`
- Modify `src/render/shader_setup.cpp`:
  - Add case in `GetTransformEffect()` for TRANSFORM_MOIRE_INTERFERENCE
  - Implement `SetupMoireInterference()`:
    - Read config from `pe->effects.moireInterference`
    - Accumulate rotation: `pe->moireInterferenceRotationAccum += cfg->animationSpeed * pe->currentDeltaTime`
    - Set all uniforms including rotationAccum

**Done when**: Effect appears in transform pipeline when enabled.

---

## Phase 5: UI Panel

**Goal**: Add Moiré Interference controls to Symmetry category.

**Build**:
- Modify `src/ui/imgui_effects.cpp`:
  - Add case in `GetTransformCategory()` for TRANSFORM_MOIRE_INTERFERENCE returning category index for Symmetry
- Modify `src/ui/imgui_effects_transforms.cpp`:
  - Add `static bool sectionMoireInterference = false` at top
  - Add `DrawSymmetryMoireInterference()` helper function:
    - DrawSectionBegin/End pattern
    - Enabled checkbox with MoveTransformToEnd
    - ModulatableSliderAngleDeg for rotationAngle ("Rotation", "°")
    - ModulatableSlider for scaleDiff ("Scale Diff", "%.3f")
    - ImGui::SliderInt for layers (2-4)
    - ImGui::Combo for blendMode ("Multiply\0Min\0Average\0Difference\0")
    - ModulatableSliderAngleDeg for animationSpeed ("Spin", "°/s")
    - TreeNodeAccented "Center" with SliderFloat for centerX/centerY
  - Add call in `DrawSymmetryCategory()` after KIFS

**Done when**: UI controls appear under Symmetry and modify effect in real-time.

---

## Phase 6: Preset Serialization

**Goal**: Save and load moiré settings in presets.

**Build**:
- Modify `src/config/preset.cpp`:
  - Add NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT for MoireInterferenceConfig with all fields
  - Add to_json entry for moireInterference
  - Add from_json entry for moireInterference

**Done when**: Preset save/load preserves moiré settings.

---

## Phase 7: Parameter Registration

**Goal**: Enable modulation routing to moiré parameters.

**Build**:
- Modify `src/automation/param_registry.cpp`:
  - Add PARAM_TABLE entries:
    - `{"moireInterference.rotationAngle", {0.0f, 0.52f}}`
    - `{"moireInterference.scaleDiff", {0.95f, 1.05f}}`
    - `{"moireInterference.animationSpeed", {-ROTATION_SPEED_MAX, ROTATION_SPEED_MAX}}`
  - Add matching entries to targets[] array:
    - `&effects->moireInterference.rotationAngle`
    - `&effects->moireInterference.scaleDiff`
    - `&effects->moireInterference.animationSpeed`

**Done when**: LFOs and audio bands can modulate moiré parameters.

---

## Phase 8: Verification

**Goal**: Test all integration points.

**Verify**:
- [ ] Effect appears in transform order pipeline
- [ ] Effect shows "Symmetry" category badge (not "???")
- [ ] Effect can be reordered via drag-drop
- [ ] Enabling effect adds it to pipeline list
- [ ] All 4 blend modes produce distinct results
- [ ] Animation rotates interference pattern smoothly
- [ ] Preset save/load preserves all settings
- [ ] Modulation routes to registered parameters
- [ ] Build succeeds with no warnings

**Done when**: All verification items pass.
