# Constellation Audio Reactivity

Add per-semitone FFT reactivity to the constellation generator. Each star gets a random semitone via cell hash, colors from the gradient LUT by pitch class, and brightness that scales with FFT magnitude. The existing radial wave acts as a traveling brightness multiplier.

**Research**: `docs/research/constellation_audio.md`

## Design

### Types

**ConstellationConfig** — add 6 new fields after `waveCenterY`:

```
float baseFreq = 55.0f;      // Lowest mapped frequency in Hz (20.0-200.0)
int numOctaves = 5;           // Octave range mapped across stars (1-8)
float gain = 2.0f;            // FFT magnitude amplification (0.1-10.0)
float curve = 0.7f;           // Contrast shaping exponent (0.1-3.0)
float baseBright = 0.15f;     // Minimum glow when note is silent (0.0-1.0)
float waveInfluence = 0.5f;   // Wave brightness modulation strength (0.0-1.0)
```

**ConstellationEffect** — add 8 new uniform locations:

```
int fftTextureLoc;
int sampleRateLoc;
int baseFreqLoc;
int numOctavesLoc;
int gainLoc;
int curveLoc;
int baseBrightLoc;
int waveInfluenceLoc;
```

**ConstellationEffectSetup** — signature changes from 3 args to 4:

```
void ConstellationEffectSetup(ConstellationEffect *e,
                              const ConstellationConfig *cfg,
                              float deltaTime, Texture2D fftTexture);
```

### Algorithm

All shader changes happen in `shaders/constellation.fs`.

**New uniforms** (add after existing uniforms):

```glsl
uniform sampler2D fftTexture;
uniform float sampleRate;
uniform float baseFreq;
uniform int numOctaves;
uniform float gain;
uniform float curve;
uniform float baseBright;
uniform float waveInfluence;
```

**Semitone assignment** — in `Point()`, replace hash-based LUT index with pitch class:

```glsl
int totalSemitones = numOctaves * 12;
float semi = floor(N21(cellID) * float(totalSemitones));
float pitchClass = fract(semi / 12.0);
```

Use `pitchClass` as the pointLUT coordinate instead of `N21(cellID)`.

**FFT lookup** — in `Point()`, convert semitone to frequency bin and read magnitude:

```glsl
float freq = baseFreq * pow(2.0, semi / 12.0);
float bin = freq / (sampleRate * 0.5);
float mag = (bin <= 1.0) ? texture(fftTexture, vec2(bin, 0.5)).r : 0.0;
mag = pow(clamp(mag * gain, 0.0, 1.0), curve);
```

**Wave brightness modulation** — in `Point()`, compute wave factor from cell distance to waveCenter (same distance `GetPos()` uses for position, recomputed here):

```glsl
float waveDist = length(cellID + cellOffset - waveCenter);
float waveFactor = sin(waveDist * waveFreq - wavePhase) * 0.5 + 0.5;
float brightness = (baseBright + mag) * mix(1.0, waveFactor, waveInfluence);
```

Where `cellOffset` is the neighbor offset for this point (passed as new parameter to `Point()`).

**Point() signature change**: Add `vec2 cellOffset` parameter so it can compute wave distance. Callers already have this value.

**Final point color**:

```glsl
vec3 col = textureLod(pointLUT, vec2(pitchClass, 0.5), 0.0).rgb;
return col * sparkle * pointBrightness * brightness;
```

**BarycentricColor** and **Line interpolateLineColor** — both sample `pointLUT` with `N21(cellID)`. Change to use the same pitch-class formula:

```glsl
float semi = floor(N21(id) * float(numOctaves * 12));
float pc = fract(semi / 12.0);
vec3 col = textureLod(pointLUT, vec2(pc, 0.5), 0.0).rgb;
```

This applies in `BarycentricColor()` (3 lookups) and in `Line()` when `interpolateLineColor != 0` (2 lookups). The `lineLUT` length-based path stays unchanged.

**Fill**: No FFT brightness scaling. Fill triangles get semitone colors from BarycentricColor automatically.

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| baseFreq | float | 20.0–200.0 | 55.0 | yes | Base Freq |
| numOctaves | int | 1–8 | 5 | yes (ModulatableSliderInt) | Octaves |
| gain | float | 0.1–10.0 | 2.0 | yes | FFT Gain |
| curve | float | 0.1–3.0 | 0.7 | yes | FFT Curve |
| baseBright | float | 0.0–1.0 | 0.15 | yes | Base Bright |
| waveInfluence | float | 0.0–1.0 | 0.5 | yes | Wave Influence |

### Constants

No new enum entries. `TRANSFORM_CONSTELLATION_BLEND` already exists. No descriptor changes needed.

---

## Tasks

### Wave 1: Config & Header

#### Task 1.1: Expand ConstellationConfig and ConstellationEffect

**Files**: `src/effects/constellation.h`
**Creates**: New config fields and uniform locations that all other tasks depend on

**Do**:
- Add 6 config fields (baseFreq, numOctaves, gain, curve, baseBright, waveInfluence) with defaults and range comments, after `waveCenterY`
- Add 8 uniform location ints to ConstellationEffect (fftTextureLoc, sampleRateLoc, baseFreqLoc, numOctavesLoc, gainLoc, curveLoc, baseBrightLoc, waveInfluenceLoc)
- Change `ConstellationEffectSetup` signature to add `Texture2D fftTexture` as 4th parameter — same pattern as `NebulaEffectSetup` in `src/effects/nebula.h`
- Add `#include "audio/audio.h"` to the .h file is NOT needed — it goes in the .cpp

**Verify**: `cmake.exe --build build` compiles (will fail at link due to signature change, that's expected).

---

### Wave 2: Parallel Implementation

#### Task 2.1: Effect module implementation

**Files**: `src/effects/constellation.cpp`
**Depends on**: Wave 1

**Do**:
- Add `#include "audio/audio.h"` for `AUDIO_SAMPLE_RATE`
- In `ConstellationEffectInit`: cache the 8 new uniform locations (fftTexture, sampleRate, baseFreq, numOctaves, gain, curve, baseBright, waveInfluence)
- In `ConstellationEffectSetup`: accept `Texture2D fftTexture` as 4th param. Bind 6 new config uniforms. Bind `sampleRate` as `(float)AUDIO_SAMPLE_RATE`. Bind `fftTexture` via `SetShaderValueTexture`. Follow nebula.cpp:50-70 as pattern.
- In `ConstellationRegisterParams`: register 6 new params with ranges from the Parameters table

**Verify**: Compiles.

#### Task 2.2: Shader FFT integration

**Files**: `shaders/constellation.fs`
**Depends on**: Wave 1 (needs to know uniform names)

**Do**: Implement the Algorithm section above. Specifically:
- Add 8 new uniforms (fftTexture, sampleRate, baseFreq, numOctaves, gain, curve, baseBright, waveInfluence)
- Modify `Point()`: add `vec2 cellOffset` parameter. Compute semitone from `N21(cellID)`, FFT lookup, pitch-class LUT color, wave brightness modulation. Multiply final return by `brightness`.
- Modify `BarycentricColor()`: change 3 LUT lookups from `N21(id)` to pitch-class formula
- Modify `Line()`: change 2 interpolateLineColor LUT lookups from `N21(cellID)` to pitch-class formula
- Update all `Point()` call sites in `Layer()` to pass the neighbor offset as `cellOffset`

**Verify**: Compiles (shader compilation tested at runtime).

#### Task 2.3: Shader setup integration

**Files**: `src/render/shader_setup_generators.cpp`
**Depends on**: Wave 1

**Do**:
- Change `SetupConstellation` to pass `pe->fftTexture` as 4th arg to `ConstellationEffectSetup` — same pattern as `SetupNebula` at line 168-170

**Verify**: Compiles.

#### Task 2.4: UI panel update

**Files**: `src/ui/imgui_effects_gen_filament.cpp`
**Depends on**: Wave 1

**Do**:
- In `DrawGeneratorsConstellation`, add an "Audio" section separator after the wave controls, before the point rendering section
- Add 6 new sliders:
  - `ModulatableSlider("Base Freq##constellation", &c->baseFreq, "constellation.baseFreq", "%.1f Hz", modSources)`
  - `ModulatableSliderInt("Octaves##constellation", &c->numOctaves, "constellation.numOctaves", modSources)` with range 1-8
  - `ModulatableSlider("FFT Gain##constellation", &c->gain, "constellation.gain", "%.2f", modSources)`
  - `ModulatableSlider("FFT Curve##constellation", &c->curve, "constellation.curve", "%.2f", modSources)`
  - `ModulatableSlider("Base Bright##constellation", &c->baseBright, "constellation.baseBright", "%.2f", modSources)`
  - `ModulatableSlider("Wave Influence##constellation", &c->waveInfluence, "constellation.waveInfluence", "%.2f", modSources)`
- Follow the nebula UI pattern for audio parameter grouping

**Verify**: Compiles.

#### Task 2.5: Preset serialization

**Files**: `src/config/preset.cpp`
**Depends on**: Wave 1

**Do**:
- Add 6 new fields to the `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT` macro for ConstellationConfig: `baseFreq, numOctaves, gain, curve, baseBright, waveInfluence`
- Place them after the existing fields, before `pointGradient`

**Verify**: Compiles.

---

## Final Verification

- [x] Build succeeds with no warnings
- [ ] waveInfluence=0 gives uniform brightness; waveInfluence=1 shows traveling brightness rings on points and lines
- [ ] pointOpacity=0 hides points; lineOpacity=0 hides lines (independent control)
- [ ] depthLayers=1 matches original behavior; 2-3 adds parallax layers
- [ ] waveFreq stays coherent up to 2.0 (no grid aliasing)
- [ ] Preset save/load round-trips all new parameters

---

## Implementation Notes

The original plan targeted per-semitone FFT reactivity: each star mapped to a random semitone, colored by pitch class, brightness driven by FFT magnitude. This was implemented and tested but produced visually unsatisfying results — the uniform random spreading of semitones across the grid made audio reactivity look like random blinking rather than coherent musical response. The constellation's design (uniform grid, random wandering) works against frequency-selective reactivity.

**What was cut:**
- All FFT integration (baseFreq, numOctaves, gain, curve, baseBright, fftTexture, sampleRate)
- Pitch-class LUT coloring (reverted to original `N21(cellID)` hash-based gradient sampling)
- `CellBrightness` helper and per-point FFT magnitude lookup

**What was added instead:**
- `pointOpacity` (0.0-1.0, default 1.0) — independent point visibility control, parallels existing `lineOpacity`
- `depthLayers` (1-3, default 1) — parallax depth via multiple `Layer()` calls at increasing scale (1.6x, 2.2x) with decreasing brightness. Layer 1 = original behavior.
- `waveInfluence` repurposed as the primary visual modulation tool. Lines now sample wave brightness at their actual world position along the segment (not just endpoint interpolation), so the wave crest sweeps along lines as it propagates — creating a natural "energy transmission" look without a separate pulse system.
- `waveFreq` range clamped from 0.1-5.0 to 0.1-2.0. Higher values alias against the grid, making cell boundaries visible.

**Design rationale:** The existing radial wave already provides coherent spatial modulation. Unifying point brightness, line brightness, and energy pulses through a single wave creates a more visually cohesive effect than layering independent systems (separate twinkle phases, bidirectional line pulses). All new params default to off (waveInfluence=0, depthLayers=1, pointOpacity=1) so existing presets are unaffected.
