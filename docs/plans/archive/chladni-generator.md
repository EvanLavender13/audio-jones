# Chladni Generator

FFT-driven resonant plate mode visualization. Frequency content selects which Chladni eigenmodes are active — bass excites simple patterns (few nodal lines), treble excites complex ones. Multiple modes superimpose weighted by their frequency band energy, creating standing wave patterns that morph with the music. A coherence slider blends between full superposition and dominant-mode tracking.

**Research**: `docs/research/chladni-generator.md`

<!-- Intentional deviations from research:
  - Rectangular plate only (no circular/Bessel) — scope reduction
  - Intensity color mode only (no per-mode, no chromatic) — scope reduction
  - Trail blend uses screen-blend instead of max() — prevents burn-in
  - Color mapping uses tanh() instead of linear — follows cymatics.fs pattern
  - visualGain folds into tanh() steepness, not post-process scalar — follows cymatics.fs pattern
  - Brightness alpha masking (premultiplied alpha output) — follows cymatics.fs pattern
-->

## Implementation Notes

Changes made after initial implementation that the plan and research originally got wrong. These are the ground truth — do not revert to the original plan's approach.

### No hardcoded mode table

The original plan hardcoded 12 (n,m) pairs in constant arrays. This was wrong — it limited the effect to 12 frequency bands covering the entire spectrum, making FFT response barely visible.

**Correct approach**: Generate all (n,m) pairs mathematically in the shader. Iterate n,m from 1 to `min(sqrt(maxFreq/freqScale)+1, 20)`, skip n==m and n>m. Each pair's resonant frequency is `freqScale * (n²+m²)` where `freqScale = baseFreq / 5.0`. This produces dozens of modes spanning the full spectrum.

### No baseBright

The original plan included `baseBright` — a minimum mode weight when silent. This is wrong for Chladni. Showing patterns where there is no audio energy is meaningless. Silent = black.

**Removed from**: config struct, shader, UI, param registration, serialization fields macro.

### No modeCount parameter

With dynamically generated modes, there is no fixed mode count. The number of active modes is determined by baseFreq, maxFreq, and the audio content.

**Removed from**: config struct, shader uniform, UI, uniform location cache.

### Direct FFT bin lookup, not band averaging

The original plan divided the spectrum into `modeCount` log-spaced bands and averaged 4 samples per band. This was designed for effects with a fixed layer count. Chladni modes have exact resonant frequencies — each mode maps directly to one FFT bin:

```glsl
float bin = modeFreq / nyquist;
float mag = texture(fftTexture, vec2(bin, 0.5)).r;
```

### Coherence via power curve, not softmax

The original softmax approach required fixed-size arrays and multiple passes. With dynamic mode count, a simpler approach works: `pow(mag, mix(1.0, 4.0, coherence))`. At coherence=0, raw magnitudes. At coherence=1, weak modes raised to 4th power, crushed toward zero.

### Aspect correction

The original plan omitted aspect correction. Without it, patterns stretch on non-square screens.

**Added**: `uv.x *= aspect` where `aspect = resolution.x / resolution.y`.

### No plate boundary mask

A circular plate mask was tested and rejected — it makes the visual a small circle, cutting off content. The Chladni equation is periodic, but with many modes superimposed the tiling is not visually obvious. Full-screen coverage is correct.

### Screen-blend trail accumulation

The original plan used `max(existing, newColor)` (copied from cymatics). This causes burn-in where bright peaks persist indefinitely. Screen blend is correct:

```glsl
finalColor = existing + newColor * (1.0 - existing);
```

### Nodal emphasis brightness fix

The original brightness calculation `abs(tanh(pattern * visualGain))` made emphasis=0 and emphasis=1 look identical because abs() collapses the signed field. The fix blends the brightness model:

```glsl
float signedBright = compressed * 0.5 + 0.5;  // asymmetric: positive=bright, negative=dark
float absBright = abs(compressed);              // symmetric: peaks bright, nodes dark
float brightness = mix(signedBright, absBright, nodalEmphasis);
```

At 0: asymmetric. At 1: symmetric. At 0.5: blend (visually the richest).

### Weight normalization

`totalPattern /= totalWeight` after the mode loop. Keeps output range consistent regardless of how many modes are active.

---

## Design

### Types

**ChladniConfig** (`src/effects/chladni.h`):

```c
struct ChladniConfig {
  bool enabled = false;

  // Wave
  float plateSize = 1.0f;      // Virtual plate scale (0.5-3.0)
  float coherence = 0.5f;       // Sharpens FFT response toward dominant modes (0.0-1.0)
  float visualGain = 1.5f;      // Output intensity multiplier (0.5-5.0)
  float nodalEmphasis = 0.0f;   // 0=asymmetric signed, 0.5=blend, 1=symmetric abs (0.0-1.0)

  // Audio
  float baseFreq = 55.0f;      // Lowest resonant frequency (27.5-440)
  float maxFreq = 14000.0f;    // Highest resonant frequency (1000-16000)
  float gain = 2.0f;           // FFT energy sensitivity (0.1-10.0)
  float curve = 1.5f;          // Contrast exponent on FFT energy (0.1-3.0)

  // Trail
  float decayHalfLife = 0.3f;  // Trail persistence seconds (0.05-5.0)
  int diffusionScale = 2;      // Spatial blur tap spacing (0-8)

  // Blend
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f; // Blend compositor strength (0.0-5.0)
  ColorConfig gradient;
};
```

Fields macro:

```c
#define CHLADNI_CONFIG_FIELDS                                                  \
  enabled, plateSize, coherence, visualGain, nodalEmphasis, baseFreq, maxFreq, \
      gain, curve, decayHalfLife, diffusionScale, blendMode, blendIntensity,   \
      gradient
```

**ChladniEffect** (`src/effects/chladni.h`):

```c
typedef struct ChladniEffect {
  Shader shader;
  ColorLUT *colorLUT;
  RenderTexture2D pingPong[2];
  int readIdx;
  Texture2D currentFftTexture;

  // Uniform locations
  int resolutionLoc;
  int plateSizeLoc;
  int coherenceLoc;
  int visualGainLoc;
  int nodalEmphasisLoc;
  int fftTextureLoc;
  int sampleRateLoc;
  int baseFreqLoc;
  int maxFreqLoc;
  int gainLoc;
  int curveLoc;
  int diffusionScaleLoc;
  int decayFactorLoc;
  int gradientLUTLoc;
} ChladniEffect;
```

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| plateSize | float | 0.5-3.0 | 1.0 | yes | Plate Size |
| coherence | float | 0.0-1.0 | 0.5 | yes | Coherence |
| visualGain | float | 0.5-5.0 | 1.5 | yes | Vis Gain |
| nodalEmphasis | float | 0.0-1.0 | 0.0 | yes | Nodal Emphasis |
| baseFreq | float | 27.5-440 | 55.0 | yes | Base Freq (Hz) |
| maxFreq | float | 1000-16000 | 14000.0 | yes | Max Freq (Hz) |
| gain | float | 0.1-10.0 | 2.0 | yes | Gain |
| curve | float | 0.1-3.0 | 1.5 | yes | Contrast |
| decayHalfLife | float | 0.05-5.0 | 0.3 | no | Decay |
| diffusionScale | int | 0-8 | 2 | no | Diffusion |
| blendIntensity | float | 0.0-5.0 | 1.0 | yes | Blend Intensity |

### Constants

- Enum name: `TRANSFORM_CHLADNI_BLEND`
- Display name: `"Chladni"`
- Category badge: `"GEN"` (auto-set by `REGISTER_GENERATOR_FULL`)
- Section index: 13 (Field)
- Flags: `EFFECT_FLAG_BLEND | EFFECT_FLAG_NEEDS_RESIZE` (auto-set by `REGISTER_GENERATOR_FULL`)

### File List

| File | Action |
|------|--------|
| `src/effects/chladni.h` | Create |
| `src/effects/chladni.cpp` | Create |
| `shaders/chladni.fs` | Create |
| `src/config/effect_config.h` | Modify |
| `src/render/post_effect.h` | Modify |
| `src/config/effect_serialization.cpp` | Modify |
| `CMakeLists.txt` | Modify |

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] Chladni appears in Field section (section 13) of the effects panel
- [ ] Enabling Chladni shows eigenmode patterns that respond to audio
- [ ] FFT reactivity: bass excites simple modes, treble excites complex modes
- [ ] Many modes active across the spectrum (not limited to a fixed count)
- [ ] Silent audio = black output (no baseBright)
- [ ] Coherence slider suppresses weak modes at high values
- [ ] Nodal emphasis: 0=asymmetric, 0.5=blend, 1=symmetric (all three visually distinct)
- [ ] Full-screen coverage (no circular mask)
- [ ] Trail persistence uses screen-blend (no burn-in)
- [ ] Gradient LUT colors the pattern correctly
- [ ] Preset save/load round-trips all parameters
- [ ] Modulation routes to registered parameters
