# Density Wave Spiral

UV warp transform that creates logarithmic spiral arm structure through concentric, differentially-rotating ellipse rings with progressive tilt. Visual approximation of Lin-Shu density wave theory.

## Current State

Transform effect system files to modify:
- `src/config/effect_config.h:48-423` - enum, order array, config member, IsTransformEnabled
- `src/render/post_effect.h:17-400` - shader and uniform location members
- `src/render/post_effect.cpp:65-110` - shader loading
- `src/render/shader_setup.h` - setup function declaration
- `src/render/shader_setup.cpp:14-102` - GetTransformEffect dispatch, setup implementation
- `src/ui/imgui_effects_transforms.cpp:564-573` - Motion category (DrawMotionCategory)
- `src/config/preset.cpp` - JSON serialization
- `src/automation/param_registry.cpp:12-199` - modulation parameter registration

## Technical Implementation

**Source**: [Shadertoy 4dcyzX](https://shadertoy.com/view/4dcyzX) by Fabrice Neyret, [dexyfex galaxy rendering](https://dexyfex.com/2016/09/09/galaxy-rendering-revisited/)

### Core Algorithm

Each ring at radius `l` receives tilt angle `tightness * l`. Inner rings rotate faster than outer (angular velocity ~ 1/l).

```glsl
// Main loop: accumulate ring contributions
vec4 result = vec4(0.0);
for (int i = 0; i < ringCount; i++) {
    float l = 0.1 + float(i) * (3.0 / float(ringCount));  // Radius 0.1 to 3.0
    float n = float(i) + 1.0;

    // 1. Apply tilt based on radius (creates spiral arms)
    float angle = tightness + tightness * l;
    float c = cos(angle), s = sin(angle);
    vec2 V = mat2(c, -s, s, c) * (uv - center);
    V /= aspect;  // Apply ellipse eccentricity

    // 2. Calculate ring mask via ellipse distance
    float d = dot(V, V);
    float ring = smoothstep(thickness, 0.0, abs(sqrt(d) - l));

    // 3. Sample texture with differential rotation
    float va = rotationAccum / l;  // Differential: inner faster
    float c2 = cos(va + n), s2 = sin(va + n);
    vec2 sampleUV = mat2(c2, -s2, s2, c2) * (0.5 * V / l);
    sampleUV = sampleUV * 0.5 + 0.5;  // Remap to [0,1]

    // Mirror repeat for smooth boundaries
    sampleUV = 1.0 - abs(mod(sampleUV, 2.0) - 1.0);

    vec4 texColor = texture(texture0, sampleUV);

    // 4. Accumulate with distance falloff
    result += ring * texColor / pow(l, falloff);
}
```

### Parameters

| Uniform | Type | Default | Range | Description |
|---------|------|---------|-------|-------------|
| center | vec2 | (0.0, 0.0) | [-0.5, 0.5] | Galaxy center offset from screen center |
| aspect | vec2 | (0.5, 0.3) | [0.1, 1.0] | Ellipse eccentricity (x < y = barred spiral) |
| tightness | float | -1.57 (-90°) | [-π, π] | Arm winding. Negative = trailing arms |
| rotationAccum | float | (CPU) | - | Accumulated rotation (CPU: accum += speed * dt) |
| thickness | float | 0.3 | [0.05, 0.5] | Ring edge softness |
| ringCount | int | 30 | [10, 50] | Quality/performance tradeoff |
| falloff | float | 1.0 | [0.5, 2.0] | Brightness decay with radius |

### CPU Time Accumulation

`rotationSpeed` is a **speed field** (radians/second). The CPU accumulates:
```cpp
pe->densityWaveSpiralRotation += config->rotationSpeed * pe->currentDeltaTime;
```

Pass the accumulated value to the shader as `rotationAccum`. This avoids animation jumps when speed changes mid-animation.

---

## Phase 1: Config and Registration

**Goal**: Define config struct and register in effect system.

**Build**:
- Create `src/config/density_wave_spiral_config.h` with DensityWaveSpiralConfig struct
- Modify `src/config/effect_config.h`:
  - Add include for density_wave_spiral_config.h
  - Add TRANSFORM_DENSITY_WAVE_SPIRAL to enum
  - Add "Density Wave Spiral" case to TransformEffectName()
  - Add to TransformOrderConfig::order array
  - Add DensityWaveSpiralConfig member to EffectConfig
  - Add case to IsTransformEnabled()

**Done when**: Config struct exists and TRANSFORM_DENSITY_WAVE_SPIRAL compiles.

---

## Phase 2: Shader

**Goal**: Implement the fragment shader.

**Build**:
- Create `shaders/density_wave_spiral.fs` implementing the ring-based differential rotation algorithm

**Shader uniforms**:
```glsl
uniform sampler2D texture0;
uniform vec2 center;      // Galaxy center offset
uniform vec2 aspect;      // Ellipse shape (x, y)
uniform float tightness;  // Arm winding (radians)
uniform float rotationAccum;  // CPU-accumulated rotation
uniform float thickness;  // Ring softness
uniform int ringCount;    // Number of rings
uniform float falloff;    // Brightness decay exponent
```

**Done when**: Shader file exists with complete algorithm.

---

## Phase 3: PostEffect Integration

**Goal**: Load shader and get uniform locations.

**Build**:
- Modify `src/render/post_effect.h`:
  - Add `Shader densityWaveSpiralShader`
  - Add uniform location ints for all parameters
  - Add `float densityWaveSpiralRotation` for CPU accumulation
- Modify `src/render/post_effect.cpp`:
  - Load shader in LoadPostEffectShaders()
  - Add to success check
  - Get uniform locations in GetShaderUniformLocations()
  - Unload shader in PostEffectUninit()

**Done when**: Shader loads without errors and uniform locations are retrieved.

---

## Phase 4: Shader Setup

**Goal**: Wire config values to shader uniforms.

**Build**:
- Modify `src/render/shader_setup.h`:
  - Declare SetupDensityWaveSpiral()
- Modify `src/render/shader_setup.cpp`:
  - Add TRANSFORM_DENSITY_WAVE_SPIRAL case in GetTransformEffect()
  - Implement SetupDensityWaveSpiral() - set all uniforms, accumulate rotation

**Done when**: Effect renders when enabled (may look wrong without UI tuning).

---

## Phase 5: UI Panel

**Goal**: Add controls in Motion category.

**Build**:
- Modify `src/ui/imgui_effects.cpp`:
  - Add TRANSFORM_DENSITY_WAVE_SPIRAL case to GetTransformCategory() returning CATEGORY_MOTION
- Modify `src/ui/imgui_effects_transforms.cpp`:
  - Add `static bool sectionDensityWaveSpiral = false`
  - Add DrawMotionDensityWaveSpiral() helper before DrawMotionCategory()
  - Call from DrawMotionCategory() with spacing

**UI Controls**:
- Checkbox: Enabled
- SliderFloat2: Center (x, y)
- SliderFloat2: Aspect (x, y)
- ModulatableSliderAngleDeg: Tightness (displays degrees, stores radians)
- ModulatableSliderAngleDeg: Rotation Speed (°/s)
- ModulatableSlider: Thickness
- SliderInt: Ring Count
- SliderFloat: Falloff

**Done when**: All controls appear in Motion category and modify config values.

---

## Phase 6: Preset Serialization

**Goal**: Save and load effect settings.

**Build**:
- Modify `src/config/preset.cpp`:
  - Add NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT macro for DensityWaveSpiralConfig
  - Add to_json entry (conditional on enabled)
  - Add from_json entry

**Done when**: Presets preserve density wave spiral settings across save/load.

---

## Phase 7: Parameter Registration

**Goal**: Enable modulation for core parameters.

**Build**:
- Modify `src/automation/param_registry.cpp`:
  - Add PARAM_TABLE entries:
    - `{"densityWaveSpiral.tightness", {-3.14159f, 3.14159f}}`
    - `{"densityWaveSpiral.rotationSpeed", {-ROTATION_SPEED_MAX, ROTATION_SPEED_MAX}}`
    - `{"densityWaveSpiral.thickness", {0.05f, 0.5f}}`
  - Add corresponding targets array entries

**Done when**: LFOs and audio sources can modulate tightness, rotationSpeed, and thickness.

---

## Phase 8: Verification

**Goal**: Confirm complete integration.

**Verify**:
- [ ] Effect appears in transform order pipeline list
- [ ] Shows "Motion" category badge (not "???")
- [ ] Drag-drop reordering works
- [ ] Enabling adds to pipeline, disabling removes
- [ ] UI controls affect visual output in real-time
- [ ] Preset save/load preserves all settings
- [ ] Modulation routes to tightness, rotationSpeed, thickness
- [ ] Build succeeds with no warnings
- [ ] Spiral arms form with default settings
- [ ] Negative tightness creates trailing arms, positive creates leading

**Done when**: All verification items pass.
