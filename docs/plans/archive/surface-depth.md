# Surface Depth

Treats the input texture's luminance as a heightfield and applies parallax displacement, normal-based lighting, and optional fresnel rim glow. Bright pixels become raised surfaces, dark pixels become recessed voids. The viewer's angle can be animated via Lissajous to create a shifting perspective. Absorbs and replaces the existing Heightfield Relief effect.

**Research**: `docs/research/surface_depth.md`

## Design

### Types

**SurfaceDepthConfig** (user-facing parameters):

```cpp
struct SurfaceDepthConfig {
  bool enabled = false;

  // Depth
  int depthMode = 1;            // 0=None, 1=Simple, 2=POM
  float heightScale = 0.15f;    // UV displacement magnitude (0.0-0.5)
  float heightPower = 1.0f;     // Luminance->height gamma (0.5-3.0)
  int steps = 15;               // Ray-march quality (8-64)

  // View
  float viewAngleX = 0.0f;      // Horizontal viewing tilt (-1.0 to 1.0)
  float viewAngleY = 0.0f;      // Vertical viewing tilt (-1.0 to 1.0)
  DualLissajousConfig viewLissajous = {.amplitude = 0.0f};

  // Lighting
  bool lighting = true;
  float lightAngle = 0.785f;    // Light direction in radians (0-2pi)
  float lightHeight = 0.5f;     // Light elevation (0.1-2.0)
  float intensity = 0.7f;       // Lighting blend strength (0.0-1.0)
  float reliefScale = 0.2f;     // Normal flatness, higher = subtler (0.02-1.0)
  float shininess = 32.0f;      // Specular exponent (1.0-128.0)

  // Fresnel
  bool fresnel = false;
  float fresnelExponent = 2.0f;   // Rim tightness (1.0-10.0)
  float fresnelIntensity = 2.0f;  // Rim glow brightness (0.0-3.0)
};
```

**SurfaceDepthEffect** (runtime state):

```cpp
typedef struct SurfaceDepthEffect {
  Shader shader;
  int resolutionLoc;
  int depthModeLoc;
  int heightScaleLoc;
  int heightPowerLoc;
  int stepsLoc;
  int viewAngleLoc;        // vec2 — computed on CPU from X/Y + lissajous
  int lightingLoc;         // int (bool as 0/1)
  int lightAngleLoc;
  int lightHeightLoc;
  int intensityLoc;
  int reliefScaleLoc;
  int shininessLoc;
  int fresnelEnabledLoc;   // int (bool as 0/1)
  int fresnelExponentLoc;
  int fresnelIntensityLoc;
  int timeLoc;             // float — for jitter temporal component
  float time;              // Animation accumulator
} SurfaceDepthEffect;
```

### Algorithm

The shader has six helper functions and a main function that dispatches on `depthMode`.

**Height interpretation** — converts texture luminance to height/depth:

```glsl
float getHeight(vec2 uv) {
    float lum = dot(texture(texture0, uv).rgb, vec3(0.299, 0.587, 0.114));
    return pow(lum, heightPower);
}

float getDepth(vec2 uv) {
    return 1.0 - getHeight(uv);
}
```

No sRGB linearization — render textures are already linear.

**Simple parallax** (Steep Parallax Mapping from LearnOpenGL) — ray-step with jitter, no interpolation. Steps linearly through depth layers, offsets UV by `viewDir.xy * heightScale`, breaks when accumulated depth exceeds sampled depth. This is POM without the final interpolation step — same algorithm family, cheaper. NOT the Xs2cDz zoom-to-center approach which is a fundamentally different algorithm:

```glsl
vec2 parallaxSimple(vec2 uv, vec3 viewDir) {
    float jitter = fract(sin(fract(time) + dot(uv, vec2(41.97, 289.13))) * 43758.5453);
    jitter *= min(0.25 + length(uv - 0.5), 1.0);  // reduce jitter near center

    vec2 p = viewDir.xy * heightScale;
    vec2 deltaUVs = p / float(steps);
    float layerDepth = 1.0 / float(steps);

    float d = layerDepth * jitter;
    uv -= deltaUVs * jitter;

    for (int i = 0; i < steps; i++) {
        float texDepth = getDepth(uv);
        if (d > texDepth) break;
        uv -= deltaUVs;
        d += layerDepth;
    }
    return uv;
}
```

**POM parallax** (from Shadertoy 3ds3zf, adapted) — layer stepping with linear interpolation between hit and previous layer for smoother displacement:

```glsl
vec2 parallaxPOM(vec2 uv, vec3 viewDir) {
    float numLayers = float(steps);
    float layerDepth = 1.0 / numLayers;

    vec2 p = viewDir.xy * heightScale;
    vec2 deltaUVs = p / numLayers;

    float texDepth = getDepth(uv);
    float d = 0.0;

    while (d < texDepth) {
        uv -= deltaUVs;
        texDepth = getDepth(uv);
        d += layerDepth;
    }

    // Linear interpolation between hit layer and previous layer
    vec2 lastUVs = uv + deltaUVs;
    float after = texDepth - d;
    float before = getDepth(lastUVs) - d + layerDepth;
    float w = after / (after - before);
    return mix(uv, lastUVs, w);
}
```

**Sobel normals** (from existing Heightfield Relief shader) — 9-tap 3x3 Sobel gradient, used for None mode to preserve existing quality:

```glsl
vec3 normalSobel(vec2 uv, vec2 texel) {
    vec4 n[9];
    n[0] = texture(texture0, uv + vec2(-texel.x, -texel.y));
    n[1] = texture(texture0, uv + vec2(    0.0, -texel.y));
    n[2] = texture(texture0, uv + vec2( texel.x, -texel.y));
    n[3] = texture(texture0, uv + vec2(-texel.x,     0.0));
    n[4] = texture(texture0, uv);
    n[5] = texture(texture0, uv + vec2( texel.x,     0.0));
    n[6] = texture(texture0, uv + vec2(-texel.x,  texel.y));
    n[7] = texture(texture0, uv + vec2(    0.0,  texel.y));
    n[8] = texture(texture0, uv + vec2( texel.x,  texel.y));

    vec4 sobelH = n[2] + 2.0*n[5] + n[8] - (n[0] + 2.0*n[3] + n[6]);
    vec4 sobelV = n[0] + 2.0*n[1] + n[2] - (n[6] + 2.0*n[7] + n[8]);

    float gx = dot(sobelH.rgb, vec3(0.299, 0.587, 0.114));
    float gy = dot(sobelV.rgb, vec3(0.299, 0.587, 0.114));

    return normalize(vec3(-gx, -gy, reliefScale));
}
```

**4-tap cross normals** (from Shadertoy 3ds3zf, adapted) — used for Simple/POM modes. Cheaper than Sobel and sufficient since the displaced UV already captures surface structure. Two intentional substitutions from the reference:
- `texelFetch(iChannel0, ...).r` → `getHeight(uv)`: uses luminance + heightPower instead of raw red channel, so normals respond to the same height interpretation as the parallax displacement
- Hardcoded `0.03` z-component → `reliefScale` uniform: exposes the normal flatness as a user-facing parameter (same role as in existing Heightfield Relief)

```glsl
vec3 normal4Tap(vec2 uv, vec2 texel) {
    float r = getHeight(uv + vec2(texel.x, 0.0));
    float t = getHeight(uv + vec2(0.0, texel.y));
    float l = getHeight(uv - vec2(texel.x, 0.0));
    float b = getHeight(uv - vec2(0.0, texel.y));

    float dx = l - r;
    float dy = b - t;

    return normalize(vec3(dx, dy, reliefScale));
}
```

**Fresnel** (from Shadertoy 3ds3zf variant) — rim glow tinted by texture color:

```glsl
float fresnelTerm(vec3 normal, vec3 view, float exponent) {
    return pow(1.0 - clamp(dot(normalize(normal), normalize(view)), 0.0, 1.0), exponent);
}
```

**Main function** — dispatches depth mode, then applies lighting and fresnel:

```glsl
void main() {
    vec2 uv = fragTexCoord;
    vec2 texel = 1.0 / resolution;

    // Construct view direction from CPU-computed view angle
    vec3 viewDir = normalize(vec3(viewAngle, 1.0));

    // Depth mode dispatch: displace UV
    if (depthMode == 1) {
        uv = parallaxSimple(uv, viewDir);
    } else if (depthMode == 2) {
        uv = parallaxPOM(uv, viewDir);
    }

    vec4 texColor = texture(texture0, uv);
    vec3 result = texColor.rgb;

    // Lighting
    if (lighting != 0) {
        // Choose normal derivation method
        vec3 normal;
        if (depthMode == 0) {
            normal = normalSobel(uv, texel);
        } else {
            normal = normal4Tap(uv, texel);
        }

        vec3 lightDir = normalize(vec3(cos(lightAngle), sin(lightAngle), lightHeight));

        // Lambertian diffuse with bias to avoid pure black
        float diffuse = dot(normal, lightDir) * 0.5 + 0.5;

        // Blinn-Phong specular
        vec3 halfDir = normalize(lightDir + viewDir);
        float spec = pow(max(dot(normal, halfDir), 0.0), shininess);

        float lit = diffuse + spec;
        result = texColor.rgb * mix(1.0, lit, intensity);
    }

    // Fresnel rim glow (texture-derived color)
    if (fresnelEnabled != 0) {
        vec3 normal;
        if (depthMode == 0) {
            normal = normalSobel(uv, texel);
        } else {
            normal = normal4Tap(uv, texel);
        }
        float f = fresnelTerm(normal, viewDir, fresnelExponent);
        result += f * fresnelIntensity * texColor.rgb;
    }

    finalColor = vec4(result, texColor.a);
}
```

**Optimization note**: When both lighting and fresnel are enabled, the normal is computed twice. The agent should hoist the normal computation above both blocks so it's computed once and reused.

### CPU-side Setup

The `SurfaceDepthEffectSetup` function:

1. Accumulate time: `e->time += deltaTime`
2. Compute effective view angle: start with `cfg->viewAngleX`, `cfg->viewAngleY`. If `cfg->viewLissajous.amplitude > 0`, call `DualLissajousUpdate(&cfg->viewLissajous, deltaTime, 0.0f, &lx, &ly)` and add offsets.
3. Send `viewAngle` as `float[2] = {effectiveX, effectiveY}` via `SHADER_UNIFORM_VEC2`.
4. Send `lighting` and `fresnelEnabled` as `int` (0/1) via `SHADER_UNIFORM_INT`.
5. Send all other uniforms as `SHADER_UNIFORM_FLOAT` or `SHADER_UNIFORM_INT` matching their types.

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| `depthMode` | int | 0-2 | 1 | No | Combo: "None"/"Simple"/"POM" |
| `heightScale` | float | 0.0-0.5 | 0.15 | Yes | "Height Scale" |
| `heightPower` | float | 0.5-3.0 | 1.0 | Yes | "Height Power" |
| `steps` | int | 8-64 | 15 | No | "Steps" |
| `viewAngleX` | float | -1.0-1.0 | 0.0 | Yes | "View X" |
| `viewAngleY` | float | -1.0-1.0 | 0.0 | Yes | "View Y" |
| `viewLissajous` | DualLissajousConfig | - | {.amplitude=0.0f} | amp+speed | DrawLissajousControls |
| `lighting` | bool | - | true | No | Checkbox |
| `lightAngle` | float | 0-2pi | 0.785 | Yes | ModulatableSliderAngleDeg |
| `lightHeight` | float | 0.1-2.0 | 0.5 | No | "Light Height" |
| `intensity` | float | 0.0-1.0 | 0.7 | Yes | "Intensity" |
| `reliefScale` | float | 0.02-1.0 | 0.2 | No | "Relief Scale" |
| `shininess` | float | 1.0-128.0 | 32.0 | No | "Shininess" |
| `fresnel` | bool | - | false | No | Checkbox |
| `fresnelExponent` | float | 1.0-10.0 | 2.0 | No | "Exponent" |
| `fresnelIntensity` | float | 0.0-3.0 | 2.0 | Yes | "Glow" |

### Constants

- Enum name: `TRANSFORM_SURFACE_DEPTH` (replaces `TRANSFORM_HEIGHTFIELD_RELIEF` at its position)
- Display name: `"Surface Depth"`
- Category badge: `"OPT"`, section index: `7`
- Flags: `EFFECT_FLAG_NONE`
- Config field macro: `SURFACE_DEPTH_CONFIG_FIELDS`

---

## Tasks

### Wave 1: Header

#### Task 1.1: Create Surface Depth header

**Files**: `src/effects/surface_depth.h` (create)
**Creates**: `SurfaceDepthConfig`, `SurfaceDepthEffect` types, lifecycle declarations

**Do**: Create header with the two structs from the Design section. Include `config/dual_lissajous_config.h`, `raylib.h`, `<stdbool.h>`. Define `SURFACE_DEPTH_CONFIG_FIELDS` macro listing all serializable fields (all except `time` internal state). Follow `infinite_zoom.h` as pattern for DualLissajousConfig embedding. Declare: `SurfaceDepthEffectInit(SurfaceDepthEffect*)`, `SurfaceDepthEffectSetup(SurfaceDepthEffect*, SurfaceDepthConfig*, float deltaTime)`, `SurfaceDepthEffectUninit(SurfaceDepthEffect*)`, `SurfaceDepthConfigDefault(void)`, `SurfaceDepthRegisterParams(SurfaceDepthConfig*)`. Note: Setup takes non-const config because `DualLissajousUpdate` mutates `viewLissajous.phase`.

**Verify**: `cmake.exe --build build` compiles (header-only, no consumers yet).

---

### Wave 2: Parallel Implementation

#### Task 2.1: Create Surface Depth implementation

**Files**: `src/effects/surface_depth.cpp` (create)
**Depends on**: Wave 1 complete

**Do**: Create the implementation file. Follow `heightfield_relief.cpp` as structural pattern, `infinite_zoom.cpp` for lissajous handling.

- **Init**: Load `shaders/surface_depth.fs`, cache all uniform locations listed in the Effect struct. Return false if shader load fails.
- **Setup**: Implement the CPU-side Setup described in the Design section. Accumulate `e->time += deltaTime`. Compute effective view angle with lissajous. Send all uniforms. Send `lighting` and `fresnelEnabled` as int (0/1) via `SHADER_UNIFORM_INT`. Send `steps` as `SHADER_UNIFORM_INT`. Send `resolution` as `float[2]`.
- **Uninit**: `UnloadShader(e->shader)`.
- **RegisterParams**: Register these modulatable params with `ModEngineRegisterParam`:
  - `"surfaceDepth.heightScale"` (0.0, 0.5)
  - `"surfaceDepth.heightPower"` (0.5, 3.0)
  - `"surfaceDepth.viewAngleX"` (-1.0, 1.0)
  - `"surfaceDepth.viewAngleY"` (-1.0, 1.0)
  - `"surfaceDepth.viewLissajous.amplitude"` (0.0, 0.5)
  - `"surfaceDepth.viewLissajous.motionSpeed"` (0.0, 10.0)
  - `"surfaceDepth.lightAngle"` (0.0, TWO_PI_F)
  - `"surfaceDepth.intensity"` (0.0, 1.0)
  - `"surfaceDepth.fresnelIntensity"` (0.0, 3.0)
- **ConfigDefault**: `return SurfaceDepthConfig{};`

**UI section** (`// === UI ===`):

`static void DrawSurfaceDepthParams(EffectConfig *e, const ModSources *ms, ImU32 glow)`:
- Depth section: `ImGui::SeparatorText("Depth")`. Combo for depthMode ("None"/"Simple"/"POM"). `ModulatableSlider` for heightScale, heightPower. `ImGui::SliderInt` for steps.
- View section: `ImGui::SeparatorText("View")`. `ModulatableSlider` for viewAngleX, viewAngleY. `DrawLissajousControls` for viewLissajous.
- Lighting section: `ImGui::SeparatorText("Lighting")`. `ImGui::Checkbox` for lighting. `ModulatableSliderAngleDeg` for lightAngle (full rotation). `ImGui::SliderFloat` for lightHeight, reliefScale, shininess. `ModulatableSlider` for intensity.
- Fresnel section: `ImGui::SeparatorText("Fresnel")`. `ImGui::Checkbox` for fresnel. `ImGui::SliderFloat` for fresnelExponent. `ModulatableSlider` for fresnelIntensity.

**Bridge function** (NOT static):
```cpp
void SetupSurfaceDepth(PostEffect *pe) {
  SurfaceDepthEffectSetup(&pe->surfaceDepth, &pe->effects.surfaceDepth, pe->currentDeltaTime);
}
```

**Registration macro**:
```cpp
REGISTER_EFFECT(TRANSFORM_SURFACE_DEPTH, SurfaceDepth, surfaceDepth,
                "Surface Depth", "OPT", 7, EFFECT_FLAG_NONE,
                SetupSurfaceDepth, NULL, DrawSurfaceDepthParams)
```

Includes: `"surface_depth.h"`, `"automation/mod_sources.h"`, `"automation/modulation_engine.h"`, `"config/constants.h"`, `"config/effect_descriptor.h"`, `"render/post_effect.h"`, `"imgui.h"`, `"ui/modulatable_slider.h"`, `"ui/ui_units.h"`, `<stddef.h>`.

**Verify**: Compiles (once integration task is also complete).

---

#### Task 2.2: Create Surface Depth shader

**Files**: `shaders/surface_depth.fs` (create)
**Depends on**: Wave 1 complete (for uniform names only; no #include dependency)

**Do**: Implement the complete Algorithm section from the Design. The shader has these parts:

1. `#version 330` header, `in/out/uniform` declarations for all uniforms in `SurfaceDepthEffect`
2. `getHeight(uv)` and `getDepth(uv)` functions
3. `parallaxSimple(uv, viewDir)` function — steep parallax (POM without interpolation) with jitter using `time` uniform
4. `parallaxPOM(uv, viewDir)` function — layer-step with linear interpolation
5. `normalSobel(uv, texel)` function — 9-tap Sobel from existing `heightfield_relief.fs`
6. `normal4Tap(uv, texel)` function — 4-tap cross pattern
7. `fresnelTerm(normal, view, exponent)` function
8. `main()` — depth mode dispatch, lighting, fresnel. Hoist normal computation so it's computed once when both lighting and fresnel are active.

Transcribe each function from the Algorithm section. Do not reinterpret or graft patterns from other shaders.

```glsl
// Surface Depth: luminance heightfield with parallax displacement and normal-based lighting
```

**Verify**: Shader file exists, uniforms match the Effect struct's loc fields.

---

#### Task 2.3: Integration — replace Heightfield Relief

**Files**: `src/config/effect_config.h`, `src/render/post_effect.h`, `src/config/effect_serialization.cpp`, `CMakeLists.txt` (modify). `src/effects/heightfield_relief.h`, `src/effects/heightfield_relief.cpp`, `shaders/heightfield_relief.fs` (delete)
**Depends on**: Wave 1 complete

**Do**: Replace all Heightfield Relief references with Surface Depth:

**`src/config/effect_config.h`**:
- Replace `#include "effects/heightfield_relief.h"` with `#include "effects/surface_depth.h"`
- Replace `TRANSFORM_HEIGHTFIELD_RELIEF,` with `TRANSFORM_SURFACE_DEPTH,` (same enum position)
- Replace `HeightfieldReliefConfig heightfieldRelief;` with `SurfaceDepthConfig surfaceDepth;` in `EffectConfig`

**`src/render/post_effect.h`**:
- Replace `#include "effects/heightfield_relief.h"` with `#include "effects/surface_depth.h"`
- Replace `HeightfieldReliefEffect heightfieldRelief;` with `SurfaceDepthEffect surfaceDepth;` in `PostEffect`

**`src/config/effect_serialization.cpp`**:
- Replace `#include "effects/heightfield_relief.h"` with `#include "effects/surface_depth.h"`
- Replace `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(HeightfieldReliefConfig, HEIGHTFIELD_RELIEF_CONFIG_FIELDS)` with `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(SurfaceDepthConfig, SURFACE_DEPTH_CONFIG_FIELDS)`
- Replace `X(heightfieldRelief)` with `X(surfaceDepth)` in the `EFFECT_CONFIG_FIELDS` X-macro

**`CMakeLists.txt`**:
- Replace `src/effects/heightfield_relief.cpp` with `src/effects/surface_depth.cpp` in `EFFECTS_SOURCES`

**Delete old files**:
- `src/effects/heightfield_relief.h`
- `src/effects/heightfield_relief.cpp`
- `shaders/heightfield_relief.fs`

**Verify**: `cmake.exe --build build` compiles with no references to heightfield_relief remaining.

---

#### Task 2.4: Preset migration

**Files**: `presets/CURLER.json`, `presets/SMOOTHBOB.json` (modify)
**Depends on**: Wave 1 complete

**Do**: In each preset JSON file:

1. Find the `"heightfieldRelief"` key and rename it to `"surfaceDepth"`
2. Add `"depthMode": 0` and `"lighting": true` to the object (depthMode None with lighting reproduces the old Heightfield Relief behavior exactly)
3. All other fields (`intensity`, `reliefScale`, `lightAngle`, `lightHeight`, `shininess`) keep the same names and values — they are identical between old and new configs

Also update any `"transformOrder"` array: find the entry matching the old enum index of `TRANSFORM_HEIGHTFIELD_RELIEF` — it keeps the same numeric index since we replaced the enum at the same position, so no transformOrder change is needed.

**Verify**: JSON is valid. The `"surfaceDepth"` key exists with `"depthMode": 0`.

---

## Implementation Notes

**Parallax centering**: Both `parallaxSimple` and `parallaxPOM` offset the starting UV by `+p * 0.5` (half the total displacement range) before marching. This centers the displacement so mid-brightness pixels stay put, bright pixels shift one way, and dark pixels shift the other. Without this, the entire image slides in the view direction.

**Edge stretching**: At extreme heightScale or viewAngle values, displaced UVs can sample outside [0,1], causing edge stretching from GL_CLAMP_TO_EDGE. The centering halves the maximum edge displacement but doesn't eliminate it. Accepted tradeoff — hide with other effects or keep heightScale moderate.

**UI conditional visibility**: Depth params (Height Scale, Height Power, Steps) and View section only show when Depth Method is Simple or POM. Lighting params hide when Lighting is unchecked. Fresnel params hide when Fresnel is unchecked.

**Mode naming**: Depth Method combo uses "Flat" (no displacement, lighting only), "Simple" (steep parallax with jitter), "POM" (parallax occlusion with interpolation). Originally "None"/"Simple"/"POM" — renamed for clarity.

**Preset migration**: CURLER.json and SMOOTHBOB.json migrated — key renamed from `heightfieldRelief` to `surfaceDepth`, added `depthMode: 0` and `lighting: true` for backwards-compatible Flat mode.

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] No references to `heightfield_relief` remain in source (grep)
- [ ] Effect appears in Optical section with "OPT" badge
- [ ] depthMode=None + lighting=true reproduces old Heightfield Relief visually
- [ ] depthMode=Simple shows parallax displacement
- [ ] depthMode=POM shows smoother parallax than Simple
- [ ] View angle sliders shift perspective
- [ ] View Lissajous animates the view angle
- [ ] Lighting checkbox toggles directional light
- [ ] Fresnel checkbox toggles rim glow
- [ ] Preset CURLER/SMOOTHBOB loads with lighting matching previous behavior
- [ ] Modulation routes to registered parameters
