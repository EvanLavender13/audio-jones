# Spectral Rings

Dense concentric rings radiating from center, each band lit by its corresponding FFT frequency. Colors sampled from gradient LUT with noise perturbation for organic variation. Rings expand/contract with a pulse parameter, colors scroll independently, and the whole structure can skew from perfect circles to off-center ellipses. Per-pixel evaluation (no loop) — ring pattern via `fract()`, FFT lookup quantized by `layers`.

**Research**: `docs/research/spectral_rings.md`

## Design

### Types

```cpp
// src/effects/spectral_rings.h

struct SpectralRingsConfig {
  bool enabled = false;

  // Ring layout
  float ringDensity = 24.0f;    // Visible ring bands (4.0-64.0)
  float ringWidth = 0.5f;       // Bright band vs dark gap (0.05-1.0)
  int layers = 48;              // FFT sampling resolution across radius (8-128)

  // Animation
  float pulseSpeed = 0.0f;      // Ring expansion/contraction rate (rads/sec, -2.0-2.0)
  float colorShiftSpeed = 0.1f; // LUT scroll speed (rads/sec, -2.0-2.0)
  float rotationSpeed = 0.1f;   // Ring structure rotation rate (rads/sec, -PI-PI)

  // Eccentricity
  float eccentricity = 0.0f;    // Circle-to-ellipse deformation (0.0-0.8)
  float skewAngle = 0.0f;       // Ellipse major axis angle (radians, -PI-PI)

  // Noise
  float noiseAmount = 0.15f;    // Color variation within/between rings (0.0-0.5)
  float noiseScale = 5.0f;      // Noise spatial frequency (1.0-20.0)

  // FFT mapping
  float baseFreq = 55.0f;       // Low frequency bound Hz (27.5-440)
  float maxFreq = 14000.0f;     // High frequency bound Hz (1000-16000)
  float gain = 2.0f;            // FFT amplitude multiplier (0.1-10)
  float curve = 1.5f;           // FFT response curve (0.1-3.0)
  float baseBright = 0.15f;     // Minimum brightness when silent (0.0-1.0)

  // Color
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};

  // Blend
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f;  // Blend strength (0-5)
};

typedef struct SpectralRingsEffect {
  Shader shader;
  ColorLUT *gradientLUT;
  float pulseAccum;
  float colorShiftAccum;
  float rotationAccum;
  float time;
  int resolutionLoc;
  int fftTextureLoc;
  int sampleRateLoc;
  int timeLoc;
  int ringDensityLoc;
  int ringWidthLoc;
  int layersLoc;
  int pulseAccumLoc;
  int colorShiftAccumLoc;
  int rotationAccumLoc;
  int eccentricityLoc;
  int skewAngleLoc;
  int noiseAmountLoc;
  int noiseScaleLoc;
  int baseFreqLoc;
  int maxFreqLoc;
  int gainLoc;
  int curveLoc;
  int baseBrightLoc;
  int gradientLUTLoc;
} SpectralRingsEffect;
```

### Algorithm

The shader is per-pixel with no loop. Ring pattern is created by `fract()`, FFT lookup is quantized by `layers`, and color is sampled from the gradient LUT with 2D value noise perturbation.

```glsl
// --- Noise utilities (inline) ---

float hash21(vec2 p) {
    p = fract(p * vec2(123.34, 456.21));
    p += dot(p, p + 45.32);
    return fract(p.x * p.y);
}

float noise2D(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    float a = hash21(i);
    float b = hash21(i + vec2(1.0, 0.0));
    float c = hash21(i + vec2(0.0, 1.0));
    float d = hash21(i + vec2(1.0, 1.0));
    return mix(mix(a, b, f.x), mix(c, d, f.x), f.y);
}

// --- Main ---

void main() {
    // 1. Center coordinates (convention: origin at screen center)
    vec2 pos = fragTexCoord * resolution - resolution * 0.5;

    // 2. Apply eccentricity (parameterized from reference's mat2(2,1,-2,4))
    //    skewAngle rotates the stretch axis; eccentricity controls magnitude
    float cs = cos(skewAngle), sn = sin(skewAngle);
    mat2 eccMat = mat2(1.0 + eccentricity * cs, eccentricity * sn,
                       -eccentricity * sn, 1.0 - eccentricity * cs);
    pos = eccMat * pos;

    // 3. Apply rotation (CPU-accumulated from rotationSpeed)
    float rc = cos(rotationAccum), rs = sin(rotationAccum);
    pos = mat2(rc, rs, -rs, rc) * pos;

    // 4. Radial distance normalized to [0,1]
    float maxRadius = min(resolution.x, resolution.y) * 0.5;
    float dist = length(pos) / maxRadius;

    // 5. Ring band brightness
    //    fract() creates repeating bands; pulseAccum shifts them outward/inward
    //    smoothstep edges give sharp rings with anti-aliased transitions
    float band = fract(dist * ringDensity + pulseAccum);
    float aa = fwidth(dist * ringDensity);
    float ringBright = smoothstep(0.0, aa, band)
                     * (1.0 - smoothstep(ringWidth - aa, ringWidth, band));

    // 6. FFT mapping — quantize distance into layers frequency bands
    //    Each pixel's radial distance maps to a discrete frequency band
    //    BAND_SAMPLES sub-samples within each band for smooth FFT reading
    float fl = float(layers);
    float bandIdx = clamp(floor(dist * fl), 0.0, fl - 1.0);
    float t0 = bandIdx / fl;
    float t1 = (bandIdx + 1.0) / fl;
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
    float fftBright = baseBright + mag;

    // 7. Color sampling with noise perturbation
    //    LUT position scrolls independently of ring pulse via colorShiftAccum
    //    noise2D breaks uniform banding for organic variation
    float lutU = dist * ringDensity + colorShiftAccum;
    vec2 noisePos = (pos / maxRadius) * noiseScale + vec2(time * 0.1);
    lutU += noise2D(noisePos) * noiseAmount;
    vec3 color = texture(gradientLUT, vec2(fract(lutU), 0.5)).rgb;

    // 8. Composite + fifth-root contrast (from reference's pow(O, 0.2))
    vec3 result = color * ringBright * fftBright;
    finalColor = vec4(pow(result, vec3(0.2)), 1.0);
}
```

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| ringDensity | float | 4.0-64.0 | 24.0 | Yes | "Ring Density" |
| ringWidth | float | 0.05-1.0 | 0.5 | Yes | "Ring Width" |
| layers | int | 8-128 | 48 | No | "Layers" |
| pulseSpeed | float | -2.0-2.0 | 0.0 | Yes | "Pulse Speed" |
| colorShiftSpeed | float | -2.0-2.0 | 0.1 | Yes | "Color Shift" |
| rotationSpeed | float | -PI-PI | 0.1 | Yes | "Rotation Speed" |
| eccentricity | float | 0.0-0.8 | 0.0 | Yes | "Eccentricity" |
| skewAngle | float | -PI-PI | 0.0 | Yes | "Skew Angle" |
| noiseAmount | float | 0.0-0.5 | 0.15 | Yes | "Noise Amount" |
| noiseScale | float | 1.0-20.0 | 5.0 | Yes | "Noise Scale" |
| baseFreq | float | 27.5-440 | 55.0 | Yes | "Base Freq (Hz)" |
| maxFreq | float | 1000-16000 | 14000.0 | Yes | "Max Freq (Hz)" |
| gain | float | 0.1-10.0 | 2.0 | Yes | "Gain" |
| curve | float | 0.1-3.0 | 1.5 | Yes | "Contrast" |
| baseBright | float | 0.0-1.0 | 0.15 | Yes | "Base Bright" |

**UI Slider Types:**
- `ringDensity`, `ringWidth`, `noiseAmount`, `noiseScale`, `gain`, `curve`, `baseBright`: `ModulatableSlider`
- `pulseSpeed`, `colorShiftSpeed`: `ModulatableSlider` with `"%.2f"` (linear rate, not angular)
- `rotationSpeed`: `ModulatableSliderSpeedDeg`
- `skewAngle`: `ModulatableSliderAngleDeg`
- `baseFreq`: `ModulatableSlider` with `"%.1f"` format
- `maxFreq`: `ModulatableSlider` with `"%.0f"` format
- `layers`: `ImGui::SliderInt`

**UI Sections (SeparatorText order):**
1. "Audio" — baseFreq, maxFreq, gain, curve (labeled "Contrast"), baseBright
2. "Geometry" — layers, ringDensity, ringWidth
3. "Eccentricity" — eccentricity, skewAngle
4. "Animation" — pulseSpeed, colorShiftSpeed, rotationSpeed
5. "Noise" — noiseAmount, noiseScale

### Constants

- Enum: `TRANSFORM_SPECTRAL_RINGS_BLEND`
- Display name: `"Spectral Rings"`
- Generator section: `10` (Geometric)
- Badge: `"GEN"` (auto-set by `REGISTER_GENERATOR`)
- Flags: `EFFECT_FLAG_BLEND` (auto-set by `REGISTER_GENERATOR`)
- Config fields macro: `SPECTRAL_RINGS_CONFIG_FIELDS`
- Modulation prefix: `"spectralRings."`

---

## Tasks

### Wave 1: Config Header

#### Task 1.1: Create effect header

**Files**: `src/effects/spectral_rings.h` (create)
**Creates**: Config struct, Effect struct, lifecycle declarations that all other files `#include`

**Do**: Create header following `iris_rings.h` as pattern. Include `"raylib.h"`, `"render/blend_mode.h"`, `"render/color_config.h"`, `<stdbool.h>`. Define `SpectralRingsConfig` and `SpectralRingsEffect` structs exactly as shown in Design > Types. Define `SPECTRAL_RINGS_CONFIG_FIELDS` macro listing all serializable fields. Forward-declare `ColorLUT`. Declare Init (takes `SpectralRingsEffect*` + `const SpectralRingsConfig*`), Setup (takes effect, config, deltaTime, fftTexture), Uninit, ConfigDefault, RegisterParams.

**Verify**: `cmake.exe --build build` compiles (header-only, no linker dependency yet).

---

### Wave 2: Parallel Implementation

#### Task 2.1: Effect implementation

**Files**: `src/effects/spectral_rings.cpp` (create)
**Depends on**: Wave 1 complete

**Do**: Create implementation following `iris_rings.cpp` as pattern. Includes: own header, `"audio/audio.h"`, `"automation/mod_sources.h"`, `"automation/modulation_engine.h"`, `"config/constants.h"`, `"config/effect_config.h"`, `"config/effect_descriptor.h"`, `"imgui.h"`, `"render/blend_compositor.h"`, `"render/color_lut.h"`, `"render/post_effect.h"`, `"ui/imgui_panels.h"`, `"ui/modulatable_slider.h"`, `"ui/ui_units.h"`, `<stddef.h>`.

**Init**: Load shader `"shaders/spectral_rings.fs"`, cache all uniform locations (20 uniforms: resolution, fftTexture, sampleRate, time, ringDensity, ringWidth, layers, pulseAccum, colorShiftAccum, rotationAccum, eccentricity, skewAngle, noiseAmount, noiseScale, baseFreq, maxFreq, gain, curve, baseBright, gradientLUT), init `ColorLUT` from config gradient. Zero all accumulators. On shader fail return false. On LUT fail, unload shader, return false.

**Setup**: Accumulate `pulseAccum += pulseSpeed * deltaTime`, `colorShiftAccum += colorShiftSpeed * deltaTime`, `rotationAccum += rotationSpeed * deltaTime`, `time += deltaTime`. Call `ColorLUTUpdate`. Bind all uniforms. Bind `fftTexture` and `gradientLUT` textures. Use `AUDIO_SAMPLE_RATE` for sampleRate.

**Uninit**: `UnloadShader` + `ColorLUTUninit`.

**RegisterParams**: Register all 13 modulatable float params (all except `layers`) with `"spectralRings."` prefix and ranges from the Parameters table. Include `blendIntensity`.

**ConfigDefault**: `return SpectralRingsConfig{};`

**UI section** (`// === UI ===`): `static void DrawSpectralRingsParams(EffectConfig*, const ModSources*, ImU32)`. Five SeparatorText sections in order: Audio, Geometry, Eccentricity, Animation, Noise. Use slider types from Parameters > UI Slider Types.

**Bridge functions**: Non-static `SetupSpectralRings(PostEffect* pe)` delegating to `SpectralRingsEffectSetup`. Non-static `SetupSpectralRingsBlend(PostEffect* pe)` calling `BlendCompositorApply`.

**Registration**: `STANDARD_GENERATOR_OUTPUT(spectralRings)` then `REGISTER_GENERATOR(TRANSFORM_SPECTRAL_RINGS_BLEND, SpectralRings, spectralRings, "Spectral Rings", SetupSpectralRingsBlend, SetupSpectralRings, 10, DrawSpectralRingsParams, DrawOutput_spectralRings)`.

**Verify**: Compiles.

---

#### Task 2.2: Fragment shader

**Files**: `shaders/spectral_rings.fs` (create)
**Depends on**: Wave 1 complete

**Do**: Create fragment shader. Begin with attribution comment block (XorDev, shadertoy.com/view/sstyDX, CC BY-NC-SA 3.0, modified for parameterized generator with FFT/LUT). Then `#version 330`. Declare all uniforms matching the loc names from Design > Types. Implement the Algorithm section verbatim — noise utilities (`hash21`, `noise2D`), then `main()` with all 8 steps. Use `#define PI 3.14159265359`.

**Verify**: Shader file exists, uniforms match cpp loc names.

---

#### Task 2.3: Config registration

**Files**: `src/config/effect_config.h` (modify)
**Depends on**: Wave 1 complete

**Do**: Four changes following the pattern of `TRANSFORM_IRIS_RINGS_BLEND` / `IrisRingsConfig irisRings`:
1. Add `#include "effects/spectral_rings.h"` with the other effect includes
2. Add `TRANSFORM_SPECTRAL_RINGS_BLEND,` to `TransformEffectType` enum before `TRANSFORM_EFFECT_COUNT`
3. Add `TRANSFORM_SPECTRAL_RINGS_BLEND` to the `TransformOrderConfig::order` array (at end, before closing brace)
4. Add `SpectralRingsConfig spectralRings;` to `EffectConfig` struct

**Verify**: Compiles.

---

#### Task 2.4: PostEffect struct

**Files**: `src/render/post_effect.h` (modify)
**Depends on**: Wave 1 complete

**Do**: Two changes following iris_rings pattern:
1. Add `#include "effects/spectral_rings.h"` with other effect includes
2. Add `SpectralRingsEffect spectralRings;` member to `PostEffect` struct

**Verify**: Compiles.

---

#### Task 2.5: Build system

**Files**: `CMakeLists.txt` (modify)
**Depends on**: Wave 1 complete

**Do**: Add `src/effects/spectral_rings.cpp` to `EFFECTS_SOURCES` list.

**Verify**: Compiles.

---

#### Task 2.6: Preset serialization

**Files**: `src/config/effect_serialization.cpp` (modify)
**Depends on**: Wave 1 complete

**Do**: Three changes following iris_rings pattern:
1. Add `#include "effects/spectral_rings.h"`
2. Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(SpectralRingsConfig, SPECTRAL_RINGS_CONFIG_FIELDS)`
3. Add `X(spectralRings) \` to the `EFFECT_CONFIG_FIELDS(X)` X-macro table

**Verify**: Compiles.

---

## Final Verification

- [x] Build succeeds with no warnings
- [x] Spectral Rings appears in Geometric generator section with "GEN" badge
- [x] Effect can be enabled and shows concentric rings
- [x] Rings respond to audio (FFT-reactive brightness)
- [x] Gradient LUT colors apply correctly
- [x] Pulse, color shift, and rotation animation work
- [x] Eccentricity deforms circles into ellipses
- [x] Noise perturbation creates organic color variation
- [x] Preset save/load preserves all settings
- [x] Modulation routes to registered parameters

---

## Implementation Notes

The plan's algorithm (per-pixel `fract()` rings with procedural `noise2D`) was abandoned during implementation. The procedural noise approach could not replicate the reference shader's rich 2D color variation — gradient LUT sampled via 1D noise produces smooth/bubbly output instead of the reference's intricate ring texture.

### What changed

**Algorithm**: Replaced entirely. The implementation uses the reference shader's DOF-style iteration loop (`i += 1.0/i`), golden-angle vector rotation (`mat2(73,67,-67,73)/99.0`), and per-channel power curves `vec3(5,8,9)` with fifth-root contrast `pow(O, 0.2)`. This is the core visual mechanism — the loop accumulates overlapping ring samples that create the dense concentric structure.

**Noise**: Uses the project's shared 1024x1024 RGBA noise texture (`noise_texture.h`) instead of procedural hash noise. The `.r` channel is sampled at two different coordinate transforms (radial and rotated-cartesian, matching the reference) and mapped through the gradient LUT. `noiseScale` (default 0.25) compensates for the 4x texture size difference vs the reference's 256x256 noise.

**Removed params**: `ringDensity`, `ringWidth`, `layers`, `noiseAmount` — these controlled the `fract()`-based ring pattern which no longer exists. Ring structure emerges naturally from the iteration loop.

**Added params**: `quality` (int 8-256, default 128) controls iteration count. `DualLissajousConfig lissajous` provides animated center drift.

**Renamed**: `skewAngle` → `tiltAngle` (controls ellipse stretch direction, not skew).

**Eccentricity**: Plan had `eccMat` interpolating toward the reference's fixed `mat2(2,1,-2,4)`. Implementation uses proper rotatable elliptical stretch: `invRot * mat2(1+ecc*3, 0, 0, 1) * rot` where `tiltAngle` rotates the stretch axis. Default eccentricity 0.3 gives ~1.9x stretch matching the reference.

**Center motion**: Plan had no center motion. Implementation adds DualLissajous drift with `transformedCenter = (r * 0.5) * M / r.y + centerOffset` — screen center transformed through the stretch matrix, lissajous offset added in screen space after.

**FFT**: Same BAND_SAMPLES=4 pattern but applied per-iteration at the radial distance of each sample point, not quantized into discrete `layers` bands.

**UI sections**: Audio, Shape, Center Motion, Animation, Noise, Quality (was: Audio, Geometry, Eccentricity, Animation, Noise).
