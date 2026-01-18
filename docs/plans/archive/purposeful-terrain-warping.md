# Purposeful Terrain Warping

Two enhancements for controllable terrain-like effects: directional bias for existing Texture Warp, and a new Domain Warp effect based on Inigo Quilez's nested fbm technique. Both pair with Heightfield Relief for pseudo-3D landscapes.

## Current State

- `shaders/texture_warp.fs:1-61` - Iterative UV displacement using texture color as offset direction
- `src/config/texture_warp_config.h:14-19` - Config with `enabled`, `strength`, `iterations`, `channelMode`
- `src/automation/param_registry.cpp:73,248` - Only `textureWarp.strength` registered for modulation

## Research Source

- `docs/research/purposeful-terrain-warping.md` - Algorithm details, audio mapping ideas
- [Inigo Quilez - Domain Warping](https://iquilezles.org/articles/warp/) - Nested fbm technique

---

## Technical Implementation

### Noise Functions (shared by both effects)

```glsl
float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

float noise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);  // Smoothstep interpolation

    float a = hash(i);
    float b = hash(i + vec2(1.0, 0.0));
    float c = hash(i + vec2(0.0, 1.0));
    float d = hash(i + vec2(1.0, 1.0));

    return mix(mix(a, b, f.x), mix(c, d, f.x), f.y);
}

float fbm(vec2 p) {
    float value = 0.0;
    float amplitude = 0.5;
    for (int i = 0; i < 5; i++) {
        value += amplitude * noise(p);
        p *= 2.0;
        amplitude *= 0.5;
    }
    return value;
}
```

### Approach A: Directional Texture Warp Enhancement

Add directional anisotropy after computing the offset vector:

```glsl
// New uniforms
uniform float ridgeAngle;    // Direction of ridges (radians)
uniform float anisotropy;    // 0.0 = isotropic, 1.0 = fully directional
uniform float noiseAmount;   // Procedural noise blend (0.0 to 1.0)
uniform float noiseScale;    // Frequency of injected noise

// After computing offset from channel mode:
vec2 ridgeDir = vec2(cos(ridgeAngle), sin(ridgeAngle));
vec2 perpDir = vec2(-ridgeDir.y, ridgeDir.x);

// Decompose into ridge-parallel and perpendicular components
float ridgeComponent = dot(offset, ridgeDir);
float perpComponent = dot(offset, perpDir);

// Suppress perpendicular component by anisotropy factor
perpComponent *= (1.0 - anisotropy);

// Reconstruct biased offset
offset = ridgeDir * ridgeComponent + perpDir * perpComponent;

// Inject procedural noise for uniform inputs
if (noiseAmount > 0.0) {
    float n = fbm(warpedUV * noiseScale);
    vec2 noiseOffset = vec2(cos(n * 6.28318), sin(n * 6.28318));
    offset = mix(offset, noiseOffset, noiseAmount);
}
```

### Approach B: Domain Warp Effect

Nested fbm-based UV displacement (Inigo Quilez technique):

```glsl
uniform sampler2D texture0;
uniform float warpStrength;   // Overall displacement magnitude (0.0 to 0.5)
uniform float warpScale;      // Noise frequency (1.0 to 10.0)
uniform int warpIterations;   // Nesting depth (1 to 3)
uniform float falloff;        // Amplitude reduction per iteration (0.3 to 0.8)
uniform vec2 timeOffset;      // Animated drift (accumulated on CPU)

vec2 fbm2(vec2 p) {
    return vec2(
        fbm(p + vec2(0.0, 0.0)),
        fbm(p + vec2(5.2, 1.3))
    );
}

void main() {
    vec2 uv = fragTexCoord;
    vec2 p = uv * warpScale + timeOffset;

    float amp = warpStrength;
    for (int i = 0; i < warpIterations; i++) {
        vec2 displacement = fbm2(p);
        uv += displacement * amp;
        p = uv * warpScale + timeOffset + vec2(1.7 * float(i), 9.2 * float(i));
        amp *= falloff;
    }

    finalColor = texture(texture0, clamp(uv, 0.0, 1.0));
}
```

---

## Phase 1: Directional Texture Warp Enhancement

**Goal**: Add ridge direction, anisotropy, and noise injection to existing Texture Warp.

### Config Changes

Modify `src/config/texture_warp_config.h`:
- Add `ridgeAngle` (float, default 0.0) - Ridge direction in radians
- Add `anisotropy` (float, default 0.0) - 0=isotropic, 1=fully directional
- Add `noiseAmount` (float, default 0.0) - Procedural noise blend
- Add `noiseScale` (float, default 5.0) - Noise frequency

### Shader Changes

Modify `shaders/texture_warp.fs`:
- Add 4 new uniforms
- Add hash/noise/fbm functions
- Insert directional bias logic after offset computation (line 54)
- Insert noise injection before applying offset (line 56)

### PostEffect Integration

Modify `src/render/post_effect.h`:
- Add 4 uniform location ints: `textureWarpRidgeAngleLoc`, `textureWarpAnisotropyLoc`, `textureWarpNoiseAmountLoc`, `textureWarpNoiseScaleLoc`

Modify `src/render/post_effect.cpp`:
- Get 4 new uniform locations in `GetShaderUniformLocations()`

Modify `src/render/shader_setup.cpp`:
- Pass 4 new uniforms in `SetupTextureWarp()`

### UI Changes

Modify `src/ui/imgui_effects_transforms.cpp`:
- Add `ModulatableSliderAngleDeg` for Ridge Angle
- Add `ModulatableSlider` for Anisotropy (0.0-1.0)
- Add `ModulatableSlider` for Noise Amount (0.0-1.0)
- Add `ImGui::SliderFloat` for Noise Scale (1.0-20.0)

### Preset Serialization

Modify `src/config/preset.cpp`:
- Update `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT` macro with new fields

### Parameter Registration

Modify `src/automation/param_registry.cpp`:
- Add `textureWarp.ridgeAngle` with `{-ROTATION_OFFSET_MAX, ROTATION_OFFSET_MAX}`
- Add `textureWarp.anisotropy` with `{0.0f, 1.0f}`
- Add `textureWarp.noiseAmount` with `{0.0f, 1.0f}`
- Add matching entries to targets array

**Done when**: Texture Warp in Polar mode with high anisotropy creates visible ridge lines; noise injection adds terrain features to uniform color inputs.

---

## Phase 2: Domain Warp Effect

**Goal**: Add new Domain Warp effect following add-effect checklist.

### Config Header

Create `src/config/domain_warp_config.h`:
```cpp
struct DomainWarpConfig {
    bool enabled = false;
    float warpStrength = 0.1f;   // 0.0 to 0.5
    float warpScale = 4.0f;      // 1.0 to 10.0
    int warpIterations = 2;      // 1 to 3
    float falloff = 0.5f;        // 0.3 to 0.8
    float driftSpeed = 0.0f;     // radians/frame, CPU accumulated
    float driftAngle = 0.0f;     // direction of drift (radians)
};
```

### Effect Registration

Modify `src/config/effect_config.h`:
- Include `domain_warp_config.h`
- Add `TRANSFORM_DOMAIN_WARP` to enum
- Add name case returning "Domain Warp"
- Add to `TransformOrderConfig::order` array
- Add `DomainWarpConfig domainWarp` member
- Add `IsTransformEnabled` case

### Shader

Create `shaders/domain_warp.fs`:
- Implement fbm2-based UV displacement as specified in Technical Implementation
- Uniforms: `warpStrength`, `warpScale`, `warpIterations`, `falloff`, `timeOffset`

### PostEffect Integration

Modify `src/render/post_effect.h`:
- Add `Shader domainWarpShader`
- Add uniform locations: `domainWarpStrengthLoc`, `domainWarpScaleLoc`, `domainWarpIterationsLoc`, `domainWarpFalloffLoc`, `domainWarpTimeOffsetLoc`
- Add `float currentDomainWarpDrift` for animation state

Modify `src/render/post_effect.cpp`:
- Load shader in `LoadPostEffectShaders()`
- Add to success check
- Get uniform locations
- Unload in `PostEffectUninit()`

Modify `src/render/render_pipeline.cpp`:
- Accumulate `pe->currentDomainWarpDrift += pe->effects.domainWarp.driftSpeed`

### Shader Setup

Modify `src/render/shader_setup.h`:
- Declare `void SetupDomainWarp(PostEffect* pe)`

Modify `src/render/shader_setup.cpp`:
- Add dispatch case for `TRANSFORM_DOMAIN_WARP`
- Implement `SetupDomainWarp()`:
  - Compute timeOffset as `vec2(cos(driftAngle), sin(driftAngle)) * currentDomainWarpDrift`
  - Pass all uniforms

### UI Panel

Modify `src/ui/imgui_effects.cpp`:
- Add `TRANSFORM_DOMAIN_WARP` case to `GetTransformCategory()` returning `CATEGORY_WARP`

Modify `src/ui/imgui_effects_transforms.cpp`:
- Add `static bool sectionDomainWarp = false`
- Add `DrawWarpDomainWarp()` helper function with:
  - Enable checkbox
  - `ModulatableSlider` for Strength (0.0-0.5)
  - `ImGui::SliderFloat` for Scale (1.0-10.0)
  - `ImGui::SliderInt` for Iterations (1-3)
  - `ModulatableSlider` for Falloff (0.3-0.8)
  - `ModulatableSliderAngleDeg` for Drift Speed
  - `ModulatableSliderAngleDeg` for Drift Angle
- Call from `DrawWarpCategory()`

### Preset Serialization

Modify `src/config/preset.cpp`:
- Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT` for `DomainWarpConfig`
- Add `to_json` entry
- Add `from_json` entry

### Parameter Registration

Modify `src/automation/param_registry.cpp`:
- Add entries to PARAM_TABLE:
  - `domainWarp.warpStrength` with `{0.0f, 0.5f}`
  - `domainWarp.falloff` with `{0.3f, 0.8f}`
  - `domainWarp.driftSpeed` with `{-ROTATION_SPEED_MAX, ROTATION_SPEED_MAX}`
  - `domainWarp.driftAngle` with `{-ROTATION_OFFSET_MAX, ROTATION_OFFSET_MAX}`
- Add matching entries to targets array at same indices

**Done when**: Domain Warp produces consistent organic terrain patterns on any input; drift animation creates flowing landscape effect; parameters modulate from audio sources.

---

## Phase 3: Validation

**Goal**: Verify both effects work correctly and interact well with Heightfield Relief.

### Testing Checklist

- [ ] Texture Warp: Ridge angle rotates warp direction visually
- [ ] Texture Warp: Anisotropy=1 creates clear ridge lines
- [ ] Texture Warp: Noise injection adds terrain features to solid color input
- [ ] Domain Warp: Appears in transform pipeline and can be reordered
- [ ] Domain Warp: Iterations=1 vs 3 shows increasing complexity
- [ ] Domain Warp: Drift animates smoothly without jumps when speed changes
- [ ] Both: Pair with Heightfield Relief for pseudo-3D terrain
- [ ] Both: Audio modulation routes work for registered parameters
- [ ] Both: Preset save/load preserves all settings
- [ ] Build succeeds with no warnings

**Done when**: All checklist items pass.
