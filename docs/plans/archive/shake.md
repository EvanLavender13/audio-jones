# Shake

Multi-sample stochastic jitter that samples the input texture at randomized UV offsets and averages them. Creates a vibrating/motion-blur effect where edges appear soft and unstable. Unlike smooth camera sway, this produces per-frame randomized displacement with controllable blur intensity.

## Specification

### Types

```cpp
// src/config/shake_config.h
#ifndef SHAKE_CONFIG_H
#define SHAKE_CONFIG_H

struct ShakeConfig {
  bool enabled = false;
  float intensity = 0.02f;  // UV displacement distance (0.0 - 0.2)
  int samples = 4;          // Samples per pixel (1 - 16)
  float rate = 12.0f;       // Jitter change frequency in Hz (1.0 - 60.0)
  bool gaussian = false;    // false = uniform distribution, true = gaussian
};

#endif // SHAKE_CONFIG_H
```

### Shader

```glsl
// shaders/shake.fs
#version 330

uniform sampler2D inputTex;
uniform vec2 resolution;
uniform float time;

uniform float intensity;    // 0.0 - 0.2
uniform int samples;        // 1 - 16
uniform float rate;         // Hz
uniform bool gaussian;      // distribution type

in vec2 fragTexCoord;
out vec4 fragColor;

float seed;
float rnd() { return fract(sin(seed++) * 43758.5453123); }

void main() {
    vec2 uv = fragTexCoord;

    if (samples < 1) {
        fragColor = texture(inputTex, uv);
        return;
    }

    // Initialize seed with temporal and spatial variation
    float seedTime = floor(time * rate) / max(rate, 0.001);
    seed = fract(seedTime * 43.758) + uv.x * 12.9898 + uv.y * 78.233;

    vec3 result = vec3(0.0);

    for (int i = 0; i < samples; i++) {
        vec2 jitter;

        if (gaussian) {
            // Box-Muller transform for Gaussian distribution
            vec2 xi = vec2(rnd(), rnd());
            float r = sqrt(-log(max(1.0 - xi.x, 0.0001)));
            float theta = 6.28318 * xi.y;
            jitter = vec2(cos(theta), sin(theta)) * r * intensity;
        } else {
            // Uniform distribution in [-1, 1]
            jitter = (vec2(rnd(), rnd()) * 2.0 - 1.0) * intensity;
        }

        result += texture(inputTex, uv + jitter).rgb;
    }

    fragColor = vec4(result / float(samples), 1.0);
}
```

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| enabled | bool | - | false | No | Enabled |
| intensity | float | 0.0 - 0.2 | 0.02 | Yes | Intensity |
| samples | int | 1 - 16 | 4 | Yes | Samples |
| rate | float | 1.0 - 60.0 | 12.0 | Yes | Rate |
| gaussian | bool | - | false | No | Gaussian |

### Constants

- Enum name: `TRANSFORM_SHAKE`
- Display name: `"Shake"`
- Category: Warp (add to `imgui_effects_warp.cpp`)

---

## Tasks

### Wave 1: Config Header

#### Task 1.1: Create ShakeConfig

**Files**: `src/config/shake_config.h`
**Creates**: `ShakeConfig` struct used by all other tasks

**Build**:
1. Create new header file with include guard `SHAKE_CONFIG_H`
2. Define `ShakeConfig` struct exactly as shown in Types section above
3. No includes needed

**Verify**: File exists with correct struct definition.

---

### Wave 2: Integration (7 parallel tasks)

#### Task 2.1: Add Shader

**Files**: `shaders/shake.fs`
**Depends on**: None (independent)

**Build**:
1. Create fragment shader exactly as shown in Shader section above
2. Note: shader uses `samples` in loop—GLSL requires compile-time loop bounds, but modern drivers unroll dynamically

**Verify**: File exists with correct GLSL.

---

#### Task 2.2: Add Transform Enum

**Files**: `src/config/effect_config.h`
**Depends on**: Task 1.1 complete

**Build**:
1. Add `#include "shake_config.h"` in alphabetical order with other includes
2. Add `TRANSFORM_SHAKE,` to `TransformEffectType` enum before `TRANSFORM_EFFECT_COUNT`
3. Add `"Shake",` to `TRANSFORM_EFFECT_NAMES` array at matching position (add comment)
4. Add `ShakeConfig shake;` to `EffectConfig` struct (alphabetical among warp effects)
5. Add case to `IsTransformEnabled()`:
   ```cpp
   case TRANSFORM_SHAKE:
     return e->shake.enabled;
   ```

**Verify**: Compiles with `cmake.exe --build build`.

---

#### Task 2.3: Load Shader and Get Uniform Locations

**Files**: `src/render/post_effect.h`, `src/render/post_effect.cpp`
**Depends on**: Task 2.1 (shader file exists)

**Build**:

In `post_effect.h`:
1. Add `Shader shakeShader;` field (alphabetical among shaders)
2. Add uniform location fields:
   ```cpp
   int shakeTimeLoc;
   int shakeIntensityLoc;
   int shakeSamplesLoc;
   int shakeRateLoc;
   int shakeGaussianLoc;
   ```
3. Add accumulator field: `float shakeTime;`

In `post_effect.cpp`:
1. In `PostEffectInit()`, add shader load:
   ```cpp
   pe->shakeShader = LoadShader(0, "shaders/shake.fs");
   ```
2. Add uniform location lookups:
   ```cpp
   pe->shakeTimeLoc = GetShaderLocation(pe->shakeShader, "time");
   pe->shakeIntensityLoc = GetShaderLocation(pe->shakeShader, "intensity");
   pe->shakeSamplesLoc = GetShaderLocation(pe->shakeShader, "samples");
   pe->shakeRateLoc = GetShaderLocation(pe->shakeShader, "rate");
   pe->shakeGaussianLoc = GetShaderLocation(pe->shakeShader, "gaussian");
   ```
3. In `PostEffectUninit()`, add:
   ```cpp
   UnloadShader(pe->shakeShader);
   ```
4. Initialize `pe->shakeTime = 0.0f;` in init

**Verify**: Compiles.

---

#### Task 2.4: Add Shader Setup Function

**Files**: `src/render/shader_setup_warp.h`, `src/render/shader_setup_warp.cpp`
**Depends on**: Task 2.3 (uniform locs exist)

**Build**:

In `shader_setup_warp.h`:
1. Add declaration: `void SetupShake(PostEffect *pe);`

In `shader_setup_warp.cpp`:
1. Add function:
   ```cpp
   void SetupShake(PostEffect *pe) {
     const ShakeConfig *s = &pe->effects.shake;

     // Accumulate time on CPU for motionScale compatibility
     pe->shakeTime += pe->currentDeltaTime;

     SetShaderValue(pe->shakeShader, pe->shakeTimeLoc, &pe->shakeTime,
                    SHADER_UNIFORM_FLOAT);
     SetShaderValue(pe->shakeShader, pe->shakeIntensityLoc, &s->intensity,
                    SHADER_UNIFORM_FLOAT);
     SetShaderValue(pe->shakeShader, pe->shakeSamplesLoc, &s->samples,
                    SHADER_UNIFORM_INT);
     SetShaderValue(pe->shakeShader, pe->shakeRateLoc, &s->rate,
                    SHADER_UNIFORM_FLOAT);
     int gaussian = s->gaussian ? 1 : 0;
     SetShaderValue(pe->shakeShader, pe->shakeGaussianLoc, &gaussian,
                    SHADER_UNIFORM_INT);
   }
   ```

**Verify**: Compiles.

---

#### Task 2.5: Add Transform Dispatch Entry

**Files**: `src/render/shader_setup.cpp`
**Depends on**: Task 2.4 (SetupShake exists)

**Build**:
1. Add case to `GetTransformEffect()` switch before `default`:
   ```cpp
   case TRANSFORM_SHAKE:
     return {&pe->shakeShader, SetupShake, &pe->effects.shake.enabled};
   ```

**Verify**: Compiles.

---

#### Task 2.6: Add UI Panel

**Files**: `src/ui/imgui_effects_warp.cpp`
**Depends on**: Task 2.2 (enum exists)

**Build**:
1. Add static bool: `static bool sectionShake = false;`
2. Add draw function:
   ```cpp
   static void DrawWarpShake(EffectConfig *e, const ModSources *modSources,
                             const ImU32 categoryGlow) {
     if (DrawSectionBegin("Shake", categoryGlow, &sectionShake)) {
       const bool wasEnabled = e->shake.enabled;
       ImGui::Checkbox("Enabled##shake", &e->shake.enabled);
       if (!wasEnabled && e->shake.enabled) {
         MoveTransformToEnd(&e->transformOrder, TRANSFORM_SHAKE);
       }
       if (e->shake.enabled) {
         ModulatableSlider("Intensity##shake", &e->shake.intensity,
                           "shake.intensity", "%.3f", modSources);
         ModulatableSliderInt("Samples##shake", &e->shake.samples,
                              "shake.samples", modSources);
         ModulatableSlider("Rate##shake", &e->shake.rate,
                           "shake.rate", "%.1f Hz", modSources);
         ImGui::Checkbox("Gaussian##shake", &e->shake.gaussian);
       }
       DrawSectionEnd();
     }
   }
   ```
3. Call `DrawWarpShake(e, modSources, categoryGlow);` in `DrawWarpCategory()` after other warp effects

**Verify**: Compiles.

---

#### Task 2.7: Register Modulatable Parameters

**Files**: `src/automation/param_registry.cpp`
**Depends on**: Task 2.2 (config field exists)

**Build**:
1. Add to `PARAM_TABLE` (maintain alphabetical order by key):
   ```cpp
   {"shake.intensity", {0.0f, 0.2f}},
   {"shake.rate", {1.0f, 60.0f}},
   {"shake.samples", {1.0f, 16.0f}},
   ```
2. Add to `ParamRegistryInit()` pointer array (same order as PARAM_TABLE):
   ```cpp
   &effects->shake.intensity,
   &effects->shake.rate,
   ```
3. For `samples` (int), check if `ModulatableSliderInt` exists. If not, skip samples modulation or cast pointer.

**Verify**: Compiles.

---

#### Task 2.8: Add Preset Serialization

**Files**: `src/config/preset.cpp`
**Depends on**: Task 1.1 (ShakeConfig exists)

**Build**:
1. Add JSON macro after other warp configs:
   ```cpp
   NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ShakeConfig, enabled,
                                                   intensity, samples, rate,
                                                   gaussian)
   ```
2. In `to_json(EffectConfig)`, add:
   ```cpp
   if (e.shake.enabled) {
     j["shake"] = e.shake;
   }
   ```
3. In `from_json(EffectConfig)`, add:
   ```cpp
   e.shake = j.value("shake", e.shake);
   ```

**Verify**: Compiles.

---

## Final Verification

- [ ] `cmake.exe --build build` succeeds with no warnings
- [ ] Run application, enable Shake effect
- [ ] Verify visible jitter/blur at default settings
- [ ] Test intensity slider (0 = no effect, 0.2 = heavy shake)
- [ ] Test rate slider (1 Hz = slow choppy, 60 Hz = smooth vibration)
- [ ] Test samples slider (1 = noisy, 16 = smooth blur)
- [ ] Test gaussian checkbox (uniform vs gaussian distribution)
- [ ] Verify preset save/load works
- [ ] Verify modulation targets appear in LFO routing
