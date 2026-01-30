# Corridor Warp

Projects the input texture onto an infinite floor, ceiling, or both surfaces (corridor/tunnel), creating a perspective illusion of depth stretching to a vanishing point. Animated rotation and scrolling simulate walking or spinning through an endless hallway.

## Specification

### Types

```cpp
// src/config/corridor_warp_config.h
#ifndef CORRIDOR_WARP_CONFIG_H
#define CORRIDOR_WARP_CONFIG_H

enum CorridorWarpMode {
  CORRIDOR_WARP_FLOOR = 0,    // Only render below horizon
  CORRIDOR_WARP_CEILING = 1,  // Only render above horizon
  CORRIDOR_WARP_CORRIDOR = 2  // Render both (mirror for ceiling)
};

struct CorridorWarpConfig {
  bool enabled = false;
  float horizon = 0.5f;              // Range: 0.0-1.0, vanishing point vertical position
  float perspectiveStrength = 1.0f;  // Range: 0.5-2.0, depth convergence aggressiveness
  int mode = CORRIDOR_WARP_CORRIDOR; // Floor, ceiling, or both
  float viewRotationSpeed = 0.0f;    // Range: -PI to PI, scene rotation rate (rad/s)
  float planeRotationSpeed = 0.0f;   // Range: -PI to PI, floor texture rotation rate (rad/s)
  float scale = 2.0f;                // Range: 0.5-10.0, texture tiling density
  float scrollSpeed = 0.5f;          // Range: -2.0 to 2.0, forward/backward motion (units/s)
  float strafeSpeed = 0.0f;          // Range: -2.0 to 2.0, side-to-side drift (units/s)
  float fogStrength = 1.0f;          // Range: 0.0-4.0, distance fade intensity
};

#endif // CORRIDOR_WARP_CONFIG_H
```

### Shader Algorithm

```glsl
// shaders/corridor_warp.fs
#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;
uniform vec2 resolution;
uniform float horizon;            // 0.0-1.0 vertical position
uniform float perspectiveStrength;
uniform int mode;                 // 0=floor, 1=ceiling, 2=corridor
uniform float viewRotation;       // Pre-accumulated scene rotation
uniform float planeRotation;      // Pre-accumulated floor rotation
uniform float scale;
uniform float scrollOffset;       // Pre-accumulated scroll
uniform float strafeOffset;       // Pre-accumulated strafe
uniform float fogStrength;

mat2 rotate2d(float angle) {
    float c = cos(angle);
    float s = sin(angle);
    return mat2(c, -s, s, c);
}

void main() {
    // Center coordinates, normalize by height for aspect-correct projection
    vec2 screen = (fragTexCoord - 0.5) * vec2(resolution.x / resolution.y, 1.0);

    // Apply view rotation (spins whole scene)
    screen *= rotate2d(viewRotation);

    // Compute vertical distance from horizon
    float horizonY = horizon - 0.5;
    float depth = screen.y - horizonY;

    // Mode handling: determine if we should render this pixel
    bool isFloor = (depth < 0.0);
    bool isCeiling = (depth > 0.0);

    if (mode == 0 && !isFloor) { // FLOOR mode
        finalColor = vec4(0.0);
        return;
    }
    if (mode == 1 && !isCeiling) { // CEILING mode
        finalColor = vec4(0.0);
        return;
    }

    // For corridor mode, mirror depth for ceiling
    if (mode == 2 && isCeiling) {
        depth = -depth;
    }

    // Avoid division by zero near horizon
    float safeDepth = max(abs(depth), 0.001);
    if (depth < 0.0) safeDepth = -safeDepth;

    // Perspective projection onto infinite plane
    vec2 planeUV = vec2(screen.x / -safeDepth, perspectiveStrength / -safeDepth);

    // Apply floor rotation (spins just the texture)
    planeUV *= rotate2d(planeRotation);

    // Scale and scroll
    planeUV *= scale;
    planeUV += vec2(strafeOffset, scrollOffset);

    // Sample with mirror repeat for seamless tiling
    vec2 sampleUV = 1.0 - abs(mod(planeUV, 2.0) - 1.0);
    vec4 color = texture(texture0, sampleUV);

    // Apply fog based on depth (further = darker)
    float fog = pow(safeDepth, fogStrength);
    fog = clamp(fog, 0.0, 1.0);
    color.rgb *= fog;

    finalColor = color;
}
```

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| horizon | float | 0.0-1.0 | 0.5 | Yes | "Horizon" |
| perspectiveStrength | float | 0.5-2.0 | 1.0 | Yes | "Perspective" |
| mode | int | 0-2 | 2 | No | "Mode" (combo) |
| viewRotationSpeed | float | -PI to PI | 0.0 | Yes | "View Rotation" |
| planeRotationSpeed | float | -PI to PI | 0.0 | Yes | "Plane Rotation" |
| scale | float | 0.5-10.0 | 2.0 | Yes | "Scale" |
| scrollSpeed | float | -2.0 to 2.0 | 0.5 | Yes | "Scroll Speed" |
| strafeSpeed | float | -2.0 to 2.0 | 0.0 | Yes | "Strafe Speed" |
| fogStrength | float | 0.0-4.0 | 1.0 | Yes | "Fog Strength" |

### Constants

- Enum name: `TRANSFORM_CORRIDOR_WARP`
- Display name: `"Corridor Warp"`
- Category: Warp (section index 1)

---

## Tasks

### Wave 1: Config Header

Creates the config struct that all other files depend on.

#### Task 1.1: Create Config Header

**Files**: `src/config/corridor_warp_config.h`
**Creates**: `CorridorWarpConfig` struct and `CorridorWarpMode` enum

**Build**:
- Create new file with the complete struct definition from the Types section above
- Include header guards (`#ifndef CORRIDOR_WARP_CONFIG_H`)

**Verify**: `cmake.exe --build build` compiles.

---

### Wave 2: Parallel Implementation

All tasks run simultaneously. No file overlap.

#### Task 2.1: Register Transform Type

**Files**: `src/config/effect_config.h`
**Depends on**: Wave 1 complete

**Build**:
- Add `#include "corridor_warp_config.h"` with other config includes (alphabetical order, near `chladni_warp_config.h`)
- Add `TRANSFORM_CORRIDOR_WARP,` to `TransformEffectType` enum before `TRANSFORM_EFFECT_COUNT`
- Add `"Corridor Warp",` to `TRANSFORM_EFFECT_NAMES` array at matching index (add comment `// TRANSFORM_CORRIDOR_WARP`)
- Add `CorridorWarpConfig corridorWarp;` member to `EffectConfig` struct (alphabetical, near `colorGrade`)
- Add case to `IsTransformEnabled()`:
  ```cpp
  case TRANSFORM_CORRIDOR_WARP:
    return e->corridorWarp.enabled;
  ```

**Verify**: Compiles.

#### Task 2.2: Create Shader

**Files**: `shaders/corridor_warp.fs`
**Depends on**: Wave 1 complete

**Build**:
- Create shader file with the complete GLSL code from the Shader Algorithm section above
- Uniforms: `resolution`, `horizon`, `perspectiveStrength`, `mode`, `viewRotation`, `planeRotation`, `scale`, `scrollOffset`, `strafeOffset`, `fogStrength`

**Verify**: File exists (shader compilation verified later).

#### Task 2.3: Add Shader to PostEffect

**Files**: `src/render/post_effect.h`, `src/render/post_effect.cpp`
**Depends on**: Wave 1 complete

**Build**:

In `post_effect.h`:
- Add `Shader corridorWarpShader;` field (alphabetical, near `colorGradeShader`)
- Add uniform location fields:
  ```cpp
  int corridorWarpResolutionLoc;
  int corridorWarpHorizonLoc;
  int corridorWarpPerspectiveStrengthLoc;
  int corridorWarpModeLoc;
  int corridorWarpViewRotationLoc;
  int corridorWarpPlaneRotationLoc;
  int corridorWarpScaleLoc;
  int corridorWarpScrollOffsetLoc;
  int corridorWarpStrafeOffsetLoc;
  int corridorWarpFogStrengthLoc;
  ```
- Add time accumulator fields:
  ```cpp
  float corridorWarpViewRotation;
  float corridorWarpPlaneRotation;
  float corridorWarpScrollOffset;
  float corridorWarpStrafeOffset;
  ```

In `post_effect.cpp`:
- In `PostEffectLoadShaders()`: Add `pe->corridorWarpShader = LoadShader(0, "shaders/corridor_warp.fs");`
- In shader validation: Add `pe->corridorWarpShader.id != 0`
- In `PostEffectCacheUniformLocations()`: Add all uniform location lookups:
  ```cpp
  pe->corridorWarpResolutionLoc = GetShaderLocation(pe->corridorWarpShader, "resolution");
  pe->corridorWarpHorizonLoc = GetShaderLocation(pe->corridorWarpShader, "horizon");
  pe->corridorWarpPerspectiveStrengthLoc = GetShaderLocation(pe->corridorWarpShader, "perspectiveStrength");
  pe->corridorWarpModeLoc = GetShaderLocation(pe->corridorWarpShader, "mode");
  pe->corridorWarpViewRotationLoc = GetShaderLocation(pe->corridorWarpShader, "viewRotation");
  pe->corridorWarpPlaneRotationLoc = GetShaderLocation(pe->corridorWarpShader, "planeRotation");
  pe->corridorWarpScaleLoc = GetShaderLocation(pe->corridorWarpShader, "scale");
  pe->corridorWarpScrollOffsetLoc = GetShaderLocation(pe->corridorWarpShader, "scrollOffset");
  pe->corridorWarpStrafeOffsetLoc = GetShaderLocation(pe->corridorWarpShader, "strafeOffset");
  pe->corridorWarpFogStrengthLoc = GetShaderLocation(pe->corridorWarpShader, "fogStrength");
  ```
- In `PostEffectUninit()`: Add `UnloadShader(pe->corridorWarpShader);`

**Verify**: Compiles.

#### Task 2.4: Add Shader Setup

**Files**: `src/render/shader_setup_warp.h`, `src/render/shader_setup_warp.cpp`, `src/render/shader_setup.cpp`
**Depends on**: Wave 1 complete

**Build**:

In `shader_setup_warp.h`:
- Add declaration: `void SetupCorridorWarp(PostEffect *pe);`

In `shader_setup_warp.cpp`:
- Add `SetupCorridorWarp()` function:
  ```cpp
  void SetupCorridorWarp(PostEffect *pe) {
    const CorridorWarpConfig *cw = &pe->effects.corridorWarp;

    // Accumulate rotations and offsets on CPU
    pe->corridorWarpViewRotation += cw->viewRotationSpeed * pe->currentDeltaTime;
    pe->corridorWarpPlaneRotation += cw->planeRotationSpeed * pe->currentDeltaTime;
    pe->corridorWarpScrollOffset += cw->scrollSpeed * pe->currentDeltaTime;
    pe->corridorWarpStrafeOffset += cw->strafeSpeed * pe->currentDeltaTime;

    float resolution[2] = {(float)pe->screenWidth, (float)pe->screenHeight};
    SetShaderValue(pe->corridorWarpShader, pe->corridorWarpResolutionLoc, resolution, SHADER_UNIFORM_VEC2);
    SetShaderValue(pe->corridorWarpShader, pe->corridorWarpHorizonLoc, &cw->horizon, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->corridorWarpShader, pe->corridorWarpPerspectiveStrengthLoc, &cw->perspectiveStrength, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->corridorWarpShader, pe->corridorWarpModeLoc, &cw->mode, SHADER_UNIFORM_INT);
    SetShaderValue(pe->corridorWarpShader, pe->corridorWarpViewRotationLoc, &pe->corridorWarpViewRotation, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->corridorWarpShader, pe->corridorWarpPlaneRotationLoc, &pe->corridorWarpPlaneRotation, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->corridorWarpShader, pe->corridorWarpScaleLoc, &cw->scale, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->corridorWarpShader, pe->corridorWarpScrollOffsetLoc, &pe->corridorWarpScrollOffset, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->corridorWarpShader, pe->corridorWarpStrafeOffsetLoc, &pe->corridorWarpStrafeOffset, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->corridorWarpShader, pe->corridorWarpFogStrengthLoc, &cw->fogStrength, SHADER_UNIFORM_FLOAT);
  }
  ```

In `shader_setup.cpp`:
- Add dispatch case in `GetTransformEffect()`:
  ```cpp
  case TRANSFORM_CORRIDOR_WARP:
    return {&pe->corridorWarpShader, SetupCorridorWarp, &pe->effects.corridorWarp.enabled};
  ```

**Verify**: Compiles.

#### Task 2.5: Add UI Panel

**Files**: `src/ui/imgui_effects_warp.cpp`, `src/ui/imgui_effects.cpp`
**Depends on**: Wave 1 complete

**Build**:

In `imgui_effects.cpp`:
- Add `case TRANSFORM_CORRIDOR_WARP:` to the warp category group in `GetEffectCategoryInfo()` (near other warp cases)

In `imgui_effects_warp.cpp`:
- Add `static bool sectionCorridorWarp = false;` with other section state bools
- Add `DrawWarpCorridorWarp()` function:
  ```cpp
  static void DrawWarpCorridorWarp(EffectConfig *e, const ModSources *modSources,
                                   const ImU32 categoryGlow) {
    if (DrawSectionBegin("Corridor Warp", categoryGlow, &sectionCorridorWarp)) {
      const bool wasEnabled = e->corridorWarp.enabled;
      ImGui::Checkbox("Enabled##corridorwarp", &e->corridorWarp.enabled);
      if (!wasEnabled && e->corridorWarp.enabled) {
        MoveTransformToEnd(&e->transformOrder, TRANSFORM_CORRIDOR_WARP);
      }
      if (e->corridorWarp.enabled) {
        ModulatableSlider("Horizon##corridorwarp", &e->corridorWarp.horizon,
                          "corridorWarp.horizon", "%.2f", modSources);
        ModulatableSlider("Perspective##corridorwarp", &e->corridorWarp.perspectiveStrength,
                          "corridorWarp.perspectiveStrength", "%.2f", modSources);

        const char *modeNames[] = {"Floor", "Ceiling", "Corridor"};
        ImGui::Combo("Mode##corridorwarp", &e->corridorWarp.mode, modeNames, 3);

        ModulatableSliderAngleDeg("View Rotation##corridorwarp", &e->corridorWarp.viewRotationSpeed,
                                  "corridorWarp.viewRotationSpeed", modSources, "%.1f °/s");
        ModulatableSliderAngleDeg("Plane Rotation##corridorwarp", &e->corridorWarp.planeRotationSpeed,
                                  "corridorWarp.planeRotationSpeed", modSources, "%.1f °/s");
        ModulatableSlider("Scale##corridorwarp", &e->corridorWarp.scale,
                          "corridorWarp.scale", "%.1f", modSources);
        ModulatableSlider("Scroll Speed##corridorwarp", &e->corridorWarp.scrollSpeed,
                          "corridorWarp.scrollSpeed", "%.2f", modSources);
        ModulatableSlider("Strafe Speed##corridorwarp", &e->corridorWarp.strafeSpeed,
                          "corridorWarp.strafeSpeed", "%.2f", modSources);
        ModulatableSlider("Fog Strength##corridorwarp", &e->corridorWarp.fogStrength,
                          "corridorWarp.fogStrength", "%.2f", modSources);
      }
      DrawSectionEnd();
    }
  }
  ```
- Call `DrawWarpCorridorWarp(e, modSources, categoryGlow);` in `DrawWarpCategory()` (after other warp sections, before closing brace)

**Verify**: Compiles.

#### Task 2.6: Add Preset Serialization

**Files**: `src/config/preset.cpp`
**Depends on**: Wave 1 complete

**Build**:
- Add JSON serialization for `CorridorWarpConfig`:
  ```cpp
  NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(
      CorridorWarpConfig, enabled, horizon, perspectiveStrength, mode,
      viewRotationSpeed, planeRotationSpeed, scale, scrollSpeed, strafeSpeed,
      fogStrength)
  ```
- In `to_json(EffectConfig)`: Add conditional serialization:
  ```cpp
  if (e.corridorWarp.enabled) {
    j["corridorWarp"] = e.corridorWarp;
  }
  ```
- In `from_json(EffectConfig)`: Add deserialization:
  ```cpp
  e.corridorWarp = j.value("corridorWarp", e.corridorWarp);
  ```

**Verify**: Compiles.

#### Task 2.7: Register Modulatable Parameters

**Files**: `src/automation/param_registry.cpp`
**Depends on**: Wave 1 complete

**Build**:
- Add entries to `PARAM_TABLE` (before closing brace):
  ```cpp
  {"corridorWarp.horizon", {0.0f, 1.0f}},
  {"corridorWarp.perspectiveStrength", {0.5f, 2.0f}},
  {"corridorWarp.viewRotationSpeed", {-ROTATION_SPEED_MAX, ROTATION_SPEED_MAX}},
  {"corridorWarp.planeRotationSpeed", {-ROTATION_SPEED_MAX, ROTATION_SPEED_MAX}},
  {"corridorWarp.scale", {0.5f, 10.0f}},
  {"corridorWarp.scrollSpeed", {-2.0f, 2.0f}},
  {"corridorWarp.strafeSpeed", {-2.0f, 2.0f}},
  {"corridorWarp.fogStrength", {0.0f, 4.0f}},
  ```
- Add corresponding pointers to `targets[]` array in `ParamRegistryInit()`:
  ```cpp
  &effects->corridorWarp.horizon,
  &effects->corridorWarp.perspectiveStrength,
  &effects->corridorWarp.viewRotationSpeed,
  &effects->corridorWarp.planeRotationSpeed,
  &effects->corridorWarp.scale,
  &effects->corridorWarp.scrollSpeed,
  &effects->corridorWarp.strafeSpeed,
  &effects->corridorWarp.fogStrength,
  ```

**Verify**: Compiles.

---

## Final Verification

After all waves complete:
- [ ] Build succeeds: `cmake.exe --build build`
- [ ] Run app, enable Corridor Warp in Effects > Warp panel
- [ ] Verify default corridor mode shows floor and ceiling stretching to horizon
- [ ] Verify Scroll Speed animates forward motion
- [ ] Verify View Rotation spins the whole scene
- [ ] Verify Plane Rotation spins just the floor/ceiling texture
- [ ] Verify Fog Strength fades distant areas
- [ ] Verify Floor/Ceiling modes show only one surface
- [ ] Verify parameters save/load with presets
