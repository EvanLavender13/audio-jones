# Triskelion

Fractal tiled grid generator — neon circles blooming through iterated space-folding. Each iteration rotates and scales the coordinate space, folds into a tiling cell (hex or square), then draws interfering concentric circles colored via gradient LUT. Four independent phase accumulators (rotation, scale, color, circle) create dissonant animation. Bass drives the large-scale structure, treble sparkles in the fine detail. Based on "Triskelion" by rrrola (CC BY-NC-SA 3.0).

**Research**: `docs/research/triskelion.md`

## Design

### Types

**TriskelionConfig** (`src/effects/triskelion.h`):

```cpp
struct TriskelionConfig {
  bool enabled = false;

  // Geometry
  int foldMode = 1;            // Tiling type: 0=square, 1=hex
  int layers = 16;             // Iteration depth (4-32)
  float circleFreq = 5.0f;    // Concentric circle ring density (1.0-20.0)

  // Animation
  float rotationSpeed = 0.25f;    // Fold matrix rotation rate (rad/s, -PI..PI)
  float scaleSpeed = 0.15f;       // Scale oscillation rate (rad/s, -PI..PI)
  float scaleAmount = 0.1f;       // Scale deviation from 1.0 (0.01-0.3)
  float colorSpeed = 0.15f;       // Color cycling rate (rad/s, -PI..PI)
  float circleSpeed = 0.35f;      // Circle interference phase rate (rad/s, -PI..PI)

  // Audio
  float baseFreq = 55.0f;     // FFT low frequency bound Hz (27.5-440)
  float maxFreq = 14000.0f;   // FFT high frequency bound Hz (1000-16000)
  float gain = 2.0f;          // FFT magnitude multiplier (0.1-10)
  float curve = 1.5f;         // FFT response power curve (0.1-3.0)
  float baseBright = 0.15f;   // Minimum brightness when silent (0.0-1.0)

  // Color
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};

  // Blend
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f;  // Blend strength (0-5)
};
```

**TriskelionEffect** (`src/effects/triskelion.h`):

```cpp
typedef struct TriskelionEffect {
  Shader shader;
  ColorLUT *gradientLUT;
  float rotationAngle;   // accumulated rotation
  float scaleAngle;      // accumulated scale oscillation
  float colorPhase;      // accumulated color cycling
  float circlePhase;     // accumulated circle interference phase
  int resolutionLoc;
  int gradientLUTLoc;
  int fftTextureLoc;
  int foldModeLoc;
  int layersLoc;
  int circleFreqLoc;
  int rotationAngleLoc;
  int scaleAngleLoc;
  int scaleAmountLoc;
  int colorPhaseLoc;
  int circlePhaseLoc;
  int sampleRateLoc;
  int baseFreqLoc;
  int maxFreqLoc;
  int gainLoc;
  int curveLoc;
  int baseBrightLoc;
} TriskelionEffect;
```

**Config fields macro**: `enabled, foldMode, layers, circleFreq, rotationSpeed, scaleSpeed, scaleAmount, colorSpeed, circleSpeed, baseFreq, maxFreq, gain, curve, baseBright, gradient, blendMode, blendIntensity`

### Algorithm

Full shader (`shaders/triskelion.fs`). Transcribed from the research doc's Algorithm section. Every substitution annotated with the reference line it replaces.

```glsl
// Based on "Triskelion" by rrrola
// https://www.shadertoy.com/view/dl3SRr
// License: CC BY-NC-SA 3.0 Unported
// Modified: AudioJones generator with gradient LUT, FFT reactivity, fold mode uniform
#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform vec2 resolution;
uniform sampler2D gradientLUT;
uniform sampler2D fftTexture;
uniform int foldMode;          // 0=square, 1=hex
uniform int layers;
uniform float circleFreq;
uniform float rotationAngle;   // CPU-accumulated rotation phase
uniform float scaleAngle;      // CPU-accumulated scale oscillation phase
uniform float scaleAmount;
uniform float colorPhase;      // CPU-accumulated color cycling phase
uniform float circlePhase;     // CPU-accumulated circle interference phase
uniform float sampleRate;
uniform float baseFreq;
uniform float maxFreq;
uniform float gain;
uniform float curve;
uniform float baseBright;

const int BAND_SAMPLES = 4;

// Fold space into a vertex-centered grid.
// Both paths verbatim from reference fold().
vec2 fold(vec2 p) {
  if (foldMode == 0) {
    // Square cells (reference: #ifdef TETRASKELION path)
    return fract(p) - 0.5;
  } else {
    // Hexagonal cells (reference: #else path)
    vec4 m = vec4(2.0, -1.0, 0.0, sqrt(3.0));
    p.y += m.w / 3.0;
    vec2 t = mat2(m) * p;
    return p - 0.5 * mat2(m.xzyw) * round((floor(t) + ceil(t.x + t.y)) / 3.0);
  }
}

void main() {
  // Centered coordinates (reference: (2.0*fragCoord - iResolution.xy) / iResolution.y)
  vec2 p = fragTexCoord * 2.0 - 1.0;
  p.x *= resolution.x / resolution.y;

  // Reference: float d = 0.5*length(p)
  float d = 0.5 * length(p);

  // Reference: mat2(cos(t),sin(t),-sin(t),cos(t)) * (1.0 - 0.1*cos(t2))
  // Replace: t -> rotationAngle, 0.1 -> scaleAmount, t2 -> scaleAngle
  float ca = cos(rotationAngle), sa = sin(rotationAngle);
  mat2 M = mat2(ca, sa, -sa, ca) * (1.0 - scaleAmount * cos(scaleAngle));

  float fLayers = float(layers);
  float logRatio = log(maxFreq / baseFreq);

  // Reference: for (float i = 0.0; i < 24.0; i++)
  // Replace: 24.0 -> layers
  vec3 sum = vec3(0.0);
  for (int i = 0; i < layers; i++) {
    float fi = float(i);

    // Reference: p = fold(M * p)  (verbatim)
    p = fold(M * p);

    // FFT: map iteration i to a frequency band in log space (standard generator pattern)
    float t0 = fi / fLayers;
    float t1 = (fi + 1.0) / fLayers;
    float freqLo = baseFreq * exp(t0 * logRatio);
    float freqHi = baseFreq * exp(t1 * logRatio);
    float binLo = freqLo / (sampleRate * 0.5);
    float binHi = freqHi / (sampleRate * 0.5);
    float energy = 0.0;
    for (int b = 0; b < BAND_SAMPLES; b++) {
      float bin = mix(binLo, binHi, (float(b) + 0.5) / float(BAND_SAMPLES));
      if (bin <= 1.0)
        energy += texture(fftTexture, vec2(bin, 0.5)).r;
    }
    float bright = baseBright +
        pow(clamp(energy / float(BAND_SAMPLES) * gain, 0.0, 1.0), curve);

    // Reference: pal(0.01*i - d + t2)
    // Replace: pal() -> gradientLUT, 0.01*i -> fi/fLayers, t2 -> colorPhase
    vec3 col = texture(gradientLUT, vec2(fract(fi / fLayers - d + colorPhase), 0.5)).rgb;

    // Reference: sum += pal(...) / cos(d - t3 + 5.0*length(p))
    // Replace: t3 -> circlePhase, 5.0 -> circleFreq
    // bright multiplier adds FFT reactivity per iteration
    sum += col * bright / cos(d - circlePhase + circleFreq * length(p));
  }

  // Reference: fragColor = vec4(0.0002*sum*sum, 1)  (tuned for 24 iterations)
  // Normalize sum to 24-iteration equivalent so 0.0002 constant stays valid at any layer count
  sum *= 24.0 / fLayers;
  finalColor = vec4(0.0002 * sum * sum, 1.0);
}
```

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| foldMode | int | 0-1 | 1 | No | "Fold Mode" (Combo: Square/Hex) |
| layers | int | 4-32 | 16 | No | "Layers" (SliderInt) |
| circleFreq | float | 1.0-20.0 | 5.0 | Yes | "Circle Freq" |
| rotationSpeed | float | -ROTATION_SPEED_MAX..ROTATION_SPEED_MAX | 0.25 | Yes | "Rotation Speed" (SpeedDeg) |
| scaleSpeed | float | -ROTATION_SPEED_MAX..ROTATION_SPEED_MAX | 0.15 | Yes | "Scale Speed" (SpeedDeg) |
| scaleAmount | float | 0.01-0.3 | 0.1 | Yes | "Scale Amount" |
| colorSpeed | float | -ROTATION_SPEED_MAX..ROTATION_SPEED_MAX | 0.15 | Yes | "Color Speed" (SpeedDeg) |
| circleSpeed | float | -ROTATION_SPEED_MAX..ROTATION_SPEED_MAX | 0.35 | Yes | "Circle Speed" (SpeedDeg) |
| baseFreq | float | 27.5-440 | 55.0 | Yes | "Base Freq (Hz)" |
| maxFreq | float | 1000-16000 | 14000.0 | Yes | "Max Freq (Hz)" |
| gain | float | 0.1-10 | 2.0 | Yes | "Gain" |
| curve | float | 0.1-3.0 | 1.5 | Yes | "Contrast" |
| baseBright | float | 0.0-1.0 | 0.15 | Yes | "Base Bright" |
| blendIntensity | float | 0.0-5.0 | 1.0 | Yes | "Blend Intensity" |

### Constants

- Enum name: `TRANSFORM_TRISKELION_BLEND`
- Display name: `"Triskelion"`
- Category badge: `"GEN"` (auto from `REGISTER_GENERATOR`)
- Section index: `10` (Geometric)

---

## Tasks

### Wave 1: Header

#### Task 1.1: Config and Effect header

**Files**: `src/effects/triskelion.h`
**Creates**: `TriskelionConfig`, `TriskelionEffect` types, function declarations

**Do**: Create `triskelion.h`. Follow `spectral_rings.h` as structural pattern. Define `TriskelionConfig` and `TriskelionEffect` exactly as specified in the Design section. Define `TRISKELION_CONFIG_FIELDS` macro. Declare Init (takes `TriskelionEffect*` + `const TriskelionConfig*`), Setup (takes `TriskelionEffect*`, `TriskelionConfig*`, `float deltaTime`, `Texture2D fftTexture`), Uninit, ConfigDefault, RegisterParams. Include `"raylib.h"`, `"render/blend_mode.h"`, `"render/color_config.h"`, `<stdbool.h>`. Forward-declare `ColorLUT`. Top-of-file comment: "Triskelion generator — fractal tiled grid with iterated space-folding and concentric circle interference".

**Verify**: `cmake.exe --build build` compiles (header-only, no linker dependency yet).

---

### Wave 2: Parallel implementation

#### Task 2.1: Effect implementation

**Files**: `src/effects/triskelion.cpp`
**Depends on**: Wave 1

**Do**: Create `triskelion.cpp`. Follow `spectral_rings.cpp` as structural pattern.

- `TriskelionEffectInit`: Load `shaders/triskelion.fs`, cache all uniform locations listed in `TriskelionEffect`, init `gradientLUT` via `ColorLUTInit(&cfg->gradient)`. Cascade cleanup on failure. Init all four accumulators to 0.
- `TriskelionEffectSetup`: Accumulate four phases: `rotationAngle += rotationSpeed * dt`, `scaleAngle += scaleSpeed * dt`, `colorPhase += colorSpeed * dt`, `circlePhase += circleSpeed * dt`. Call `ColorLUTUpdate`. Bind all uniforms. Bind `fftTexture` and `gradientLUT` textures. `sampleRate` from `AUDIO_SAMPLE_RATE`.
- `TriskelionEffectUninit`: Unload shader, `ColorLUTUninit`.
- `TriskelionConfigDefault`: Return `TriskelionConfig{}`.
- `TriskelionRegisterParams`: Register all modulatable params from the Parameters table. Use `"triskelion.*"` IDs. Use `ROTATION_SPEED_MAX` bounds for all four speed params.
- Bridge functions: `SetupTriskelion(PostEffect* pe)` delegates to `TriskelionEffectSetup`. `SetupTriskelionBlend(PostEffect* pe)` calls `BlendCompositorApply`.
- UI section (`// === UI ===`): `DrawTriskelionParams` with sections:
  - **Geometry**: Fold Mode combo (Square/Hex), Layers `SliderInt`, Circle Freq `ModulatableSlider`
  - **Animation**: Rotation Speed `ModulatableSliderSpeedDeg`, Scale Speed `ModulatableSliderSpeedDeg`, Scale Amount `ModulatableSlider`, Color Speed `ModulatableSliderSpeedDeg`, Circle Speed `ModulatableSliderSpeedDeg`
  - **Audio**: Standard order — Base Freq, Max Freq, Gain, Contrast, Base Bright (conventions: `ModulatableSlider`, not log)
- Bottom: `STANDARD_GENERATOR_OUTPUT(triskelion)` then `REGISTER_GENERATOR(TRANSFORM_TRISKELION_BLEND, Triskelion, triskelion, "Triskelion", SetupTriskelionBlend, SetupTriskelion, 10, DrawTriskelionParams, DrawOutput_triskelion)` wrapped in `// clang-format off/on`.

Includes: `"triskelion.h"`, `"audio/audio.h"`, `"automation/mod_sources.h"`, `"automation/modulation_engine.h"`, `"config/constants.h"`, `"config/effect_config.h"`, `"config/effect_descriptor.h"`, `"imgui.h"`, `"render/blend_compositor.h"`, `"render/blend_mode.h"`, `"render/color_lut.h"`, `"render/post_effect.h"`, `"ui/imgui_panels.h"`, `"ui/modulatable_slider.h"`, `"ui/ui_units.h"`, `<stddef.h>`.

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 2.2: Fragment shader

**Files**: `shaders/triskelion.fs`
**Depends on**: Wave 1 (for uniform name agreement only — no #include)

**Do**: Create `triskelion.fs`. Implement the Algorithm section verbatim — attribution block, all uniforms, fold function with both paths, main loop with FFT band sampling, gradient LUT color, concentric circle interference, and `sum *= 24.0/fLayers; 0.0002 * sum * sum` normalization. Copy exactly.

**Verify**: Shader file exists with correct uniforms matching the `GetShaderLocation` calls in Task 2.1.

---

#### Task 2.3: Effect config registration

**Files**: `src/config/effect_config.h`
**Depends on**: Wave 1

**Do**:
1. Add `#include "effects/triskelion.h"` with the other effect includes (alphabetical)
2. Add `TRANSFORM_TRISKELION_BLEND,` to `TransformEffectType` enum before `TRANSFORM_EFFECT_COUNT` (the order array auto-initializes from the enum)
3. Add `TriskelionConfig triskelion;` to the `EffectConfig` struct with the other config members

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 2.4: PostEffect struct

**Files**: `src/render/post_effect.h`
**Depends on**: Wave 1

**Do**:
1. Add `#include "effects/triskelion.h"` with other effect includes
2. Add `TriskelionEffect triskelion;` member to `PostEffect` struct (near other generator effect members)

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 2.5: Build system

**Files**: `CMakeLists.txt`
**Depends on**: Wave 1

**Do**: Add `src/effects/triskelion.cpp` to `EFFECTS_SOURCES` (alphabetical order).

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 2.6: Preset serialization

**Files**: `src/config/effect_serialization.cpp`
**Depends on**: Wave 1

**Do**:
1. Add `#include "effects/triskelion.h"` with other effect includes
2. Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(TriskelionConfig, TRISKELION_CONFIG_FIELDS)` with the other per-config macros
3. Add `X(triskelion)` to the `EFFECT_CONFIG_FIELDS(X)` macro (append after the last entry)

**Verify**: `cmake.exe --build build` compiles.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] Triskelion appears in the Geometric generator section of the effects panel
- [ ] Fold Mode combo switches between Square / Hex tiling
- [ ] Layers slider changes fractal iteration depth
- [ ] Four animation speeds (rotation/scale/color/circle) produce dissonant morphing
- [ ] FFT reactivity: bass drives large-scale brightness, treble sparkles in detail
- [ ] Gradient LUT colors the pattern (gradient editor changes are reflected)
- [ ] Preset save/load round-trips all parameters
- [ ] Modulation routes to registered parameters

---

## Implementation Notes

### Gradient LUT Integration

The reference `pal()` function (`0.5 + cos(3*a + vec3(2,1,0))`) creates per-channel phase-offset oscillation in [-0.5, 1.5]. Each RGB channel cancels independently across iterations, producing thin neon lines with color variation. A gradient LUT (always [0,1], correlated channels) cannot replicate this interference behavior directly.

**Solution**: Scalar accumulation with reference-style cosine oscillation for interference, gradient applied post-loop as radial color mapping.

- `osc = 0.5 + cos(3*(0.01*fi - d + colorPhase))` — scalar, goes negative, creates proper cancellation
- Per-iteration FFT `bright` multiplies `osc` — cancellation preserved because `osc` still goes negative
- Post-loop: gradient sampled via ping-pong (`1 - abs(mod(x,2) - 1)`) to avoid `fract()` wrap discontinuities
- Exponential tonemap (`1 - exp(-col * brightness)`) washes singularity spikes to white fringes like the reference

### Added Parameter: colorCycles

Not in the original reference — added to control radial gradient ring density. Default 3.0 (matching reference `cos(3*a)` frequency), range 0.1–10.0. Drives the ping-pong gradient mapping coordinate.

### Normalization

Reference uses `0.0002 * sum*sum` tuned for 24 fixed iterations. Derived layer-independent constant: `val /= fLayers; brightness = 0.1152 * val * val` where `0.1152 = 0.0002 * 24²`.
