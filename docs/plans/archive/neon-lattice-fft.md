# Neon Lattice FFT Reactivity

Add FFT audio reactivity to Neon Lattice so each grid cell pulses with energy from a frequency band determined by its gradient position. The existing `t = fract(f.x + f.y + axisOffset)` value drives both color and frequency lookup — no new mapping logic needed.

**Research**: `docs/research/neon_lattice_fft.md`

## Design

### Types

**NeonLatticeConfig additions** (insert after `glowExponent`, before Speed section):

```c
// Audio
float baseFreq = 55.0f;   // Lowest mapped frequency in Hz (27.5-440.0)
float maxFreq = 14000.0f;  // Highest mapped frequency in Hz (1000-16000)
float gain = 2.0f;         // FFT magnitude amplifier (0.1-10.0)
float curve = 1.5f;        // Contrast exponent on magnitude (0.1-3.0)
float baseBright = 0.15f;  // Baseline brightness when silent (0.0-1.0)
```

**NeonLatticeEffect additions** (after `axisCountLoc`):

```c
int fftTextureLoc;
int sampleRateLoc;
int baseFreqLoc;
int maxFreqLoc;
int gainLoc;
int curveLoc;
int baseBrightLoc;
```

**NEON_LATTICE_CONFIG_FIELDS**: Add `baseFreq, maxFreq, gain, curve, baseBright` after `glowExponent`.

### Algorithm

In `map()`, each axis computes `t = fract(f.x + f.y + axisOffset)`, samples the gradient LUT at `t` for color, then samples the FFT at the corresponding frequency for magnitude. The glow result is scaled by `(baseBright + mag)`.

**Shader helper function** (add before `map()`):

```glsl
float fftMag(float t) {
    float freq = baseFreq * pow(maxFreq / baseFreq, t);
    float bin = freq / (sampleRate * 0.5);
    float energy = texture(fftTexture, vec2(bin, 0.5)).r;
    return pow(clamp(energy * gain, 0.0, 1.0), curve);
}
```

**Modified `map()` body** — each axis block changes from:

```glsl
col += getLight(po, texture(gradientLUT, vec2(fract(f.x + f.y + OFFSET), 0.5)).rgb);
```

to:

```glsl
float t1 = fract(f.x + f.y + OFFSET);
col += getLight(po, texture(gradientLUT, vec2(t1, 0.5)).rgb) * (baseBright + fftMag(t1));
```

Where `OFFSET` is `0.0`, `0.33`, or `0.66` for axes 1, 2, 3 respectively. Variable names `t1`, `t2`, `t3` to avoid shadowing the raymarch `t`.

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| baseFreq | float | 27.5-440.0 | 55.0 | yes | Base Freq (Hz) |
| maxFreq | float | 1000-16000 | 14000.0 | yes | Max Freq (Hz) |
| gain | float | 0.1-10.0 | 2.0 | yes | Gain |
| curve | float | 0.1-3.0 | 1.5 | yes | Contrast |
| baseBright | float | 0.0-1.0 | 0.15 | yes | Base Bright |

### Constants

No new enum values or categories. The existing `TRANSFORM_NEON_LATTICE_BLEND` enum and section 10 (Geometric) are unchanged.

---

## Tasks

### Wave 1: Header

#### Task 1.1: Add FFT fields to header

**Files**: `src/effects/neon_lattice.h`
**Creates**: Config fields and uniform locations that .cpp and .fs reference

**Do**:
- Add the 5 audio config fields to `NeonLatticeConfig` after `glowExponent` (see Design > Types)
- Add the 7 uniform location fields to `NeonLatticeEffect` after `axisCountLoc` (see Design > Types)
- Add `baseFreq, maxFreq, gain, curve, baseBright` to `NEON_LATTICE_CONFIG_FIELDS` macro after `glowExponent`
- Change `NeonLatticeEffectSetup` signature: add `Texture2D fftTexture` as final parameter
- Add `#include "audio/audio.h"` is NOT needed here — `AUDIO_SAMPLE_RATE` is used in .cpp only

**Verify**: `cmake.exe --build build` compiles (will warn about missing fftTexture arg in .cpp until Task 2.1).

---

### Wave 2: Parallel Implementation

#### Task 2.1: Add FFT uniform binding and UI

**Files**: `src/effects/neon_lattice.cpp`
**Depends on**: Wave 1 complete

**Do**:
- Add `#include "audio/audio.h"` to includes
- In `CacheLocations()`: cache the 7 new uniform locations (`fftTexture`, `sampleRate`, `baseFreq`, `maxFreq`, `gain`, `curve`, `baseBright`)
- Update `NeonLatticeEffectSetup` signature to add `Texture2D fftTexture` parameter
- In `NeonLatticeEffectSetup`: bind `fftTexture` via `SetShaderValueTexture`, bind `sampleRate` as `(float)AUDIO_SAMPLE_RATE`, bind `baseFreq`, `maxFreq`, `gain`, `curve`, `baseBright` as floats. Follow spectral_arcs.cpp lines 67-90 as pattern.
- In `SetupNeonLattice` bridge: add `pe->fftTexture` as final arg to `NeonLatticeEffectSetup` call
- In `NeonLatticeRegisterParams`: register 5 new params (`neonLattice.baseFreq` etc.) with ranges matching the config comments. Follow spectral_arcs.cpp lines 107-124 as pattern.
- In `DrawNeonLatticeParams` UI: add Audio section at the TOP (before Grid section) with standard slider order: Base Freq, Max Freq, Gain, Contrast, Base Bright. Use `##neonLattice` suffixes. Follow spectral_arcs.cpp lines 150-160 as pattern. Remove the `(void)categoryGlow;` line — it's no longer needed if no other unused params remain (check).

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 2.2: Add FFT sampling to shader

**Files**: `shaders/neon_lattice.fs`
**Depends on**: Wave 1 complete

**Do**:
- Add new uniforms after the `axisCount` uniform:
  ```glsl
  uniform sampler2D fftTexture;
  uniform float sampleRate;
  uniform float baseFreq;
  uniform float maxFreq;
  uniform float gain;
  uniform float curve;
  uniform float baseBright;
  ```
- Add `fftMag()` helper function before `map()` (see Design > Algorithm for exact code)
- Modify `map()`: for each axis, extract `t` into a local variable, sample `fftMag(t)`, multiply `getLight()` result by `(baseBright + fftMag(t))`. See Design > Algorithm for exact transformation. Use `t1`, `t2`, `t3` variable names to avoid shadowing the `t` in `main()`.

**Verify**: `cmake.exe --build build` compiles.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] Effect runs with audio playing — cells pulse with frequency-mapped brightness
- [ ] Silent audio shows dim cells at `baseBright` level
- [ ] Gain/Contrast/Base Bright sliders respond correctly
- [ ] Preset save/load round-trips the new fields
