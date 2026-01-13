# Halftone Effect

CMYK halftone transform that simulates print halftoning by converting to CMYK, sampling each channel on a rotated grid, and rendering anti-aliased dots sized proportionally to ink density. Creates a newspaper/comic book aesthetic.

## Current State

- `docs/research/halftone.md` - Complete GLSL algorithm with CMYK conversion, screen angles, grid sampling
- `src/config/duotone_config.h` - Recent color effect config pattern
- `src/ui/imgui_effects.cpp:60-63` - Color category (COL, section 5) with ColorGrade and Duotone
- `src/ui/imgui_effects_transforms.cpp` - DrawColorCategory orchestrator

## Technical Implementation

### Source

- [libretro CMYK Halftone Dot](https://github.com/libretro/common-shaders/blob/master/misc/cmyk-halftone-dot.cg) - Public domain Cg shader
- [Shadertoy CMYK Halftone](https://www.shadertoy.com/view/Mdf3Dn) - Interactive reference

### RGB to CMYK Conversion

Inverted CMYK representation stores CMY as ratios, K as max channel:

```glsl
vec4 rgb2cmyki(vec3 c) {
    float k = max(max(c.r, c.g), c.b);
    return min(vec4(c.rgb / k, k), 1.0);
}

vec3 cmyki2rgb(vec4 c) {
    return c.rgb * c.a;
}
```

### Screen Angles

Standard printing angles prevent moire interference:
- Cyan: base + 15deg
- Magenta: base + 75deg
- Yellow: base + 0deg
- Black (K): base + 45deg

```glsl
mat2 rotm(float r) {
    float cr = cos(r);
    float sr = sin(r);
    return mat2(cr, -sr, sr, cr);
}
```

### Grid Sampling

For each CMYK channel, snap to rotated grid cell center and sample:

```glsl
vec2 grid(vec2 px, float S) {
    return px - mod(px, S);
}

vec4 halftone(vec2 fc, mat2 m, float S, float dotSize, sampler2D tex, vec2 origin, vec2 resolution) {
    vec2 smp = (grid(m * fc, S) + 0.5 * S) * m;  // Rotate, grid, unrotate
    float s = min(length(fc - smp) / (dotSize * 0.5 * S), 1.0);
    vec3 texc = texture(tex, (smp + origin) / resolution).rgb;
    texc = pow(texc, vec3(2.2));  // Gamma decode
    vec4 c = rgb2cmyki(texc);
    return c + s;  // Add distance to ink values
}
```

### Dot Generation with Smoothstep

Apply smoothstep for anti-aliased dot edges:

```glsl
vec4 ss(vec4 v, float threshold, float softness) {
    return smoothstep(threshold - softness, threshold + softness, v);
}

// Final combination
vec3 c = cmyki2rgb(ss(vec4(
    halftone(fc, mc, S, dotSize, tex, origin, res).r,
    halftone(fc, mm, S, dotSize, tex, origin, res).g,
    halftone(fc, my, S, dotSize, tex, origin, res).b,
    halftone(fc, mk, S, dotSize, tex, origin, res).a
), threshold, softness));

c = pow(c, vec3(1.0 / 2.2));  // Gamma encode
```

### Parameters

| Uniform | Type | Range | Default | Effect |
|---------|------|-------|---------|--------|
| dotScale | float | 2.0-20.0 | 8.0 | Grid cell size in pixels |
| dotSize | float | 0.5-2.0 | 1.0 | Dot radius multiplier within cell |
| rotation | float | 0-2pi | 0.0 | Combined rotationAngle + accumulated rotationSpeed |
| threshold | float | 0.5-1.0 | 0.888 | Smoothstep center for dot edge |
| softness | float | 0.1-0.5 | 0.288 | Smoothstep width; higher = softer edges |

### Rotation Handling

CPU accumulates rotationSpeed per frame. Shader receives combined value:

```cpp
// In SetupHalftone():
static float accumulatedRotation = 0.0f;
accumulatedRotation += cfg->rotationSpeed;
float rotation = accumulatedRotation + cfg->rotationAngle;
SetShaderValue(shader, rotationLoc, &rotation, SHADER_UNIFORM_FLOAT);
```

---

## Phase 1: Config Header

**Goal**: Create HalftoneConfig struct.

**Build**:
- `src/config/halftone_config.h` - HalftoneConfig with enabled, dotScale, dotSize, rotationSpeed, rotationAngle, threshold, softness

**Done when**: Header compiles.

---

## Phase 2: Effect Registration

**Goal**: Register halftone in effect system.

**Build** (`src/config/effect_config.h`):
- Add `#include "halftone_config.h"`
- Add `TRANSFORM_HALFTONE` to enum before COUNT
- Add "Halftone" case to `TransformEffectName()`
- Add `TRANSFORM_HALFTONE` to `TransformOrderConfig::order` array
- Add `HalftoneConfig halftone;` member to EffectConfig
- Add `TRANSFORM_HALFTONE` case to `IsTransformEnabled()`

**Done when**: Build succeeds with halftone in effect enum.

---

## Phase 3: Shader

**Goal**: Create CMYK halftone fragment shader.

**Build**:
- `shaders/halftone.fs` - Full CMYK halftone implementation with:
  - rgb2cmyki/cmyki2rgb conversion functions
  - rotm() rotation matrix helper
  - grid() snapping function
  - halftone() per-channel sampling
  - ss() smoothstep antialiasing
  - Four rotation matrices at standard angles (C:15, M:75, Y:0, K:45)
  - Gamma-correct color handling (decode before CMYK, encode after)

**Done when**: Shader parses without syntax errors.

---

## Phase 4: PostEffect Integration

**Goal**: Load shader and define uniform locations.

**Build** (`src/render/post_effect.h`):
- Add `Shader halftoneShader;`
- Add uniform locations: halftoneResolutionLoc, halftoneDotScaleLoc, halftoneDotSizeLoc, halftoneRotationLoc, halftoneThresholdLoc, halftoneSoftnessLoc

**Build** (`src/render/post_effect.cpp`):
- Load shader in `LoadPostEffectShaders()`
- Add to success check
- Get uniform locations in `GetShaderUniformLocations()`
- Add to resolution uniforms in `SetResolutionUniforms()`
- Unload in `PostEffectUninit()`

**Done when**: Shader loads and uniforms bind without warnings.

---

## Phase 5: Shader Setup

**Goal**: Wire uniforms and implement rotation accumulation.

**Build** (`src/render/shader_setup.h`):
- Declare `SetupHalftone()`

**Build** (`src/render/shader_setup.cpp`):
- Add dispatch case for `TRANSFORM_HALFTONE` in `GetTransformEffect()`
- Implement `SetupHalftone()` with static rotation accumulator, combines rotationSpeed + rotationAngle

**Done when**: Effect renders when enabled.

---

## Phase 6: UI Panel

**Goal**: Add halftone controls to Color category.

**Build** (`src/ui/imgui_effects.cpp`):
- Add `case TRANSFORM_HALFTONE:` returning `{"COL", 5}` in `GetTransformCategory()`

**Build** (`src/ui/imgui_effects_transforms.cpp`):
- Add `static bool sectionHalftone = false;`
- Create `DrawColorHalftone()` helper with:
  - Checkbox for enabled (with MoveTransformToEnd)
  - ModulatableSlider for dotScale (2.0-20.0, "%.1f px")
  - SliderFloat for dotSize (0.5-2.0, "%.2f")
  - ModulatableSliderAngleDeg for rotationSpeed (0 to ROTATION_SPEED_MAX, "%.2f deg/f")
  - SliderAngleDeg for rotationAngle (0 to ROTATION_OFFSET_MAX)
  - ModulatableSlider for threshold (0.5-1.0, "%.3f")
  - SliderFloat for softness (0.1-0.5, "%.3f")
- Add call in `DrawColorCategory()` after Duotone

**Done when**: Halftone section appears in Color category with all controls.

---

## Phase 7: Preset Serialization

**Goal**: Save/load halftone settings.

**Build** (`src/config/preset.cpp`):
- Add JSON macro for HalftoneConfig (enabled, dotScale, dotSize, rotationSpeed, rotationAngle, threshold, softness)
- Add to_json entry
- Add from_json entry

**Done when**: Preset save/load preserves all halftone settings.

---

## Phase 8: Parameter Registration

**Goal**: Enable modulation for dotScale and threshold.

**Build** (`src/automation/param_registry.cpp`):
- Add `{"halftone.dotScale", {2.0f, 20.0f}}` to PARAM_TABLE
- Add `{"halftone.threshold", {0.5f, 1.0f}}` to PARAM_TABLE
- Add `&effects->halftone.dotScale` to targets array at matching index
- Add `&effects->halftone.threshold` to targets array at matching index

**Done when**: dotScale and threshold respond to LFO/audio modulation.
