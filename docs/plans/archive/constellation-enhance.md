# Constellation Enhancement: Square Points + FFT Reactivity

Two additions to the existing Constellation generator: a square point shape mode using Chebyshev distance, and per-point FFT-reactive brightness using the BAND_SAMPLES frequency spread pattern. Points map to frequency bands via their cell hash, so color tracks frequency.

**Research**: `docs/research/constellation_enhance.md`

## Design

### Types

**ConstellationConfig additions** (insert after `pointOpacity`):

```cpp
int pointShape = 0;       // Point shape: 0 = circle, 1 = square (0-1)
```

**ConstellationConfig additions** (insert after `blendIntensity`, new Audio group):

```cpp
// Audio (FFT reactivity)
float baseFreq = 55.0f;   // Lowest mapped frequency in Hz (27.5-440.0)
float maxFreq = 14000.0f; // Highest mapped frequency in Hz (1000-16000)
float gain = 2.0f;        // FFT sensitivity (0.1-10.0)
float curve = 1.5f;       // Contrast exponent on FFT magnitudes (0.1-3.0)
float baseBright = 0.15f; // Point brightness when frequency is silent (0.0-1.0)
int starBins = 60;        // Number of frequency bands for point mapping (12-120)
```

**ConstellationEffect additions** (append after `depthLayersLoc`):

```cpp
int pointShapeLoc;
int fftTextureLoc;
int sampleRateLoc;
int baseFreqLoc;
int maxFreqLoc;
int gainLoc;
int curveLoc;
int baseBrightLoc;
int starBinsLoc;
```

**ConstellationEffectSetup signature change** — add `Texture2D fftTexture` as last parameter:

```cpp
void ConstellationEffectSetup(ConstellationEffect *e,
                              const ConstellationConfig *cfg, float deltaTime,
                              Texture2D fftTexture);
```

**CONFIG_FIELDS macro** — append `pointShape, baseFreq, maxFreq, gain, curve, baseBright, starBins`.

### Algorithm

The `Point()` function in `constellation.fs` gains two modifications. All other functions (`Line`, `BarycentricColor`, `Layer`, `main`) remain unchanged.

**New uniforms** (add after existing uniforms):

```glsl
uniform int pointShape;
uniform sampler2D fftTexture;
uniform float sampleRate;
uniform float baseFreq;
uniform float maxFreq;
uniform float gain;
uniform float curve;
uniform float baseBright;
uniform int starBins;
```

**Replacement `Point()` function** — replaces the existing `Point()` at lines 110-117:

```glsl
vec3 Point(vec2 p, vec2 pointPos, vec2 cellID, vec2 cellOffset) {
    vec2 j = (pointPos - p) * (15.0 / pointSize);

    // Shape: circle (Euclidean) or square (Chebyshev)
    float sparkle;
    if (pointShape == 0) {
        sparkle = 1.0 / dot(j, j);
    } else {
        float d = max(abs(j.x), abs(j.y));
        sparkle = 1.0 / (d * d);
    }
    sparkle = clamp(sparkle, 0.0, 1.0);

    // FFT band index from cell hash
    int semi = int(N21(cellID) * float(starBins));
    float freqLo = baseFreq * pow(maxFreq / baseFreq, float(semi) / float(starBins));
    float freqHi = baseFreq * pow(maxFreq / baseFreq, float(semi + 1) / float(starBins));
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
    float fftBrightness = baseBright + mag;

    // Color from frequency bin (not raw hash) so color == frequency
    vec3 col = textureLod(pointLUT, vec2(float(semi) / float(starBins), 0.5), 0.0).rgb;
    return col * sparkle * pointBrightness * pointOpacity * WaveBrightness(cellID, cellOffset) * fftBrightness;
}
```

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| pointShape | int | 0-1 | 0 | No | Point Shape (Combo: Circle/Square) |
| baseFreq | float | 27.5-440.0 | 55.0 | Yes | Base Freq (Hz) |
| maxFreq | float | 1000-16000 | 14000.0 | Yes | Max Freq (Hz) |
| gain | float | 0.1-10.0 | 2.0 | Yes | Gain |
| curve | float | 0.1-3.0 | 1.5 | Yes | Contrast |
| baseBright | float | 0.0-1.0 | 0.15 | Yes | Base Bright |
| starBins | int | 12-120 | 60 | No | Star Bins |

---

## Tasks

### Wave 1: Header

#### Task 1.1: Config and effect struct updates

**Files**: `src/effects/constellation.h`
**Creates**: Struct fields and updated signature that Wave 2 tasks depend on

**Do**:

1. Add `int pointShape = 0;` field after `pointOpacity` in `ConstellationConfig`, with range comment `// Point shape: 0 = circle, 1 = square (0-1)`
2. Add Audio group fields after `blendIntensity`: `baseFreq`, `maxFreq`, `gain`, `curve`, `baseBright`, `starBins` — types, defaults, and range comments per Design section
3. Append `pointShape, baseFreq, maxFreq, gain, curve, baseBright, starBins` to `CONSTELLATION_CONFIG_FIELDS` macro
4. Add uniform location fields to `ConstellationEffect` struct after `depthLayersLoc`: `pointShapeLoc`, `fftTextureLoc`, `sampleRateLoc`, `baseFreqLoc`, `maxFreqLoc`, `gainLoc`, `curveLoc`, `baseBrightLoc`, `starBinsLoc`
5. Change `ConstellationEffectSetup` declaration: add `Texture2D fftTexture` as the last parameter

**Verify**: `cmake.exe --build build` compiles.

---

### Wave 2: Implementation (parallel)

#### Task 2.1: C++ uniform binding, UI, and bridge

**Files**: `src/effects/constellation.cpp`
**Depends on**: Wave 1 complete

**Do**:

1. Add `#include "audio/audio.h"` to includes (same as nebula.cpp)
2. In `ConstellationEffectInit()`: cache all new uniform locations (`pointShape`, `fftTexture`, `sampleRate`, `baseFreq`, `maxFreq`, `gain`, `curve`, `baseBright`, `starBins`) — follow existing pattern
3. Update `ConstellationEffectSetup()` signature to match header (add `Texture2D fftTexture` param). Add uniform binding for all new fields:
   - `float sampleRate = (float)AUDIO_SAMPLE_RATE;` then `SetShaderValue` for `sampleRateLoc` (same pattern as nebula.cpp line 69)
   - `SetShaderValueTexture` for `fftTextureLoc` with `fftTexture`
   - `SetShaderValue` for `pointShapeLoc` (`int`, same pattern as `interpolateLineColor`)
   - `SetShaderValue` for `baseFreqLoc`, `maxFreqLoc`, `gainLoc`, `curveLoc`, `baseBrightLoc` (all `SHADER_UNIFORM_FLOAT`)
   - `SetShaderValue` for `starBinsLoc` (`SHADER_UNIFORM_INT`)
4. Update `SetupConstellation()` bridge: pass `pe->fftTexture` as last arg (same as nebula's bridge at line 164-166)
5. In `DrawConstellationParams()`:
   - Add `ImGui::Combo("Point Shape##constellation", &c->pointShape, "Circle\0Square\0");` in the Points section, before the Point Size slider
   - Add Audio section after Triangles section: `ImGui::SeparatorText("Audio")`, then the 5 standard sliders (Base Freq, Max Freq, Gain, Contrast, Base Bright) plus `ImGui::SliderInt("Star Bins##constellation", ...)`. Follow nebula's Audio section pattern (lines 183-193) exactly for widget types and format strings.
6. In `ConstellationRegisterParams()`: register `baseFreq` (27.5-440), `maxFreq` (1000-16000), `gain` (0.1-10), `curve` (0.1-3), `baseBright` (0-1) — all with `"constellation."` prefix

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 2.2: Shader — square points and FFT reactivity

**Files**: `shaders/constellation.fs`
**Depends on**: Wave 1 complete (for uniform name reference only — no #include dependency)

**Do**:

1. Add new uniforms after existing uniform declarations (after `uniform vec2 waveCenter;`): `pointShape` (int), `fftTexture` (sampler2D), `sampleRate` (float), `baseFreq` (float), `maxFreq` (float), `gain` (float), `curve` (float), `baseBright` (float), `starBins` (int)
2. Replace the `Point()` function with the implementation from the Algorithm section. This is the complete replacement — do not keep any lines from the old function.

**Verify**: `cmake.exe --build build` compiles. Enable Constellation, confirm points render. Toggle Point Shape combo between Circle and Square.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] Constellation renders with default settings (circle points, baseBright 0.15 gives dim glow)
- [ ] Point Shape combo switches between circle and square glow shapes
- [ ] With audio playing, points pulse in brightness corresponding to their frequency band
- [ ] Star Bins slider changes frequency granularity
- [ ] Gradient colors map to frequency bands (low-freq points = one end, high-freq = other end)
