# Glyph Field Audio Reactivity

Add per-semitone FFT reactivity to the glyph field generator. Each cell derives a semitone from its hash, colors from the gradient LUT by pitch class, and brightness scales with FFT magnitude. Layers stack by octave — front covers the highest octave, back covers the lowest — so bass lights up deep layers while treble lights up the front. All existing glyph field mechanics (scroll, flutter, wave, drift, inversion, LCD) remain unchanged.

**Research**: `docs/research/glyph_field_audio.md`

## Design

### Types

**GlyphFieldConfig** — add 5 fields after existing `lcdFreq`:

```
float baseFreq = 55.0f;   // Lowest mapped pitch Hz (20.0-200.0)
int numOctaves = 5;        // Octave range across layers (1-8)
float gain = 2.0f;         // FFT magnitude amplification (0.1-10.0)
float curve = 0.7f;        // Contrast shaping exponent (0.1-3.0)
float baseBright = 0.15f;  // Minimum brightness when silent (0.0-1.0)
```

Note: `baseBright=1.0` + `gain=0.0` reproduces the original non-reactive look.

**GlyphFieldEffect** — add 6 uniform location fields:

```
int fftTextureLoc;
int sampleRateLoc;
int baseFreqLoc;
int numOctavesLoc;
int gainLoc;
int curveLoc;
int baseBrightLoc;
```

**Setup signature change**: `GlyphFieldEffectSetup(e, cfg, deltaTime)` → `GlyphFieldEffectSetup(e, cfg, deltaTime, fftTexture)` — matches nebula's pattern.

### Algorithm

All formulas from the research doc, implemented in `glyph_field.fs`.

**New uniforms in shader:**

```glsl
uniform sampler2D fftTexture;
uniform float sampleRate;
uniform float baseFreq;
uniform int numOctaves;
uniform float gain;
uniform float curve;
uniform float baseBright;
```

**Octave stacking** — each layer maps to one octave. Layer 0 (front, smallest scale) = highest octave. Layer N (back, largest scale) = lowest octave:

```glsl
int octaveIndex = clamp(numOctaves - 1 - layer, 0, numOctaves - 1);
int semitonesThisLayer = 12;
int semi = int(floor(h.z * float(semitonesThisLayer)));
int globalSemi = octaveIndex * 12 + semi;
```

When `layerCount > numOctaves`, multiple layers share octaves via clamp.

**FFT lookup** — convert semitone to frequency, then to normalized FFT bin:

```glsl
float freq = baseFreq * pow(2.0, float(globalSemi) / 12.0);
float bin = freq / (sampleRate * 0.5);
float mag = (bin <= 1.0) ? texture(fftTexture, vec2(bin, 0.5)).r : 0.0;
mag = pow(clamp(mag * gain, 0.0, 1.0), curve);
```

**Color by pitch class** — replace `lutPos = h.z` with pitch class position:

```glsl
float pitchClass = fract(float(globalSemi) / 12.0);
vec3 glyphColor = textureLod(gradientLUT, vec2(pitchClass, 0.5), 0.0).rgb;
```

Same note = same color regardless of octave.

**Brightness modulation** — multiply into existing accumulation:

```glsl
float react = baseBright + mag * mag * gain;
total += glyphColor * glyphAlpha * opacity * react;
```

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| baseFreq | float | 20.0–200.0 | 55.0 | yes | Base Freq |
| numOctaves | int | 1–8 | 5 | yes | Octaves |
| gain | float | 0.1–10.0 | 2.0 | yes | FFT Gain |
| curve | float | 0.1–3.0 | 0.7 | yes | FFT Curve |
| baseBright | float | 0.0–1.0 | 0.15 | yes | Base Bright |

### Constants

No new enums or descriptors — this enhances an existing effect.

---

## Tasks

### Wave 1: Config & Declarations

#### Task 1.1: Add FFT fields to config and effect structs

**Files**: `src/effects/glyph_field.h`, `src/effects/glyph_field.cpp`

**Do**:
- Add `baseFreq`, `numOctaves`, `gain`, `curve`, `baseBright` fields to `GlyphFieldConfig` after `lcdFreq`
- Add `fftTextureLoc`, `sampleRateLoc`, `baseFreqLoc`, `numOctavesLoc`, `gainLoc`, `curveLoc`, `baseBrightLoc` to `GlyphFieldEffect`
- Change `GlyphFieldEffectSetup` signature to accept `Texture2D fftTexture` as 4th parameter (same as `NebulaEffectSetup` in `nebula.h:84`)
- In `glyph_field.cpp`: add `#include "audio/audio.h"`, cache the 7 new uniform locations in `CacheLocations`, bind them in `BindUniforms` (follow nebula.cpp:52-71 pattern — `sampleRate` from `AUDIO_SAMPLE_RATE`, `fftTexture` via `SetShaderValueTexture`)
- Register 5 new params in `GlyphFieldRegisterParams` (follow nebula.cpp:105-109 pattern)

**Verify**: `cmake.exe --build build` compiles (expect errors in callers — resolved in Wave 2).

---

### Wave 2: Shader, Wiring, UI

#### Task 2.1: Add FFT reactivity to shader

**Files**: `shaders/glyph_field.fs`

**Do**:
- Add the 7 new uniforms (`fftTexture`, `sampleRate`, `baseFreq`, `numOctaves`, `gain`, `curve`, `baseBright`) after existing uniform block
- Inside the layer loop, after `vec3 h = pcg3df(...)` at line 138: compute `octaveIndex`, `globalSemi`, FFT lookup, pitch class color, and brightness `react` — implement the Algorithm section formulas exactly
- Replace `float lutPos = h.z` (line 158) with `float pitchClass = fract(float(globalSemi) / 12.0)` and sample `gradientLUT` at `pitchClass`
- Replace `total += glyphColor * glyphAlpha * opacity` (line 162) with `total += glyphColor * glyphAlpha * opacity * react`

**Verify**: Shader loads without errors at runtime.

#### Task 2.2: Wire fftTexture through shader setup

**Files**: `src/render/shader_setup_generators.cpp`

**Do**:
- In `SetupGlyphField` (line 135): add `pe->fftTexture` as 4th argument to `GlyphFieldEffectSetup` call (same as `SetupNebula` at line 168-170)

**Verify**: `cmake.exe --build build` compiles with no errors.

#### Task 2.3: Add FFT parameter UI

**Files**: `src/ui/imgui_effects_gen_texture.cpp`

**Do**:
- In `DrawGeneratorsGlyphField`, add an "Audio" section with `ImGui::SeparatorText("Audio")` after the Grid section (before Scroll)
- Add 5 modulatable sliders matching nebula's UI pattern in `imgui_effects_gen_texture.cpp`:
  - `ModulatableSliderLog("Base Freq##glyphfield", &c->baseFreq, "glyphField.baseFreq", "%.1f Hz", modSources)` — log scale for frequency
  - `ModulatableSliderInt("Octaves##glyphfield", &c->numOctaves, "glyphField.numOctaves", modSources)` — integer slider
  - `ModulatableSlider("FFT Gain##glyphfield", &c->gain, "glyphField.gain", "%.2f", modSources)`
  - `ModulatableSlider("FFT Curve##glyphfield", &c->curve, "glyphField.curve", "%.2f", modSources)`
  - `ModulatableSlider("Base Bright##glyphfield", &c->baseBright, "glyphField.baseBright", "%.2f", modSources)`
- Follow the exact slider function conventions from nebula's UI section for `baseFreq` (log) and `numOctaves` (int)

**Verify**: UI renders, sliders control params.

---

## Final Verification

- [x] Build succeeds with no warnings
- [x] Glyph field renders with FFT reactivity — cells brighten with active notes
- [x] `baseBright=1.0` + `gain=0.0` reproduces original non-reactive appearance
- [x] Color = frequency — same color always reacts to the same FFT bin
- [x] All existing mechanics (scroll, flutter, wave, drift, inversion, LCD) unchanged
- [x] Preset save/load round-trips new parameters

---

## Implementation Notes

**Semitone mapping changed from per-layer octaves to per-cell full-range.** The plan assigned one octave per layer, so all cells within a layer reacted to a narrow 12-semitone band. Bass frequencies in that band have correlated magnitudes, so entire layers flickered uniformly. The fix: each cell hashes to a semitone across the full `numOctaves * 12` range independent of layer. One cell reacts to bass, its neighbor to treble, on any layer.

**Color = frequency.** The plan used `pitchClass = fract(globalSemi / 12.0)` so the same note in different octaves shared a color but reacted to different bins — same color, different reactivity. Now `lutPos = (globalSemi + 0.5) / totalSemitones` maps linearly across the full range. Each unique color corresponds to exactly one FFT bin.

**Brightness formula simplified.** The plan's `react = baseBright + mag * mag * gain` double-applied gain (already baked into `mag` via the clamp/pow step) and multiplied with `opacity`, so `baseBright=0` zeroed out cells entirely regardless of layer opacity. Replaced with `brightness = baseBright + (1.0 - baseBright) * mag` — opacity and brightness are independent, no double-gain.

**`numOctaves` stored as float.** The plan declared `int numOctaves` but the modulation engine requires `float*`. Changed to `float`, cast to `int` at uniform bind time. Enables modulation support.

**`baseBright` default raised to 1.0.** Ensures glyphs are visible out of the box. Lower to see audio-reactive contrast.

**UI section labeled "Audio" instead of "FFT".** Less technical. Slider labels match other generators: "Gain", "Contrast", "Base Bright".
