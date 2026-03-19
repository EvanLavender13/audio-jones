# Cyber March

Raymarched fractal fold generator that creates an endless dark space filled with colorful square shapes at every scale - a cyberspace flythrough. Based on iterated Mandelbox-family fold-scale-translate operations with configurable camera, FFT audio reactivity, and gradient LUT coloring. Follows the same generator pattern as Voxel March.

**Research**: `docs/research/cyber_march.md`

## Design

### Types

```c
// src/effects/cyber_march.h

struct CyberMarchConfig {
  bool enabled = false;

  // Geometry
  int marchSteps = 60;          // Raymarch iterations (30-100)
  float stepSize = 0.5f;        // March step multiplier (0.1-1.0)
  float fov = 0.3f;             // Ray direction Z component (0.2-1.0)
  float domainSize = 30.0f;     // Domain repetition half-period (10.0-60.0)
  int foldIterations = 8;       // Fold-scale-translate iterations (2-12)
  float foldScale = 1.4f;       // Base scale factor per fold (1.0-2.0)
  float morphAmount = 0.1f;     // Fold scale oscillation amplitude (0.0-0.5)
  float foldOffsetX = 15.0f;    // Fold translate X (1.0-200.0)
  float foldOffsetY = 120.0f;   // Fold translate Y (1.0-200.0)
  float foldOffsetZ = 40.0f;    // Fold translate Z (1.0-200.0)
  float initialScale = 3.0f;    // Starting scale accumulator (1.0-10.0)
  float colorSpread = 0.1f;     // March distance to gradient spread (0.01-1.0)
  float tonemapGain = 0.001f;   // Tonemap exposure (0.0001-0.01)

  // Speed (CPU-accumulated)
  float flySpeed = 1.0f;        // Forward camera speed (0.0-5.0)
  float morphSpeed = 0.1f;      // Fold scale oscillation rate (0.0-2.0)

  // Camera
  DualLissajousConfig lissajous = {
      .amplitude = 0.5f, .motionSpeed = 1.0f, .freqX1 = 0.3f, .freqY1 = 0.2f};

  // Audio
  bool colorFreqMap = false;     // Map FFT bands to gradient color instead of depth
  float baseFreq = 55.0f;       // Low frequency bound Hz (27.5-440)
  float maxFreq = 14000.0f;     // High frequency bound Hz (1000-16000)
  float gain = 2.0f;            // FFT amplitude multiplier (0.1-10)
  float curve = 1.5f;           // FFT response curve exponent (0.1-3.0)
  float baseBright = 0.15f;     // Minimum brightness when silent (0.0-1.0)

  // Color
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};

  // Blend
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f;
};

#define CYBER_MARCH_CONFIG_FIELDS                                              \
  enabled, marchSteps, stepSize, fov, domainSize, foldIterations, foldScale,  \
      morphAmount, foldOffsetX, foldOffsetY, foldOffsetZ, initialScale,       \
      colorSpread, tonemapGain, flySpeed, morphSpeed, lissajous,              \
      colorFreqMap, baseFreq, maxFreq, gain, curve, baseBright, gradient,     \
      blendMode, blendIntensity

typedef struct CyberMarchEffect {
  Shader shader;
  ColorLUT *gradientLUT;
  float flyPhase;
  float morphPhase;
  int resolutionLoc;
  int fftTextureLoc;
  int sampleRateLoc;
  int flyPhaseLoc;
  int morphPhaseLoc;
  int marchStepsLoc;
  int stepSizeLoc;
  int fovLoc;
  int domainSizeLoc;
  int foldIterationsLoc;
  int foldScaleLoc;
  int morphAmountLoc;
  int foldOffsetLoc;
  int initialScaleLoc;
  int colorSpreadLoc;
  int tonemapGainLoc;
  int yawLoc;
  int pitchLoc;
  int baseFreqLoc;
  int maxFreqLoc;
  int gainLoc;
  int curveLoc;
  int colorFreqMapLoc;
  int baseBrightLoc;
  int gradientLUTLoc;
} CyberMarchEffect;
```

### Algorithm

#### Ray Setup

```glsl
vec2 uv = fragTexCoord * 2.0 - 1.0;
uv.x *= resolution.x / resolution.y;
vec3 rd = normalize(vec3(uv, fov));

// Camera rotation (yaw/pitch from CPU Lissajous)
float sp = sin(pitch), cp = cos(pitch);
rd.yz *= mat2(cp, -sp, sp, cp);
float sy = sin(yaw), cy = cos(yaw);
rd.xz *= mat2(cy, -sy, sy, cy);

vec3 ro = vec3(0.0, 0.0, flyPhase);
```

#### Pre-Fold Space Operations

Expanded inline from reference macros. These are the effect's identity - keep verbatim:

```glsl
// r(a) helper: 2D rotation matrix
mat2 rot(float a) { return mat2(cos(a), -sin(a), sin(a), cos(a)); }

// Inside march loop, after domain repetition:
p = mod(p - domainSize, domainSize * 2.0) - domainSize;

// G(p.xz, 1.0)
p.xz = floor(p.xz * 1.0) / 1.0;
// Q(p.xz)
p.xz *= rot(round(atan(p.x, p.z) * 4.0) / 4.0);
// G(p.xy, 1.0)
p.xy = floor(p.xy * 1.0) / 1.0;
// F(p.xz, 2.0)
p.xz = abs(p.xz * rot(2.0)); p.z -= 0.5;
// G(p.yz, 1.0)
p.yz = floor(p.yz * 1.0) / 1.0;
// M(p.xz)
p.xz = abs(p.xz) - 0.5; p.xz *= rot(0.785); p.xz = abs(p.xz) - 0.2;
// M(p.zy)
p.zy = abs(p.zy) - 0.5; p.zy *= rot(0.785); p.zy = abs(p.zy) - 0.2;
// Q2(p.xy)
p.xy = abs(p.xy - 1.0) - abs(p.xy + 1.0) + p.xy;
```

#### Fractal Fold Loop

```glsl
float s = initialScale;
for (int j = 0; j < foldIterations; j++) {
    // Q2(p.zy)
    p.zy = abs(p.zy - 1.0) - abs(p.zy + 1.0) + p.zy;

    p = 0.3 - abs(p);

    // Axis sort
    if (p.x < p.z) { p = p.zyx; }
    if (p.z < p.y) { p = p.xzy; }

    float e = foldScale + morphAmount * sin(morphPhase);
    s *= e;
    p = abs(p) * e - foldOffset;
}
```

#### Distance Estimate, Color, and Accumulation

```glsl
float e = length(p.yzzz) / s;
g += stepSize * e;

// Gradient color from march distance
float gradientT = fract(g * colorSpread);
vec3 surfColor = texture(gradientLUT, vec2(gradientT, 0.5)).rgb;

// FFT brightness (same sampleFFTBand as Voxel March)
float brightness;
if (colorFreqMap == 0) {
    float ft0 = float(i) / float(marchSteps - 1);
    float ft1 = float(i + 1) / float(marchSteps);
    brightness = sampleFFTBand(ft0, ft1);
} else {
    float bw = 0.1;
    brightness = sampleFFTBand(gradientT, gradientT + bw);
}

color += surfColor / e * brightness;
```

#### Final Tonemap

```glsl
finalColor = vec4(tanh(color * tonemapGain), 1.0);
```

#### Camera (CPU side)

DualLissajous drives yaw/pitch. `DualLissajousUpdate` outputs X,Y which map directly to yaw and pitch in radians. Clamp pitch to +/- 0.785 radians (45 degrees) after update to prevent disorienting flips if user cranks amplitude.

```cpp
float yawVal, pitchVal;
DualLissajousUpdate(&cfg->lissajous, deltaTime, 0.0f, &yawVal, &pitchVal);
const float maxPitch = 0.785f;
pitchVal = fmaxf(-maxPitch, fminf(maxPitch, pitchVal));
```

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| marchSteps | int | 30-100 | 60 | No | March Steps |
| stepSize | float | 0.1-1.0 | 0.5 | Yes | Step Size |
| fov | float | 0.2-1.0 | 0.3 | Yes | FOV |
| domainSize | float | 10.0-60.0 | 30.0 | Yes | Domain Size |
| foldIterations | int | 2-12 | 8 | No | Fold Iterations |
| foldScale | float | 1.0-2.0 | 1.4 | Yes | Fold Scale |
| morphAmount | float | 0.0-0.5 | 0.1 | Yes | Morph Amount |
| foldOffsetX | float | 1.0-200.0 | 15.0 | Yes | Fold Offset X |
| foldOffsetY | float | 1.0-200.0 | 120.0 | Yes | Fold Offset Y |
| foldOffsetZ | float | 1.0-200.0 | 40.0 | Yes | Fold Offset Z |
| initialScale | float | 1.0-10.0 | 3.0 | Yes | Initial Scale |
| colorSpread | float | 0.01-1.0 | 0.1 | Yes | Color Spread |
| tonemapGain | float | 0.0001-0.01 | 0.001 | Yes | Tonemap Gain |
| flySpeed | float | 0.0-5.0 | 1.0 | Yes | Fly Speed |
| morphSpeed | float | 0.0-2.0 | 0.1 | Yes | Morph Speed |
| colorFreqMap | bool | - | false | No | Color Freq Map |
| baseFreq | float | 27.5-440 | 55 | Yes | Base Freq (Hz) |
| maxFreq | float | 1000-16000 | 14000 | Yes | Max Freq (Hz) |
| gain | float | 0.1-10 | 2.0 | Yes | Gain |
| curve | float | 0.1-3.0 | 1.5 | Yes | Contrast |
| baseBright | float | 0.0-1.0 | 0.15 | Yes | Base Bright |

### Constants

- Enum name: `TRANSFORM_CYBER_MARCH_BLEND`
- Display name: `"Cyber March"`
- Category section: 13 (Field - same as Voxel March)
- Badge: `"GEN"` (auto via `REGISTER_GENERATOR`)
- Flags: `EFFECT_FLAG_BLEND` (auto via `REGISTER_GENERATOR`)

---

## Tasks

### Wave 1: Header

#### Task 1.1: Create CyberMarchConfig and CyberMarchEffect

**Files**: `src/effects/cyber_march.h` (create)
**Creates**: Config struct, Effect struct, function declarations

**Do**: Create header following the Types section above. Same structure as `src/effects/voxel_march.h`. Includes: `"config/dual_lissajous_config.h"`, `"raylib.h"`, `"render/blend_mode.h"`, `"render/color_config.h"`, `<stdbool.h>`. Declare `CyberMarchEffectInit` (takes `CyberMarchEffect*` and `const CyberMarchConfig*`), `CyberMarchEffectSetup` (takes `CyberMarchEffect*`, `CyberMarchConfig*`, `float deltaTime`, `const Texture2D& fftTexture`), `CyberMarchEffectUninit`, `CyberMarchRegisterParams`.

**Verify**: Header compiles when included.

---

### Wave 2: Parallel Implementation

#### Task 2.1: Shader

**Files**: `shaders/cyber_march.fs` (create)
**Depends on**: Wave 1 complete

**Do**: Implement the Algorithm section. Attribution header:
```
// Based on "Fractalic Explorer 2" by nayk
// https://www.shadertoy.com/view/7fS3Wh
// License: CC BY-NC-SA 3.0 Unported
// Modified: parameterized uniforms, FFT audio reactivity, gradient LUT coloring
```

Uniforms: `resolution`, `fftTexture`, `sampleRate`, `flyPhase`, `morphPhase`, `marchSteps`, `stepSize`, `fov`, `domainSize`, `foldIterations`, `foldScale`, `morphAmount`, `foldOffset` (vec3), `initialScale`, `colorSpread`, `tonemapGain`, `yaw`, `pitch`, `baseFreq`, `maxFreq`, `gain`, `curve`, `colorFreqMap` (int), `baseBright`, `gradientLUT`.

Copy `sampleFFTBand()` verbatim from `shaders/voxel_march.fs`. The `rot()` helper, pre-fold chain, fold loop, distance estimate, color accumulation, and tonemap are all specified in the Algorithm section. The march loop variable `i` must be `float` and serve as both loop counter and FFT band index (same pattern as reference: `for(float i=0., s, e, g=0.; i++ < marchSteps;)`). Note: GLSL 330 does not allow swizzle assignment to non-contiguous components like `p.xz = ...`. Use a temp: `vec2 t = ...; p.x = t.x; p.z = t.y;` for all 2-component swizzle writes on `xz`, `zy`, `yz`, `xy`.

**Verify**: Shader file exists with correct attribution.

---

#### Task 2.2: Effect Implementation

**Files**: `src/effects/cyber_march.cpp` (create)
**Depends on**: Wave 1 complete

**Do**: Follow `src/effects/voxel_march.cpp` as template. Same include set but with `"cyber_march.h"`. Init loads `"shaders/cyber_march.fs"`, caches all uniform locations, creates ColorLUT. Setup accumulates `flyPhase += flySpeed * deltaTime` and `morphPhase += morphSpeed * deltaTime`, calls `DualLissajousUpdate` for yaw/pitch (map output X to yaw, Y to pitch, clamp pitch to +/- 0.785 radians), binds all uniforms. `foldOffset` is bound as vec3 from the three config floats: `float foldOff[3] = {cfg->foldOffsetX, cfg->foldOffsetY, cfg->foldOffsetZ}; SetShaderValue(..., SHADER_UNIFORM_VEC3)`. Uninit unloads shader and frees LUT. RegisterParams registers all modulatable params from the Parameters table (dot-prefixed IDs: `"cyberMarch.fov"`, `"cyberMarch.stepSize"`, etc.). Include the `// === UI ===` section with `DrawCyberMarchParams` - same layout as Voxel March: Audio section (Checkbox colorFreqMap, sliders for baseFreq/maxFreq/gain/curve/baseBright), Geometry section (SliderInt marchSteps/foldIterations, sliders for stepSize/fov/domainSize/foldScale/morphAmount/foldOffsetX/foldOffsetY/foldOffsetZ/initialScale/colorSpread, Log slider for tonemapGain), Motion section (flySpeed/morphSpeed), Camera section (DrawLissajousControls). Bridge functions: `SetupCyberMarch` and `SetupCyberMarchBlend`. Registration: `STANDARD_GENERATOR_OUTPUT(cyberMarch)` then `REGISTER_GENERATOR(TRANSFORM_CYBER_MARCH_BLEND, CyberMarch, cyberMarch, "Cyber March", SetupCyberMarchBlend, SetupCyberMarch, 13, DrawCyberMarchParams, DrawOutput_cyberMarch)`.

**Verify**: Compiles.

---

#### Task 2.3: Integration

**Files**: `src/config/effect_config.h` (modify), `src/render/post_effect.h` (modify), `CMakeLists.txt` (modify), `src/config/effect_serialization.cpp` (modify)
**Depends on**: Wave 1 complete

**Do**:

1. `src/config/effect_config.h`:
   - Add `#include "effects/cyber_march.h"` (alphabetical among includes)
   - Add `TRANSFORM_CYBER_MARCH_BLEND,` before `TRANSFORM_ACCUM_COMPOSITE` in the enum
   - Add `CyberMarchConfig cyberMarch;` to `EffectConfig` struct (alphabetical among members)

2. `src/render/post_effect.h`:
   - Add `#include "effects/cyber_march.h"` (alphabetical)
   - Add `CyberMarchEffect cyberMarch;` to `PostEffect` struct

3. `CMakeLists.txt`:
   - Add `src/effects/cyber_march.cpp` to `EFFECTS_SOURCES` (alphabetical)

4. `src/config/effect_serialization.cpp`:
   - Add `#include "effects/cyber_march.h"` (alphabetical)
   - Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(CyberMarchConfig, CYBER_MARCH_CONFIG_FIELDS)` with the other per-config macros
   - Add `X(cyberMarch)` to the `EFFECT_CONFIG_FIELDS(X)` macro

**Verify**: `cmake.exe -G Ninja -B build -S . -DCMAKE_BUILD_TYPE=Release && cmake.exe --build build` compiles with no warnings.

---

## Final Verification

- [x] Build succeeds with no warnings
- [ ] Effect appears in Field generator section (section 13)
- [ ] Enabling shows the fractal field visual
- [ ] Camera flies forward through the space
- [ ] Camera yaw/pitch drifts via Lissajous
- [ ] FFT audio drives brightness (depth-mapped by default)
- [ ] Color Freq Map toggle switches FFT mapping mode
- [ ] Gradient LUT colors the geometry
- [ ] All modulatable sliders respond
- [ ] Preset save/load round-trips all settings

## Implementation Notes

- **Overexposure fix**: Added `max(e, 0.001)` clamp on distance estimate divisor in shader to prevent div-by-zero blow-up when rays hit surfaces dead-on
- **UI reorganization**: Removed Motion section. Fly Speed moved to Camera section. Morph Speed moved next to Morph Amount in Geometry (after Fold Scale) to group related controls.
- **flySpeed max**: Increased from 5.0 to 20.0 - original range too slow for the scale of the space
- **tonemapGain max**: Tightened from 0.01 to 0.003 - higher values wash out to white since tanh saturates all channels
- **colorSpread max**: Increased from 1.0 to 2.0 - high values produce a glitchy chromatic effect that is intentionally usable
