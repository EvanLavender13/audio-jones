# Star Trail

Hundreds of glowing stars orbit screen center at different radii and speeds, leaving persistent luminous trails that fade over time. Differential rotation creates spiraling arcs. Each star reacts to its own FFT frequency bin, pulsing brighter when its mapped frequency is active. A configurable mapping mode determines how stars relate to the spectrum.

**Research**: `docs/research/star_trail.md`

## Design

### Types

**StarTrailConfig** (`src/effects/star_trail.h`):

```cpp
struct StarTrailConfig {
  bool enabled = false;

  // Stars
  int starCount = 200;       // Number of orbiting stars (50-500)
  int freqMapMode = 0;       // FFT mapping: 0=radius, 1=hash, 2=angle (0-2)
  float spreadRadius = 1.0f; // How far stars spread from center (0.2-2.0)

  // Orbit
  float orbitSpeed = 0.4f;      // Base angular velocity multiplier (0.05-2.0)
  float speedWaviness = 10.0f;  // Radial speed ripple frequency (0.0-20.0)
  float speedVariation = 0.1f;  // Radial speed ripple amplitude (0.0-0.5)

  // Appearance
  float glowSize = 0.008f;      // Star sprite radius in screen units (0.005-0.05)
  float glowIntensity = 0.9f;   // Peak brightness of star sprites (0.1-3.0)
  float decayHalfLife = 2.0f;   // Trail persistence in seconds (0.1-10.0)

  // Audio
  float baseFreq = 55.0f;    // Lowest FFT frequency Hz (27.5-440.0)
  float maxFreq = 14000.0f;  // Highest FFT frequency Hz (1000-16000)
  float gain = 2.0f;         // FFT sensitivity (0.1-10.0)
  float curve = 1.5f;        // FFT contrast exponent (0.1-3.0)
  float baseBright = 0.15f;  // Min brightness floor when silent (0.0-1.0)

  // Color
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};

  // Blend compositing
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f; // (0.0-5.0)
};
```

Config fields macro:

```cpp
#define STAR_TRAIL_CONFIG_FIELDS                                               \
  enabled, starCount, freqMapMode, spreadRadius, orbitSpeed, speedWaviness,    \
      speedVariation, glowSize, glowIntensity, decayHalfLife, baseFreq,        \
      maxFreq, gain, curve, baseBright, gradient, blendMode, blendIntensity
```

**StarTrailEffect** (`src/effects/star_trail.h`):

```cpp
typedef struct StarTrailEffect {
  Shader shader;
  ColorLUT *gradientLUT;
  RenderTexture2D pingPong[2];
  int readIdx;
  Texture2D currentFFTTexture;
  float time;          // Accumulated orbit phase (orbitSpeed * sum(dt))
  float variationTime; // Accumulated variation phase (speedVariation * sum(dt))
  // Uniform locations
  int resolutionLoc;
  int timeLoc;
  int variationTimeLoc;
  int previousFrameLoc;
  int starCountLoc;
  int freqMapModeLoc;
  int spreadRadiusLoc;
  int speedWavinessLoc;
  int glowSizeLoc;
  int glowIntensityLoc;
  int decayFactorLoc;
  int gradientLUTLoc;
  int fftTextureLoc;
  int sampleRateLoc;
  int baseFreqLoc;
  int maxFreqLoc;
  int gainLoc;
  int curveLoc;
  int baseBrightLoc;
} StarTrailEffect;
```

### Algorithm

Single-pass ping-pong shader. Stars are positioned analytically (no simulation buffer). The reference's 3-buffer architecture (simulation, rendering, tonemap) is collapsed into one fragment shader.

Attribution header (required -- reference is CC BY-NC-SA 3.0):

```glsl
// Based on "StarTrail" by hdrp0720
// https://www.shadertoy.com/view/wXcyz7
// License: CC BY-NC-SA 3.0 Unported
// Modified: collapsed 3-buffer to single-pass ping-pong, analytical orbits,
// per-star FFT reactivity, gradient LUT color
```

Reference functions kept verbatim:

```glsl
float hash11(float p) {
    p = fract(p * .1031);
    p *= p + 33.33;
    p *= p + p;
    return fract(p);
}

float smootherstep(float edge0, float edge1, float x) {
    x = clamp((x - edge0) / (edge1 - edge0), 0.0, 1.0);
    return x * x * x * (3.0 * x * (2.0 * x - 5.0) + 10.0);
}
```

Main shader body:

```glsl
#define TWO_PI 6.2831853

void main() {
    vec2 uv = fragTexCoord;

    // Trail fade: multiplicative decay (replaces reference's subtractive dissipation)
    vec3 color = texture(previousFrame, uv).rgb * decayFactor;

    // Screen-centered coords normalized by height
    // Reference: (2.0 * fragCoord - iResolution.xy) / iResolution.y
    // Ours: equivalent via fragTexCoord
    vec2 posScreen = (fragTexCoord * resolution - resolution * 0.5) / resolution.y;

    float radiusSq = glowSize * glowSize;
    vec3 accumColor = vec3(0.0);

    // Reference loop was over NUM_PARTICLES with texelFetch from simulation buffer.
    // Ours: analytical orbit positions, dynamic loop bound.
    for (int i = 0; i < starCount; i++) {
        // Reference: seeded from hash in Buffer A init
        float starRadius = hash11(float(i) * 12.345) * spreadRadius;
        float initialAngle = hash11(float(i) * 67.890) * TWO_PI;

        // Reference: speed = 0.5 + sin(dist * 10.0) * 0.1
        // Ours: additive formula with both terms CPU-accumulated to avoid jumps.
        // time = sum(orbitSpeed * dt), variationTime = sum(speedVariation * dt)
        float variation = sin(starRadius * speedWaviness);

        // Reference: velocity integration pos += velocity * dt
        // Ours: analytical angle from two accumulated phases
        float angle = initialAngle + time + variation * variationTime;

        vec2 starPos = starRadius * vec2(cos(angle), sin(angle));

        // Reference: squared distance check against radius
        vec2 d = starPos - posScreen;
        float distSq = dot(d, d);

        if (distSq < radiusSq) {
            // Compute frequency index t from freqMapMode
            float t;
            if (freqMapMode == 0) {
                t = clamp(starRadius / spreadRadius, 0.0, 1.0);
            } else if (freqMapMode == 1) {
                t = hash11(float(i) * 45.678);
            } else {
                t = fract(initialAngle / TWO_PI);
            }

            // FFT lookup (standard generator pattern)
            float freq = baseFreq * pow(maxFreq / baseFreq, t);
            float bin = freq / (sampleRate * 0.5);
            float energy = 0.0;
            if (bin <= 1.0) { energy = texture(fftTexture, vec2(bin, 0.5)).r; }
            float mag = pow(clamp(energy * gain, 0.0, 1.0), curve);
            float brightness = baseBright + mag;

            // Reference: getParticleColor(i) hardcoded palette
            // Ours: gradient LUT with same t as FFT
            vec3 starColor = texture(gradientLUT, vec2(t, 0.5)).rgb;

            // Reference: smootherstep(radius, 0.0, dist)
            float dist = sqrt(distSq);
            float intensity = smootherstep(glowSize, 0.0, dist);

            // Reference: accumColor += pColor * intensity, then color += accumColor * 0.9
            // Ours: glowIntensity replaces the 0.9 constant
            accumColor += starColor * brightness * intensity * glowIntensity;
        }
    }

    color += accumColor;

    // Reference: Reinhard tonemap was separate Image pass
    // Ours: merged into same shader
    finalColor = vec4(color / (color + vec3(1.0)), 1.0);
}
```

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| starCount | int | 50-500 | 200 | No | Stars |
| freqMapMode | int | 0-2 | 0 | No | Freq Mapping |
| spreadRadius | float | 0.2-2.0 | 1.0 | Yes | Spread Radius |
| orbitSpeed | float | 0.05-2.0 | 0.4 | Yes | Orbit Speed |
| speedWaviness | float | 0.0-20.0 | 10.0 | Yes | Speed Waviness |
| speedVariation | float | 0.0-0.5 | 0.1 | Yes | Speed Variation |
| glowSize | float | 0.005-0.05 | 0.008 | Yes | Glow Size |
| glowIntensity | float | 0.1-3.0 | 0.9 | Yes | Glow Intensity |
| decayHalfLife | float | 0.1-10.0 | 2.0 | Yes | Decay Half-Life |
| baseFreq | float | 27.5-440.0 | 55.0 | Yes | Base Freq (Hz) |
| maxFreq | float | 1000-16000 | 14000.0 | Yes | Max Freq (Hz) |
| gain | float | 0.1-10.0 | 2.0 | Yes | Gain |
| curve | float | 0.1-3.0 | 1.5 | Yes | Contrast |
| baseBright | float | 0.0-1.0 | 0.15 | Yes | Base Bright |
| blendIntensity | float | 0.0-5.0 | 1.0 | Yes | Blend Intensity |

### Constants

- Enum name: `TRANSFORM_STAR_TRAIL_BLEND`
- Display name: `"Star Trail"`
- Category badge: `"GEN"` (auto-set by `REGISTER_GENERATOR_FULL`)
- Category section: 15 (Scatter)
- Flags: `EFFECT_FLAG_BLEND | EFFECT_FLAG_NEEDS_RESIZE` (auto-set by macro)

<!-- Intentional deviation: research mentions "optionally" modulating glow size by brightness. Omitted for simplicity -- can be added later. -->

---

## Tasks

### Wave 1: Header

#### Task 1.1: Create star_trail.h

**Files**: `src/effects/star_trail.h`
**Creates**: `StarTrailConfig`, `StarTrailEffect` types, function declarations

**Do**: Create the header file. Use the Types section above for exact struct layouts. Include `"raylib.h"`, `"render/blend_mode.h"`, `"render/color_config.h"`, `<stdbool.h>`. Forward-declare `ColorLUT`. Declare these functions:

- `bool StarTrailEffectInit(StarTrailEffect *e, const StarTrailConfig *cfg, int width, int height)`
- `void StarTrailEffectSetup(StarTrailEffect *e, const StarTrailConfig *cfg, float deltaTime, const Texture2D &fftTexture)`
- `void StarTrailEffectRender(StarTrailEffect *e, const StarTrailConfig *cfg, float deltaTime, int screenWidth, int screenHeight)`
- `void StarTrailEffectResize(StarTrailEffect *e, int width, int height)`
- `void StarTrailEffectUninit(StarTrailEffect *e)`
- `void StarTrailRegisterParams(StarTrailConfig *cfg)`

Top-of-file comment: `// Star trail effect module` / `// Orbiting stars with persistent luminous trails and per-star FFT reactivity`.

**Verify**: File exists with correct guard `STAR_TRAIL_H`.

---

### Wave 2: Parallel Implementation

#### Task 2.1: Create star_trail.cpp

**Files**: `src/effects/star_trail.cpp`
**Depends on**: Wave 1 complete

**Do**: Implement following `muons.cpp` structure exactly (same ping-pong + FFT + gradient LUT pattern). Includes: own header, `"audio/audio.h"`, `"automation/mod_sources.h"`, `"automation/modulation_engine.h"`, `"config/constants.h"`, `"config/effect_config.h"`, `"config/effect_descriptor.h"`, `"imgui.h"`, `"render/blend_compositor.h"`, `"render/blend_mode.h"`, `"render/color_lut.h"`, `"render/post_effect.h"`, `"render/render_utils.h"`, `"ui/imgui_panels.h"`, `"ui/modulatable_slider.h"`, `"ui/ui_units.h"`, `<math.h>`, `<stddef.h>`.

Functions to implement:

1. `static void InitPingPong(StarTrailEffect *e, int width, int height)` - calls `RenderUtilsInitTextureHDR` for both textures with tag `"STAR_TRAIL"`
2. `static void UnloadPingPong(const StarTrailEffect *e)` - unloads both render textures
3. `StarTrailEffectInit` - load shader `"shaders/star_trail.fs"`, cache all uniform locations, init gradient LUT via `ColorLUTInit(&cfg->gradient)`, init ping-pong, clear both textures, set `readIdx = 0`, `time = 0.0f`, `variationTime = 0.0f`. Cleanup cascade on failure: unload shader if LUT fails.
4. `StarTrailEffectSetup` - CPU accumulates `e->time += cfg->orbitSpeed * deltaTime` and `e->variationTime += cfg->speedVariation * deltaTime` (two accumulators to match research's additive speed formula without causing jumps). Store `fftTexture` in `e->currentFFTTexture`. Call `ColorLUTUpdate`. Bind all scalar uniforms including `time` and `variationTime`. Do NOT pass `speedVariation` as a shader uniform (it is consumed by CPU accumulation only). Compute `decayFactor = expf(-0.693147f * deltaTime / fmaxf(cfg->decayHalfLife, 0.001f))`. Bind `sampleRate` as `(float)AUDIO_SAMPLE_RATE`.
5. `StarTrailEffectRender` - ping-pong swap, BeginTextureMode/BeginShaderMode, bind textures (previousFrame, gradientLUT, fftTexture) AFTER begin calls, `RenderUtilsDrawFullscreenQuad`, end modes, swap readIdx. Follow `MuonsEffectRender` exactly.
6. `StarTrailEffectResize` - unload + reinit ping-pong, reset readIdx
7. `StarTrailEffectUninit` - unload shader, `ColorLUTUninit`, unload ping-pong
8. `StarTrailRegisterParams` - register all modulatable params from the Parameters table. Use `"starTrail."` prefix.

Three bridge functions (NOT static):
- `SetupStarTrail(PostEffect *pe)` - calls `StarTrailEffectSetup` with `pe->fftTexture`
- `SetupStarTrailBlend(PostEffect *pe)` - calls `BlendCompositorApply` with ping-pong read texture
- `RenderStarTrail(PostEffect *pe)` - calls `StarTrailEffectRender`

UI section (`// === UI ===`):

`static void DrawStarTrailParams(EffectConfig *e, const ModSources *modSources, ImU32 categoryGlow)`:
- `(void)categoryGlow;`
- Stars section: `ImGui::SliderInt("Stars##starTrail", ...)` for starCount (50-500). `ImGui::Combo("Freq Mapping##starTrail", ...)` with labels `{"Radius", "Hash", "Angle"}`. `ModulatableSlider("Spread Radius##starTrail", ...)`.
- Orbit section: `ModulatableSlider` for orbitSpeed, speedWaviness, speedVariation.
- Appearance section: `ModulatableSlider` for glowSize (`"%.3f"`), glowIntensity (`"%.2f"`), decayHalfLife (`"%.1f"`).
- Audio section (standard order): baseFreq (`"%.1f"`), maxFreq (`"%.0f"`), gain (`"%.1f"`), curve (`"%.2f"`), baseBright (`"%.2f"`).

Registration:

```cpp
STANDARD_GENERATOR_OUTPUT(starTrail)
REGISTER_GENERATOR_FULL(TRANSFORM_STAR_TRAIL_BLEND, StarTrail, starTrail,
                        "Star Trail", SetupStarTrailBlend, SetupStarTrail,
                        RenderStarTrail, 15, DrawStarTrailParams,
                        DrawOutput_starTrail)
```

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 2.2: Create star_trail.fs shader

**Files**: `shaders/star_trail.fs`
**Depends on**: Wave 1 complete (for uniform name reference only)

**Do**: Implement the Algorithm section. The shader file must begin with the attribution comment block (CC BY-NC-SA 3.0), then `#version 330`.

Uniforms: `resolution` (vec2), `time` (float), `variationTime` (float), `previousFrame` (sampler2D), `starCount` (int), `freqMapMode` (int), `spreadRadius` (float), `speedWaviness` (float), `glowSize` (float), `glowIntensity` (float), `decayFactor` (float), `gradientLUT` (sampler2D), `fftTexture` (sampler2D), `sampleRate` (float), `baseFreq` (float), `maxFreq` (float), `gain` (float), `curve` (float), `baseBright` (float). Note: `speedVariation` and `orbitSpeed` are NOT shader uniforms -- they are consumed by CPU accumulation into `time` and `variationTime`.

Copy the `hash11` and `smootherstep` functions verbatim from the Algorithm section. Implement `main()` exactly as shown in the Algorithm section. The `for` loop uses `starCount` directly as the bound (GLSL 330 supports dynamic loop bounds).

**Verify**: File exists, begins with attribution, has `#version 330`.

---

#### Task 2.3: Integrate into effect_config.h

**Files**: `src/config/effect_config.h`
**Depends on**: Wave 1 complete

**Do**:
1. Add `#include "effects/star_trail.h"` with the other effect includes (alphabetical)
2. Add `TRANSFORM_STAR_TRAIL_BLEND,` to `TransformEffectType` enum, after `TRANSFORM_SPARK_FLASH_BLEND`
3. Add `StarTrailConfig starTrail;` to `EffectConfig` struct (alphabetical among other config members)

The order array auto-populates from the enum -- no manual insertion needed.

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 2.4: Integrate into post_effect.h

**Files**: `src/render/post_effect.h`
**Depends on**: Wave 1 complete

**Do**:
1. Add `#include "effects/star_trail.h"` with the other effect includes (alphabetical)
2. Add `StarTrailEffect starTrail;` to `PostEffect` struct (alphabetical among other effect members)

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 2.5: Add to CMakeLists.txt

**Files**: `CMakeLists.txt`
**Depends on**: Wave 1 complete

**Do**: Add `src/effects/star_trail.cpp` to the `EFFECTS_SOURCES` list (alphabetical).

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 2.6: Add serialization

**Files**: `src/config/effect_serialization.cpp`
**Depends on**: Wave 1 complete

**Do**:
1. Add `#include "effects/star_trail.h"` with the other effect includes (alphabetical)
2. Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(StarTrailConfig, STAR_TRAIL_CONFIG_FIELDS)` with the other per-config macros
3. Add `X(starTrail)` to the `EFFECT_CONFIG_FIELDS(X)` X-macro table

**Verify**: `cmake.exe --build build` compiles.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] Effect appears in Scatter section (section 15) with "GEN" badge
- [ ] Stars orbit and leave fading trails
- [ ] Freq Mapping combo switches between Radius/Hash/Angle modes
- [ ] FFT reactivity visible: stars pulse brighter when their frequency is active
- [ ] Gradient LUT colors applied correctly
- [ ] Preset save/load preserves all settings
- [ ] Modulation routes to registered parameters
