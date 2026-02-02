# Plasma

Glowing vertical lightning bolts that drift horizontally across the screen. FBM noise displaces bolt paths to create jagged lightning or smooth plasma tendrils. Multiple depth layers create volumetric appearance. Gradient LUT samples by distance for core-to-halo color variation.

## Specification

### Types

```cpp
// src/config/plasma_config.h
#ifndef PLASMA_CONFIG_H
#define PLASMA_CONFIG_H

#include "render/color_config.h"
#include <stdbool.h>

typedef struct PlasmaConfig {
  bool enabled = false;

  // Bolt configuration
  int boltCount = 3;          // Number of vertical bolts (1-8)
  int layerCount = 2;         // Depth layers: background bolts at smaller scale (1-3)
  int octaves = 6;            // FBM octaves: 1-3 smooth plasma, 6+ jagged lightning (1-10)
  int falloffType = 1;        // 0=Sharp (1/d²), 1=Linear (1/d), 2=Soft (1/√d)

  // Animation
  float driftSpeed = 0.5f;    // Horizontal wandering rate (0.0-2.0)
  float driftAmount = 0.3f;   // Horizontal wandering distance (0.0-1.0)
  float animSpeed = 0.8f;     // Noise animation rate (0.0-5.0)

  // Appearance
  float displacement = 1.0f;  // Path distortion strength (0.0-2.0)
  float glowRadius = 0.07f;   // Halo width multiplier (0.01-0.3)
  float coreBrightness = 1.5f; // Overall intensity (0.5-3.0)
  float flickerAmount = 0.2f; // Random intensity jitter, 0=smooth, 1=harsh (0.0-1.0)

  // Color (gradient sampled by distance: core → halo)
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};

} PlasmaConfig;

#endif // PLASMA_CONFIG_H
```

### Shader Algorithm

```glsl
// shaders/plasma.fs
#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;    // Source texture for additive blend
uniform sampler2D gradientLUT; // 1D gradient: core (0.0) → halo (1.0)
uniform vec2 resolution;

// Phase accumulators (accumulated on CPU)
uniform float animPhase;       // For FBM noise evolution
uniform float driftPhase;      // For bolt horizontal drift
uniform float flickerTime;     // Independent time for flicker (1:1 with real time)

// Parameters
uniform int boltCount;
uniform int layerCount;
uniform int octaves;
uniform int falloffType;
uniform float driftAmount;
uniform float displacement;
uniform float glowRadius;
uniform float coreBrightness;
uniform float flickerAmount;

// Simple hash for randomness
float hash(float n) {
    return fract(sin(n) * 43758.5453);
}

// 2D noise
float noise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    float n = i.x + i.y * 57.0;
    return mix(mix(hash(n), hash(n + 1.0), f.x),
               mix(hash(n + 57.0), hash(n + 58.0), f.x), f.y);
}

// 2D rotation matrix
mat2 rotate2d(float angle) {
    float c = cos(angle);
    float s = sin(angle);
    return mat2(c, -s, s, c);
}

// Fractal Brownian Motion
float fbm(vec2 p, int oct) {
    float value = 0.0;
    float amplitude = 0.5;
    for (int i = 0; i < oct; i++) {
        value += amplitude * noise(p);
        p = rotate2d(0.45) * p; // Rotate to break grid artifacts
        p *= 2.0;               // Lacunarity
        amplitude *= 0.5;       // Persistence
    }
    return value;
}

// Falloff functions
float applyFalloff(float d, int type) {
    float safeDist = max(d, 0.001);
    if (type == 0) {
        // Sharp: 1/d² - tight bright core
        return 1.0 / (safeDist * safeDist);
    } else if (type == 2) {
        // Soft: 1/√d - wide hazy plasma
        return 1.0 / sqrt(safeDist);
    }
    // Linear (default): 1/d - balanced glow
    return 1.0 / safeDist;
}

void main() {
    vec2 uv = fragTexCoord - 0.5;
    uv.x *= resolution.x / resolution.y; // Aspect correction

    vec3 total = vec3(0.0);

    // Process each depth layer (back to front)
    for (int layer = layerCount - 1; layer >= 0; layer--) {
        float layerScale = 1.0 - float(layer) * 0.3;     // Layers get smaller toward back
        float layerBright = 1.0 - float(layer) * 0.35;   // Layers get dimmer toward back
        float layerSpeed = 1.0 - float(layer) * 0.2;     // Layers animate slower toward back

        for (int i = 0; i < boltCount; i++) {
            // Per-bolt flicker (uses independent flickerTime, not driftPhase)
            float flickerSeed = floor(flickerTime * 30.0) + float(i) * 7.3 + float(layer) * 13.7;
            float flicker = mix(1.0, hash(flickerSeed), flickerAmount);

            // Base X position evenly distributed
            float baseX = (float(i) + 0.5) / float(boltCount) * 2.0 - 1.0;

            // Horizontal drift with golden ratio offset per bolt
            float phase = driftPhase * layerSpeed + float(i) * 1.618 + float(layer) * 0.5;
            float boltX = baseX + sin(phase) * driftAmount;

            // Displace UV with FBM noise (both X and Y per research)
            vec2 displaced = uv * layerScale;
            displaced += (2.0 * fbm(displaced + animPhase * layerSpeed, octaves) - 1.0) * displacement;

            // Distance to vertical line at boltX
            float dist = abs(displaced.x - boltX);

            // Glow intensity (no clamp - let tonemap handle HDR)
            float glow = applyFalloff(dist / glowRadius, falloffType);
            glow *= coreBrightness * layerBright * flicker;

            // Sample gradient by distance: 0.0 = core, 1.0 = halo edge
            // Normalize distance to [0,1] range (dist/glowRadius capped)
            float lutCoord = clamp(dist / (glowRadius * 3.0), 0.0, 1.0);
            vec3 col = textureLod(gradientLUT, vec2(lutCoord, 0.5), 0.0).rgb;

            total += col * glow;
        }
    }

    // Tonemap to prevent harsh clipping when bolts overlap
    total = 1.0 - exp(-total);

    // Additive blend with source
    vec3 source = texture(texture0, fragTexCoord).rgb;
    finalColor = vec4(source + total, 1.0);
}
```

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| boltCount | int | 1-8 | 3 | No | Bolt Count |
| layerCount | int | 1-3 | 2 | No | Layers |
| octaves | int | 1-10 | 6 | No | Octaves |
| falloffType | int | 0-2 | 1 | No | Falloff |
| driftSpeed | float | 0.0-2.0 | 0.5 | Yes | Drift Speed |
| driftAmount | float | 0.0-1.0 | 0.3 | Yes | Drift Amount |
| animSpeed | float | 0.0-5.0 | 0.8 | Yes | Anim Speed |
| displacement | float | 0.0-2.0 | 1.0 | Yes | Displacement |
| glowRadius | float | 0.01-0.3 | 0.07 | Yes | Glow Radius |
| coreBrightness | float | 0.5-3.0 | 1.5 | Yes | Brightness |
| flickerAmount | float | 0.0-1.0 | 0.2 | Yes | Flicker |
| gradient | ColorConfig | - | gradient mode | No | Colors |

### Deviations from Research

<!-- Intentional: Added layerCount for depth effect (user-requested enhancement) -->
<!-- Intentional: Gradient samples by distance-from-bolt instead of bolt-index for core→halo color (user-requested) -->
<!-- Intentional: Per-bolt flicker uses hash of (boltIndex + layerIndex + time) instead of global time (user-confirmed) -->
<!-- Intentional: boltCount not modulatable to avoid visual discontinuities (user-confirmed) -->
<!-- Fidelity fixes applied: FBM displaces both X and Y (not X-only), glow not clamped before tonemap, flicker uses dedicated flickerTime (not driftPhase) -->

### Pipeline Integration

Plasma is a generator like Constellation. Renders after Constellation, before transforms.

1. Executes after Constellation pass
2. Renders procedurally
3. Blends additively onto ping-pong buffer
4. Controlled by `plasma.enabled` checkbox

---

## Tasks

### Wave 1: Config and Shader Foundation

#### Task 1.1: Create plasma_config.h

**Files**: `src/config/plasma_config.h`
**Creates**: PlasmaConfig struct that other files include

**Build**:
1. Create header with include guard `PLASMA_CONFIG_H`
2. Include `render/color_config.h` and `<stdbool.h>`
3. Define PlasmaConfig struct per specification above
4. Initialize gradient with `{.mode = COLOR_MODE_GRADIENT}`

**Verify**: Header compiles when included.

---

#### Task 1.2: Create plasma.fs shader

**Files**: `shaders/plasma.fs`
**Creates**: Fragment shader for plasma rendering

**Build**:
1. Create shader file with GLSL 330
2. Implement all uniforms per specification
3. Implement hash, noise, rotate2d, fbm functions
4. Implement applyFalloff for 3 falloff types
5. Main function: layered bolt loop, FBM displacement, gradient sampling by distance, tonemap, additive blend

**Verify**: Shader file exists with valid GLSL syntax.

---

### Wave 2: Core Integration (Parallel)

#### Task 2.1: Add to effect_config.h

**Files**: `src/config/effect_config.h`
**Depends on**: Wave 1 complete

**Build**:
1. Add `#include "plasma_config.h"` with other config includes
2. Add `PlasmaConfig plasma;` field to EffectConfig struct (alphabetical position)

**Verify**: Compiles.

---

#### Task 2.2: Add shader and LUT to post_effect.h

**Files**: `src/render/post_effect.h`
**Depends on**: Wave 1 complete

**Build**:
1. Add `Shader plasmaShader;` field (alphabetical among shaders)
2. Add uniform location fields:
   - `int plasmaResolutionLoc;`
   - `int plasmaAnimPhaseLoc;`
   - `int plasmaDriftPhaseLoc;`
   - `int plasmaFlickerTimeLoc;`
   - `int plasmaBoltCountLoc;`
   - `int plasmaLayerCountLoc;`
   - `int plasmaOctavesLoc;`
   - `int plasmaFalloffTypeLoc;`
   - `int plasmaDriftAmountLoc;`
   - `int plasmaDisplacementLoc;`
   - `int plasmaGlowRadiusLoc;`
   - `int plasmaCoreBrightnessLoc;`
   - `int plasmaFlickerAmountLoc;`
   - `int plasmaGradientLUTLoc;`
3. Add LUT pointer: `ColorLUT *plasmaGradientLUT;`
4. Add phase/time accumulators:
   - `float plasmaAnimPhase;`
   - `float plasmaDriftPhase;`
   - `float plasmaFlickerTime;`

**Verify**: Compiles.

---

#### Task 2.3: Load shader and init LUT in post_effect.cpp

**Files**: `src/render/post_effect.cpp`
**Depends on**: Task 2.2 complete

**Build**:
1. In PostEffectInit, load shader:
   - `pe->plasmaShader = LoadShader(0, "shaders/plasma.fs");`
2. Add to shader validation check
3. Get all uniform locations using GetShaderLocation
4. Initialize LUT:
   - `pe->plasmaGradientLUT = ColorLUTInit(&pe->effects.plasma.gradient);`
5. Set resolution uniform in PostEffectResize (add to existing resolution block)
6. Initialize phase/time accumulators:
   - `pe->plasmaAnimPhase = 0.0f;`
   - `pe->plasmaDriftPhase = 0.0f;`
   - `pe->plasmaFlickerTime = 0.0f;`
7. In PostEffectUninit:
   - `UnloadShader(pe->plasmaShader);`
   - `ColorLUTUninit(pe->plasmaGradientLUT);`

**Verify**: Compiles.

---

#### Task 2.4: Add SetupPlasma to shader_setup_generators.cpp

**Files**: `src/render/shader_setup_generators.cpp`, `src/render/shader_setup_generators.h`
**Depends on**: Wave 1 complete

**Build**:
1. Add `void SetupPlasma(PostEffect *pe);` declaration to header
2. Implement SetupPlasma in cpp:
   - Update ColorLUT from config
   - Set all uniform values from `pe->effects.plasma`
   - Pass phase accumulators to shader
   - Bind LUT texture

**Verify**: Compiles.

---

#### Task 2.5: Add generator pass to render_pipeline.cpp

**Files**: `src/render/render_pipeline.cpp`
**Depends on**: Tasks 2.3, 2.4 complete

**Build**:
1. After Constellation pass block (around line 430), add Plasma pass:
   ```cpp
   // Generator pass: Plasma
   pe->plasmaAnimPhase += deltaTime * pe->effects.plasma.animSpeed;
   pe->plasmaDriftPhase += deltaTime * pe->effects.plasma.driftSpeed;
   pe->plasmaFlickerTime += deltaTime; // Independent 1:1 time for flicker
   if (pe->effects.plasma.enabled) {
     RenderPass(pe, src, &pe->pingPong[writeIdx], pe->plasmaShader, SetupPlasma);
     src = &pe->pingPong[writeIdx];
     writeIdx = 1 - writeIdx;
   }
   ```

**Verify**: Compiles.

---

### Wave 3: UI and Serialization (Parallel)

#### Task 3.1: Add UI section to imgui_effects_generators.cpp

**Files**: `src/ui/imgui_effects_generators.cpp`
**Depends on**: Wave 2 complete

**Build**:
1. Add `static bool sectionPlasma = false;` at file top
2. Add `DrawGeneratorsPlasma` helper function before `DrawGeneratorsCategory`:
   - Section header "Plasma"
   - Enabled checkbox
   - SliderInt for boltCount (1-8)
   - SliderInt for layerCount (1-3)
   - SliderInt for octaves (1-10)
   - Combo for falloffType ("Sharp", "Linear", "Soft")
   - Separator
   - ModulatableSlider for driftSpeed, driftAmount, animSpeed
   - Separator
   - ModulatableSlider for displacement, glowRadius, coreBrightness, flickerAmount
   - Separator
   - ImGuiDrawColorMode for gradient
3. Call `DrawGeneratorsPlasma` from `DrawGeneratorsCategory` after Constellation

**Verify**: Compiles.

---

#### Task 3.2: Register modulatable parameters

**Files**: `src/automation/param_registry.cpp`
**Depends on**: Wave 2 complete

**Build**:
1. Add to PARAM_TABLE (alphabetically among plasma entries):
   ```cpp
   {"plasma.animSpeed", {0.0f, 5.0f}, offsetof(EffectConfig, plasma.animSpeed)},
   {"plasma.coreBrightness", {0.5f, 3.0f}, offsetof(EffectConfig, plasma.coreBrightness)},
   {"plasma.displacement", {0.0f, 2.0f}, offsetof(EffectConfig, plasma.displacement)},
   {"plasma.driftAmount", {0.0f, 1.0f}, offsetof(EffectConfig, plasma.driftAmount)},
   {"plasma.driftSpeed", {0.0f, 2.0f}, offsetof(EffectConfig, plasma.driftSpeed)},
   {"plasma.flickerAmount", {0.0f, 1.0f}, offsetof(EffectConfig, plasma.flickerAmount)},
   {"plasma.glowRadius", {0.01f, 0.3f}, offsetof(EffectConfig, plasma.glowRadius)},
   ```

**Verify**: Compiles.

---

#### Task 3.3: Add preset serialization

**Files**: `src/config/preset.cpp`
**Depends on**: Wave 2 complete

**Build**:
1. Add JSON macro with other config macros:
   ```cpp
   NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(PlasmaConfig,
       enabled, boltCount, layerCount, octaves, falloffType,
       driftSpeed, driftAmount, animSpeed,
       displacement, glowRadius, coreBrightness, flickerAmount, gradient)
   ```
2. Add to_json entry in `to_json(json& j, const EffectConfig& e)`:
   ```cpp
   if (e.plasma.enabled) { j["plasma"] = e.plasma; }
   ```
3. Add from_json entry in `from_json(const json& j, EffectConfig& e)`:
   ```cpp
   e.plasma = j.value("plasma", e.plasma);
   ```

**Verify**: Compiles.

---

## Final Verification

- [ ] `cmake.exe --build build` compiles with no errors
- [ ] Application launches without crash
- [ ] Plasma appears in GENERATORS section after Constellation
- [ ] Enabling Plasma renders glowing vertical bolts
- [ ] Bolts drift horizontally with configurable speed/amount
- [ ] FBM displacement creates jagged (high octaves) or smooth (low octaves) paths
- [ ] Falloff combo changes glow character (sharp/linear/soft)
- [ ] Layer count adds depth perception
- [ ] Gradient editor controls core-to-halo colors
- [ ] Modulatable parameters respond to LFO routing
- [ ] Preset save/load preserves Plasma settings
