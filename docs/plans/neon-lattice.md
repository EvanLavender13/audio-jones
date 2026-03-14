# Neon Lattice

Raymarched infinite lattice of torus shapes with neon glow — flying through a vast repeating digital infrastructure. Columns of tiny rings tile space along configurable axes (1–3), each scrolling at hashed speeds. Soft light falloff reveals the structure as luminous streaks against black void. Based on "Inside the System" by kishimisu.

**Research**: `docs/research/neon-lattice.md`

## Design

### Types

**NeonLatticeConfig** (in `src/effects/neon_lattice.h`):

```c
struct NeonLatticeConfig {
  bool enabled = false;

  // Grid
  int axisCount = 3;           // Orthogonal torus column axes (1-3)
  float spacing = 7.0f;        // Grid cell size (2.0-20.0)
  float lightSpacing = 2.0f;   // Light repetition multiplier (0.5-5.0)
  float attenuation = 22.0f;   // Glow tightness (5.0-60.0)
  float glowExponent = 1.3f;   // Glow curve shape (0.5-3.0)

  // Speed (radians/second — accumulated on CPU)
  float cameraSpeed = 0.7f;    // Orbital camera drift rate (0.0-3.0)
  float columnsSpeed = 2.8f;   // Column scroll speed (0.0-15.0)
  float lightsSpeed = 21.0f;   // Light streak speed (0.0-60.0)

  // Camera orbit
  float orbitRadius = 65.0f;     // Camera distance from origin (20.0-120.0)
  float orbitVariation = 15.0f;  // Radius oscillation amplitude (0.0-40.0)
  float orbitRatioX = 0.97f;     // X-axis Lissajous ratio (0.5-2.0)
  float orbitRatioY = 1.11f;     // Y-axis Lissajous ratio (0.5-2.0)
  float orbitRatioZ = 1.27f;     // Z-axis Lissajous ratio (0.5-2.0)

  // Quality
  int iterations = 50;         // Raymarch step count (20-80)
  float maxDist = 80.0f;       // Ray termination distance (20.0-120.0)

  // Torus shape
  float torusRadius = 0.6f;    // Torus major radius (0.2-1.5)
  float torusTube = 0.06f;     // Torus tube thickness (0.02-0.2)

  // Color
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};

  // Blend
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f;
};
```

**Config fields macro**:
```c
#define NEON_LATTICE_CONFIG_FIELDS                                              \
  enabled, axisCount, spacing, lightSpacing, attenuation, glowExponent,        \
      cameraSpeed, columnsSpeed, lightsSpeed, orbitRadius, orbitVariation,      \
      orbitRatioX, orbitRatioY, orbitRatioZ, iterations, maxDist, torusRadius,  \
      torusTube, gradient, blendMode, blendIntensity
```

**NeonLatticeEffect** (in `src/effects/neon_lattice.h`):

```c
typedef struct NeonLatticeEffect {
  Shader shader;
  ColorLUT *gradientLUT;
  float cameraPhase;   // CPU-accumulated camera time
  float columnsPhase;  // CPU-accumulated columns time
  float lightsPhase;   // CPU-accumulated lights time

  int resolutionLoc;
  int spacingLoc;
  int lightSpacingLoc;
  int attenuationLoc;
  int glowExponentLoc;
  int cameraTimeLoc;
  int columnsTimeLoc;
  int lightsTimeLoc;
  int orbitRadiusLoc;
  int orbitVariationLoc;
  int orbitRatioXLoc;
  int orbitRatioYLoc;
  int orbitRatioZLoc;
  int iterationsLoc;
  int maxDistLoc;
  int torusRadiusLoc;
  int torusTubeLoc;
  int gradientLUTLoc;
  int axisCountLoc;
} NeonLatticeEffect;
```

### Algorithm

Full shader (`shaders/neon_lattice.fs`). This is the single source of truth — transcribed from the reference code with the substitution table applied.

```glsl
// Based on "Inside the System" by kishimisu (2022)
// https://www.shadertoy.com/view/msj3D3
// License: CC BY-NC-SA 4.0
// Neon glow falloff inspired by: https://www.shadertoy.com/view/3dlcWl
// Modified: uniforms replace defines, gradientLUT for color, configurable axis count

#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform vec2 resolution;
uniform sampler2D gradientLUT;

// Grid
uniform float spacing;       // 7.0
uniform float lightSpacing;  // 2.0
uniform float attenuation;   // 22.0
uniform float glowExponent;  // 1.3

// Speed phases (CPU-accumulated)
uniform float cameraTime;
uniform float columnsTime;
uniform float lightsTime;

// Camera orbit
uniform float orbitRadius;    // 65.0
uniform float orbitVariation; // 15.0
uniform float orbitRatioX;    // 0.97
uniform float orbitRatioY;    // 1.11
uniform float orbitRatioZ;    // 1.27

// Quality
uniform int iterations;       // 50
uniform float maxDist;        // 80.0

// Torus shape
uniform float torusRadius;    // 0.6
uniform float torusTube;      // 0.06

// Axis count
uniform int axisCount;         // 3

#define EPSILON 0.005

#define rot(a) mat2(cos(a), -sin(a), sin(a), cos(a))
#define rep(p, r) (mod(p + r / 2.0, r) - r / 2.0)

float torusSDF(vec3 p) {
    return length(vec2(length(p.xz) - torusRadius, p.y)) - torusTube;
}

float hash12(vec2 p) {
    vec3 p3 = fract(vec3(p.xyx) * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}

vec3 getLight(vec3 p, vec3 color) {
    return max(vec3(0.0), color / (1.0 + pow(abs(torusSDF(p) * attenuation), glowExponent)) - 0.001);
}

vec3 geo(vec3 po, inout float d, inout vec2 f) {
    // Shape repetition
    float r = hash12(floor(po.yz / spacing + vec2(0.5))) - 0.5;
    vec3 p = rep(po + vec3(columnsTime * r, 0.0, 0.0), vec3(0.5, spacing, spacing));
    p.xy *= rot(1.5707963);
    d = min(d, torusSDF(p));

    // Light repetition
    f = floor(po.yz / (spacing * lightSpacing) - vec2(0.5));
    r = hash12(f) - 0.5;
    if (r > -0.45)
        p = rep(po + vec3(lightsTime * r, 0.0, 0.0), spacing * lightSpacing * vec3(r + 0.54, 1.0, 1.0));
    else
        p = rep(po + vec3(lightsTime * 0.5 * (1.0 + r * 0.003 * hash12(floor(po.yz * spacing))), 0.0, 0.0), spacing * lightSpacing);
    p.xy *= rot(1.5707963);
    f = (cos(f.xy) * 0.5 + 0.5) * 0.4;

    return p;
}

vec4 map(vec3 p) {
    float d = 1e6;
    vec3 po, col = vec3(0.0);
    vec2 f;

    // Axis 1 (always)
    po = geo(p, d, f);
    col += getLight(po, texture(gradientLUT, vec2(0.0, 0.5)).rgb);

    if (axisCount >= 2) {
        // Rotate into axis 2
        p.z += spacing / 2.0;
        p.xy *= rot(1.5707963);
        po = geo(p, d, f);
        col += getLight(po, texture(gradientLUT, vec2(0.33, 0.5)).rgb);
    }

    if (axisCount >= 3) {
        // Rotate into axis 3
        p.xy += spacing / 2.0;
        p.xz *= rot(1.5707963);
        po = geo(p, d, f);
        col += getLight(po, texture(gradientLUT, vec2(0.66, 0.5)).rgb);
    }

    return vec4(col, d);
}

vec3 getOrigin(float t) {
    // Geometric path constants — NOT speed (speed is in cameraTime)
    t = (t + 35.0) * -0.05;
    float rad = mix(orbitRadius - orbitVariation, orbitRadius + orbitVariation, cos(t * 1.24) * 0.5 + 0.5);
    return vec3(rad * sin(t * orbitRatioX), rad * cos(t * orbitRatioY), rad * sin(t * orbitRatioZ));
}

void initRay(vec2 uv, out vec3 ro, out vec3 rd) {
    ro = getOrigin(cameraTime);
    vec3 f = normalize(getOrigin(cameraTime + 0.5) - ro);
    vec3 r = normalize(cross(normalize(ro), f));
    rd = normalize(f + uv.x * r + uv.y * cross(f, r));
}

void main() {
    // Centered coordinates from pixel position (raymarcher — fragTexCoord convention N/A)
    vec2 uv = (2.0 * gl_FragCoord.xy - resolution) / resolution.y;
    vec3 p, ro, rd, col = vec3(0.0);

    initRay(uv, ro, rd);

    float t = 2.0;
    for (int i = 0; i < iterations; i++) {
        p = ro + t * rd;

        vec4 res = map(p);
        col += res.rgb;
        t += abs(res.w);

        if (abs(res.w) < EPSILON) t += EPSILON;

        // Early saturation exit — significant perf win in bright scenes
        if (col.r >= 1.0 && col.g >= 1.0 && col.b >= 1.0) break;
        if (t > maxDist) break;
    }

    col = pow(col, vec3(0.45));
    finalColor = vec4(col, 1.0);
}
```

### CPU-side speed accumulation

Three independent phase accumulators in `NeonLatticeEffectSetup()`:

```c
e->cameraPhase  += cfg->cameraSpeed  * deltaTime;
e->columnsPhase += cfg->columnsSpeed * deltaTime;
e->lightsPhase  += cfg->lightsSpeed  * deltaTime;
```

Pass as `cameraTime`, `columnsTime`, `lightsTime` uniforms.

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| axisCount | int | 1-3 | 3 | No | `Axes##neonLattice` (Combo: "1 Axis\0 2 Axes\0 3 Axes\0") |
| spacing | float | 2.0-20.0 | 7.0 | Yes | `Spacing##neonLattice` |
| lightSpacing | float | 0.5-5.0 | 2.0 | Yes | `Light Spacing##neonLattice` |
| attenuation | float | 5.0-60.0 | 22.0 | Yes | `Attenuation##neonLattice` |
| glowExponent | float | 0.5-3.0 | 1.3 | Yes | `Glow Exponent##neonLattice` |
| cameraSpeed | float | 0.0-3.0 | 0.7 | Yes | `Camera Speed##neonLattice` |
| columnsSpeed | float | 0.0-15.0 | 2.8 | Yes | `Columns Speed##neonLattice` |
| lightsSpeed | float | 0.0-60.0 | 21.0 | Yes | `Lights Speed##neonLattice` |
| orbitRadius | float | 20.0-120.0 | 65.0 | Yes | `Orbit Radius##neonLattice` |
| orbitVariation | float | 0.0-40.0 | 15.0 | Yes | `Orbit Variation##neonLattice` |
| orbitRatioX | float | 0.5-2.0 | 0.97 | Yes | `Ratio X##neonLattice` |
| orbitRatioY | float | 0.5-2.0 | 1.11 | Yes | `Ratio Y##neonLattice` |
| orbitRatioZ | float | 0.5-2.0 | 1.27 | Yes | `Ratio Z##neonLattice` |
| iterations | int | 20-80 | 50 | No | `Iterations##neonLattice` (SliderInt) |
| maxDist | float | 20.0-120.0 | 80.0 | Yes | `Max Distance##neonLattice` |
| torusRadius | float | 0.2-1.5 | 0.6 | Yes | `Ring Radius##neonLattice` |
| torusTube | float | 0.02-0.2 | 0.06 | Yes | `Tube Width##neonLattice` |

### Constants

- Enum name: `TRANSFORM_NEON_LATTICE_BLEND`
- Display name: `"Neon Lattice"`
- Category badge: `"GEN"` (auto-set by `REGISTER_GENERATOR`)
- Section index: 10 (Geometric)
- Flags: `EFFECT_FLAG_BLEND` (auto-set by `REGISTER_GENERATOR`)

### UI sections

```
SeparatorText("Grid")
  Combo axisCount
  ModulatableSlider spacing
  ModulatableSlider lightSpacing
  ModulatableSlider attenuation
  ModulatableSlider glowExponent

SeparatorText("Speed")
  ModulatableSlider cameraSpeed
  ModulatableSlider columnsSpeed
  ModulatableSlider lightsSpeed

SeparatorText("Camera")
  ModulatableSlider orbitRadius
  ModulatableSlider orbitVariation
  ModulatableSlider orbitRatioX
  ModulatableSlider orbitRatioY
  ModulatableSlider orbitRatioZ

SeparatorText("Shape")
  ModulatableSlider torusRadius
  ModulatableSlider torusTube

SeparatorText("Quality")
  SliderInt iterations
  ModulatableSlider maxDist
```

---

## Tasks

### Wave 1: Header

#### Task 1.1: Create neon_lattice.h

**Files**: `src/effects/neon_lattice.h` (create)
**Creates**: `NeonLatticeConfig`, `NeonLatticeEffect`, lifecycle declarations

**Do**: Create the header file following the Design > Types section above. Include `"raylib.h"`, `"render/blend_mode.h"`, `"render/color_config.h"`, `<stdbool.h>`. Forward-declare `typedef struct ColorLUT ColorLUT;`. Declare lifecycle functions: `NeonLatticeEffectInit(NeonLatticeEffect*, const NeonLatticeConfig*)`, `NeonLatticeEffectSetup(NeonLatticeEffect*, const NeonLatticeConfig*, float deltaTime)`, `NeonLatticeEffectUninit(NeonLatticeEffect*)`, `NeonLatticeConfigDefault(void)`, `NeonLatticeRegisterParams(NeonLatticeConfig*)`. Add top-of-file one-line comment and inline range comments on config fields. Follow `twist_tunnel.h` as structural pattern.

**Verify**: `cmake.exe --build build` compiles.

---

### Wave 2: Parallel Implementation

#### Task 2.1: Create neon_lattice.cpp

**Files**: `src/effects/neon_lattice.cpp` (create)
**Depends on**: Wave 1 complete

**Do**: Implement the full module following `twist_tunnel.cpp` as structural pattern. Includes: `"neon_lattice.h"`, `"automation/mod_sources.h"`, `"automation/modulation_engine.h"`, `"config/constants.h"`, `"config/effect_config.h"`, `"config/effect_descriptor.h"`, `"render/blend_compositor.h"`, `"render/blend_mode.h"`, `"render/color_lut.h"`, `"render/post_effect.h"`, `"imgui.h"`, `"ui/imgui_panels.h"`, `"ui/modulatable_slider.h"`, `"ui/ui_units.h"`, `<stddef.h>`.

**Init**: Load shader `"shaders/neon_lattice.fs"`, cache all uniform locations in a `static CacheLocations()` helper, init `ColorLUTInit(&cfg->gradient)` (unload shader on LUT fail), zero all three phase accumulators.

**Setup**: Accumulate three speed phases on CPU per the Design > CPU-side speed accumulation section. Upload all uniforms via `SetShaderValue`. Call `ColorLUTUpdate` and bind gradientLUT texture.

**Uninit**: `UnloadShader` + `ColorLUTUninit`.

**RegisterParams**: Register all modulatable params per the Parameters table. Use `"neonLattice.<field>"` IDs.

**Bridge functions**: `SetupNeonLattice(PostEffect* pe)` — calls `NeonLatticeEffectSetup`. `SetupNeonLatticeBlend(PostEffect* pe)` — calls `BlendCompositorApply` with `pe->generatorScratch.texture`, `blendIntensity`, `blendMode`. Both non-static.

**UI**: `static void DrawNeonLatticeParams(EffectConfig*, const ModSources*, ImU32)` implementing the UI sections from the design. axisCount as `ImGui::Combo` with `"1 Axis\0 2 Axes\0 3 Axes\0"` — note: axisCount is stored as int 1-3 in the config but Combo is 0-indexed, so store as 0-2 in the combo and add 1 when binding to shader. **Correction**: use the config value directly (1-3) and map Combo index: `int comboIdx = cfg->axisCount - 1; ImGui::Combo(..., &comboIdx, ...); cfg->axisCount = comboIdx + 1;`. iterations as `ImGui::SliderInt`.

**Registration**: `STANDARD_GENERATOR_OUTPUT(neonLattice)` then `REGISTER_GENERATOR(TRANSFORM_NEON_LATTICE_BLEND, NeonLattice, neonLattice, "Neon Lattice", SetupNeonLatticeBlend, SetupNeonLattice, 10, DrawNeonLatticeParams, DrawOutput_neonLattice)` wrapped in `// clang-format off` / `// clang-format on`.

**Verify**: Compiles after all Wave 2 tasks complete.

---

#### Task 2.2: Create neon_lattice.fs shader

**Files**: `shaders/neon_lattice.fs` (create)
**Depends on**: Wave 1 complete

**Do**: Implement the Algorithm section verbatim. Copy the full GLSL from the Design > Algorithm section. The attribution comment block at the top is mandatory. Do not modify the algorithm — it is the single source of truth.

**Verify**: File exists, valid GLSL syntax.

---

#### Task 2.3: Register in effect_config.h

**Files**: `src/config/effect_config.h` (modify)
**Depends on**: Wave 1 complete

**Do**:
1. Add `#include "effects/neon_lattice.h"` in alphabetical order among the effect includes (between `muons.h` and `nebula.h`)
2. Add `TRANSFORM_NEON_LATTICE_BLEND,` to the `TransformEffectType` enum before `TRANSFORM_FLIP_BOOK` (alongside other generators — after `TRANSFORM_TWIST_TUNNEL_BLEND`)
3. Add config member to `EffectConfig` struct: `NeonLatticeConfig neonLattice;` with comment `// Neon Lattice (raymarched infinite torus lattice generator)` — place after `twistTunnel` member

**Verify**: Compiles after all Wave 2 tasks complete.

---

#### Task 2.4: Register in post_effect.h

**Files**: `src/render/post_effect.h` (modify)
**Depends on**: Wave 1 complete

**Do**:
1. Add `#include "effects/neon_lattice.h"` in alphabetical order among the effect includes (between `muons.h` and `nebula.h`)
2. Add `NeonLatticeEffect neonLattice;` to the `PostEffect` struct after `TwistTunnelEffect twistTunnel;`

**Verify**: Compiles after all Wave 2 tasks complete.

---

#### Task 2.5: Add to CMakeLists.txt

**Files**: `CMakeLists.txt` (modify)
**Depends on**: Wave 1 complete

**Do**: Add `src/effects/neon_lattice.cpp` to `EFFECTS_SOURCES` in alphabetical order (between `nebula.cpp` and `oil_paint.cpp` or wherever alphabetical falls).

**Verify**: Compiles after all Wave 2 tasks complete.

---

#### Task 2.6: Register serialization

**Files**: `src/config/effect_serialization.cpp` (modify)
**Depends on**: Wave 1 complete

**Do**:
1. Add `#include "effects/neon_lattice.h"` with the other effect includes
2. Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(NeonLatticeConfig, NEON_LATTICE_CONFIG_FIELDS)` among the other JSON macros
3. Add `X(neonLattice) \` to the `EFFECT_CONFIG_FIELDS(X)` X-macro table (in alphabetical order among generators)

**Verify**: Compiles after all Wave 2 tasks complete.

---

## Final Verification

- [ ] Build succeeds with no warnings: `cmake.exe --build build`
- [ ] Effect appears in Geometric generator section with "GEN" badge
- [ ] Enabling renders a raymarched torus lattice with neon glow
- [ ] Gradient editor controls the per-axis colors
- [ ] Speed sliders animate camera drift, column scrolling, and light streaks independently
- [ ] Axis count combo reduces/adds visible lattice axes
- [ ] Preset save/load round-trips all settings
- [ ] Modulation routes to registered parameters

---

## Implementation Notes

- **Gradient coloring**: The plan's shader sampled `gradientLUT` at 3 fixed UV positions (0.0, 0.33, 0.66), giving each axis a single solid color. Changed to use the per-cell `f` output from `geo()` as the LUT coordinate: `fract(f.x + f.y + axisOffset)`. This gives spatial color variation across lattice cells.
- **Camera**: The configurable orbit parameters (orbitRadius, orbitVariation, orbitRatioX/Y/Z) were removed. The original kishimisu camera code is used verbatim with hardcoded constants — the values were tuned as a set and don't work when parameterized independently. Only `cameraSpeed` is exposed.
- **Camera speed default**: 1.4 (range 0.0–5.0). The original shader used raw `iTime` with a `* -0.05` scaling baked into `getOrigin()`. At 1.4, the effective rate is slightly faster than the original, making the orbit more visible.
