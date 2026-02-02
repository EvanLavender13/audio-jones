# Interference

Concentric circular waves radiating from multiple point sources, creating classic constructive/destructive interference patterns. Where wavefronts align, intensity doubles; where they oppose, they cancel to darkness. Sources drift via Lissajous motion, producing moiré-like geometry that breathes and morphs.

## Specification

### Types

```cpp
// src/config/interference_config.h
#ifndef INTERFERENCE_CONFIG_H
#define INTERFERENCE_CONFIG_H

#include "config/dual_lissajous_config.h"
#include "render/color_config.h"
#include <stdbool.h>

typedef struct InterferenceConfig {
  bool enabled = false;

  // Sources
  int sourceCount = 3;        // Number of wave emitters (1-8)
  float baseRadius = 0.4f;    // Distance of sources from center (0.0-1.0)
  float patternAngle = 0.0f;  // Rotation of source arrangement (radians)
  DualLissajousConfig lissajous; // Source motion pattern

  // Wave properties
  float waveFreq = 30.0f;     // Ripple density (5.0-100.0)
  float waveSpeed = 2.0f;     // Animation speed (0.0-10.0)

  // Falloff
  int falloffType = 3;        // 0=None, 1=Inverse, 2=InvSquare, 3=Gaussian
  float falloffStrength = 1.0f; // Attenuation rate (0.0-5.0)

  // Boundaries (mirror sources at screen edges)
  bool boundaries = false;
  float reflectionGain = 0.5f; // Mirror source attenuation (0.0-1.0)

  // Visualization
  int visualMode = 0;         // 0=Raw, 1=Absolute, 2=Contour
  int contourCount = 0;       // Band count for contour mode (0-20, 0=smooth)
  float visualGain = 1.5f;    // Output intensity (0.5-5.0)

  // Chromatic mode (RGB wavelength separation)
  bool chromatic = false;
  float chromaSpread = 0.03f; // RGB wavelength difference (0.0-0.1)

  // Color
  int colorMode = 0;          // 0=Intensity (global), 1=PerSource
  ColorConfig color = {.mode = COLOR_MODE_GRADIENT};

} InterferenceConfig;

#endif // INTERFERENCE_CONFIG_H
```

### Algorithm

#### Core Wave Equation (GPU)

```glsl
// Sum waves from all sources
float totalWave = 0.0;
for (int i = 0; i < sourceCount; i++) {
    float dist = length(uv - sources[i]);
    float phase = phases[i]; // Per-source phase offset
    float wave = sin(dist * waveFreq - time * waveSpeed + phase);
    float atten = falloff(dist, falloffType, falloffStrength);
    totalWave += wave * atten;
}
```

#### Falloff Functions

```glsl
float falloff(float d, int type, float strength) {
    float safeDist = max(d, 0.001);
    if (type == 0) return 1.0;                                    // None
    if (type == 1) return 1.0 / (1.0 + safeDist * strength);      // Inverse
    if (type == 2) return 1.0 / (1.0 + safeDist * safeDist * strength); // Inverse square
    return exp(-safeDist * safeDist * strength);                  // Gaussian (default)
}
```

#### Visualization Modes

```glsl
float visualize(float wave, int mode, int contourCount, float gain) {
    if (mode == 1) {
        // Absolute: sharper rings with dark nodes
        return abs(wave) * gain;
    }
    if (mode == 2) {
        // Contour: discrete bands
        return floor(wave * float(contourCount) + 0.5) / float(contourCount) * gain;
    }
    // Raw: smooth gradients
    return wave * gain;
}
```

#### Color Modes

**Mode 0 - Intensity (global):**
```glsl
// Map [-1,1] to [0,1] for LUT sampling
float t = totalWave * 0.5 + 0.5;
vec3 color = texture(colorLUT, vec2(t, 0.5)).rgb;
float brightness = abs(totalWave) * visualGain;
finalColor = vec4(color * brightness, 1.0);
```

**Mode 1 - PerSource:**
Each source samples the LUT centered at its uniform position, with wave intensity shifting within that neighborhood.
```glsl
vec3 color = vec3(0.0);
for (int i = 0; i < sourceCount; i++) {
    float dist = length(uv - sources[i]);
    float wave = sin(dist * waveFreq - time * waveSpeed + phases[i]);
    float atten = falloff(dist, falloffType, falloffStrength);

    // Sample LUT centered at source's position, wave shifts within neighborhood
    float baseOffset = (float(i) + 0.5) / float(sourceCount);
    float neighborhoodSize = 0.5 / float(sourceCount);
    float lutPos = baseOffset + wave * neighborhoodSize;
    vec3 srcColor = texture(colorLUT, vec2(lutPos, 0.5)).rgb;

    color += srcColor * abs(wave) * atten;
}
finalColor = vec4(color * visualGain, 1.0);
```

#### Chromatic Mode

When enabled, compute separate wavelengths for R/G/B channels. Bypasses colorMode—computes RGB directly.
```glsl
vec3 chromaScale = vec3(1.0 - chromaSpread, 1.0, 1.0 + chromaSpread);
vec3 wave;
for (int c = 0; c < 3; c++) {
    wave[c] = 0.0;
    for (int i = 0; i < sourceCount; i++) {
        float dist = length(uv - sources[i]);
        float w = sin(dist * waveFreq * chromaScale[c] - time * waveSpeed + phases[i]);
        wave[c] += w * falloff(dist, falloffType, falloffStrength);
    }
}
wave = abs(wave); // Dark nodes where waves cancel
wave = tanh(wave * visualGain); // Tonemap
finalColor = vec4(wave, 1.0);
```

#### Mirror Sources (Boundaries)

For each real source, create 4 mirror sources reflected across screen edges:
```glsl
if (boundaries) {
    for (int i = 0; i < sourceCount; i++) {
        vec2 src = sources[i];
        vec2 mirrors[4];
        mirrors[0] = vec2(-2.0 * aspect - src.x, src.y);  // Left
        mirrors[1] = vec2( 2.0 * aspect - src.x, src.y);  // Right
        mirrors[2] = vec2(src.x, -2.0 - src.y);           // Bottom
        mirrors[3] = vec2(src.x,  2.0 - src.y);           // Top

        for (int m = 0; m < 4; m++) {
            float dist = length(uv - mirrors[m]);
            float wave = sin(dist * waveFreq - time * waveSpeed + phases[i]);
            float atten = falloff(dist, falloffType, falloffStrength) * reflectionGain;
            totalWave += wave * atten;
        }
    }
}
```

#### Source Animation (CPU)

Reuse Cymatics source pattern with DualLissajousConfig:
```cpp
// In render_pipeline.cpp, before shader dispatch
float sourcePhase = pe->interferenceSourcePhase;
for (int i = 0; i < cfg.sourceCount; i++) {
    float angle = TWO_PI * i / cfg.sourceCount + cfg.patternAngle;
    float baseX = cfg.baseRadius * cosf(angle);
    float baseY = cfg.baseRadius * sinf(angle);
    float offsetX, offsetY;
    DualLissajousCompute(&cfg.lissajous, sourcePhase, i * TWO_PI / cfg.sourceCount, &offsetX, &offsetY);
    sources[i] = {baseX + offsetX, baseY + offsetY};
    phases[i] = i * TWO_PI / cfg.sourceCount; // Uniform phase offset
}
```

#### Tonemap

When sources overlap, intensities can exceed [-1,1]. Apply tanh for soft clipping:
```glsl
totalWave = tanh(totalWave * visualGain);
```

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| sourceCount | int | 1-8 | 3 | No | Sources |
| baseRadius | float | 0.0-1.0 | 0.4 | Yes | Radius |
| patternAngle | float | 0-2π | 0.0 | Yes | Pattern Angle |
| lissajous | DualLissajousConfig | — | default | Yes (nested) | Motion |
| waveFreq | float | 5.0-100.0 | 30.0 | Yes | Wave Freq |
| waveSpeed | float | 0.0-10.0 | 2.0 | Yes | Wave Speed |
| falloffType | int | 0-3 | 3 | No | Falloff |
| falloffStrength | float | 0.0-5.0 | 1.0 | Yes | Falloff Strength |
| boundaries | bool | — | false | No | Boundaries |
| reflectionGain | float | 0.0-1.0 | 0.5 | Yes | Reflection |
| visualMode | int | 0-2 | 0 | No | Visual Mode |
| contourCount | int | 0-20 | 0 | No | Contours |
| visualGain | float | 0.5-5.0 | 1.5 | Yes | Intensity |
| chromatic | bool | — | false | No | Chromatic |
| chromaSpread | float | 0.0-0.1 | 0.03 | Yes | Chroma Spread |
| colorMode | int | 0-1 | 0 | No | Color Mode |
| color | ColorConfig | — | gradient | No | Color |

### Constants

- Enum name: `GENERATOR_INTERFERENCE` (not a transform—generators don't use TransformEffectType)
- Display name: `"Interference"`
- Category: GENERATORS
- Max sources with boundaries: 8 real + 32 mirrors = 40 distance calculations per pixel

---

## Tasks

### Wave 1: Foundation

#### Task 1.1: Config Header

**Files**: `src/config/interference_config.h`
**Creates**: InterferenceConfig struct that other files #include

**Build**:
1. Create header with include guard `INTERFERENCE_CONFIG_H`
2. Include `config/dual_lissajous_config.h`, `render/color_config.h`, `<stdbool.h>`
3. Define `InterferenceConfig` struct exactly as specified in Types section above
4. All fields use defaults from Parameters table

**Verify**: `cmake.exe --build build` compiles.

---

### Wave 2: Parallel Implementation

#### Task 2.1: Shader

**Files**: `shaders/interference.fs`
**Depends on**: Wave 1 complete

**Build**:
1. Create fragment shader with `#version 330`
2. Inputs: `fragTexCoord`, uniforms for all config params, `texture0` (source), `colorLUT`, `resolution`, `time`
3. Source positions passed as `uniform vec2 sources[8]` and `uniform float phases[8]`
4. Implement `falloff()` function per Algorithm section
5. Implement `visualize()` function per Algorithm section
6. Main function:
   - Convert fragTexCoord to centered UV with aspect correction
   - If `chromatic`: use chromatic algorithm, skip colorMode logic
   - Else if `colorMode == 1`: use PerSource algorithm
   - Else: use Intensity algorithm
   - If `boundaries`: add mirror source contributions
   - Apply `visualize()` and `tanh()` tonemap
   - Additive blend with source texture

**Verify**: Shader file created, build still compiles.

---

#### Task 2.2: PostEffect Integration

**Files**: `src/render/post_effect.h`, `src/render/post_effect.cpp`
**Depends on**: Wave 1 complete

**Build**:
1. In `post_effect.h`:
   - Add `Shader interferenceShader;`
   - Add uniform location ints: `interferenceResolutionLoc`, `interferenceTimeLoc`, `interferenceSourcesLoc`, `interferencePhasesLoc`, `interferenceWaveFreqLoc`, `interferenceWaveSpeedLoc`, `interferenceFalloffTypeLoc`, `interferenceFalloffStrengthLoc`, `interferenceBoundariesLoc`, `interferenceReflectionGainLoc`, `interferenceVisualModeLoc`, `interferenceContourCountLoc`, `interferenceVisualGainLoc`, `interferenceChromaticLoc`, `interferenceChromaSpreadLoc`, `interferenceColorModeLoc`, `interferenceColorLUTLoc`, `interferenceAspectLoc`, `interferenceSourceCountLoc`
   - Add `ColorLUT *interferenceColorLUT;`
   - Add `float interferenceTime;` (wave animation accumulator)
   - Add `float interferenceSourcePhase;` (Lissajous phase accumulator)
2. In `post_effect.cpp`:
   - Load shader: `pe->interferenceShader = LoadShader(NULL, "shaders/interference.fs");`
   - Get all uniform locations with `GetShaderLocation()`
   - Initialize `pe->interferenceColorLUT = ColorLUTInit(&pe->effects.interference.color);`
   - Initialize `pe->interferenceTime = 0.0f;` and `pe->interferenceSourcePhase = 0.0f;`
   - In `PostEffectUninit()`: unload shader, uninit ColorLUT

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 2.3: EffectConfig Integration

**Files**: `src/config/effect_config.h`
**Depends on**: Wave 1 complete

**Build**:
1. Add `#include "interference_config.h"` in alphabetical order with other config includes
2. Add `InterferenceConfig interference;` field to `EffectConfig` struct (after `constellation`, before other fields alphabetically)

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 2.4: Preset Serialization

**Files**: `src/config/preset.cpp`
**Depends on**: Wave 1 complete

**Build**:
1. Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT()` macro for `InterferenceConfig` with all fields (near other config serializers)
2. In `to_json(EffectConfig)`: add `if (e.interference.enabled) { j["interference"] = e.interference; }`
3. In `from_json(EffectConfig)`: add `e.interference = j.value("interference", e.interference);`

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 2.5: Shader Setup

**Files**: `src/render/shader_setup_generators.h`, `src/render/shader_setup_generators.cpp`
**Depends on**: Wave 1 complete

**Build**:
1. In `shader_setup_generators.h`: add `void SetupInterference(PostEffect *pe);`
2. In `shader_setup_generators.cpp`:
   - Add `#include "config/interference_config.h"`
   - Implement `SetupInterference()`:
     - Get `InterferenceConfig &cfg = pe->effects.interference;`
     - Update ColorLUT: `ColorLUTUpdate(pe->interferenceColorLUT, &cfg.color);`
     - Compute source positions using `DualLissajousCompute()` loop per Algorithm section
     - Set all uniforms: resolution, time, sources array, phases array, all config params
     - Bind colorLUT texture

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 2.6: Render Pipeline

**Files**: `src/render/render_pipeline.cpp`
**Depends on**: Wave 1 complete

**Build**:
1. Add `#include "config/interference_config.h"` if not already included
2. In `RenderPipelineApplyOutput()`, after Plasma generator pass (around line 440):
   - Accumulate phases: `pe->interferenceTime += deltaTime * pe->effects.interference.waveSpeed;`
   - Accumulate source phase: `pe->interferenceSourcePhase += deltaTime;`
   - Add generator pass:
     ```cpp
     if (pe->effects.interference.enabled) {
       RenderPass(pe, src, &pe->pingPong[writeIdx], pe->interferenceShader,
                  SetupInterference);
       src = &pe->pingPong[writeIdx];
       writeIdx = 1 - writeIdx;
     }
     ```

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 2.7: UI Panel

**Files**: `src/ui/imgui_effects_generators.h`, `src/ui/imgui_effects_generators.cpp`
**Depends on**: Wave 1 complete

**Build**:
1. In `imgui_effects_generators.cpp`:
   - Add `#include "config/interference_config.h"`
   - Add `static bool sectionInterference = false;`
   - Create `DrawGeneratorsInterference()` function:
     - `DrawSectionBegin("Interference", ...)` with checkbox
     - If enabled, show controls grouped by category:
       - **Sources**: sourceCount (SliderInt 1-8), baseRadius, patternAngle (use `ModulatableSliderAngleDeg`)
       - **Motion**: DualLissajous controls (amplitude, freqX1, freqY1, freqX2, freqY2, offsetX2, offsetY2)
       - **Waves**: waveFreq, waveSpeed
       - **Falloff**: falloffType (Combo: None/Inverse/InvSquare/Gaussian), falloffStrength
       - **Boundaries**: boundaries checkbox, reflectionGain (if boundaries enabled)
       - **Visualization**: visualMode (Combo: Raw/Absolute/Contour), contourCount (if contour), visualGain
       - **Chromatic**: chromatic checkbox, chromaSpread (if chromatic enabled)
       - **Color**: colorMode (Combo: Intensity/PerSource), ImGuiDrawColorMode
   - Add call to `DrawGeneratorsInterference()` in `DrawGeneratorsCategory()` after Plasma
2. Use `ModulatableSlider` for float params that are modulatable per Parameters table

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 2.8: Param Registry

**Files**: `src/automation/param_registry.cpp`
**Depends on**: Wave 1 complete

**Build**:
1. Add entries to `PARAM_TABLE` for modulatable interference params:
   - `interference.baseRadius` {0.0f, 1.0f}
   - `interference.patternAngle` {0.0f, 6.283f}
   - `interference.waveFreq` {5.0f, 100.0f}
   - `interference.waveSpeed` {0.0f, 10.0f}
   - `interference.falloffStrength` {0.0f, 5.0f}
   - `interference.reflectionGain` {0.0f, 1.0f}
   - `interference.visualGain` {0.5f, 5.0f}
   - `interference.chromaSpread` {0.0f, 0.1f}
   - `interference.lissajous.amplitude` {0.0f, 0.5f}
   - `interference.lissajous.freqX1` {0.0f, 1.0f}
   - `interference.lissajous.freqY1` {0.0f, 1.0f}
   - `interference.lissajous.freqX2` {0.0f, 1.0f}
   - `interference.lissajous.freqY2` {0.0f, 1.0f}
   - `interference.lissajous.offsetX2` {0.0f, 6.283f}
   - `interference.lissajous.offsetY2` {0.0f, 6.283f}
2. Keep entries sorted alphabetically within the table

**Verify**: `cmake.exe --build build` compiles.

---

## Final Verification

- [ ] `cmake.exe --build build` succeeds with no warnings
- [ ] Effect appears in Generators category in UI
- [ ] Enable interference, see concentric rings from multiple sources
- [ ] Sources animate with Lissajous motion
- [ ] Wave frequency slider changes ring density
- [ ] Falloff type changes how waves fade with distance
- [ ] Boundaries toggle adds edge reflections
- [ ] Visual modes (Raw/Absolute/Contour) produce distinct looks
- [ ] Chromatic mode creates RGB fringing
- [ ] Color modes (Intensity/PerSource) produce different coloring
- [ ] Gradient editor controls output colors
- [ ] Parameters respond to LFO modulation

---

<!-- Intentional deviation: PerSource color mode uses dynamic LUT neighborhood shifting based on wave intensity, rather than fixed per-source colors. User-confirmed design decision for richer color variation within each source's contribution. -->

*Research: `docs/research/interference.md`*
