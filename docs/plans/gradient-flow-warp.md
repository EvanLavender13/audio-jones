# Gradient Flow Warp

Iteratively displaces UV coordinates perpendicular to luminance edges, creating streamline patterns (wood-grain, magnetic field aesthetics). Unlike Texture Warp (color-driven), this follows edge structure via Sobel gradient.

## Current State

Relevant files for pattern-following:
- `shaders/texture_warp.fs` - Iterative UV warp loop to replicate
- `shaders/toon.fs:72-89` - Sobel 3x3 sampling pattern
- `src/config/texture_warp_config.h` - Config struct pattern
- `src/render/shader_setup.cpp:220-230` - SetupTextureWarp pattern

## Technical Implementation

**Source**: [docs/research/gradient-flow-warp.md](../research/gradient-flow-warp.md)

### Core Algorithm

```glsl
// Per iteration: compute Sobel gradient at current UV
vec2 texel = 1.0 / resolution;

// Sample 3x3 neighborhood (luminance)
float tl = luminance(texture(texture0, uv + vec2(-texel.x, -texel.y)).rgb);
float t  = luminance(texture(texture0, uv + vec2(    0.0, -texel.y)).rgb);
float tr = luminance(texture(texture0, uv + vec2( texel.x, -texel.y)).rgb);
float l  = luminance(texture(texture0, uv + vec2(-texel.x,     0.0)).rgb);
float r  = luminance(texture(texture0, uv + vec2( texel.x,     0.0)).rgb);
float bl = luminance(texture(texture0, uv + vec2(-texel.x,  texel.y)).rgb);
float b  = luminance(texture(texture0, uv + vec2(    0.0,  texel.y)).rgb);
float br = luminance(texture(texture0, uv + vec2( texel.x,  texel.y)).rgb);

// Sobel gradients
float gx = (tr + 2.0*r + br) - (tl + 2.0*l + bl);
float gy = (tl + 2.0*t + tr) - (bl + 2.0*b + br);
vec2 gradient = vec2(gx, gy);
```

### Flow Direction with Configurable Angle

```glsl
// Perpendicular to gradient = tangent to edges
vec2 tangent = vec2(-gradient.y, gradient.x);

// flowAngle rotates between tangent (0) and gradient (PI/2)
float s = sin(flowAngle);
float c = cos(flowAngle);
vec2 flow = vec2(c * tangent.x + s * gradient.x,
                 c * tangent.y + s * gradient.y);

// Normalize to get direction only
float gradMag = length(gradient);
flow = gradMag > 0.001 ? normalize(flow) : vec2(0.0);
```

### Edge-Weighted Displacement

```glsl
// Base displacement
float displacement = strength;

// Optional: scale by edge strength (faster near edges, stagnant in flat areas)
if (edgeWeighted) {
    displacement *= gradMag;
}

warpedUV += flow * displacement;
```

### Full Iteration Loop

```glsl
vec2 warpedUV = fragTexCoord;

for (int i = 0; i < iterations; i++) {
    // Compute gradient at current position (9 texture samples)
    vec2 gradient = computeSobelGradient(warpedUV);

    // Compute flow direction with angle rotation
    vec2 flow = computeFlow(gradient, flowAngle);

    // Displace with optional edge weighting
    float displacement = edgeWeighted ? strength * length(gradient) : strength;
    warpedUV += flow * displacement;
}

finalColor = texture(texture0, clamp(warpedUV, 0.0, 1.0));
```

### Parameters

| Uniform | Type | Range | Default | Effect |
|---------|------|-------|---------|--------|
| resolution | vec2 | - | - | Screen dimensions for texel calculation |
| strength | float | 0.0-0.1 | 0.01 | Displacement per iteration |
| iterations | int | 1-32 | 8 | Cascade depth (9 samples each) |
| flowAngle | float | 0-PI | 0 | 0 = tangent (along edges), PI/2 = gradient (across edges) |
| edgeWeighted | bool | - | true | Scale displacement by gradient magnitude |

---

## Phase 1: Shader and Config

**Goal**: Working gradient flow shader with basic parameters.

**Build**:
- Create `src/config/gradient_flow_config.h` with GradientFlowConfig struct:
  - `bool enabled = false`
  - `float strength = 0.01f` (0.0-0.1)
  - `int iterations = 8` (1-32)
  - `float flowAngle = 0.0f` (0-PI, radians)
  - `bool edgeWeighted = true`
- Create `shaders/gradient_flow.fs`:
  - Uniforms: resolution, strength, iterations, flowAngle, edgeWeighted
  - Implement Sobel gradient computation (luminance-based)
  - Implement flow direction rotation by flowAngle
  - Implement iterative UV displacement loop
  - Optional edge-weighting per iteration
- Add `#include "gradient_flow_config.h"` to `effect_config.h`
- Add `TRANSFORM_GRADIENT_FLOW` to TransformEffectType enum
- Add `TransformEffectName` case returning "Gradient Flow"
- Add `GradientFlowConfig gradientFlow` member to EffectConfig
- Add to `TransformOrderConfig::order` default array

**Done when**: Shader compiles, config struct exists, enum registered.

---

## Phase 2: Pipeline Integration

**Goal**: Wire shader into render pipeline.

**Build**:
- Add to `post_effect.h`:
  - `Shader gradientFlowShader`
  - Uniform locations: `gradientFlowResolutionLoc`, `gradientFlowStrengthLoc`, `gradientFlowIterationsLoc`, `gradientFlowFlowAngleLoc`, `gradientFlowEdgeWeightedLoc`
- Add to `post_effect.cpp`:
  - Load shader in `LoadPostEffectShaders`
  - Get uniform locations in `GetShaderUniformLocations`
  - Add to resolution uniform setup in `SetResolutionUniforms`
  - Unload shader in `PostEffectUninit`
  - Add to shader load success check
- Add to `shader_setup.h`:
  - Declare `SetupGradientFlow(PostEffect* pe)`
- Add to `shader_setup.cpp`:
  - Add case in `GetTransformEffect` returning gradientFlowShader, SetupGradientFlow, enabled pointer
  - Implement `SetupGradientFlow`: set all uniforms from config

**Done when**: Effect appears in transform order, shader uniforms set correctly.

---

## Phase 3: UI and Serialization

**Goal**: User controls and preset persistence.

**Build**:
- Add to `imgui_effects.cpp`:
  - Add `static bool sectionGradientFlow = false`
  - Add UI section in Warp category (near Texture Warp):
    - Enabled checkbox
    - ModulatableSlider for strength (0.0-0.1, "%.3f")
    - SliderInt for iterations (1-32)
    - ModulatableSliderAngleDeg for flowAngle (0-PI range, displays degrees)
    - Checkbox for edgeWeighted
- Add to `preset.cpp`:
  - `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(GradientFlowConfig, enabled, strength, iterations, flowAngle, edgeWeighted)`
  - Add `if (e.gradientFlow.enabled) { j["gradientFlow"] = e.gradientFlow; }` in to_json
  - Add `e.gradientFlow = j.value("gradientFlow", e.gradientFlow);` in from_json
- Add to `param_registry.cpp`:
  - `{"gradientFlow.strength", {0.0f, 0.1f}}`
  - `{"gradientFlow.flowAngle", {0.0f, 3.14159f}}`
  - Add corresponding entries in targets array and PARAM_COUNT

**Done when**: UI controls work, presets save/load, parameters modulatable.

---

## Phase 4: Verification

**Goal**: Confirm correct behavior.

**Test scenarios**:
1. Enable effect, verify UV displacement visible
2. flowAngle=0: streamlines follow edges (wood-grain)
3. flowAngle=PI/2: flow crosses edges
4. edgeWeighted off: uniform flow everywhere
5. edgeWeighted on: faster flow near edges, stagnant in flat areas
6. Iterations slider: more = longer trails
7. Strength slider: higher = stronger displacement per step
8. Modulation: attach bass to strength, verify audio-reactive flow
9. Preset save/load: verify all parameters persist
10. Transform order: verify effect chains with other transforms

**Done when**: All scenarios pass.
