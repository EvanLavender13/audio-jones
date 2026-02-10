# Motherboard

Recursive abs-fold kaleidoscope generator that creates glowing circuit traces radiating from center. Each fold iteration maps to a semitone index and samples the FFT texture — bass notes flare the broad outer structure while treble crackles through fine inner detail. Horizontal glow scanlines add a pulsing sheen across the traces. Standard generator blend effect with ColorConfig + ColorLUT coloring.

**Research**: `docs/research/motherboard.md`

## Design

### Types

**MotherboardConfig** (`src/effects/motherboard.h`):

```
enabled         bool                false
iterations      int                 12          // Fold layers, 12 = one per semitone (4-16)
rangeX          float               0.5         // Horizontal fold spacing (0.1-1.0)
rangeY          float               0.2         // Vertical fold offset (0.1-0.5)
size            float               0.1         // Trace thickness (0.01-0.3)
fallOff         float               1.15        // Scale decay per iteration (1.01-2.0)
rotAngle        float               PI/4        // Rotation per fold, radians (0-PI)
glowIntensity   float               0.01        // Glow numerator (0.001-0.1)
thin            float               0.1         // Smoothstep edge width (0.01-0.3)
gain            float               2.0         // Semitone energy multiplier (0.1-10.0)
curve           float               1.5         // Energy exponent (0.5-4.0)
baseBright      float               0.15        // Baseline brightness when silent (0.0-1.0)
scanlineIntensity float             0.04        // Scanline strength, 0=off (0.0-0.2)
scanlineFreq    float               12.0        // Scanline spatial frequency (4.0-24.0)
scanlineSpeed   float               1.0         // Scanline scroll speed (0.0-4.0)
gradient        ColorConfig         {.mode = COLOR_MODE_GRADIENT}
blendMode       EffectBlendMode     EFFECT_BLEND_SCREEN
blendIntensity  float               1.0
```

**MotherboardEffect** (`src/effects/motherboard.h`):

```
shader          Shader
gradientLUT     ColorLUT*
time            float               // Scanline scroll accumulator
resolutionLoc   int
fftTextureLoc   int
sampleRateLoc   int
iterationsLoc   int
rangeXLoc       int
rangeYLoc       int
sizeLoc         int
fallOffLoc      int
rotAngleLoc     int
glowIntensityLoc int
thinLoc         int
gainLoc         int
curveLoc        int
baseBrightLoc   int
scanlineIntensityLoc int
scanlineFreqLoc int
scanlineSpeedLoc int
timeLoc         int
gradientLUTLoc  int
```

### Algorithm

The shader renders a full-screen generative pattern. No `texture0` input — pure generator.

**Coordinate setup**: Map `fragTexCoord * resolution` to centered, aspect-corrected coords `p` in [-2, 2] range:
```glsl
vec2 p = (fragTexCoord * resolution - resolution * 0.5) / resolution.y * 4.0;
```

**Scale variable**: `float a = 1.0;` — decays each iteration by `fallOff`.

**Core fold loop** (`i = 0` to `iterations - 1`):

1. **Abs-fold**: `p = abs(p) - vec2(rangeX, rangeY) * a;`
2. **Rotate**: Apply 2D rotation by `rotAngle`:
   ```glsl
   float c = cos(rotAngle), s = sin(rotAngle);
   p = mat2(c, -s, s, c) * p;
   ```
3. **Y-fold**: `p.y = abs(p.y) - rangeY;`
4. **Distance field**:
   ```glsl
   float dist = max(abs(p.x) + a * sin(TAU * float(i) / float(iterations)), p.y - size);
   ```
5. **Depth-weighted semitone lookup**: Each iteration maps to a semitone (`i % 12`) at an octave center interpolated from low (iteration 0) to high (iteration N-1). Sample FFT texture at the semitone's frequency bin:
   ```glsl
   int note = i % 12;
   float octaveCenter = mix(float(lowestOctave), float(highestOctave),
                            float(i) / max(float(iterations - 1), 1.0));
   float freq = baseFreq * pow(2.0, float(note) / 12.0 + octaveCenter);
   float bin = freq / (sampleRate * 0.5);
   float energy = 0.0;
   if (bin <= 1.0) {
       energy = texture(fftTexture, vec2(bin, 0.5)).r;
       energy = pow(clamp(energy * gain, 0.0, 1.0), curve);
   }
   float brightness = baseBright + energy;
   ```
   Where `baseFreq` = 55.0 Hz (A1), `lowestOctave` = 0, `highestOctave` = `numOctaves - 1`. Pass `numOctaves = 8` as a constant uniform derived from `iterations`: `numOctaves = max(iterations / 12, 1) + 3` or simply hardcode 8 octaves. Use `sampleRate` = 48000.
6. **Glow accumulation**:
   ```glsl
   color += smoothstep(thin, 0.0, dist) * glowIntensity / max(abs(dist), 0.001) * brightness;
   ```
7. **Scale decay**: `a /= fallOff;`

**Color**: Each iteration samples `gradientLUT` at `fract(float(i) / 12.0)` for per-layer tinting. Multiply the per-iteration glow contribution by this color before adding to the accumulator:
```glsl
vec3 layerColor = texture(gradientLUT, vec2(fract(float(i) / 12.0), 0.5)).rgb;
color += glow * layerColor * brightness;
```
Where `glow = smoothstep(thin, 0.0, dist) * glowIntensity / max(abs(dist), 0.001)`.

**Scanlines** (after loop): Add horizontal glow:
```glsl
if (scanlineIntensity > 0.0) {
    color += scanlineIntensity / max(abs(sin(p.y * scanlineFreq + time * scanlineSpeed)), 0.01);
}
```
Note: `p` here is the final transformed coordinate from the last iteration. For a screen-space scanline, use the original `fragTexCoord.y * resolution.y` instead.

**Output**: `finalColor = vec4(color, 1.0);`

<!-- Intentional deviation: Research doc says "weight contributions from multiple octaves with a falloff." Plan uses single-bin FFT lookup per iteration, matching spectral_arcs pattern. Simpler, sufficient for visual quality. -->
<!-- Intentional deviation: Research doc says "Replaces Circuit Board." User directed to keep Circuit Board as-is — Motherboard is an addition, not a replacement. -->

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| iterations | int | 4-16 | 12 | No | Iterations |
| rangeX | float | 0.1-1.0 | 0.5 | Yes | Range X |
| rangeY | float | 0.1-0.5 | 0.2 | Yes | Range Y |
| size | float | 0.01-0.3 | 0.1 | Yes | Size |
| fallOff | float | 1.01-2.0 | 1.15 | Yes | Fall Off |
| rotAngle | float | 0-PI | PI/4 | Yes | Rotation |
| glowIntensity | float | 0.001-0.1 | 0.01 | Yes | Glow |
| thin | float | 0.01-0.3 | 0.1 | Yes | Thin |
| gain | float | 0.1-10.0 | 2.0 | Yes | Gain |
| curve | float | 0.5-4.0 | 1.5 | Yes | Curve |
| baseBright | float | 0.0-1.0 | 0.15 | Yes | Base Bright |
| scanlineIntensity | float | 0.0-0.2 | 0.04 | Yes | Scanlines |
| scanlineFreq | float | 4.0-24.0 | 12.0 | Yes | Scanline Freq |
| scanlineSpeed | float | 0.0-4.0 | 1.0 | Yes | Scanline Speed |
| blendIntensity | float | 0.0-5.0 | 1.0 | Yes | Blend Intensity |

### Constants

- Enum name: `TRANSFORM_MOTHERBOARD_BLEND`
- Display name: `"Motherboard Blend"`
- Category badge: `"GEN"`
- Section index: `10`
- Flags: `EFFECT_FLAG_BLEND`

---

## Tasks

### Wave 1: Config & Header

#### Task 1.1: Create effect module header and source

**Files**: `src/effects/motherboard.h` (create), `src/effects/motherboard.cpp` (create)
**Creates**: `MotherboardConfig`, `MotherboardEffect` structs and lifecycle function declarations

**Do**:
- Follow `src/effects/spectral_arcs.h` as the structural pattern (audio-reactive generator with ColorLUT)
- Define `MotherboardConfig` and `MotherboardEffect` per the Design section
- Init takes `(MotherboardEffect*, const MotherboardConfig*)` — allocates ColorLUT from config
- Setup takes `(MotherboardEffect*, const MotherboardConfig*, float deltaTime, Texture2D fftTexture)` — same signature as `SpectralArcsEffectSetup`
- Accumulate `time += scanlineSpeed * deltaTime` in Setup
- Include `audio/audio.h` for `AUDIO_SAMPLE_RATE`, `config/constants.h`, `render/color_lut.h`
- RegisterParams registers all float params except `iterations` (see Design > Parameters for ranges)
- `rotAngle` uses `ROTATION_OFFSET_MAX` as the max bound

**Verify**: `cmake.exe --build build` compiles.

---

### Wave 2: Integration (all parallel — no file overlap)

#### Task 2.1: Effect registration

**Files**: `src/config/effect_config.h` (modify), `src/config/effect_descriptor.h` (modify)
**Depends on**: Wave 1 complete

**Do**:
- Add `#include "effects/motherboard.h"` to `effect_config.h`
- Add `TRANSFORM_MOTHERBOARD_BLEND` to `TransformEffectType` enum before `TRANSFORM_EFFECT_COUNT`, placed among other generator blend entries (near `TRANSFORM_NEBULA_BLEND`)
- Add `MotherboardConfig motherboard;` to `EffectConfig` struct
- Add descriptor row in `EFFECT_DESCRIPTORS[]`:
  ```
  [TRANSFORM_MOTHERBOARD_BLEND] = {
      TRANSFORM_MOTHERBOARD_BLEND, "Motherboard Blend", "GEN", 10,
      offsetof(EffectConfig, motherboard.enabled), EFFECT_FLAG_BLEND
  },
  ```

**Verify**: Compiles.

#### Task 2.2: Shader

**Files**: `shaders/motherboard.fs` (create)
**Depends on**: Wave 1 complete

**Do**:
- Implement the Algorithm section from this plan document
- Uniforms: `resolution`, `fftTexture`, `gradientLUT`, `sampleRate`, `iterations`, `rangeX`, `rangeY`, `size`, `fallOff`, `rotAngle`, `glowIntensity`, `thin`, `gain`, `curve`, `baseBright`, `scanlineIntensity`, `scanlineFreq`, `scanlineSpeed`, `time`
- Follow `shaders/spectral_arcs.fs` for FFT texture sampling pattern
- Use `#version 330`, `in vec2 fragTexCoord`, `out vec4 finalColor`
- Hardcode `numOctaves = 8` and `baseFreq = 55.0` in shader (A1 base)

**Verify**: Compiles (shader loaded at runtime).

#### Task 2.3: PostEffect integration

**Files**: `src/render/post_effect.h` (modify), `src/render/post_effect.cpp` (modify)
**Depends on**: Wave 1 complete

**Do**:
- Add `#include "effects/motherboard.h"` to both files
- Add `MotherboardEffect motherboard;` to `PostEffect` struct
- Add init in `PostEffectInit()`: `MotherboardEffectInit(&pe->motherboard, &pe->effects.motherboard)` with error handling matching spectral arcs pattern
- Add uninit in `PostEffectUninit()`: `MotherboardEffectUninit(&pe->motherboard)`
- Add `MotherboardRegisterParams(&pe->effects.motherboard)` in `PostEffectRegisterParams()`

**Verify**: Compiles.

#### Task 2.4: Shader setup

**Files**: `src/render/shader_setup_generators.h` (modify), `src/render/shader_setup_generators.cpp` (modify), `src/render/shader_setup.cpp` (modify)
**Depends on**: Wave 1 complete

**Do**:
- In `shader_setup_generators.h`: declare `void SetupMotherboard(PostEffect *pe);` and `void SetupMotherboardBlend(PostEffect *pe);`
- In `shader_setup_generators.cpp`:
  - Add `#include "effects/motherboard.h"`
  - Implement `SetupMotherboard`: calls `MotherboardEffectSetup(&pe->motherboard, &pe->effects.motherboard, pe->currentDeltaTime, pe->fftTexture)`
  - Implement `SetupMotherboardBlend`: calls `BlendCompositorApply(pe->blendCompositor, pe->generatorScratch.texture, pe->effects.motherboard.blendIntensity, pe->effects.motherboard.blendMode)`
  - Add case to `GetGeneratorScratchPass()`: `case TRANSFORM_MOTHERBOARD_BLEND: return {pe->motherboard.shader, SetupMotherboard};`
- In `shader_setup.cpp`:
  - Add dispatch case: `case TRANSFORM_MOTHERBOARD_BLEND: return {&pe->blendCompositor->shader, SetupMotherboardBlend, &pe->effects.motherboard.enabled};`

**Verify**: Compiles.

#### Task 2.5: Build system

**Files**: `CMakeLists.txt` (modify)
**Depends on**: Wave 1 complete

**Do**:
- Add `src/effects/motherboard.cpp` to `EFFECTS_SOURCES`

**Verify**: Compiles.

#### Task 2.6: UI panel

**Files**: `src/ui/imgui_effects_gen_texture.cpp` (modify)
**Depends on**: Wave 1 complete

**Do**:
- Add `#include "effects/motherboard.h"` at top
- Add `static bool sectionMotherboard = false;`
- Create `DrawGeneratorsMotherboard(EffectConfig*, const ModSources*, const ImU32)` following `DrawGeneratorsPlasma` pattern:
  - Checkbox enables with `MoveTransformToEnd(&e->transformOrder, TRANSFORM_MOTHERBOARD_BLEND)`
  - `ImGui::SliderInt` for iterations (4-16)
  - Group params into sections with `ImGui::SeparatorText`:
    - "Geometry": rangeX, rangeY, size, fallOff, rotAngle (`ModulatableSliderAngleDeg`)
    - "Glow": glowIntensity (`ModulatableSliderLog`), thin
    - "Audio": gain, curve, baseBright
    - "Scanlines": scanlineIntensity, scanlineFreq, scanlineSpeed
    - "Color": `ImGuiDrawColorMode(&cfg->gradient)`
    - "Output": blendIntensity, blendMode combo
- Add call in `DrawGeneratorsTexture()`: `ImGui::Spacing(); DrawGeneratorsMotherboard(e, modSources, categoryGlow);`

**Verify**: Compiles.

#### Task 2.7: Preset serialization

**Files**: `src/config/preset.cpp` (modify)
**Depends on**: Wave 1 complete

**Do**:
- Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(MotherboardConfig, enabled, iterations, rangeX, rangeY, size, fallOff, rotAngle, glowIntensity, thin, gain, curve, baseBright, scanlineIntensity, scanlineFreq, scanlineSpeed, blendIntensity)` with other config macros
  - Note: `gradient` and `blendMode` — check if `ColorConfig` and `EffectBlendMode` already have JSON macros in preset.cpp. If yes, include them. If not, omit (they'll use defaults on load).
- Add `if (e.motherboard.enabled) { j["motherboard"] = e.motherboard; }` in `to_json`
- Add `e.motherboard = j.value("motherboard", e.motherboard);` in `from_json`

**Verify**: Compiles.

---

## Final Verification

- [x] Build succeeds with no warnings
- [x] Effect appears in Generators > Texture section
- [x] Effect shows "GEN" category badge in transform pipeline
- [x] Enabling effect adds it to the pipeline list
- [x] Fold geometry responds to rangeX, rangeY, rotAngle, size, fallOff
- [x] Audio reactivity: different semitones flare different fold layers
- [x] Color gradient applies per-layer coloring
- [x] Blend mode and intensity compositing works
- [x] Modulation routes to registered parameters
- [x] Preset save/load preserves all settings

---

## Implementation Notes

Deviations from the original plan during implementation:

1. **Removed `thin` parameter** — hardcoded `0.1` in shader smoothstep. Internal constant, not a useful creative control.

2. **Removed scanline system** (scanlineIntensity, scanlineFreq, scanlineSpeed) — the plan specified screen-space horizontal scanlines via `fragTexCoord.y * resolution.y`, but the original Shadertoy uses the post-fold `p.y` coordinate. Replaced with a single `accentIntensity` param (default 0.04, range 0.0-0.2) controlling `0.04 / max(abs(sin(p.y * 12.0 + time)), 0.01)` on the folded coordinate. Frequency hardcoded at 12.0 — varying it in chaotic fold-space produces no intuitive visual change.

3. **Added `baseFreq` and `numOctaves` as config params** — plan hardcoded these in the shader. Every other FFT-reactive generator (spectral arcs, filaments, signal frames) exposes them as config fields with UI sliders.

4. **Added `rotationSpeed`** — plan omitted animation. Every other generator has CPU-accumulated rotation. Shader computes `totalAngle = rotAngle + rotationAccum`.

5. **Changed `gain` default** from 2.0 to 5.0, range from 0.1-10.0 to 1.0-20.0 — matches the standard generator FFT parameter pattern.

6. **Widened parameter ranges** — plan ranges were too narrow for creative exploration:
   - rangeX: 0.1-1.0 → 0.0-2.0
   - rangeY: 0.1-0.5 → 0.0-1.0
   - size: 0.01-0.3 → 0.0-1.0
   - fallOff: 1.01-2.0 → 1.0-3.0

7. **UI follows standard FFT generator layout** — "FFT" section first (Octaves, Base Freq, Gain, Contrast, Base Bright), then Geometry, Glow, Animation, Color, Output. `curve` labeled "Contrast" matching other generators.
