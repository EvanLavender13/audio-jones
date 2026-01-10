# Glitch Effect

Simulates analog and digital video corruption through UV distortion, chromatic aberration, and overlay noise. Single transform effect with four toggleable sub-modes: CRT, Analog, Digital, VHS.

## Current State

Existing transform effect pattern to follow:
- `src/config/pixelation_config.h` - config struct template
- `src/config/effect_config.h:17-60` - enum and EffectConfig integration
- `shaders/pixelation.fs` - shader template
- `src/render/post_effect.h:33` - shader field pattern
- `src/render/shader_setup.cpp:29-30,259-266` - setup function pattern
- `src/ui/imgui_effects.cpp:608-620` - UI section pattern
- `src/config/preset.cpp:125-126` - JSON serialization
- `src/automation/param_registry.cpp:71-72` - modulation registration

## Technical Implementation

### Shader Algorithm

**Source**: `docs/research/glitch.md` (user-provided ShaderToy code + [VHS tape effect](https://www.shadertoy.com/view/Ms3XWH))

#### Gradient Noise (shared utility)

```glsl
// Hash by David_Hoskins
vec3 hash33(vec3 p) {
    uvec3 q = uvec3(ivec3(p)) * uvec3(1597334673U, 3812015801U, 2798796415U);
    q = (q.x ^ q.y ^ q.z) * uvec3(1597334673U, 3812015801U, 2798796415U);
    return -1.0 + 2.0 * vec3(q) * (1.0 / float(0xffffffffU));
}

// Gradient noise by iq, returns [-1, 1]
float gnoise(vec3 x) {
    vec3 p = floor(x);
    vec3 w = fract(x);
    vec3 u = w * w * w * (w * (w * 6.0 - 15.0) + 10.0);  // quintic interpolant

    vec3 ga = hash33(p + vec3(0,0,0)); vec3 gb = hash33(p + vec3(1,0,0));
    vec3 gc = hash33(p + vec3(0,1,0)); vec3 gd = hash33(p + vec3(1,1,0));
    vec3 ge = hash33(p + vec3(0,0,1)); vec3 gf = hash33(p + vec3(1,0,1));
    vec3 gg = hash33(p + vec3(0,1,1)); vec3 gh = hash33(p + vec3(1,1,1));

    float va = dot(ga, w - vec3(0,0,0)); float vb = dot(gb, w - vec3(1,0,0));
    float vc = dot(gc, w - vec3(0,1,0)); float vd = dot(gd, w - vec3(1,1,0));
    float ve = dot(ge, w - vec3(0,0,1)); float vf = dot(gf, w - vec3(1,0,1));
    float vg = dot(gg, w - vec3(0,1,1)); float vh = dot(gh, w - vec3(1,1,1));

    return 2.0 * (va + u.x*(vb-va) + u.y*(vc-va) + u.z*(ve-va) +
                  u.x*u.y*(va-vb-vc+vd) + u.y*u.z*(va-vc-ve+vg) +
                  u.z*u.x*(va-vb-ve+vf) + u.x*u.y*u.z*(-va+vb+vc-vd+ve-vf-vg+vh));
}

float gnoise01(vec3 x) { return 0.5 + 0.5 * gnoise(x); }
```

#### CRT Mode: Barrel Distortion

```glsl
vec2 crt(vec2 uv, float curvature) {
    vec2 centered = uv * 2.0 - 1.0;
    float r = length(centered);
    r /= (1.0 - curvature * r * r);
    float theta = atan(centered.y, centered.x);
    return vec2(r * cos(theta), r * sin(theta)) * 0.5 + 0.5;
}

// Vignette: 8 * u * v * (1-u) * (1-v), apply as pow(vig, 0.25) * 1.5
```

#### Analog Mode: Horizontal Noise Distortion

```glsl
float y = uv.y * resolution.y;
float t = time;

float distortion = gnoise(vec3(0.0, y * 0.01, t * 500.0)) * analogIntensity;
distortion *= gnoise(vec3(0.0, y * 0.02, t * 250.0)) * analogIntensity * 0.5;

// Sync pulse artifacts
distortion += smoothstep(0.999, 1.0, sin((uv.y + t * 1.6) * 2.0)) * 0.02;
distortion -= smoothstep(0.999, 1.0, sin((uv.y + t) * 2.0)) * 0.02;

vec2 st = uv + vec2(distortion, 0.0);
vec2 eps = vec2(aberration / resolution.x, 0.0);

// Chromatic aberration: R+offset, G center, B-offset
color.r = texture(input, st + eps + distortion).r;
color.g = texture(input, st).g;
color.b = texture(input, st - eps - distortion).b;
```

#### Digital Mode: Block Displacement

```glsl
float bt = floor(t * 30.0) * 300.0;  // quantized time

float blockX = step(gnoise01(vec3(0.0, uv.x * 3.0, bt)), blockThreshold);
float blockX2 = step(gnoise01(vec3(0.0, uv.x * 1.5, bt * 1.2)), blockThreshold);
float blockY = step(gnoise01(vec3(0.0, uv.y * 4.0, bt)), blockThreshold);
float blockY2 = step(gnoise01(vec3(0.0, uv.y * 6.0, bt * 1.2)), blockThreshold);
float block = blockX2 * blockY2 + blockX * blockY;

vec2 st = vec2(uv.x + sin(bt) * hash33(vec3(uv, 0.5)).x * block * blockOffset, uv.y);
color = mix(originalColor, glitchedColor, clamp(block, 0.0, 1.0));
```

#### VHS Mode: Tracking Bars + Scanline Noise

```glsl
float rand(vec2 co) {
    return fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453);
}

// Vertical tracking bar
float verticalBar(float pos, float uvY, float offset) {
    float range = 0.05;
    float x = smoothstep(pos - range, pos, uvY) * offset;
    x -= smoothstep(pos, pos + range, uvY) * offset;
    return x;
}

// Multiple traveling bars
for (float i = 0.0; i < 0.71; i += 0.1313) {
    float d = mod(time * i, 1.7);
    float o = sin(1.0 - tan(time * 0.24 * i)) * trackingBarIntensity;
    uv.x += verticalBar(d, uv.y, o);
}

// Per-scanline noise
float uvY = floor(uv.y * noiseQuality) / noiseQuality;
float noise = rand(vec2(time * 0.00001, uvY));
uv.x += noise * scanlineNoiseIntensity;

// Drifting chromatic aberration
vec2 offsetR = vec2(0.006 * sin(time), 0.0) * colorDriftIntensity;
vec2 offsetG = vec2(0.0073 * cos(time * 0.97), 0.0) * colorDriftIntensity;
color.r = texture(input, uv + offsetR).r;
color.g = texture(input, uv + offsetG).g;
color.b = texture(input, uv).b;
```

#### Overlay: Scanlines + White Noise

```glsl
// White noise
float noise = hash33(vec3(fragCoord, mod(frame, 1000.0))).r * noiseAmount;

// Scanlines
float scanline = sin(4.0 * t + uv.y * resolution.y * 1.75) * scanlineAmount;

color.rgb += noise;
color.rgb -= scanline;
```

### Uniforms

| Uniform | Type | Source |
|---------|------|--------|
| resolution | vec2 | Screen dimensions |
| time | float | Animation time (CPU accumulated) |
| frame | int | Frame counter for noise variation |
| crtEnabled | bool | Toggle CRT barrel |
| curvature | float | Barrel strength 0-0.2 |
| vignetteEnabled | bool | Toggle edge darkening |
| analogIntensity | float | Distortion amount 0-1 (0 = disabled) |
| aberration | float | Channel offset pixels 0-20 |
| blockThreshold | float | Block probability 0-0.9 (0 = disabled) |
| blockOffset | float | Max displacement 0-0.5 |
| vhsEnabled | bool | Toggle VHS artifacts |
| trackingBarIntensity | float | Bar strength 0-0.05 |
| scanlineNoiseIntensity | float | Jitter 0-0.02 |
| colorDriftIntensity | float | R/G drift 0-2.0 |
| scanlineAmount | float | Overlay darkness 0-0.5 |
| noiseAmount | float | White noise 0-0.3 |

---

## Phase 1: Config and Shader

**Goal**: Create glitch configuration struct and shader file.

**Build**:
- Create `src/config/glitch_config.h` with GlitchConfig struct containing:
  - `enabled` bool
  - CRT mode: `crtEnabled`, `curvature` (0-0.2), `vignetteEnabled`
  - Analog mode: `analogIntensity` (0-1, 0=disabled), `aberration` (0-20)
  - Digital mode: `blockThreshold` (0-0.9, 0=disabled), `blockOffset` (0-0.5)
  - VHS mode: `vhsEnabled`, `trackingBarIntensity` (0-0.05), `scanlineNoiseIntensity` (0-0.02), `colorDriftIntensity` (0-2.0)
  - Overlay: `scanlineAmount` (0-0.5), `noiseAmount` (0-0.3)
- Create `shaders/glitch.fs` implementing all modes from Technical Implementation section above
- Shader ordering: CRT barrel first (UV distort) → Analog/Digital/VHS (sample with distortion) → Overlay (additive)

**Done when**: Config struct compiles, shader file exists with all algorithms.

---

## Phase 2: Pipeline Integration

**Goal**: Load shader and wire into render pipeline.

**Build**:
- Edit `src/config/effect_config.h`:
  - Include `glitch_config.h`
  - Add `TRANSFORM_GLITCH` to `TransformEffectType` enum (after TRANSFORM_PIXELATION)
  - Add case to `TransformEffectName()`
  - Update `TransformOrderConfig::order` default initializer
  - Add `GlitchConfig glitch` field to `EffectConfig`
- Edit `src/render/post_effect.h`:
  - Add `Shader glitchShader`
  - Add uniform location ints: `glitchResolutionLoc`, `glitchTimeLoc`, `glitchFrameLoc`, plus all effect-specific uniform locs
- Edit `src/render/post_effect.cpp`:
  - Load shader in `LoadShaders()`
  - Cache uniform locations in `CacheUniformLocations()`
  - Set resolution uniform in `UpdateResolutionUniforms()`
  - Unload shader in `PostEffectUninit()`
- Edit `src/render/shader_setup.h`: Add `SetupGlitch()` declaration
- Edit `src/render/shader_setup.cpp`:
  - Add `TRANSFORM_GLITCH` case to `GetTransformEffect()`
  - Implement `SetupGlitch()` to bind all uniforms from GlitchConfig
  - Track time accumulation in PostEffect struct (add `float glitchTime`)

**Done when**: Shader loads without errors, effect appears in transform order list.

---

## Phase 3: UI and Serialization

**Goal**: Add Effects panel section and preset save/load.

**Build**:
- Edit `src/ui/imgui_effects.cpp`:
  - Add `static bool sectionGlitch = false` state
  - In TRANSFORMS group, add case for `TRANSFORM_GLITCH` in the effect order list enabled check
  - Under STYLE category (after Pixelation), add "Glitch" section with:
    - Enabled checkbox
    - Collapsible sub-sections for CRT, Analog, Digital, VHS modes
    - Each sub-section: mode checkbox + parameter sliders
    - Overlay sliders always visible when glitch enabled
  - Use `ModulatableSlider` for the 4 core intensity params (glitchAmount alias for master, analogIntensity, blockThreshold, aberration)
- Edit `src/config/preset.cpp`:
  - Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(GlitchConfig, ...)` macro
  - Add `j["glitch"] = e.glitch` in `to_json`
  - Add glitch parsing in `from_json`

**Done when**: Glitch settings appear in UI, save/load to presets correctly.

---

## Phase 4: Modulation Registration

**Goal**: Enable audio-reactive control of glitch parameters.

**Build**:
- Edit `src/automation/param_registry.cpp`:
  - Add entries to `PARAM_TABLE`:
    - `{"glitch.analogIntensity", {0.0f, 1.0f}}`
    - `{"glitch.blockThreshold", {0.0f, 0.9f}}`
    - `{"glitch.aberration", {0.0f, 20.0f}}`
    - `{"glitch.blockOffset", {0.0f, 0.5f}}`
  - Add corresponding pointers in `ParamRegistryInit()` targets array

**Done when**: Glitch parameters appear in modulation route dropdowns, audio modulation works.

---

## Post-Implementation Notes

### Bug Fixes Applied

1. **Effect stacking**: Original implementation used `else if` chain making Analog and Digital mutually exclusive. Fixed by having Analog sample first, then Digital modifies the result (multiplies existing color by `1-block`, adds displaced samples).

2. **Analog intensity formula**: Original formula `gnoise * analogIntensity * gnoise * analogIntensity * 0.5` produced negligible distortion at low values. Changed to reference-style scaling: `gnoise * (analogIntensity * 4.0 + 0.1)` for visible effect across the 0-1 range.

3. **Removed redundant toggles**: `analogEnabled` and `digitalEnabled` bools removed from config, shader, and UI. Analog now enables when `analogIntensity > 0`, Digital when `blockThreshold > 0`. Simplifies UI and reduces uniform count.

4. **Parameter bounds updated**: `analogIntensity` range changed from 0-0.1 to 0-1. `blockThreshold` range changed from 0.1-0.9 to 0-0.9 (allowing 0 = disabled).
