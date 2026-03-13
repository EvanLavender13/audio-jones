# Twist Tunnel

Nested wireframe platonic solids spiral inward with progressive scale and rotation, creating a recursive tunnel of glowing edges. Each layer is smaller and more twisted than the last, while a Lissajous-driven camera orbits the structure. Outer layers react to bass, inner layers to treble, and color flows from a gradient LUT sampled by depth.

**Research**: `docs/research/twist_tunnel.md`

## Design

### Types

**Shared header** `src/config/platonic_solids.h` — extracted from `spin_cage.cpp`:

```c
struct ShapeDescriptor {
  int vertexCount;
  int edgeCount;
  const float (*vertices)[3];
  const int (*edges)[2];
};

static const float TETRA_VERTICES[][3] = { ... };  // existing data
static const int TETRA_EDGES[][2] = { ... };
// ... all 5 shapes ...
static const ShapeDescriptor SHAPES[] = { ... };
static const int SHAPE_COUNT = 5;
static const int MAX_VERTICES = 20;
static const int MAX_EDGES = 30;
```

**TwistTunnelConfig** (`src/effects/twist_tunnel.h`):

```c
struct TwistTunnelConfig {
  bool enabled = false;

  // Geometry
  int shape = 1;          // Platonic solid: 0=Tetra, 1=Cube, 2=Octa, 3=Dodeca, 4=Icosa
  int layerCount = 12;    // Number of nested wireframe layers (2-20)
  float scaleRatio = 0.8f; // Scale multiplier between successive layers (0.5-0.99)

  // Twist
  float twistAngle = 0.5f; // Static twist offset per layer in radians (-PI..+PI)
  float twistSpeed = 0.1f; // Twist accumulation rate rad/s (-PI..+PI)

  // Projection
  float perspective = 4.0f; // Camera projection depth (1.0-20.0)
  float scale = 1.2f;       // Overall wireframe size (0.1-5.0)

  // Glow
  float lineWidth = 2.0f;     // Glow falloff width in pixel units (0.5-10.0)
  float glowIntensity = 1.0f; // Glow brightness (0.1-5.0)
  float contrast = 3.0f;      // tanh output squaring factor (0.5-10.0)

  // Camera
  DualLissajousConfig lissajous = {
    .amplitude = 0.3f,
    .motionSpeed = 1.0f,
    .freqX1 = 0.07f,
    .freqY1 = 0.11f,
  };

  // Audio
  float baseFreq = 55.0f;   // Low end FFT freq spread Hz (27.5-440.0)
  float maxFreq = 14000.0f; // High end FFT freq spread Hz (1000-16000)
  float gain = 2.0f;        // FFT magnitude amplification (0.1-10.0)
  float curve = 1.5f;       // FFT response curve / contrast (0.1-3.0)
  float baseBright = 0.15f; // Base edge visibility without audio (0.0-1.0)

  // Color
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};

  // Blend
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f;
};
```

Config fields macro: `enabled, shape, layerCount, scaleRatio, twistAngle, twistSpeed, perspective, scale, lineWidth, glowIntensity, contrast, lissajous, baseFreq, maxFreq, gain, curve, baseBright, gradient, blendMode, blendIntensity`

**TwistTunnelEffect** (`src/effects/twist_tunnel.h`):

```c
typedef struct TwistTunnelEffect {
  Shader shader;
  ColorLUT *gradientLUT;
  float twistPhase; // Accumulated twist (twistSpeed * deltaTime)

  int resolutionLoc;
  int layerCountLoc;
  int scaleRatioLoc;
  int twistAngleLoc;
  int twistPhaseLoc;
  int perspectiveLoc;
  int scaleLoc;
  int cameraPitchLoc;
  int cameraYawLoc;
  int lineWidthLoc;
  int glowIntensityLoc;
  int contrastLoc;
  int verticesLoc;   // uniform vec3 vertices[20]
  int vertexCountLoc;
  int edgeIdxLoc;    // uniform vec2 edgeIdx[30]
  int edgeCountLoc;
  int fftTextureLoc;
  int sampleRateLoc;
  int baseFreqLoc;
  int maxFreqLoc;
  int gainLoc;
  int curveLoc;
  int baseBrightLoc;
  int gradientLUTLoc;
} TwistTunnelEffect;
```

### Algorithm

#### CPU-side (`TwistTunnelEffectSetup`)

1. Accumulate twist: `e->twistPhase += cfg->twistSpeed * deltaTime`
2. Lissajous camera: `DualLissajousUpdate(&cfg->lissajous, deltaTime, 0.0f, &cameraPitch, &cameraYaw)` — outputs are raw radians
3. Look up shape from `SHAPES[cfg->shape]`
4. Upload vertex positions as `vec3 vertices[20]` — raw vertex `xyz` from the `ShapeDescriptor`, no CPU-side transform
5. Upload edge connectivity as `vec2 edgeIdx[30]` — `x=vertA`, `y=vertB` as float (cast to int in shader)
6. Upload `vertexCount`, `edgeCount`, `layerCount`, `scaleRatio`, `twistAngle`, `twistPhase`, `perspective`, `scale`, `cameraPitch`, `cameraYaw`
7. Upload glow/audio/color params, FFT texture, gradient LUT texture
8. Update gradient LUT: `ColorLUTUpdate(e->gradientLUT, &cfg->gradient)`

#### GPU-side (Fragment shader `shaders/twist_tunnel.fs`)

Attribution block (before `#version`):

```glsl
// Based on "Twisty cubes" by ChunderFPV
// https://www.shadertoy.com/view/lclXzH
// License: CC BY-NC-SA 3.0
// wireframe code modified from FabriceNeyret2: https://www.shadertoy.com/view/XfS3DK
// Modified: generic platonic solids via uniform arrays, Lissajous camera, FFT reactivity, gradient LUT coloring
```

Uniforms:

```glsl
uniform vec2 resolution;
uniform vec3 vertices[20];
uniform int vertexCount;
uniform vec2 edgeIdx[30];
uniform int edgeCount;
uniform int layerCount;
uniform float scaleRatio;
uniform float twistAngle;
uniform float twistPhase;
uniform float perspective;
uniform float scale;
uniform float cameraPitch;
uniform float cameraYaw;
uniform float lineWidth;
uniform float glowIntensity;
uniform float contrast;
uniform sampler2D fftTexture;
uniform sampler2D gradientLUT;
uniform float sampleRate;
uniform float baseFreq;
uniform float maxFreq;
uniform float gain;
uniform float curve;
uniform float baseBright;
```

Rotation matrix helper (from reference `A()` macro — adapted to take raw radians, not `v*PI`):

```glsl
#define HALF_PI 1.5707963

mat2 rotMat(float angle) {
    return mat2(cos(angle + vec4(0, -HALF_PI, HALF_PI, 0)));
}
```

Line distance with depth dimming (from reference `L()` function):

```glsl
float lineDist(vec2 p, vec3 A, vec3 B) {
    vec2 a = A.xy;
    vec2 ba = B.xy - a;
    vec2 pa = p - a;
    float h = clamp(dot(pa, ba) / dot(ba, ba), 0.0, 1.0);
    return length(pa - h * ba) + 0.01 * mix(A.z, B.z, h);
}
```

Camera transform (from reference `K()` function — pitch on zy, yaw on zx, perspective project):

```glsl
vec3 camProject(vec3 p, mat2 pitchMat, mat2 yawMat, float persp) {
    p.zy *= pitchMat;
    p.zx *= yawMat;
    return vec3(p.xy / (p.z + persp), p.z);
}
```

Main function:

```glsl
void main() {
    vec2 uv = (gl_FragCoord.xy - 0.5 * resolution) / resolution.y;

    mat2 pitchMat = rotMat(cameraPitch);
    float pixelWidth = lineWidth / resolution.y;

    vec3 c = vec3(0.0);

    for (int i = 0; i < layerCount; i++) {
        float t = float(i) / float(layerCount - 1);
        float layerScale = scale * pow(scaleRatio, float(i));
        float yawAngle = cameraYaw + twistAngle * float(i) + twistPhase;
        mat2 yawMat = rotMat(yawAngle);

        // Per-layer color from gradient LUT
        vec3 color = texture(gradientLUT, vec2(t, 0.5)).rgb;

        // Per-layer FFT brightness (band-averaged, log-space spread)
        float freqRatio = maxFreq / baseFreq;
        float bandW = 1.0 / 48.0;
        float ft0 = max(t - bandW * 0.5, 0.0);
        float ft1 = min(t + bandW * 0.5, 1.0);
        float freqLo = baseFreq * pow(freqRatio, ft0);
        float freqHi = baseFreq * pow(freqRatio, ft1);
        float binLo = freqLo / (sampleRate * 0.5);
        float binHi = freqHi / (sampleRate * 0.5);

        float fftEnergy = 0.0;
        const int BAND_SAMPLES = 4;
        for (int s = 0; s < BAND_SAMPLES; s++) {
            float bin = mix(binLo, binHi, (float(s) + 0.5) / float(BAND_SAMPLES));
            if (bin <= 1.0) {
                fftEnergy += texture(fftTexture, vec2(bin, 0.5)).r;
            }
        }
        float brightness = baseBright + pow(clamp(fftEnergy / float(BAND_SAMPLES) * gain, 0.0, 1.0), curve);

        // Per-edge line distance and glow
        for (int j = 0; j < edgeCount; j++) {
            vec3 vertA = vertices[int(edgeIdx[j].x)] * layerScale;
            vec3 vertB = vertices[int(edgeIdx[j].y)] * layerScale;

            vec3 projA = camProject(vertA, pitchMat, yawMat, perspective);
            vec3 projB = camProject(vertB, pitchMat, yawMat, perspective);

            float dist = lineDist(uv, projA, projB);
            float glow = pixelWidth / (pixelWidth + abs(dist));

            // Max blend (from reference)
            c = max(c, color * glow * glowIntensity * brightness);
        }
    }

    // Output: tanh(c*c*contrast) — squaring for contrast, tanh limits brightness
    finalColor = vec4(tanh(c * c * contrast), 1.0);
}
```

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| shape | int | 0-4 | 1 | No | Shape (combo) |
| layerCount | int | 2-20 | 12 | No | Layers |
| scaleRatio | float | 0.5-0.99 | 0.8 | Yes | Scale Ratio |
| twistAngle | float | -PI..+PI | 0.5 | Yes | Twist Angle |
| twistSpeed | float | -PI..+PI | 0.1 | Yes | Twist Speed |
| perspective | float | 1.0-20.0 | 4.0 | Yes | Perspective |
| scale | float | 0.1-5.0 | 1.2 | Yes | Scale |
| lineWidth | float | 0.5-10.0 | 2.0 | Yes | Line Width |
| glowIntensity | float | 0.1-5.0 | 1.0 | Yes | Glow Intensity |
| contrast | float | 0.5-10.0 | 3.0 | Yes | Contrast |
| lissajous | DualLissajousConfig | embedded | see config | Yes (amplitude, motionSpeed) | Lissajous section |
| baseFreq | float | 27.5-440.0 | 55.0 | Yes | Base Freq (Hz) |
| maxFreq | float | 1000-16000 | 14000.0 | Yes | Max Freq (Hz) |
| gain | float | 0.1-10.0 | 2.0 | Yes | Gain |
| curve | float | 0.1-3.0 | 1.5 | Yes | Contrast |
| baseBright | float | 0.0-1.0 | 0.15 | Yes | Base Bright |
| gradient | ColorConfig | embedded | gradient mode | N/A | Color section |
| blendMode | EffectBlendMode | enum | Screen | No | Blend Mode |
| blendIntensity | float | 0.0-5.0 | 1.0 | Yes | Blend Intensity |

### Constants

- Enum name: `TRANSFORM_TWIST_TUNNEL_BLEND`
- Display name: `"Twist Tunnel"`
- Category: Generator (`"GEN"`)
- Section index: 10 (Geometric)

---

## Tasks

### Wave 1: Foundation

#### Task 1.1: Extract platonic solids to shared header

**Files**: `src/config/platonic_solids.h` (create), `src/effects/spin_cage.cpp` (modify)
**Creates**: `platonic_solids.h` with `ShapeDescriptor` struct, all 5 shape vertex/edge tables, `SHAPES[]`, `SHAPE_COUNT`, `MAX_VERTICES`, `MAX_EDGES`

**Do**:
1. Create `src/config/platonic_solids.h` with include guard `#ifndef PLATONIC_SOLIDS_H`
2. Move from `spin_cage.cpp`: `ShapeDescriptor` struct, all 5 vertex arrays (`TETRA_VERTICES` through `ICOSA_VERTICES`), all 5 edge arrays, `SHAPES[]` table, `SHAPE_COUNT`, `MAX_EDGES`. Add `MAX_VERTICES = 20`.
3. In `spin_cage.cpp`: replace the moved code with `#include "config/platonic_solids.h"`. Remove the local `ShapeDescriptor` struct, all vertex/edge arrays, `SHAPES[]`, `SHAPE_COUNT`, `MAX_EDGES`.
4. Verify Spin Cage still compiles and all references resolve.

**Verify**: `cmake.exe --build build` compiles with no errors.

#### Task 1.2: Create Twist Tunnel config header

**Files**: `src/effects/twist_tunnel.h` (create)
**Creates**: `TwistTunnelConfig`, `TwistTunnelEffect`, function declarations

**Do**:
1. Create `src/effects/twist_tunnel.h` following the Types section above exactly.
2. Include `"raylib.h"`, `"render/blend_mode.h"`, `"render/color_config.h"`, `"config/dual_lissajous_config.h"`, `<stdbool.h>`.
3. Define `TwistTunnelConfig` with all fields from Design section.
4. Define `TWIST_TUNNEL_CONFIG_FIELDS` macro.
5. Forward declare `ColorLUT`.
6. Define `TwistTunnelEffect` typedef struct with all loc fields from Design section.
7. Declare: `TwistTunnelEffectInit(TwistTunnelEffect*, const TwistTunnelConfig*)`, `TwistTunnelEffectSetup(TwistTunnelEffect*, TwistTunnelConfig*, float, Texture2D)`, `TwistTunnelEffectUninit(TwistTunnelEffect*)`, `TwistTunnelConfigDefault(void)`, `TwistTunnelRegisterParams(TwistTunnelConfig*)`.
8. Note: Setup takes non-const `TwistTunnelConfig*` because `DualLissajousUpdate` mutates lissajous phase.

**Verify**: Header parses cleanly (included by later tasks).

---

### Wave 2: Parallel Implementation

#### Task 2.1: Twist Tunnel implementation and UI

**Files**: `src/effects/twist_tunnel.cpp` (create)
**Depends on**: Wave 1 complete

**Do**:
1. Create `src/effects/twist_tunnel.cpp` following the generator `.cpp` pattern from `spin_cage.cpp`.
2. Includes: own header, `"audio/audio.h"`, `"automation/mod_sources.h"`, `"automation/modulation_engine.h"`, `"config/constants.h"`, `"config/effect_config.h"`, `"config/effect_descriptor.h"`, `"config/platonic_solids.h"`, `"imgui.h"`, `"render/blend_compositor.h"`, `"render/blend_mode.h"`, `"render/color_lut.h"`, `"render/post_effect.h"`, `"ui/imgui_panels.h"`, `"ui/modulatable_slider.h"`, `"ui/ui_units.h"`, `<math.h>`, `<stddef.h>`.

3. **Init**: Load shader `"shaders/twist_tunnel.fs"`. Cache all uniform locations listed in the `TwistTunnelEffect` struct. Init `gradientLUT` via `ColorLUTInit(&cfg->gradient)`. Set `twistPhase = 0.0f`. On failure cascade cleanup.

4. **Setup**: Implement the CPU-side Algorithm section:
   - Accumulate `e->twistPhase += cfg->twistSpeed * deltaTime`
   - Call `DualLissajousUpdate(&cfg->lissajous, deltaTime, 0.0f, &cameraPitch, &cameraYaw)`
   - Look up shape: clamp `cfg->shape` to `[0, SHAPE_COUNT-1]`, get `&SHAPES[shapeIdx]`
   - Pack vertices as `float verts[MAX_VERTICES * 3]` from `shape->vertices`
   - Pack edges as `float edgeIdx[MAX_EDGES * 2]` from `shape->edges` (cast int to float)
   - Upload all uniforms: resolution, vertices (vec3 array), vertexCount, edgeIdx (vec2 array), edgeCount, layerCount, scaleRatio, twistAngle, twistPhase, perspective, scale, cameraPitch, cameraYaw, lineWidth, glowIntensity, contrast, fftTexture, sampleRate, baseFreq, maxFreq, gain, curve, baseBright, gradientLUT texture
   - Call `ColorLUTUpdate(e->gradientLUT, &cfg->gradient)`

5. **Uninit**: `UnloadShader`, `ColorLUTUninit`.

6. **RegisterParams**: Register all modulatable params with dot-separated IDs `"twistTunnel.*"`. Use `ROTATION_SPEED_MAX` bounds for `twistSpeed`, `ROTATION_OFFSET_MAX` for `twistAngle`. Register lissajous params: `"twistTunnel.lissajous.amplitude"` and `"twistTunnel.lissajous.motionSpeed"`.

7. **ConfigDefault**: `return TwistTunnelConfig{};`

8. **UI section** (`// === UI ===`):
   - `static void DrawTwistTunnelParams(EffectConfig*, const ModSources*, ImU32)`:
     - Geometry section: Shape combo (same as Spin Cage), `ImGui::SliderInt` for Layers (2-20)
     - Twist section: `ModulatableSliderAngleDeg` for Twist Angle, `ModulatableSliderSpeedDeg` for Twist Speed
     - Projection section: Perspective slider, Scale slider
     - Glow section: Line Width, Glow Intensity, Contrast sliders
     - Camera section: `DrawLissajousControls("twistTunnel", &cfg->lissajous, modSources)`
     - Audio section: Base Freq, Max Freq, Gain, Contrast, Base Bright (standard order and formats from conventions)

9. **Bridge functions**:
   - `void SetupTwistTunnel(PostEffect* pe)` — calls `TwistTunnelEffectSetup`
   - `void SetupTwistTunnelBlend(PostEffect* pe)` — calls `BlendCompositorApply`
   - Neither is static (bridge functions are never static).

10. **Registration**:
    ```
    STANDARD_GENERATOR_OUTPUT(twistTunnel)
    REGISTER_GENERATOR(TRANSFORM_TWIST_TUNNEL_BLEND, TwistTunnel, twistTunnel,
                       "Twist Tunnel", SetupTwistTunnelBlend, SetupTwistTunnel, 10,
                       DrawTwistTunnelParams, DrawOutput_twistTunnel)
    ```

**Verify**: Compiles (after all Wave 2 tasks complete).

#### Task 2.2: Fragment shader

**Files**: `shaders/twist_tunnel.fs` (create)
**Depends on**: Wave 1 complete

**Do**:
1. Create `shaders/twist_tunnel.fs`.
2. Add attribution block before `#version 330` (see Algorithm section).
3. Implement the full GPU-side Algorithm section from the Design. All GLSL code is inlined in the Algorithm — implement it exactly:
   - `rotMat()` helper
   - `lineDist()` with depth dimming (from reference `L()`)
   - `camProject()` — pitch on zy, yaw on zx, perspective projection returning `vec3(p.xy / (p.z + persp), p.z)`. No scale in this function.
   - Main: center UV, build pitchMat, nested `layerCount × edgeCount` loop with `layerScale = scale * pow(scaleRatio, float(i))` applied to vertices, per-layer FFT band-averaged brightness, max blending, `tanh(c * c * contrast)` output.
4. `scale` is part of `layerScale` (applied to vertices before projection), NOT inside `camProject`.

**Verify**: Shader file exists, GLSL syntax is valid.

#### Task 2.3: Integration (effect_config, post_effect, CMakeLists, serialization)

**Files**: `src/config/effect_config.h` (modify), `src/render/post_effect.h` (modify), `CMakeLists.txt` (modify), `src/config/effect_serialization.cpp` (modify)
**Depends on**: Wave 1 complete

**Do**:
1. `src/config/effect_config.h`:
   - Add `#include "effects/twist_tunnel.h"` in alphabetical position (after `triskelion.h`)
   - Add `TRANSFORM_TWIST_TUNNEL_BLEND,` in enum before `TRANSFORM_FLIP_BOOK` (alongside other generator blend entries, near `TRANSFORM_TRISKELION_BLEND`)
   - Add `TRANSFORM_TWIST_TUNNEL_BLEND` to `TransformOrderConfig::order` initializer (it's auto-generated from enum order, so just adding to enum is sufficient)
   - Add `TwistTunnelConfig twistTunnel;` member to `EffectConfig` struct (alphabetical, near `triskelion`)

2. `src/render/post_effect.h`:
   - Add `#include "effects/twist_tunnel.h"` in alphabetical position
   - Add `TwistTunnelEffect twistTunnel;` member to `PostEffect` struct

3. `CMakeLists.txt`:
   - Add `src/effects/twist_tunnel.cpp` to `EFFECTS_SOURCES` in alphabetical position

4. `src/config/effect_serialization.cpp`:
   - Add `#include "effects/twist_tunnel.h"`
   - Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(TwistTunnelConfig, TWIST_TUNNEL_CONFIG_FIELDS)`
   - Add `X(twistTunnel) \` to `EFFECT_CONFIG_FIELDS(X)` macro table

**Verify**: `cmake.exe --build build` compiles with no errors.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] Twist Tunnel appears in Geometric generator section
- [ ] Shape selection switches wireframe geometry
- [ ] Layer count changes nested depth
- [ ] Twist controls create spiral progression
- [ ] Lissajous camera orbits smoothly
- [ ] FFT reactivity: outer layers react to bass, inner to treble
- [ ] Gradient LUT colors layers by depth
- [ ] Preset save/load preserves all settings
- [ ] Spin Cage still works correctly (shared header refactor)

---

## Implementation Notes

**Per-axis twist** (post-plan addition): Added `twistPitch` and `twistPitchSpeed` params alongside the original yaw-only twist. `pitchMat` moved inside the layer loop so each layer gets `pitchAngle = cameraPitch + twistPitch * float(i) + twistPitchPhase`. Enables true 3D spiral tunnels instead of single-axis corkscrews.

**Camera amplitude range**: Widened Lissajous amplitude registration from 0.0-0.5 to 0.0-PI (ROTATION_OFFSET_MAX). The shared `DrawLissajousControls` widget reads range from param registration when modSources are provided, so no widget changes needed. Full ±180° camera orbits now reachable.
