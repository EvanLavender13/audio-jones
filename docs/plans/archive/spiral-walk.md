# Spiral Walk

A generator effect that draws a chain of glowing line segments spiraling outward from center, each turned by a fixed angle from the last and growing progressively longer. The angle step determines the spiral's form — tight coils, right-angle zigzags, golden-angle flowers, or geometric stars — and drifts over time via CPU-accumulated rotation so the pattern continuously morphs. Segments glow with gradient-LUT color and inverse-distance halos, with FFT frequency bands mapped along the chain so different parts of the spiral pulse to different parts of the spectrum.

**Research**: `docs/research/spiral-walk.md`

## Design

### Types

**SpiralWalkConfig** (in `spiral_walk.h`):
```
bool enabled = false;

// Geometry
int segments = 100;         // Number of line segments in the chain (10-200)
float growthRate = 0.02f;   // Length increment per segment (0.005-0.1)
float angleOffset = 1.5708f; // Base angle step between segments in radians (-PI..PI) — PI/2

// Animation
float rotationSpeed = 0.0f; // Rotation rate for angle morphing rad/s (-PI..PI)

// Glow
float glowIntensity = 2.0f; // Brightness multiplier (0.5-10.0)

// Audio
float baseFreq = 55.0f;    // Lowest FFT frequency Hz (27.5-440)
float maxFreq = 14000.0f;  // Highest FFT frequency Hz (1000-16000)
float gain = 2.0f;         // FFT magnitude amplifier (0.1-10.0)
float curve = 0.7f;        // FFT contrast exponent (0.1-3.0)
float baseBright = 0.15f;  // Minimum ember brightness (0.0-1.0)

// Color
ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};

// Blend
EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
float blendIntensity = 1.0f;
```

**SpiralWalkEffect** (in `spiral_walk.h`):
```
Shader shader;
ColorLUT *gradientLUT;
float rotationAccum;        // CPU-accumulated rotation angle
int resolutionLoc;
int fftTextureLoc;
int sampleRateLoc;
int segmentsLoc;
int growthRateLoc;
int angleOffsetLoc;
int rotationAccumLoc;
int glowIntensityLoc;
int baseFreqLoc;
int maxFreqLoc;
int gainLoc;
int curveLoc;
int baseBrightLoc;
int gradientLUTLoc;
```

Config fields macro:
```
#define SPIRAL_WALK_CONFIG_FIELDS \
  enabled, segments, growthRate, angleOffset, rotationSpeed, glowIntensity, \
      baseFreq, maxFreq, gain, curve, baseBright, gradient, blendMode, \
      blendIntensity
```

### Algorithm

The shader constructs a turtle-walk spiral: starting at the origin, each segment extends from where the previous one ended, rotated by `float(n) * (angleOffset + rotationAccum)` and growing longer by `growthRate` per step. Per-fragment distance to each segment uses IQ's sdSegment. Color from gradient LUT at `float(n) / float(segments)`. Brightness from FFT band energy mapped in log space from `baseFreq` to `maxFreq`.

**Attribution**: Based on "Kungs vs. Cookin" by jorge2017a3 (https://www.shadertoy.com/view/sfl3zr, CC BY-NC-SA 3.0).

**Core GLSL** (shader body):

```glsl
// Based on "Kungs vs. Cookin" by jorge2017a3
// https://www.shadertoy.com/view/sfl3zr
// License: CC BY-NC-SA 3.0 Unported
// Modified: replaced time-based angle morphing with uniform-driven rotation,
// rainbow HSL with gradient LUT, hardcoded glow with filaments-style
// inverse-distance glow, added FFT frequency band mapping per segment

#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform vec2 resolution;
uniform sampler2D fftTexture;
uniform sampler2D gradientLUT;

uniform float sampleRate;
uniform int segments;
uniform float growthRate;
uniform float angleOffset;
uniform float rotationAccum;
uniform float glowIntensity;
uniform float baseFreq;
uniform float maxFreq;
uniform float gain;
uniform float curve;
uniform float baseBright;

const float GLOW_WIDTH = 0.002;

// Point-to-segment distance (IQ sdSegment)
float segm(vec2 p, vec2 a, vec2 b) {
    vec2 pa = p - a;
    vec2 ba = b - a;
    float h = clamp(dot(pa, ba) / dot(ba, ba), 0.0, 1.0);
    return length(pa - ba * h);
}

void main() {
    vec2 r = resolution;
    vec2 uv = (fragTexCoord * r * 2.0 - r) / min(r.x, r.y);

    vec3 result = vec3(0.0);

    vec2 prev = vec2(0.0);
    float segLen = 0.0;

    for (int n = 0; n < segments; n++) {
        float angle = float(n) * (angleOffset + rotationAccum);
        segLen += growthRate;
        vec2 next = prev + vec2(cos(angle), sin(angle)) * segLen;

        // FFT band-averaging lookup (log-spaced frequency spread)
        float t0 = float(n) / float(segments);
        float t1 = float(n + 1) / float(segments);
        float freqLo = baseFreq * pow(maxFreq / baseFreq, t0);
        float freqHi = baseFreq * pow(maxFreq / baseFreq, t1);
        float binLo = freqLo / (sampleRate * 0.5);
        float binHi = freqHi / (sampleRate * 0.5);

        float energy = 0.0;
        const int BAND_SAMPLES = 4;
        for (int s = 0; s < BAND_SAMPLES; s++) {
            float bin = mix(binLo, binHi, (float(s) + 0.5) / float(BAND_SAMPLES));
            if (bin <= 1.0) {
                energy += texture(fftTexture, vec2(bin, 0.5)).r;
            }
        }
        float mag = pow(clamp(energy / float(BAND_SAMPLES) * gain, 0.0, 1.0), curve);

        // Inverse-distance glow (filaments pattern)
        float dist = segm(uv, prev, next);
        float glow = GLOW_WIDTH / (GLOW_WIDTH + dist);

        // Gradient LUT color mapped along chain
        float colorT = float(n) / float(segments);
        vec3 color = texture(gradientLUT, vec2(colorT, 0.5)).rgb;

        result += color * glow * glowIntensity * (baseBright + mag);

        prev = next;
    }

    result = tanh(result);
    finalColor = vec4(result, 1.0);
}
```

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| segments | int | 10-200 | 100 | No | Segments |
| growthRate | float | 0.005-0.1 | 0.02 | Yes | Growth Rate |
| angleOffset | float | -PI..PI | PI/2 | Yes | Angle Offset |
| rotationSpeed | float | -PI..PI | 0.0 | Yes | Rotation Speed |
| glowIntensity | float | 0.5-10.0 | 2.0 | Yes | Glow Intensity |
| baseFreq | float | 27.5-440 | 55.0 | Yes | Base Freq (Hz) |
| maxFreq | float | 1000-16000 | 14000.0 | Yes | Max Freq (Hz) |
| gain | float | 0.1-10.0 | 2.0 | Yes | Gain |
| curve | float | 0.1-3.0 | 0.7 | Yes | Contrast |
| baseBright | float | 0.0-1.0 | 0.15 | Yes | Base Bright |
| blendIntensity | float | 0.0-5.0 | 1.0 | Yes | Blend Intensity |

### Constants

- Enum name: `TRANSFORM_SPIRAL_WALK_BLEND`
- Display name: `"Spiral Walk"`
- Category section: 10 (Geometric)
- Badge: `"GEN"` (auto-set by `REGISTER_GENERATOR`)

---

## Tasks

### Wave 1: Header

#### Task 1.1: Create SpiralWalk header

**Files**: `src/effects/spiral_walk.h` (create)
**Creates**: `SpiralWalkConfig`, `SpiralWalkEffect` types, function declarations

**Do**: Follow `filaments.h` structure exactly. Include `"raylib.h"`, `"render/blend_mode.h"`, `"render/color_config.h"`, `<stdbool.h>`. Define `SpiralWalkConfig` and `SpiralWalkEffect` structs per the Design section. Define `SPIRAL_WALK_CONFIG_FIELDS` macro. Forward-declare `ColorLUT`. Declare 5 public functions: `SpiralWalkEffectInit`, `SpiralWalkEffectSetup`, `SpiralWalkEffectUninit`, `SpiralWalkConfigDefault`, `SpiralWalkRegisterParams`. Top-of-file comment: `// SpiralWalk effect module` / `// Spiraling chain of glowing segments with per-segment FFT brightness`.

**Verify**: `cmake.exe --build build` compiles (header-only, no consumers yet).

---

### Wave 2: Parallel Implementation

#### Task 2.1: Create SpiralWalk shader

**Files**: `shaders/spiral_walk.fs` (create)
**Depends on**: None (no header dependency)

**Do**: Implement the Algorithm section verbatim — the full GLSL is inlined in the Design section. The attribution comment block MUST appear at the top of the file before `#version 330`. Use the `segm()` sdSegment function from filaments.fs. Coordinate transform: `(fragTexCoord * r * 2.0 - r) / min(r.x, r.y)` — centered, aspect-correct.

**Verify**: File exists, well-formed GLSL.

#### Task 2.2: Create SpiralWalk implementation

**Files**: `src/effects/spiral_walk.cpp` (create)
**Depends on**: Task 1.1 (header)

**Do**: Follow `filaments.cpp` structure exactly. Same include set (see conventions.md generator .cpp includes). Implement:
- `SpiralWalkEffectInit`: Load `"shaders/spiral_walk.fs"`, cache all uniform locations, init `ColorLUT`, init `rotationAccum = 0.0f`. Return false on shader load failure (unload LUT if shader succeeds but LUT fails).
- `SpiralWalkEffectSetup`: Accumulate `rotationAccum += cfg->rotationSpeed * deltaTime`. Call `ColorLUTUpdate`. Bind all uniforms including `fftTexture` and `gradientLUT` textures. Use `AUDIO_SAMPLE_RATE` from `"audio/audio.h"` for sampleRate.
- `SpiralWalkEffectUninit`: Unload shader, free LUT.
- `SpiralWalkConfigDefault`: `return SpiralWalkConfig{};`
- `SpiralWalkRegisterParams`: Register all modulatable params per the Parameters table. `angleOffset` uses `ROTATION_OFFSET_MAX` bounds. `rotationSpeed` uses `ROTATION_SPEED_MAX` bounds.
- Bridge functions: `SetupSpiralWalk(PostEffect* pe)` — calls `SpiralWalkEffectSetup`. `SetupSpiralWalkBlend(PostEffect* pe)` — calls `BlendCompositorApply`.
- UI section (`// === UI ===`): `static void DrawSpiralWalkParams(EffectConfig*, const ModSources*, ImU32)`. Sections: Audio (Base Freq, Max Freq, Gain, Contrast, Base Bright), Geometry (`ImGui::SliderInt` for Segments, `ModulatableSlider` for Growth Rate, `ModulatableSliderAngleDeg` for Angle Offset), Glow (Glow Intensity), Animation (`ModulatableSliderSpeedDeg` for Rotation Speed). Follow exact slider conventions from conventions.md Audio section.
- `STANDARD_GENERATOR_OUTPUT(spiralWalk)` macro before registration.
- `REGISTER_GENERATOR(TRANSFORM_SPIRAL_WALK_BLEND, SpiralWalk, spiralWalk, "Spiral Walk", SetupSpiralWalkBlend, SetupSpiralWalk, 10, DrawSpiralWalkParams, DrawOutput_spiralWalk)` wrapped in `// clang-format off` / `// clang-format on`.

**Verify**: `cmake.exe --build build` compiles.

#### Task 2.3: Integration — effect_config.h, post_effect.h, CMakeLists.txt, effect_serialization.cpp

**Files**: `src/config/effect_config.h`, `src/render/post_effect.h`, `CMakeLists.txt`, `src/config/effect_serialization.cpp` (all modify)
**Depends on**: Task 1.1 (header)

**Do**:
1. **effect_config.h**:
   - Add `#include "effects/spiral_walk.h"` with the other effect includes (alphabetical)
   - Add `TRANSFORM_SPIRAL_WALK_BLEND,` before `TRANSFORM_ACCUM_COMPOSITE` in enum
   - Add `TRANSFORM_SPIRAL_WALK_BLEND,` at end of `TransformOrderConfig` order array (before closing brace)
   - Add `SpiralWalkConfig spiralWalk;` in `EffectConfig` struct near other generator configs

2. **post_effect.h**:
   - Add `#include "effects/spiral_walk.h"` with other effect includes
   - Add `SpiralWalkEffect spiralWalk;` in `PostEffect` struct near other effect members

3. **CMakeLists.txt**:
   - Add `src/effects/spiral_walk.cpp` to `EFFECTS_SOURCES` (alphabetical)

4. **effect_serialization.cpp**:
   - Add `#include "effects/spiral_walk.h"` with other effect includes
   - Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(SpiralWalkConfig, SPIRAL_WALK_CONFIG_FIELDS)` with other JSON macros
   - Add `X(spiralWalk) \` to `EFFECT_CONFIG_FIELDS` X-macro table

**Verify**: `cmake.exe --build build` compiles with no warnings.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] Spiral Walk appears in Geometric generator section (section 10)
- [ ] Effect can be enabled and shows glowing spiral segments
- [ ] angleOffset changes spiral shape (90 deg = right-angle, ~137.5 deg = golden angle)
- [ ] rotationSpeed causes the spiral to morph continuously
- [ ] FFT reactivity: inner segments pulse to bass, outer to treble
- [ ] Gradient LUT colors applied along the chain
- [ ] Preset save/load preserves all settings
