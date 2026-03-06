# Faraday Waves

FFT-driven standing wave lattice generator. Layers of plane waves arranged in lattice geometries (stripes, squares, hexagons, quasicrystals) superimpose and oscillate at half the driving frequency, creating the characteristic Faraday "bouncing" effect. Each FFT frequency band drives a separate spatial scale layer. Visually: structured, shimmering grids of peaks and troughs.

**Research**: `docs/research/faraday-waves.md`

<!-- Intentional deviation: damping parameter omitted — waveCount directly selects lattice geometry, making a separate damping crossfade redundant. -->
<!-- Intentional deviation: BAND_SAMPLES=4 band-averaged FFT replaces research's single-bin sampling — this is the project-wide generator convention (see generator_patterns.md). -->

## Design

### Types

**FaradayConfig** (in `src/effects/faraday.h`):

```cpp
struct FaradayConfig {
  bool enabled = false;

  // Wave
  int waveCount = 3;             // Lattice vectors: 1=stripes, 2=squares, 3=hexagons, 4+=quasicrystal (1-6)
  float spatialScale = 0.1f;    // Maps FFT frequency to spatial wave number density (0.01-1.0)
  float visualGain = 1.5f;      // Output intensity multiplier (0.5-5.0)
  float rotationSpeed = 0.0f;   // Lattice rotation rate radians/second (-PI..PI)
  float rotationAngle = 0.0f;   // Static lattice orientation offset radians (-PI..PI)
  int layers = 8;               // Number of frequency layers (1-16)

  // Audio
  float baseFreq = 55.0f;       // Lowest driven frequency (27.5-440)
  float maxFreq = 14000.0f;     // Highest driven frequency (1000-16000)
  float gain = 2.0f;            // FFT energy sensitivity (0.1-10.0)
  float curve = 1.5f;           // Contrast exponent (0.1-3.0)

  // Trail
  float decayHalfLife = 0.3f;   // Trail persistence seconds (0.05-5.0)
  int diffusionScale = 2;       // Spatial blur tap spacing (0-8)

  // Blend
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f;  // Blend compositor strength (0.0-5.0)
  ColorConfig gradient;
};
```

**FaradayEffect** (in `src/effects/faraday.h`):

```cpp
typedef struct FaradayEffect {
  Shader shader;
  ColorLUT *colorLUT;
  RenderTexture2D pingPong[2];
  int readIdx;
  Texture2D currentFftTexture;
  float time;
  float rotationAccum;

  // Uniform locations
  int resolutionLoc;
  int timeLoc;
  int waveCountLoc;
  int spatialScaleLoc;
  int visualGainLoc;
  int rotationOffsetLoc;
  int layersLoc;
  int fftTextureLoc;
  int sampleRateLoc;
  int baseFreqLoc;
  int maxFreqLoc;
  int gainLoc;
  int curveLoc;
  int diffusionScaleLoc;
  int decayFactorLoc;
  int gradientLUTLoc;
} FaradayEffect;
```

### Algorithm

The shader superimposes standing waves along N lattice directions for each FFT frequency layer. `waveCount` is an integer selecting the lattice geometry directly (1=stripes, 2=squares, 3=hexagons, etc.).

#### Shader core (faraday.fs)

```glsl
#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;      // ping-pong read buffer
uniform sampler2D fftTexture;
uniform sampler2D gradientLUT;
uniform vec2 resolution;
uniform float time;
uniform int waveCount;           // int, 1-6
uniform float spatialScale;
uniform float visualGain;
uniform float rotationOffset;    // rotationAngle + rotationAccum
uniform int layers;
uniform float sampleRate;
uniform float baseFreq;
uniform float maxFreq;
uniform float gain;
uniform float curve;
uniform int diffusionScale;
uniform float decayFactor;

const float PI = 3.14159265;
const int BAND_SAMPLES = 4;

// Evaluate lattice superposition for integer wave count N at position p
// with wave number k and rotation offset rot
float lattice(vec2 p, float k, int N, float rot) {
    float h = 0.0;
    for (int i = 0; i < N; i++) {
        float theta = float(i) * PI / float(N) + rot;
        vec2 dir = vec2(cos(theta), sin(theta));
        h += cos(dot(dir, p) * k);
    }
    return h / float(N);
}

void main() {
    float nyquist = sampleRate * 0.5;
    float aspect = resolution.x / resolution.y;
    vec2 uv = fragTexCoord * 2.0 - 1.0;
    uv.x *= aspect;

    float totalHeight = 0.0;
    float totalWeight = 0.0;

    for (int i = 0; i < layers; i++) {
        // Log-space frequency band for this layer
        float t0 = float(i) / float(layers - 1);
        float t1 = float(i + 1) / float(layers);
        float freqLo = baseFreq * pow(maxFreq / baseFreq, t0);
        float freqHi = baseFreq * pow(maxFreq / baseFreq, t1);
        float binLo = freqLo / nyquist;
        float binHi = freqHi / nyquist;

        // Band-averaged FFT energy (BAND_SAMPLES sub-samples)
        float energy = 0.0;
        for (int s = 0; s < BAND_SAMPLES; s++) {
            float bin = mix(binLo, binHi, (float(s) + 0.5) / float(BAND_SAMPLES));
            if (bin <= 1.0) {
                energy += texture(fftTexture, vec2(bin, 0.5)).r;
            }
        }
        float mag = pow(clamp(energy / float(BAND_SAMPLES) * gain, 0.0, 1.0), curve);
        if (mag < 0.001) continue;

        // Spatial wave number scales with frequency
        float freq = baseFreq * pow(maxFreq / baseFreq, t0);
        float k = freq * spatialScale;

        // Evaluate lattice superposition
        float layerHeight = lattice(uv, k, waveCount, rotationOffset);

        // Half-frequency temporal oscillation (Faraday signature)
        layerHeight *= cos(time * freq * 0.5);

        totalHeight += mag * layerHeight;
        totalWeight += mag;
    }

    if (totalWeight > 0.0) totalHeight /= totalWeight;

    // Visualization: tanh compression + gradient LUT
    float compressed = tanh(totalHeight * visualGain);
    float t = compressed * 0.5 + 0.5;
    vec3 color = texture(gradientLUT, vec2(t, 0.5)).rgb;
    float brightness = abs(compressed);
    vec4 newColor = vec4(color * brightness, brightness);

    // Trail persistence: diffuse + decay, keep brighter of old/new
    // (Same 5-tap separable Gaussian as Chladni)
    vec4 existing;
    if (diffusionScale == 0) {
        existing = texture(texture0, fragTexCoord);
    } else {
        vec2 texel = vec2(float(diffusionScale)) / resolution;

        vec4 h = texture(texture0, fragTexCoord + vec2(-2.0 * texel.x, 0.0)) * 0.0625
               + texture(texture0, fragTexCoord + vec2(-1.0 * texel.x, 0.0)) * 0.25
               + texture(texture0, fragTexCoord)                              * 0.375
               + texture(texture0, fragTexCoord + vec2( 1.0 * texel.x, 0.0)) * 0.25
               + texture(texture0, fragTexCoord + vec2( 2.0 * texel.x, 0.0)) * 0.0625;

        vec4 v = texture(texture0, fragTexCoord + vec2(0.0, -2.0 * texel.y)) * 0.0625
               + texture(texture0, fragTexCoord + vec2(0.0, -1.0 * texel.y)) * 0.25
               + texture(texture0, fragTexCoord)                              * 0.375
               + texture(texture0, fragTexCoord + vec2(0.0,  1.0 * texel.y)) * 0.25
               + texture(texture0, fragTexCoord + vec2(0.0,  2.0 * texel.y)) * 0.0625;

        existing = (h + v) * 0.5;
    }
    existing *= decayFactor;
    finalColor = max(existing, newColor);
}
```

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| waveCount | int | 1-6 | 3 | No | "Wave Count##faraday" |
| spatialScale | float | 0.01-1.0 | 0.1 | Yes | "Spatial Scale##faraday" |
| visualGain | float | 0.5-5.0 | 1.5 | Yes | "Vis Gain##faraday" |
| rotationSpeed | float | -PI..PI | 0.0 | Yes | ModulatableSliderSpeedDeg |
| rotationAngle | float | -PI..PI | 0.0 | Yes | ModulatableSliderAngleDeg |
| layers | int | 1-16 | 8 | No | ImGui::SliderInt |
| baseFreq | float | 27.5-440 | 55.0 | Yes | "Base Freq (Hz)##faraday" |
| maxFreq | float | 1000-16000 | 14000.0 | Yes | "Max Freq (Hz)##faraday" |
| gain | float | 0.1-10.0 | 2.0 | Yes | "Gain##faraday" |
| curve | float | 0.1-3.0 | 1.5 | Yes | "Contrast##faraday" |
| decayHalfLife | float | 0.05-5.0 | 0.3 | No | ImGui::SliderFloat |
| diffusionScale | int | 0-8 | 2 | No | ImGui::SliderInt |
| blendMode | EffectBlendMode | enum | EFFECT_BLEND_SCREEN | No | ImGui::Combo |
| blendIntensity | float | 0.0-5.0 | 1.0 | Yes | ModulatableSlider |

### Constants

- Enum: `TRANSFORM_FARADAY_BLEND` (placed before `TRANSFORM_ACCUM_COMPOSITE`)
- Display name: `"Faraday Waves"`
- Category badge: `"CYM"` (Cymatics)
- Section index: `16`
- Flags: `EFFECT_FLAG_BLEND | EFFECT_FLAG_NEEDS_RESIZE`
- Registration macro: `REGISTER_GENERATOR_FULL`

### Config Fields Macro

```cpp
#define FARADAY_CONFIG_FIELDS                                                  \
  enabled, waveCount, spatialScale, visualGain, rotationSpeed, rotationAngle,  \
      layers, baseFreq, maxFreq, gain, curve, decayHalfLife, diffusionScale,   \
      blendMode, blendIntensity, gradient
```

---

## Tasks

### Wave 1: Header

#### Task 1.1: Create faraday.h

**Files**: `src/effects/faraday.h` (create)
**Creates**: `FaradayConfig`, `FaradayEffect` types, function declarations

**Do**: Create the header following the Design section structs exactly. Follow `chladni.h` structure: includes (`raylib.h`, `render/blend_mode.h`, `render/color_config.h`, `<stdbool.h>`), config struct, config fields macro, forward declare `ColorLUT`, effect struct, 7 function declarations (Init, Setup, Render, Resize, Uninit, ConfigDefault, RegisterParams). Init signature: `(FaradayEffect*, const FaradayConfig*, int width, int height)`. Setup signature: `(FaradayEffect*, const FaradayConfig*, float deltaTime, Texture2D fftTexture)`. Render signature: `(FaradayEffect*, const FaradayConfig*, float deltaTime, int screenWidth, int screenHeight)`.

**Verify**: `cmake.exe --build build` compiles (header-only, no link issues).

---

### Wave 2: Parallel Implementation

#### Task 2.1: Create faraday.fs

**Files**: `shaders/faraday.fs` (create)

**Do**: Implement the Algorithm section shader code exactly as specified in the Design section. No attribution block needed (no external source). Uniforms match the `FaradayEffect` location fields. Uses the standard BAND_SAMPLES FFT sampling pattern from the generator patterns reference. The `lattice()` helper evaluates N standing waves at evenly spaced angles. Main loop iterates `layers`, evaluates lattice at integer waveCount, applies half-frequency temporal oscillation. Trail persistence uses same 5-tap separable Gaussian as Chladni.

**Verify**: File exists, all uniforms referenced in Design section are declared.

---

#### Task 2.2: Create faraday.cpp

**Files**: `src/effects/faraday.cpp` (create)
**Depends on**: Wave 1 complete

**Do**: Follow `chladni.cpp` structure exactly. Same includes. Implement:

1. `InitPingPong` / `UnloadPingPong` static helpers (same as Chladni)
2. `FaradayEffectInit`: Load `shaders/faraday.fs`, cache all uniform locations matching the `FaradayEffect` struct fields, init `ColorLUT`, init ping-pong textures, set `time = 0`, `rotationAccum = 0`, `readIdx = 0`
3. `FaradayEffectSetup`: Store fftTexture, update ColorLUT, set resolution. Accumulate `time += deltaTime` and `rotationAccum += cfg->rotationSpeed * deltaTime`. Compute `rotationOffset = cfg->rotationAngle + rotationAccum` and send to shader. Compute `decayFactor` from `decayHalfLife` (same formula as Chladni). Bind all uniforms.
4. `FaradayEffectRender`: Same ping-pong render as Chladni — `BeginTextureMode(write)`, `BeginShaderMode`, bind fftTexture + gradientLUT as textures, `RenderUtilsDrawFullscreenQuad`, swap readIdx.
5. `FaradayEffectResize`: Same as Chladni.
6. `FaradayEffectUninit`: Same as Chladni.
7. `FaradayConfigDefault`: Return `FaradayConfig{}`.
8. `FaradayRegisterParams`: Register all modulatable params per the Parameters table.
9. Bridge functions: `SetupFaraday(PostEffect*)`, `SetupFaradayBlend(PostEffect*)`, `RenderFaraday(PostEffect*)` — same pattern as Chladni's three bridge functions.
10. `// === UI ===` section: `DrawFaradayParams` with sections:
    - **Wave**: `ImGui::SliderInt` for waveCount (1-6), `ModulatableSliderLog` for spatialScale, `ModulatableSlider` for visualGain; `ModulatableSliderSpeedDeg` for rotationSpeed; `ModulatableSliderAngleDeg` for rotationAngle
    - **Geometry**: `ImGui::SliderInt` for layers (1-16)
    - **Audio**: baseFreq, maxFreq, gain, curve — same widget types and formats as Chladni
    - **Trail**: decayHalfLife (`ImGui::SliderFloat`), diffusionScale (`ImGui::SliderInt`)
11. `STANDARD_GENERATOR_OUTPUT(faraday)` macro
12. `REGISTER_GENERATOR_FULL(TRANSFORM_FARADAY_BLEND, Faraday, faraday, "Faraday Waves", SetupFaradayBlend, SetupFaraday, RenderFaraday, 16, DrawFaradayParams, DrawOutput_faraday)`

Include `"audio/audio.h"` for `AUDIO_SAMPLE_RATE`, `"render/render_utils.h"` for `RenderUtilsInitTextureHDR`/`RenderUtilsDrawFullscreenQuad`/`RenderUtilsClearTexture`.

**Verify**: Compiles.

---

#### Task 2.3: Config registration and build integration

**Files**: `src/config/effect_config.h` (modify), `src/render/post_effect.h` (modify), `CMakeLists.txt` (modify), `src/config/effect_serialization.cpp` (modify)
**Depends on**: Wave 1 complete

**Do**:

1. **effect_config.h**:
   - Add `#include "effects/faraday.h"` with other effect includes (alphabetical near chladni.h)
   - Add `TRANSFORM_FARADAY_BLEND` enum entry before `TRANSFORM_ACCUM_COMPOSITE`
   - Add `TRANSFORM_FARADAY_BLEND` to `TransformOrderConfig::order` array (before closing brace)
   - Add `FaradayConfig faraday;` member to `EffectConfig` struct

2. **post_effect.h**:
   - Add `#include "effects/faraday.h"` (alphabetical)
   - Add `FaradayEffect faraday;` member to `PostEffect` struct (near other generator effects like chladni, rippleTank)

3. **CMakeLists.txt**:
   - Add `src/effects/faraday.cpp` to `EFFECTS_SOURCES`

4. **effect_serialization.cpp**:
   - Add `#include "effects/faraday.h"`
   - Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(FaradayConfig, FARADAY_CONFIG_FIELDS)`
   - Add `X(faraday) \` to `EFFECT_CONFIG_FIELDS(X)` table (alphabetical position)

**Verify**: `cmake.exe --build build` compiles with no warnings.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] Effect appears in Cymatics category (section 16, badge "CYM")
- [ ] Effect appears in transform order pipeline
- [ ] Enabling shows lattice patterns when audio plays
- [ ] waveCount slider switches between lattice geometries (1=stripes, 2=squares, 3=hexagons)
- [ ] rotationSpeed spins the lattice over time
- [ ] Half-frequency pulsing visible (bass layers breathe slowly)
- [ ] Trail persistence works (decay + diffusion)
- [ ] Preset save/load round-trips all parameters
- [ ] Modulation routes to registered parameters
