# Tunnel Effect

2D post-processing tunnel effect that transforms the visualizer output into a classic demoscene-style infinite tunnel with configurable winding path, spiral twist, and multi-depth accumulation.

## Current State

Files to modify/create:
- `src/config/effect_config.h:15-52` - TransformEffectType enum, TransformOrderConfig, EffectConfig struct
- `src/render/post_effect.h:13-131` - PostEffect struct (shader, uniforms, time state)
- `src/render/post_effect.cpp:25-131` - Shader loading, uniform locations
- `src/render/render_pipeline.cpp:87-115` - GetTransformEffect switch, Setup function
- `src/ui/imgui_effects.cpp:10-23,39-99` - Section state, effect order switch, UI panel
- `src/config/preset.cpp:110-176` - Serialization macros and EffectConfig to_json/from_json
- `src/automation/param_registry.cpp:38-90` - Modulatable parameter registration

New files:
- `src/config/tunnel_config.h` - TunnelConfig struct
- `shaders/tunnel.fs` - Fragment shader

## Technical Implementation

**Source**: [docs/research/heavy-texture-transforms.md](../research/heavy-texture-transforms.md) - Technique 7: Tunnel Effect

### Core Algorithm

Polar coordinate remapping with multi-depth accumulation:

```glsl
// UV to polar
vec2 centered = fragTexCoord - 0.5 - focalOffset;
float dist = length(centered);
float angle = atan(centered.y, centered.x);

// Texture coordinates from polar transform
// 1/dist creates depth illusion (closer to center = further away)
float texY = 1.0 / (dist + 0.1) + time * speed;
float texX = angle / TAU + time * rotationSpeed;

// Depth-dependent spiral twist
texX += texY * twist;

// Lissajous winding offset
texX += windingAmplitude * sin(texY * windingFreqX + time);
texY += windingAmplitude * cos(texY * windingFreqY + time);
```

### Multi-Depth Accumulation

Sample at multiple depth offsets with weighted blending (matches KIFS/turbulence pattern):

```glsl
vec3 colorAccum = vec3(0.0);
float weightAccum = 0.0;

for (int i = 0; i < layers; i++) {
    float depthOffset = float(i) * depthSpacing;

    float texY = 1.0 / (dist + 0.1) + time * speed + depthOffset;
    float texX = angle / TAU + time * rotationSpeed + texY * twist;

    // Lissajous winding
    texX += windingAmplitude * sin(texY * windingFreqX + time);

    vec2 tunnelUV = vec2(texX, texY);

    // Depth-based weight (further layers contribute less)
    float weight = 1.0 / float(i + 1);
    colorAccum += texture(texture0, fract(tunnelUV)).rgb * weight;
    weightAccum += weight;
}

finalColor = vec4(colorAccum / weightAccum, 1.0);
```

### Parameters

| Parameter | Type | Range | Default | Description |
|-----------|------|-------|---------|-------------|
| enabled | bool | - | false | Enable effect |
| speed | float | -2.0 to 2.0 | 0.5 | Depth motion speed (negative = reverse) |
| rotationSpeed | float | -π to π | 0.0 | Angular rotation (radians/sec) |
| twist | float | -π to π | 0.0 | Spiral twist per depth unit (radians) |
| layers | int | 1 to 8 | 4 | Accumulation depth samples |
| windingAmplitude | float | 0.0 to 0.5 | 0.0 | Lissajous path winding intensity |
| windingFreqX | float | 0.1 to 5.0 | 1.0 | Winding X frequency |
| windingFreqY | float | 0.1 to 5.0 | 1.3 | Winding Y frequency |
| focalAmplitude | float | 0.0 to 0.2 | 0.0 | Center drift amount |
| focalFreqX | float | 0.1 to 5.0 | 0.7 | Focal X frequency |
| focalFreqY | float | 0.1 to 5.0 | 1.1 | Focal Y frequency |
| animSpeed | float | 0.0 to 2.0 | 1.0 | Global time multiplier |

### Modulatable Parameters

Register for audio reactivity:
- `tunnel.speed` - bass drives depth motion
- `tunnel.rotationSpeed` - mids drive rotation
- `tunnel.windingAmplitude` - treble drives winding intensity
- `tunnel.twist` - alternative modulation target

---

## Phase 1: Config and Shader

**Goal**: Create the tunnel config struct and GLSL shader.

**Build**:
- Create `src/config/tunnel_config.h` with TunnelConfig struct (all parameters from table above)
- Create `shaders/tunnel.fs` implementing multi-depth accumulation algorithm
- Add `#include "tunnel_config.h"` to `effect_config.h`
- Add `TRANSFORM_TUNNEL` to TransformEffectType enum (before TRANSFORM_EFFECT_COUNT)
- Add case to `TransformEffectName()` returning "Tunnel"
- Add `TRANSFORM_TUNNEL` to `TransformOrderConfig::order[]` default
- Add `TunnelConfig tunnel;` member to EffectConfig struct

**Done when**: Project compiles with new config. Shader file exists (not yet loaded).

---

## Phase 2: PostEffect Integration

**Goal**: Load shader and wire up uniforms.

**Build**:
- In `post_effect.h`:
  - Add `Shader tunnelShader;`
  - Add uniform locations: `tunnelTimeLoc`, `tunnelSpeedLoc`, `tunnelRotationSpeedLoc`, `tunnelTwistLoc`, `tunnelLayersLoc`, `tunnelWindingAmplitudeLoc`, `tunnelWindingFreqXLoc`, `tunnelWindingFreqYLoc`, `tunnelFocalLoc`
  - Add `float tunnelTime;` state
  - Add `float tunnelFocal[2];` for computed Lissajous offset
- In `post_effect.cpp`:
  - Load shader in `LoadPostEffectShaders()`
  - Add to shader validation check
  - Get all uniform locations in `GetShaderUniformLocations()`
  - Initialize `tunnelTime = 0.0f` in `PostEffectInit()`
  - Unload shader in `PostEffectUninit()`

**Done when**: Shader loads without errors. Uniform locations retrieved.

---

## Phase 3: Render Pipeline

**Goal**: Execute tunnel shader in transform effect chain.

**Build**:
- In `render_pipeline.cpp`:
  - Add `static void SetupTunnel(PostEffect* pe);` forward declaration
  - Implement `SetupTunnel()` to set all uniforms from TunnelConfig
  - Add `TRANSFORM_TUNNEL` case to `GetTransformEffect()` switch
  - Add `pe->tunnelTime += deltaTime * pe->effects.tunnel.animSpeed;` in `RenderPipelineApplyFeedback()`
  - Compute `tunnelFocal` Lissajous in `RenderPipelineApplyOutput()` (like kaleidoscope focal)

**Done when**: Enabling tunnel in code produces visual effect.

---

## Phase 4: UI Panel

**Goal**: Add ImGui controls for tunnel parameters.

**Build**:
- In `imgui_effects.cpp`:
  - Add `static bool sectionTunnel = false;`
  - Add `TRANSFORM_TUNNEL` case to effect order enabled-check switch
  - Add Tunnel section with:
    - Checkbox for enabled
    - SliderFloat for speed (-2.0 to 2.0)
    - ModulatableSliderAngleDeg for rotationSpeed
    - ModulatableSliderAngleDeg for twist
    - SliderInt for layers (1-8)
    - ModulatableSlider for windingAmplitude
    - SliderFloat for windingFreqX, windingFreqY
    - SliderFloat for focalAmplitude, focalFreqX, focalFreqY
    - SliderFloat for animSpeed

**Done when**: UI panel appears, sliders adjust config values in real-time.

---

## Phase 5: Serialization and Modulation

**Goal**: Save/load presets and enable audio reactivity.

**Build**:
- In `preset.cpp`:
  - Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(TunnelConfig, ...)` macro with all fields
  - Add `if (e.tunnel.enabled) { j["tunnel"] = e.tunnel; }` in EffectConfig to_json
  - Add `e.tunnel = j.value("tunnel", e.tunnel);` in EffectConfig from_json
- In `param_registry.cpp`:
  - Add param definitions to PARAM_DEFS array:
    - `{"tunnel.speed", {-2.0f, 2.0f}}`
    - `{"tunnel.rotationSpeed", {-ROTATION_SPEED_MAX, ROTATION_SPEED_MAX}}`
    - `{"tunnel.windingAmplitude", {0.0f, 0.5f}}`
    - `{"tunnel.twist", {-ROTATION_OFFSET_MAX, ROTATION_OFFSET_MAX}}`
  - Add pointer mappings in `ParamRegistryInit()`

**Done when**: Presets save/load tunnel config. Modulation routes work for registered params.
