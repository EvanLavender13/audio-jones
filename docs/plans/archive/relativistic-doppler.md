# Relativistic Doppler

Simulates approaching light speed: spatial compression toward travel direction (relativistic aberration) combined with Doppler color shifting where colors blue-shift ahead and red-shift behind. Creates a "warp drive" aesthetic distinct from simple zooming.

## Specification

### Types

```cpp
// src/config/relativistic_doppler_config.h
#ifndef RELATIVISTIC_DOPPLER_CONFIG_H
#define RELATIVISTIC_DOPPLER_CONFIG_H

struct RelativisticDopplerConfig {
  bool enabled = false;
  float velocity = 0.5f;    // 0.0 - 0.99, fraction of light speed
  float centerX = 0.5f;     // 0.0 - 1.0, travel direction X
  float centerY = 0.5f;     // 0.0 - 1.0, travel direction Y
  float aberration = 1.0f;  // 0.0 - 1.0, spatial compression strength
  float colorShift = 1.0f;  // 0.0 - 1.0, Doppler hue intensity
  float headlight = 0.3f;   // 0.0 - 1.0, brightness boost toward center
};

#endif // RELATIVISTIC_DOPPLER_CONFIG_H
```

### Algorithm

Fragment shader implementing relativistic aberration, Doppler hue shift, and headlight effect:

```glsl
// shaders/relativistic_doppler.fs
#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;
uniform vec2 resolution;

uniform float velocity;    // 0.0 - 0.99
uniform vec2 center;       // Normalized center point
uniform float aberration;  // 0.0 - 1.0
uniform float colorShift;  // 0.0 - 1.0
uniform float headlight;   // 0.0 - 1.0

const float PI = 3.14159265359;

// Rodrigues rotation for hue shift
vec3 hueShift(vec3 color, float hue) {
    const vec3 k = vec3(0.57735);
    float c = cos(hue);
    float s = sin(hue);
    return color * c + cross(k, color) * s + k * dot(k, color) * (1.0 - c);
}

void main() {
    vec2 uv = fragTexCoord;
    vec2 aspect = vec2(resolution.x / resolution.y, 1.0);

    // Vector from center to pixel (aspect-corrected)
    vec2 toPixel = (uv - center) * aspect;
    float dist = length(toPixel);
    vec2 dir = dist > 0.0001 ? toPixel / dist : vec2(0.0);

    // Angle from "forward" (center = 0, edge = π)
    float maxDist = length((vec2(1.0) - center) * aspect);
    float theta = (dist / maxDist) * PI;

    // Lorentz factor
    float beta = velocity;
    float gamma = 1.0 / sqrt(max(1.0 - beta * beta, 0.0001));

    // Relativistic aberration: θ' = acos((cos(θ) - β) / (1 - β·cos(θ)))
    float cosTheta = cos(theta);
    float cosThetaPrime = (cosTheta - beta) / (1.0 - beta * cosTheta);
    cosThetaPrime = clamp(cosThetaPrime, -1.0, 1.0);
    float thetaPrime = acos(cosThetaPrime);

    // Aberrated distance (compressed toward center)
    float aberratedDist = (thetaPrime / PI) * maxDist;
    vec2 aberratedUV = center + dir * aberratedDist / aspect;

    // Blend between original and aberrated UV
    vec2 finalUV = mix(uv, aberratedUV, aberration);

    // Sample texture
    vec3 color = texture(texture0, finalUV).rgb;

    // Doppler factor: D = 1 / (γ · (1 - β·cos(θ)))
    float D = 1.0 / (gamma * (1.0 - beta * cosTheta));

    // Hue shift: D > 1 = blue shift (negative), D < 1 = red shift (positive)
    float hueOffset = (1.0 - D) * 0.8 * colorShift;
    color = hueShift(color, hueOffset);

    // Headlight effect: boost brightness toward center
    float intensityBoost = pow(D, 3.0);
    color = mix(color, color * intensityBoost, headlight);

    finalColor = vec4(clamp(color, 0.0, 1.0), 1.0);
}
```

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| enabled | bool | - | false | No | Enabled |
| velocity | float | 0.0 - 0.99 | 0.5 | Yes | Velocity |
| centerX | float | 0.0 - 1.0 | 0.5 | Yes | Center X |
| centerY | float | 0.0 - 1.0 | 0.5 | Yes | Center Y |
| aberration | float | 0.0 - 1.0 | 1.0 | Yes | Aberration |
| colorShift | float | 0.0 - 1.0 | 1.0 | Yes | Color Shift |
| headlight | float | 0.0 - 1.0 | 0.3 | Yes | Headlight |

### Constants

- Enum name: `TRANSFORM_RELATIVISTIC_DOPPLER`
- Display name: `"Relativistic Doppler"`
- Category: `TRANSFORM_CATEGORY_MOTION` (section 3, badge "MOT")

---

## Tasks

### Wave 1: Config Header

#### Task 1.1: Create Config Struct

**Files**: `src/config/relativistic_doppler_config.h`
**Creates**: `RelativisticDopplerConfig` struct that all other tasks reference

**Build**:
1. Create new file with include guard `RELATIVISTIC_DOPPLER_CONFIG_H`
2. Define `RelativisticDopplerConfig` struct exactly as shown in Types section
3. Use snake_case filename, PascalCase struct name

**Verify**: File exists with correct content.

---

### Wave 2: Parallel Implementation

All tasks in this wave can execute concurrently—no file overlap.

#### Task 2.1: Effect Registration

**Files**: `src/config/effect_config.h`
**Depends on**: Wave 1 complete

**Build**:
1. Add `#include "relativistic_doppler_config.h"` with other config includes (alphabetically near other r* includes)
2. Add `TRANSFORM_RELATIVISTIC_DOPPLER,` to `TransformEffectType` enum before `TRANSFORM_EFFECT_COUNT`
3. Add `"Relativistic Doppler",` to `TRANSFORM_EFFECT_NAMES` array at matching index
4. Add `TRANSFORM_RELATIVISTIC_DOPPLER` to `TransformOrderConfig::order` array initialization (at end before closing brace)
5. Add `RelativisticDopplerConfig relativisticDoppler;` to `EffectConfig` struct
6. Add case to `IsTransformEnabled()`: `case TRANSFORM_RELATIVISTIC_DOPPLER: return e->relativisticDoppler.enabled;`

**Verify**: Compiles with no errors.

---

#### Task 2.2: Fragment Shader

**Files**: `shaders/relativistic_doppler.fs`
**Depends on**: Wave 1 complete

**Build**:
1. Create shader file with exact content from Algorithm section
2. Uniforms: `resolution` (vec2), `velocity` (float), `center` (vec2), `aberration` (float), `colorShift` (float), `headlight` (float)

**Verify**: File exists with correct GLSL content.

---

#### Task 2.3: PostEffect Header

**Files**: `src/render/post_effect.h`
**Depends on**: Wave 1 complete

**Build**:
1. Add shader member: `Shader relativisticDopplerShader;`
2. Add uniform location members:
   - `int relativisticDopplerResolutionLoc;`
   - `int relativisticDopplerVelocityLoc;`
   - `int relativisticDopplerCenterLoc;`
   - `int relativisticDopplerAberrationLoc;`
   - `int relativisticDopplerColorShiftLoc;`
   - `int relativisticDopplerHeadlightLoc;`

**Verify**: Compiles with no errors.

---

#### Task 2.4: PostEffect Implementation

**Files**: `src/render/post_effect.cpp`
**Depends on**: Wave 1 complete

**Build**:
1. In `LoadPostEffectShaders()`: Add `pe->relativisticDopplerShader = LoadShader(0, "shaders/relativistic_doppler.fs");`
2. In success check: Add `&& pe->relativisticDopplerShader.id != 0`
3. In `GetShaderUniformLocations()`: Add:
   ```cpp
   pe->relativisticDopplerResolutionLoc = GetShaderLocation(pe->relativisticDopplerShader, "resolution");
   pe->relativisticDopplerVelocityLoc = GetShaderLocation(pe->relativisticDopplerShader, "velocity");
   pe->relativisticDopplerCenterLoc = GetShaderLocation(pe->relativisticDopplerShader, "center");
   pe->relativisticDopplerAberrationLoc = GetShaderLocation(pe->relativisticDopplerShader, "aberration");
   pe->relativisticDopplerColorShiftLoc = GetShaderLocation(pe->relativisticDopplerShader, "colorShift");
   pe->relativisticDopplerHeadlightLoc = GetShaderLocation(pe->relativisticDopplerShader, "headlight");
   ```
4. In `SetResolutionUniforms()`: Add `SetShaderValue(pe->relativisticDopplerShader, pe->relativisticDopplerResolutionLoc, res, SHADER_UNIFORM_VEC2);`
5. In `PostEffectUninit()`: Add `UnloadShader(pe->relativisticDopplerShader);`

**Verify**: Compiles with no errors.

---

#### Task 2.5: Shader Setup Header

**Files**: `src/render/shader_setup_motion.h`
**Depends on**: Wave 1 complete

**Build**:
1. Add function declaration: `void SetupRelativisticDoppler(PostEffect *pe);`

**Verify**: Compiles with no errors.

---

#### Task 2.6: Shader Setup Implementation

**Files**: `src/render/shader_setup_motion.cpp`
**Depends on**: Wave 1 complete

**Build**:
Add function at end of file:
```cpp
void SetupRelativisticDoppler(PostEffect *pe) {
  const RelativisticDopplerConfig *rd = &pe->effects.relativisticDoppler;
  SetShaderValue(pe->relativisticDopplerShader, pe->relativisticDopplerVelocityLoc,
                 &rd->velocity, SHADER_UNIFORM_FLOAT);
  float center[2] = {rd->centerX, rd->centerY};
  SetShaderValue(pe->relativisticDopplerShader, pe->relativisticDopplerCenterLoc,
                 center, SHADER_UNIFORM_VEC2);
  SetShaderValue(pe->relativisticDopplerShader, pe->relativisticDopplerAberrationLoc,
                 &rd->aberration, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->relativisticDopplerShader, pe->relativisticDopplerColorShiftLoc,
                 &rd->colorShift, SHADER_UNIFORM_FLOAT);
  SetShaderValue(pe->relativisticDopplerShader, pe->relativisticDopplerHeadlightLoc,
                 &rd->headlight, SHADER_UNIFORM_FLOAT);
}
```

**Verify**: Compiles with no errors.

---

#### Task 2.7: Shader Setup Dispatcher

**Files**: `src/render/shader_setup.cpp`
**Depends on**: Wave 1 complete

**Build**:
1. Include is already present (`shader_setup_motion.h`)
2. Add dispatch case in `GetTransformEffect()` switch:
   ```cpp
   case TRANSFORM_RELATIVISTIC_DOPPLER:
     return {&pe->relativisticDopplerShader, SetupRelativisticDoppler,
             &pe->effects.relativisticDoppler.enabled};
   ```

**Verify**: Compiles with no errors.

---

#### Task 2.8: Category Mapping

**Files**: `src/ui/imgui_effects.cpp`
**Depends on**: Wave 1 complete

**Build**:
In `GetTransformCategory()` switch, add `TRANSFORM_RELATIVISTIC_DOPPLER` case with the other Motion effects (section 3):
```cpp
case TRANSFORM_RELATIVISTIC_DOPPLER:
```
(Group with existing Motion cases that return `{"MOT", 3}`)

**Verify**: Compiles with no errors.

---

#### Task 2.9: UI Panel

**Files**: `src/ui/imgui_effects_motion.cpp`
**Depends on**: Wave 1 complete

**Build**:
1. Add section state at file top with other static bools:
   ```cpp
   static bool sectionRelativisticDoppler = false;
   ```

2. Add helper function before `DrawMotionCategory()`:
   ```cpp
   static void DrawMotionRelativisticDoppler(EffectConfig *e,
                                              const ModSources *modSources,
                                              const ImU32 categoryGlow) {
     if (DrawSectionBegin("Relativistic Doppler", categoryGlow,
                          &sectionRelativisticDoppler)) {
       const bool wasEnabled = e->relativisticDoppler.enabled;
       ImGui::Checkbox("Enabled##reldop", &e->relativisticDoppler.enabled);
       if (!wasEnabled && e->relativisticDoppler.enabled) {
         MoveTransformToEnd(&e->transformOrder, TRANSFORM_RELATIVISTIC_DOPPLER);
       }
       if (e->relativisticDoppler.enabled) {
         ModulatableSlider("Velocity##reldop", &e->relativisticDoppler.velocity,
                           "relativisticDoppler.velocity", "%.2f", modSources);
         if (TreeNodeAccented("Center##reldop", categoryGlow)) {
           ModulatableSlider("X##reldopcenter", &e->relativisticDoppler.centerX,
                             "relativisticDoppler.centerX", "%.2f", modSources);
           ModulatableSlider("Y##reldopcenter", &e->relativisticDoppler.centerY,
                             "relativisticDoppler.centerY", "%.2f", modSources);
           TreeNodeAccentedPop();
         }
         ModulatableSlider("Aberration##reldop", &e->relativisticDoppler.aberration,
                           "relativisticDoppler.aberration", "%.2f", modSources);
         ModulatableSlider("Color Shift##reldop", &e->relativisticDoppler.colorShift,
                           "relativisticDoppler.colorShift", "%.2f", modSources);
         ModulatableSlider("Headlight##reldop", &e->relativisticDoppler.headlight,
                           "relativisticDoppler.headlight", "%.2f", modSources);
       }
       DrawSectionEnd();
     }
   }
   ```

3. Add call in `DrawMotionCategory()` with spacing:
   ```cpp
   ImGui::Spacing();
   DrawMotionRelativisticDoppler(e, modSources, categoryGlow);
   ```

**Verify**: Compiles with no errors.

---

#### Task 2.10: Preset Serialization

**Files**: `src/config/preset.cpp`
**Depends on**: Wave 1 complete

**Build**:
1. Add JSON macro with other config macros:
   ```cpp
   NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(RelativisticDopplerConfig, enabled,
                                                    velocity, centerX, centerY,
                                                    aberration, colorShift, headlight)
   ```

2. Add to `to_json(json& j, const EffectConfig& e)`:
   ```cpp
   if (e.relativisticDoppler.enabled) { j["relativisticDoppler"] = e.relativisticDoppler; }
   ```

3. Add to `from_json(const json& j, EffectConfig& e)`:
   ```cpp
   e.relativisticDoppler = j.value("relativisticDoppler", e.relativisticDoppler);
   ```

**Verify**: Compiles with no errors.

---

#### Task 2.11: Parameter Registration

**Files**: `src/automation/param_registry.cpp`
**Depends on**: Wave 1 complete

**Build**:
Add entries to `PARAM_TABLE` (group with other effect entries):
```cpp
{"relativisticDoppler.velocity",
 {0.0f, 0.99f},
 offsetof(EffectConfig, relativisticDoppler.velocity)},
{"relativisticDoppler.centerX",
 {0.0f, 1.0f},
 offsetof(EffectConfig, relativisticDoppler.centerX)},
{"relativisticDoppler.centerY",
 {0.0f, 1.0f},
 offsetof(EffectConfig, relativisticDoppler.centerY)},
{"relativisticDoppler.aberration",
 {0.0f, 1.0f},
 offsetof(EffectConfig, relativisticDoppler.aberration)},
{"relativisticDoppler.colorShift",
 {0.0f, 1.0f},
 offsetof(EffectConfig, relativisticDoppler.colorShift)},
{"relativisticDoppler.headlight",
 {0.0f, 1.0f},
 offsetof(EffectConfig, relativisticDoppler.headlight)},
```

**Verify**: Compiles with no errors.

---

## Final Verification

- [ ] Build succeeds with no warnings: `cmake.exe --build build`
- [ ] Effect appears in transform order pipeline
- [ ] Effect shows "MOT" category badge (not "???")
- [ ] Effect can be reordered via drag-drop
- [ ] Enabling effect adds it to the pipeline list
- [ ] UI controls modify effect in real-time
- [ ] Preset save/load preserves settings
- [ ] Modulation routes to all 6 registered parameters
- [ ] At velocity=0.5, visible spatial compression and color banding
- [ ] At velocity=0.9+, dramatic tunnel vision effect
