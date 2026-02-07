## Post-Implementation Notes

The plan below reflects the original design. During implementation the algorithm and params were overhauled:

- **Algorithm**: Replaced flat-circle SDF + smoothstep angular masking with XorDev's "Cosmic" shader technique — `mat2` perspective tilt, `cos(atan * segments)` multi-arc clipping, `sin(i*i)` per-ring rotation, inverse-distance glow. The original plan described a fundamentally different visual.
- **Removed params**: `innerRadius`, `outerRadius`, `ringWidth`, `maxSweep`, `maxBright`, `blur`, `seed`
- **Added params**: `ringScale` (ring spread), `tilt` (perspective amount), `tiltAngle` (perspective direction), `arcWidth` (cos clamp ceiling), `baseBright` (inactive arc brightness)
- **Rotation**: Changed from GPU `time * speed` to CPU-accumulated `rotationAccum += speed * deltaTime` so speed changes don't cause jumps
- **Colors**: Removed phase-rainbow multiplication — gradient LUT is sole color source
- **Pitch spiral**: Also gained `tilt` + `tiltAngle` params (shared technique)

---

# Spectral Arcs

FFT-driven concentric ring arcs — one arc per semitone across configurable octaves. Each arc's angular sweep and brightness scale with its semitone's energy. Per-arc rotation via `sin(i*i)` creates shimmering drift. Pitch-class coloring via gradient LUT. Renders to `generatorScratch`, composited via `BlendCompositor`.

**Research**: `docs/research/spectral-arcs.md`

## Design

### Types

```
SpectralArcsConfig {
  bool enabled = false;

  // Ring layout
  float baseFreq = 220.0f;       // Lowest visible frequency in Hz (A3)
  int numOctaves = 8;            // Octave count (x12 = total rings)
  float innerRadius = 0.10f;     // Radius of innermost ring (UV units)
  float outerRadius = 0.60f;     // Radius of outermost ring (UV units)
  float ringWidth = 0.005f;      // Visual thickness reference

  // FFT
  float gain = 5.0f;             // FFT magnitude amplifier
  float curve = 2.0f;            // Contrast exponent on magnitude

  // Arc animation
  float maxSweep = 3.14159f;     // Max arc angular extent at full amplitude (radians)
  float rotationSpeed = 1.0f;    // Global multiplier on per-arc rotation rates
  float seed = 0.0f;             // Shifts the rotation pattern

  // Glow
  float glowIntensity = 0.01f;   // Glow brightness at ring center
  float glowFalloff = 100.0f;    // How fast glow drops with distance
  float maxBright = 2.0f;        // Glow clamp ceiling
  float blur = 0.02f;            // Arc edge softness (smoothstep width)

  // Color
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};

  // Blend compositing
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f;
}

SpectralArcsEffect {
  Shader shader;
  ColorLUT *gradientLUT;
  float time;                    // Animation accumulator
  int resolutionLoc;
  int timeLoc;
  int fftTextureLoc;
  int sampleRateLoc;
  int baseFreqLoc;
  int numOctavesLoc;
  int innerRadiusLoc;
  int outerRadiusLoc;
  int ringWidthLoc;
  int gainLoc;
  int curveLoc;
  int maxSweepLoc;
  int rotationSpeedLoc;
  int seedLoc;
  int glowIntensityLoc;
  int glowFalloffLoc;
  int maxBrightLoc;
  int blurLoc;
  int gradientLUTLoc;
}
```

### Algorithm

Prose description — see `docs/research/spectral-arcs.md` for full formulas.

**Coordinate setup**: Convert pixel to centered UV with aspect correction. Compute `radius = length(uv)` and `angle = atan(uv.y, uv.x)` once outside the loop.

**Ring loop**: `totalRings = numOctaves * 12`. Each ring `i` maps to a semitone. Ring radius is linearly interpolated between `innerRadius` and `outerRadius`.

**FFT lookup**: Convert semitone index to frequency via `baseFreq * pow(2.0, i/12.0)`, then to normalized FFT bin `freq / (sampleRate * 0.5)`. Sample `fftTexture`, apply `gain` and `curve`.

**Per-arc rotation**: `rot_i = time * rotationSpeed * sin(i*i + seed) + i*i`. The `sin(i*i)` term produces pseudo-random speed distribution.

**Angular masking**: Wrap pixel angle relative to arc rotation into `[-PI, PI]`. Arc extent scales with magnitude: `extent = mag * maxSweep`. Use `smoothstep` with `blur` for soft edges.

**Glow**: Inverse-distance falloff from ring radius: `glowIntensity / (abs(radius - r_i) * glowFalloff + epsilon)`, clamped to `maxBright`.

**Color**: Gradient LUT indexed by pitch class: `fract(i / 12.0)`.

**Accumulation**: Additive per ring: `result += color * mag * arcMask * glow`.

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| baseFreq | float | 20.0-880.0 | 220.0 | Yes | Base Freq (Hz) |
| numOctaves | int | 1-10 | 8 | No | Octaves |
| innerRadius | float | 0.05-0.4 | 0.10 | Yes | Inner Radius |
| outerRadius | float | 0.3-0.8 | 0.60 | Yes | Outer Radius |
| ringWidth | float | 0.001-0.02 | 0.005 | Yes | Ring Width |
| gain | float | 0.1-20.0 | 5.0 | Yes | Gain |
| curve | float | 0.5-4.0 | 2.0 | Yes | Contrast |
| maxSweep | float | 0.1-6.28 | 3.14159 | Yes | Max Sweep |
| rotationSpeed | float | 0.0-5.0 | 1.0 | Yes | Rotation Speed |
| seed | float | 0.0-100.0 | 0.0 | Yes | Seed |
| glowIntensity | float | 0.001-0.1 | 0.01 | Yes | Glow Intensity |
| glowFalloff | float | 10.0-500.0 | 100.0 | Yes | Glow Falloff |
| maxBright | float | 0.5-10.0 | 2.0 | Yes | Max Bright |
| blur | float | 0.001-0.1 | 0.02 | Yes | Edge Softness |
| gradient | ColorConfig | — | gradient mode | No | Color |
| blendMode | EffectBlendMode | — | SCREEN | No | Blend Mode |
| blendIntensity | float | 0.0-5.0 | 1.0 | Yes | Blend Intensity |

<!-- Intentional deviations from research:
  - maxBright exposed as UI param (research uses it in formula but doesn't list in param table). User chose to expose both numOctaves and maxBright.
  - blendIntensity range 0-5 matches all other generators in codebase (pitch spiral, interference, etc.)
  - baseFreq, ringWidth, curve marked modulatable beyond research's candidate list for preset variety.
-->

### Constants

- Enum name: `TRANSFORM_SPECTRAL_ARCS_BLEND`
- Display name: `"Spectral Arcs Blend"`
- Category: `TRANSFORM_CATEGORY_GENERATORS` (section 10, badge `"GEN"`)
- Config field name: `spectralArcs` (on `EffectConfig`)
- Effect field name: `spectralArcs` (on `PostEffect`)
- Active flag: `spectralArcsBlendActive` (on `PostEffect`)

---

## Tasks

### Wave 1: Foundation

#### Task 1.1: Effect module header and source

**Files**: `src/effects/spectral_arcs.h` (create), `src/effects/spectral_arcs.cpp` (create)

**Creates**: `SpectralArcsConfig`, `SpectralArcsEffect` structs and lifecycle functions that all other tasks depend on.

**Do**:
- Follow `pitch_spiral.h`/`pitch_spiral.cpp` as the structural pattern — same FFT texture + gradient LUT approach.
- Header: define `SpectralArcsConfig` and `SpectralArcsEffect` per the Design section above. Declare `Init`, `Setup`, `Uninit`, `ConfigDefault`, `RegisterParams`.
- `Setup` signature takes `(SpectralArcsEffect*, const SpectralArcsConfig*, float deltaTime, Texture2D fftTexture)` — same as pitch spiral.
- `Init` loads shader from `shaders/spectral_arcs.fs`, caches all uniform locations, creates `ColorLUT` from `cfg->gradient`.
- `Setup` accumulates `time += deltaTime` (raw time — `rotationSpeed` is a shader uniform applied per-ring, not baked into the accumulator). Binds all uniforms including `fftTexture` and `gradientLUT` texture. Uses `AUDIO_SAMPLE_RATE` from `audio/audio.h` for `sampleRate` uniform.
- `RegisterParams` registers all 15 modulatable float params per the Parameters table.

**Verify**: Header compiles when included from another translation unit.

---

### Wave 2: Parallel Integration

All tasks in Wave 2 modify different files — no file overlap. Run in parallel.

#### Task 2.1: Fragment shader

**Files**: `shaders/spectral_arcs.fs` (create)
**Depends on**: None (standalone GLSL)

**Do**:
- Implement the algorithm from the Design section and `docs/research/spectral-arcs.md`.
- Uniforms: `resolution`, `time`, `fftTexture`, `sampleRate`, `baseFreq`, `numOctaves`, `innerRadius`, `outerRadius`, `ringWidth`, `gain`, `curve`, `maxSweep`, `rotationSpeed`, `seed`, `glowIntensity`, `glowFalloff`, `maxBright`, `blur`, `gradientLUT`.
- Coordinate setup outside loop: centered UV with aspect correction, compute `radius` and `angle` via `atan`.
- Ring loop: `totalRings = numOctaves * 12`. For each ring `i`: compute ring radius, FFT bin lookup, per-arc rotation, angular masking with smoothstep, inverse-distance glow, pitch-class color from LUT, additive accumulation.
- Use `#define TAU 6.28318530718`, `#define PI 3.14159265359`, `#define EPSILON 0.0001`.
- Follow `pitch_spiral.fs` for FFT bin mapping pattern: `freq / (sampleRate * 0.5)`.

**Verify**: Shader parses (no syntax errors when loaded).

#### Task 2.2: Effect registration in effect_config.h

**Files**: `src/config/effect_config.h` (modify)
**Depends on**: Wave 1 complete

**Do**:
- Add `#include "effects/spectral_arcs.h"` with other effect headers.
- Add `TRANSFORM_SPECTRAL_ARCS_BLEND` to `TransformEffectType` enum before `TRANSFORM_EFFECT_COUNT` (place after `TRANSFORM_MOIRE_GENERATOR_BLEND`).
- Add name case in `TransformEffectName()`: `"Spectral Arcs Blend"`.
- Add `TRANSFORM_SPECTRAL_ARCS_BLEND` to `TransformOrderConfig::order` array (before closing brace).
- Add `SpectralArcsConfig spectralArcs;` member to `EffectConfig`.
- Add `IsTransformEnabled` case: `return e->spectralArcs.enabled && e->spectralArcs.blendIntensity > 0.0f;`

**Verify**: `cmake.exe --build build` compiles.

#### Task 2.3: PostEffect integration

**Files**: `src/render/post_effect.h` (modify), `src/render/post_effect.cpp` (modify)
**Depends on**: Wave 1 complete

**Do**:
- `post_effect.h`: Add `#include "effects/spectral_arcs.h"`. Add `SpectralArcsEffect spectralArcs;` member. Add `bool spectralArcsBlendActive;` flag.
- `post_effect.cpp` Init: Add `PitchSpiralEffectInit`-style init call for `spectralArcs`. On failure: log error, free, return NULL.
- `post_effect.cpp` Uninit: Add `SpectralArcsEffectUninit(&pe->spectralArcs);`.
- `post_effect.cpp` RegisterParams: Add `SpectralArcsRegisterParams(&pe->effects.spectralArcs);` after pitch spiral's call.

**Verify**: `cmake.exe --build build` compiles.

#### Task 2.4: Shader setup (generators category)

**Files**: `src/render/shader_setup_generators.h` (modify), `src/render/shader_setup_generators.cpp` (modify), `src/render/shader_setup.cpp` (modify)
**Depends on**: Wave 1 complete

**Do**:
- `shader_setup_generators.h`: Declare `SetupSpectralArcs(PostEffect*)` and `SetupSpectralArcsBlend(PostEffect*)`.
- `shader_setup_generators.cpp`: Add `#include "effects/spectral_arcs.h"`. Implement `SetupSpectralArcs` — delegates to `SpectralArcsEffectSetup(&pe->spectralArcs, &pe->effects.spectralArcs, pe->currentDeltaTime, pe->fftTexture)`. Implement `SetupSpectralArcsBlend` — calls `BlendCompositorApply(pe->blendCompositor, pe->generatorScratch.texture, pe->effects.spectralArcs.blendIntensity, pe->effects.spectralArcs.blendMode)`.
- `shader_setup_generators.cpp` `GetGeneratorScratchPass`: Add case `TRANSFORM_SPECTRAL_ARCS_BLEND` returning `{pe->spectralArcs.shader, SetupSpectralArcs}`.
- `shader_setup.cpp` `GetTransformEffect`: Add case `TRANSFORM_SPECTRAL_ARCS_BLEND` returning `{&pe->blendCompositor->shader, SetupSpectralArcsBlend, &pe->spectralArcsBlendActive}`.

**Verify**: `cmake.exe --build build` compiles.

#### Task 2.5: Render pipeline integration

**Files**: `src/render/render_pipeline.cpp` (modify)
**Depends on**: Wave 1 complete

**Do**:
- Add `type == TRANSFORM_SPECTRAL_ARCS_BLEND` to `IsGeneratorBlendEffect()`.
- Add `pe->spectralArcsBlendActive = (pe->effects.spectralArcs.enabled && pe->effects.spectralArcs.blendIntensity > 0.0f);` in the generator blend active flags section.

**Verify**: `cmake.exe --build build` compiles.

#### Task 2.6: UI panel

**Files**: `src/ui/imgui_effects_generators.cpp` (modify), `src/ui/imgui_effects.cpp` (modify)
**Depends on**: Wave 1 complete

**Do**:
- `imgui_effects_generators.cpp`: Add `#include "effects/spectral_arcs.h"`. Add `static bool sectionSpectralArcs = false;`. Create `DrawGeneratorsSpectralArcs` helper following `DrawGeneratorsPitchSpiral` as pattern. Group controls into sections:
  - **Ring Layout**: `numOctaves` (SliderInt 1-10), `baseFreq` (ModulatableSlider), `innerRadius`, `outerRadius`, `ringWidth`
  - **FFT**: `gain`, `curve`
  - **Animation**: `rotationSpeed`, `maxSweep`, `seed`, `blur`
  - **Glow**: `glowIntensity`, `glowFalloff`, `maxBright`
  - **Color**: `ImGuiDrawColorMode(&cfg->gradient)`
  - **Output**: `blendIntensity`, `blendMode` combo
- Add call to `DrawGeneratorsSpectralArcs` in `DrawGeneratorsCategory` (before SolidColor).
- `imgui_effects.cpp` `GetTransformCategory`: Add `case TRANSFORM_SPECTRAL_ARCS_BLEND: return {"GEN", 10};`.

**Verify**: `cmake.exe --build build` compiles.

#### Task 2.7: Preset serialization

**Files**: `src/config/preset.cpp` (modify)
**Depends on**: Wave 1 complete

**Do**:
- Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(SpectralArcsConfig, enabled, baseFreq, numOctaves, innerRadius, outerRadius, ringWidth, gain, curve, maxSweep, rotationSpeed, seed, glowIntensity, glowFalloff, maxBright, blur, gradient, blendMode, blendIntensity)`.
- Add `to_json` entry: `if (e.spectralArcs.enabled) { j["spectralArcs"] = e.spectralArcs; }`.
- Add `from_json` entry: `e.spectralArcs = j.value("spectralArcs", e.spectralArcs);`.

**Verify**: `cmake.exe --build build` compiles.

#### Task 2.8: Build system

**Files**: `CMakeLists.txt` (modify)
**Depends on**: Wave 1 complete

**Do**:
- Add `src/effects/spectral_arcs.cpp` to `EFFECTS_SOURCES`.

**Verify**: `cmake.exe --build build` compiles.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] Effect appears in transform pipeline with "GEN" badge
- [ ] Effect can be reordered via drag-drop
- [ ] Enabling effect shows concentric ring arcs responding to audio
- [ ] Rings rotate at different speeds, arc sweep scales with amplitude
- [ ] Gradient LUT colors arcs by pitch class
- [ ] All 15 parameters respond in UI
- [ ] Modulation routes to registered parameters
- [ ] Preset save/load preserves settings
- [ ] numOctaves slider changes total ring count in real-time
