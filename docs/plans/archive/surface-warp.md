# Surface Warp

Creates the illusion of a 3D undulating surface by stretching UV coordinates based on layered sine waves. Peaks appear closer, valleys recede. Optional depth shading darkens valleys to enhance the effect.

## Specification

### Types

```cpp
// src/config/surface_warp_config.h
#ifndef SURFACE_WARP_CONFIG_H
#define SURFACE_WARP_CONFIG_H

struct SurfaceWarpConfig {
    bool enabled = false;
    float intensity = 0.5f;      // Range: 0.0-2.0, hill steepness
    float angle = 0.0f;          // Range: -PI to PI, base warp direction
    float rotateSpeed = 0.0f;    // Range: -PI to PI, direction rotation rate (rad/s)
    float scrollSpeed = 0.5f;    // Range: -2.0 to 2.0, wave drift speed
    float depthShade = 0.3f;     // Range: 0.0-1.0, valley darkening amount
};

#endif // SURFACE_WARP_CONFIG_H
```

### Algorithm

```glsl
// shaders/surface_warp.fs
#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;
uniform float time;
uniform float intensity;
uniform float angle;
uniform float rotateSpeed;
uniform float scrollSpeed;
uniform float depthShade;

void main() {
    vec2 uv = fragTexCoord;

    // Step 1: Rotate into warp space
    float effectiveAngle = angle + rotateSpeed * time;
    float c = cos(effectiveAngle);
    float s = sin(effectiveAngle);
    mat2 rot = mat2(c, -s, s, c);
    vec2 ruv = (uv - 0.5) * rot;

    // Step 2: Compute wave height (layered sines at irrational ratios)
    float t = scrollSpeed * time;
    float wave = sin(ruv.x * 2.0 + t * 0.7) * 0.5
               + sin(ruv.x * 3.7 + t * 1.1) * 0.3
               + sin(ruv.x * 5.3 + t * 0.4) * 0.2;

    // Step 3: Stretch perpendicular axis (exp ensures multiplier >= 1)
    float m = exp(abs(wave) * intensity);
    ruv.y *= m;

    // Step 4: Rotate back and sample with mirror repeat edges
    mat2 rotBack = mat2(c, s, -s, c);
    vec2 warpedUV = ruv * rotBack + 0.5;
    warpedUV = 1.0 - abs(mod(warpedUV, 2.0) - 1.0);  // Mirror repeat
    vec4 color = texture(texture0, warpedUV);

    // Step 5: Apply depth shading (darkens valleys)
    float shade = mix(1.0, smoothstep(0.0, 2.0, m), depthShade);
    color.rgb *= shade;

    finalColor = color;
}
```

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| intensity | float | 0.0-2.0 | 0.5 | Yes | "Intensity" |
| angle | float | -PI to PI | 0.0 | Yes | "Angle" (display degrees) |
| rotateSpeed | float | -PI to PI | 0.0 | Yes | "Rotate Speed" (display degrees/s) |
| scrollSpeed | float | -2.0 to 2.0 | 0.5 | Yes | "Scroll Speed" |
| depthShade | float | 0.0-1.0 | 0.3 | Yes | "Depth Shade" |

### Constants

- Enum name: `TRANSFORM_SURFACE_WARP`
- Display name: `"Surface Warp"`
- Category: `TRANSFORM_CATEGORY_WARP`

---

## Tasks

### Wave 1: Config Header

#### Task 1.1: Create Config Header

**Files**: `src/config/surface_warp_config.h`
**Creates**: `SurfaceWarpConfig` struct that other files will reference

**Build**:
- Create file with exact content from Specification > Types section
- Use include guard `SURFACE_WARP_CONFIG_H`

**Verify**: File exists with correct struct definition.

---

### Wave 2: Parallel Implementation

All tasks in this wave run simultaneously. No file overlap.

#### Task 2.1: Effect Registration

**Files**: `src/config/effect_config.h`
**Depends on**: Wave 1 complete

**Build**:
- Add `#include "surface_warp_config.h"` with other config includes
- Add `TRANSFORM_SURFACE_WARP,` to `TransformEffectType` enum before `TRANSFORM_EFFECT_COUNT`
- Add `case TRANSFORM_SURFACE_WARP: return "Surface Warp";` to `TransformEffectName()`
- Add `TRANSFORM_SURFACE_WARP` to `TransformOrderConfig::order` array (at end, before closing brace)
- Add `SurfaceWarpConfig surfaceWarp;` member to `EffectConfig` struct
- Add `case TRANSFORM_SURFACE_WARP: return e->surfaceWarp.enabled;` to `IsTransformEnabled()`

**Verify**: Compiles.

#### Task 2.2: Fragment Shader

**Files**: `shaders/surface_warp.fs`
**Depends on**: Wave 1 complete

**Build**:
- Create file with exact content from Specification > Algorithm section
- Uniforms: `time`, `intensity`, `angle`, `rotateSpeed`, `scrollSpeed`, `depthShade`

**Verify**: File exists with complete shader.

#### Task 2.3: PostEffect Integration

**Files**: `src/render/post_effect.h`, `src/render/post_effect.cpp`
**Depends on**: Wave 1 complete

**Build**:
In `post_effect.h`:
- Add `Shader surfaceWarpShader;` member
- Add uniform location members: `int surfaceWarpTimeLoc, surfaceWarpIntensityLoc, surfaceWarpAngleLoc, surfaceWarpRotateSpeedLoc, surfaceWarpScrollSpeedLoc, surfaceWarpDepthShadeLoc;`
- Add `float surfaceWarpTime;` for animation accumulation

In `post_effect.cpp`:
- In `LoadPostEffectShaders()`: add `pe->surfaceWarpShader = LoadShader(0, "shaders/surface_warp.fs");`
- Add `&& pe->surfaceWarpShader.id != 0` to shader success check
- In `GetShaderUniformLocations()`: get all 6 uniform locations (time, intensity, angle, rotateSpeed, scrollSpeed, depthShade)
- Initialize `pe->surfaceWarpTime = 0.0f;` where other effect times are initialized
- In `PostEffectUninit()`: add `UnloadShader(pe->surfaceWarpShader);`

**Verify**: Compiles.

#### Task 2.4: Shader Setup

**Files**: `src/render/shader_setup.h`, `src/render/shader_setup.cpp`
**Depends on**: Wave 1 complete

**Build**:
In `shader_setup.h`:
- Add declaration: `void SetupSurfaceWarp(PostEffect* pe);`

In `shader_setup.cpp`:
- Add dispatch case in `GetTransformEffect()`:
  ```cpp
  case TRANSFORM_SURFACE_WARP:
      return { &pe->surfaceWarpShader, SetupSurfaceWarp, &pe->effects.surfaceWarp.enabled };
  ```
- Implement `SetupSurfaceWarp()`:
  - Accumulate time: `pe->surfaceWarpTime += pe->currentDeltaTime;`
  - Set all uniforms from `pe->effects.surfaceWarp` using the location members

**Verify**: Compiles.

#### Task 2.5: UI Panel

**Files**: `src/ui/imgui_effects.cpp`, `src/ui/imgui_effects_warp.cpp`
**Depends on**: Wave 1 complete

**Build**:
In `imgui_effects.cpp`:
- Add `case TRANSFORM_SURFACE_WARP: return TRANSFORM_CATEGORY_WARP;` in `GetTransformCategory()`

In `imgui_effects_warp.cpp`:
- Add `static bool sectionSurfaceWarp = false;` with other section state bools
- Add helper function `DrawWarpSurfaceWarp(EffectConfig* e, const ModSources* modSources, const ImU32 categoryGlow)`:
  - `DrawSectionBegin("Surface Warp", categoryGlow, &sectionSurfaceWarp)`
  - Checkbox for enabled with `MoveTransformToEnd` on enable
  - `ModulatableSlider("Intensity", &e->surfaceWarp.intensity, "surfaceWarp.intensity", "%.2f", modSources)` with range 0.0-2.0
  - `ModulatableSliderAngleDeg("Angle", &e->surfaceWarp.angle, "surfaceWarp.angle", modSources)`
  - `ModulatableSliderAngleDeg("Rotate Speed", &e->surfaceWarp.rotateSpeed, "surfaceWarp.rotateSpeed", modSources)` — use `/s` suffix pattern
  - `ModulatableSlider("Scroll Speed", &e->surfaceWarp.scrollSpeed, "surfaceWarp.scrollSpeed", "%.2f", modSources)` with range -2.0 to 2.0
  - `ModulatableSlider("Depth Shade", &e->surfaceWarp.depthShade, "surfaceWarp.depthShade", "%.2f", modSources)` with range 0.0-1.0
  - `DrawSectionEnd()`
- Call `DrawWarpSurfaceWarp()` from `DrawWarpCategory()` with proper spacing

**Verify**: Compiles.

#### Task 2.6: Preset Serialization

**Files**: `src/config/preset.cpp`
**Depends on**: Wave 1 complete

**Build**:
- Add JSON macro with other config macros:
  ```cpp
  NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(SurfaceWarpConfig, enabled, intensity, angle, rotateSpeed, scrollSpeed, depthShade)
  ```
- In `to_json()`: add `if (e.surfaceWarp.enabled) { j["surfaceWarp"] = e.surfaceWarp; }`
- In `from_json()`: add `e.surfaceWarp = j.value("surfaceWarp", e.surfaceWarp);`

**Verify**: Compiles.

#### Task 2.7: Parameter Registration

**Files**: `src/automation/param_registry.cpp`
**Depends on**: Wave 1 complete

**Build**:
- Add to `PARAM_TABLE` (5 entries, must match targets array order):
  ```cpp
  {"surfaceWarp.intensity",   {0.0f, 2.0f}},
  {"surfaceWarp.angle",       {-ROTATION_OFFSET_MAX, ROTATION_OFFSET_MAX}},
  {"surfaceWarp.rotateSpeed", {-ROTATION_SPEED_MAX, ROTATION_SPEED_MAX}},
  {"surfaceWarp.scrollSpeed", {-2.0f, 2.0f}},
  {"surfaceWarp.depthShade",  {0.0f, 1.0f}},
  ```
- Add to `targets[]` array at matching indices:
  ```cpp
  &effects->surfaceWarp.intensity,
  &effects->surfaceWarp.angle,
  &effects->surfaceWarp.rotateSpeed,
  &effects->surfaceWarp.scrollSpeed,
  &effects->surfaceWarp.depthShade,
  ```

**Verify**: Compiles.

---

## Final Verification

After all waves complete:
- [ ] Build succeeds with no warnings
- [ ] Effect appears in transform order pipeline
- [ ] Effect shows "Warp" category badge (not "???")
- [ ] Effect can be reordered via drag-drop
- [ ] Enabling effect adds it to pipeline list
- [ ] UI controls modify effect in real-time
- [ ] Preset save/load preserves settings
- [ ] LFO modulation routes to all 5 parameters
- [ ] Visual: peaks stretch, valleys compress
- [ ] Visual: depth shading darkens valleys
- [ ] Visual: rotation and scroll animate smoothly
