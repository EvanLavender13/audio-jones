# Radial IFS

Iterated polar wedge folding that produces radial snowflake/flower fractals. Combines kaleidoscope's polar fold with KIFS's iteration pattern—each iteration folds UV into a wedge, translates away from center, and scales, creating recursive branching patterns that radiate outward.

<!-- Intentional deviations from research:
- rotation/twistAngle changed from static angles to rotationSpeed/twistSpeed (rad/s) with CPU accumulation—matches KIFS/TriangleFold codebase patterns
- smoothing parameter added with pmin/pabs polynomial functions from existing kaleidoscope/KIFS shaders
- offset/scale/segments marked non-modulatable to keep UI simpler (can add later)
-->

## Specification

### Types

```cpp
// src/config/radial_ifs_config.h
#ifndef RADIAL_IFS_CONFIG_H
#define RADIAL_IFS_CONFIG_H

// Radial IFS: Iterated polar wedge folding for snowflake/flower fractals
// Each iteration: polar fold -> translate -> scale -> twist
struct RadialIfsConfig {
  bool enabled = false;
  int segments = 6;           // Wedge count per fold (3-12)
  int iterations = 4;         // Recursion depth (1-8)
  float scale = 1.8f;         // Expansion per iteration (1.2-2.5)
  float offset = 0.5f;        // Translation after fold (0.0-2.0)
  float rotationSpeed = 0.0f; // Animation rotation rate (radians/second)
  float twistSpeed = 0.0f;    // Per-iteration rotation rate (radians/second)
  float smoothing = 0.0f;     // Blend width at wedge seams (0.0-0.5)
};

#endif // RADIAL_IFS_CONFIG_H
```

### Algorithm

Core shader (`shaders/radial_ifs.fs`):

```glsl
#version 330

// Radial IFS: Iterated polar wedge folding for snowflake/flower fractals

in vec2 fragTexCoord;

uniform sampler2D texture0;

uniform int segments;       // Wedge count (3-12)
uniform int iterations;     // Recursion depth (1-8)
uniform float scale;        // Expansion per iteration (1.2-2.5)
uniform float offset;       // Translation after fold (0.0-2.0)
uniform float rotation;     // Global rotation (accumulated)
uniform float twistAngle;   // Per-iteration rotation (accumulated)
uniform float smoothing;    // Blend width at wedge seams (0.0-0.5)

out vec4 finalColor;

const float TWO_PI = 6.28318530718;
const float PI = 3.14159265359;

// Polynomial smooth min (for smooth folding)
float pmin(float a, float b, float k)
{
    float h = clamp(0.5 + 0.5 * (b - a) / k, 0.0, 1.0);
    return mix(b, a, h) - k * h * (1.0 - h);
}

// Polynomial smooth absolute (for soft wedge edges)
float pabs(float a, float k)
{
    return -pmin(a, -a, k);
}

vec2 rotate2d(vec2 p, float angle)
{
    float c = cos(angle);
    float s = sin(angle);
    return vec2(c * p.x - s * p.y, s * p.x + c * p.y);
}

vec2 mirror(vec2 x)
{
    return abs(fract(x / 2.0) - 0.5) * 2.0;
}

vec2 radialFold(vec2 p, int segs, float smooth)
{
    float segmentAngle = TWO_PI / float(segs);
    float halfSeg = PI / float(segs);

    // Convert to polar
    float r = length(p);
    float a = atan(p.y, p.x);

    // Fold angle into wedge
    float c = floor((a + halfSeg) / segmentAngle);
    a = mod(a + halfSeg, segmentAngle) - halfSeg;
    a *= mod(c, 2.0) * 2.0 - 1.0;  // Mirror alternating segments

    // Apply smooth fold when smoothing > 0
    if (smooth > 0.0) {
        float sa = halfSeg - pabs(halfSeg - abs(a), smooth);
        a = sign(a) * sa;
    }

    // Back to Cartesian, then translate
    return vec2(cos(a), sin(a)) * r - vec2(offset, 0.0);
}

void main()
{
    vec2 uv = fragTexCoord - 0.5;
    vec2 p = uv * 2.0;

    // Global rotation
    p = rotate2d(p, rotation);

    // Radial IFS iteration
    for (int i = 0; i < iterations; i++) {
        p = radialFold(p, segments, smoothing);
        p *= scale;
        p = rotate2d(p, twistAngle);
    }

    // Sample texture at transformed position
    vec2 newUV = mirror(p * 0.5 + 0.5);
    finalColor = texture(texture0, newUV);
}
```

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| segments | int | 3-12 | 6 | No | Segments |
| iterations | int | 1-8 | 4 | No | Iterations |
| scale | float | 1.2-2.5 | 1.8 | No | Scale |
| offset | float | 0.0-2.0 | 0.5 | No | Offset |
| rotationSpeed | float | -ROTATION_SPEED_MAX to ROTATION_SPEED_MAX | 0.0 | Yes | Spin |
| twistSpeed | float | -ROTATION_SPEED_MAX to ROTATION_SPEED_MAX | 0.0 | Yes | Twist |
| smoothing | float | 0.0-0.5 | 0.0 | Yes | Smoothing |

### Constants

- Enum name: `TRANSFORM_RADIAL_IFS`
- Display name: `"Radial IFS"`
- Category: `TRANSFORM_CATEGORY_SYMMETRY` → `{"SYM", 0}`

---

## Tasks

### Wave 1: Config Header

#### Task 1.1: Create RadialIfsConfig

**Files**: `src/config/radial_ifs_config.h`
**Creates**: `RadialIfsConfig` struct that other files include

**Build**:
1. Create file with include guard `RADIAL_IFS_CONFIG_H`
2. Copy struct definition from Specification > Types exactly
3. Add header comment: `// Radial IFS: Iterated polar wedge folding for snowflake/flower fractals`

**Verify**: File exists with correct struct definition.

---

### Wave 2: Core Implementation (Parallel)

All Wave 2 tasks modify different files and can run simultaneously.

#### Task 2.1: Register effect in effect_config.h

**Files**: `src/config/effect_config.h`
**Depends on**: Wave 1 complete

**Build**:
1. Add include after `triangle_fold_config.h`:
   ```cpp
   #include "radial_ifs_config.h"
   ```

2. Add enum entry before `TRANSFORM_EFFECT_COUNT` (after `TRANSFORM_LEGO_BRICKS`):
   ```cpp
   TRANSFORM_RADIAL_IFS,
   ```

3. Add name in `TransformEffectName()` array (match enum order):
   ```cpp
   "Radial IFS",         // TRANSFORM_RADIAL_IFS
   ```

4. Add config member in `EffectConfig` struct after `triangleFold`:
   ```cpp
   // Radial IFS (iterated polar wedge folding for snowflake/flower fractals)
   RadialIfsConfig radialIfs;
   ```

5. Add case in `IsTransformEnabled()`:
   ```cpp
   case TRANSFORM_RADIAL_IFS:
     return e->radialIfs.enabled;
   ```

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 2.2: Create shader

**Files**: `shaders/radial_ifs.fs`
**Depends on**: Wave 1 complete

**Build**:
1. Create file with exact shader code from Specification > Algorithm

**Verify**: File exists with correct shader code.

---

#### Task 2.3: PostEffect shader loading

**Files**: `src/render/post_effect.h`, `src/render/post_effect.cpp`
**Depends on**: Wave 1 complete

**Build** (post_effect.h):
1. Add shader member after `triangleFoldShader`:
   ```cpp
   Shader radialIfsShader;
   ```

2. Add uniform location members after triangle fold locations:
   ```cpp
   int radialIfsSegmentsLoc;
   int radialIfsIterationsLoc;
   int radialIfsScaleLoc;
   int radialIfsOffsetLoc;
   int radialIfsRotationLoc;
   int radialIfsTwistAngleLoc;
   int radialIfsSmoothingLoc;
   ```

3. Add rotation accumulator after `currentTriangleFoldTwist`:
   ```cpp
   float currentRadialIfsRotation = 0.0f;
   float currentRadialIfsTwist = 0.0f;
   ```

**Build** (post_effect.cpp):
1. In `LoadPostEffectShaders()`, add after triangle_fold load:
   ```cpp
   pe->radialIfsShader = LoadShader(0, "shaders/radial_ifs.fs");
   ```

2. In success check (`&&` chain), add:
   ```cpp
   && pe->radialIfsShader.id != 0
   ```

3. In `GetShaderUniformLocations()`, add after triangle fold locations:
   ```cpp
   pe->radialIfsSegmentsLoc = GetShaderLocation(pe->radialIfsShader, "segments");
   pe->radialIfsIterationsLoc = GetShaderLocation(pe->radialIfsShader, "iterations");
   pe->radialIfsScaleLoc = GetShaderLocation(pe->radialIfsShader, "scale");
   pe->radialIfsOffsetLoc = GetShaderLocation(pe->radialIfsShader, "offset");
   pe->radialIfsRotationLoc = GetShaderLocation(pe->radialIfsShader, "rotation");
   pe->radialIfsTwistAngleLoc = GetShaderLocation(pe->radialIfsShader, "twistAngle");
   pe->radialIfsSmoothingLoc = GetShaderLocation(pe->radialIfsShader, "smoothing");
   ```

4. In `PostEffectUninit()`, add:
   ```cpp
   UnloadShader(pe->radialIfsShader);
   ```

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 2.4: Shader setup function

**Files**: `src/render/shader_setup_symmetry.h`, `src/render/shader_setup_symmetry.cpp`
**Depends on**: Wave 1 complete

**Build** (shader_setup_symmetry.h):
1. Add declaration after `SetupMoireInterference`:
   ```cpp
   void SetupRadialIfs(PostEffect *pe);
   ```

**Build** (shader_setup_symmetry.cpp):
1. Add function at end of file:
   ```cpp
   void SetupRadialIfs(PostEffect *pe) {
     const RadialIfsConfig *r = &pe->effects.radialIfs;

     // CPU rotation accumulation
     pe->currentRadialIfsRotation += r->rotationSpeed * pe->currentDeltaTime;
     pe->currentRadialIfsTwist += r->twistSpeed * pe->currentDeltaTime;

     SetShaderValue(pe->radialIfsShader, pe->radialIfsSegmentsLoc, &r->segments,
                    SHADER_UNIFORM_INT);
     SetShaderValue(pe->radialIfsShader, pe->radialIfsIterationsLoc, &r->iterations,
                    SHADER_UNIFORM_INT);
     SetShaderValue(pe->radialIfsShader, pe->radialIfsScaleLoc, &r->scale,
                    SHADER_UNIFORM_FLOAT);
     SetShaderValue(pe->radialIfsShader, pe->radialIfsOffsetLoc, &r->offset,
                    SHADER_UNIFORM_FLOAT);
     SetShaderValue(pe->radialIfsShader, pe->radialIfsRotationLoc,
                    &pe->currentRadialIfsRotation, SHADER_UNIFORM_FLOAT);
     SetShaderValue(pe->radialIfsShader, pe->radialIfsTwistAngleLoc,
                    &pe->currentRadialIfsTwist, SHADER_UNIFORM_FLOAT);
     SetShaderValue(pe->radialIfsShader, pe->radialIfsSmoothingLoc, &r->smoothing,
                    SHADER_UNIFORM_FLOAT);
   }
   ```

2. Add include for config header at top if not present:
   ```cpp
   #include "config/radial_ifs_config.h"
   ```

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 2.5: Shader dispatch

**Files**: `src/render/shader_setup.cpp`
**Depends on**: Wave 1 complete

**Build**:
1. Add dispatch case after `TRANSFORM_TRIANGLE_FOLD`:
   ```cpp
   case TRANSFORM_RADIAL_IFS:
     return {&pe->radialIfsShader, SetupRadialIfs,
             &pe->effects.radialIfs.enabled};
   ```

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 2.6: UI panel

**Files**: `src/ui/imgui_effects.cpp`, `src/ui/imgui_effects_symmetry.cpp`
**Depends on**: Wave 1 complete

**Build** (imgui_effects.cpp):
1. Add case in `GetTransformCategory()` switch, in Symmetry section (after `TRANSFORM_MOIRE_INTERFERENCE`):
   ```cpp
   case TRANSFORM_RADIAL_IFS:
   ```

**Build** (imgui_effects_symmetry.cpp):
1. Add include at top:
   ```cpp
   #include "config/radial_ifs_config.h"
   ```

2. Add section state with other statics:
   ```cpp
   static bool sectionRadialIfs = false;
   ```

3. Add helper function before `DrawSymmetryCategory`:
   ```cpp
   static void DrawSymmetryRadialIfs(EffectConfig *e,
                                     const ModSources *modSources,
                                     const ImU32 categoryGlow) {
     if (DrawSectionBegin("Radial IFS", categoryGlow, &sectionRadialIfs)) {
       const bool wasEnabled = e->radialIfs.enabled;
       ImGui::Checkbox("Enabled##radialifs", &e->radialIfs.enabled);
       if (!wasEnabled && e->radialIfs.enabled) {
         MoveTransformToEnd(&e->transformOrder, TRANSFORM_RADIAL_IFS);
       }
       if (e->radialIfs.enabled) {
         RadialIfsConfig *r = &e->radialIfs;

         ImGui::SliderInt("Segments##radialifs", &r->segments, 3, 12);
         ImGui::SliderInt("Iterations##radialifs", &r->iterations, 1, 8);
         ImGui::SliderFloat("Scale##radialifs", &r->scale, 1.2f, 2.5f, "%.2f");
         ImGui::SliderFloat("Offset##radialifs", &r->offset, 0.0f, 2.0f, "%.2f");
         ModulatableSliderAngleDeg("Spin##radialifs", &r->rotationSpeed,
                                   "radialIfs.rotationSpeed", modSources, "%.1f °/s");
         ModulatableSliderAngleDeg("Twist##radialifs", &r->twistSpeed,
                                   "radialIfs.twistSpeed", modSources, "%.1f °/s");
         ModulatableSlider("Smoothing##radialifs", &r->smoothing,
                           "radialIfs.smoothing", "%.2f", modSources);
       }
       DrawSectionEnd();
     }
   }
   ```

4. Add call in `DrawSymmetryCategory` after Triangle Fold:
   ```cpp
   ImGui::Spacing();
   DrawSymmetryRadialIfs(e, modSources, categoryGlow);
   ```

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 2.7: Preset serialization

**Files**: `src/config/preset.cpp`
**Depends on**: Wave 1 complete

**Build**:
1. Add include near top with other config headers:
   ```cpp
   #include "radial_ifs_config.h"
   ```

2. Add JSON macro after `TriangleFoldConfig` macro:
   ```cpp
   NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(RadialIfsConfig, enabled,
                                                   segments, iterations, scale,
                                                   offset, rotationSpeed,
                                                   twistSpeed, smoothing)
   ```

3. Add to_json entry in `to_json(json& j, const EffectConfig& e)`:
   ```cpp
   if (e.radialIfs.enabled) { j["radialIfs"] = e.radialIfs; }
   ```

4. Add from_json entry in `from_json(const json& j, EffectConfig& e)`:
   ```cpp
   e.radialIfs = j.value("radialIfs", e.radialIfs);
   ```

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 2.8: Parameter registration

**Files**: `src/automation/param_registry.cpp`
**Depends on**: Wave 1 complete

**Build**:
1. Add entries to `PARAM_TABLE` (after triangleFold entries, before next effect):
   ```cpp
   {"radialIfs.rotationSpeed", {-ROTATION_SPEED_MAX, ROTATION_SPEED_MAX}},
   {"radialIfs.twistSpeed", {-ROTATION_SPEED_MAX, ROTATION_SPEED_MAX}},
   {"radialIfs.smoothing", {0.0f, 0.5f}},
   ```

2. Add entries to `targets[]` array (at MATCHING indices—count entries in PARAM_TABLE to find position):
   ```cpp
   &effects->radialIfs.rotationSpeed,
   &effects->radialIfs.twistSpeed,
   &effects->radialIfs.smoothing,
   ```

**CRITICAL**: PARAM_TABLE and targets[] must have entries at matching indices.

**Verify**: `cmake.exe --build build` compiles.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] Effect appears in transform pipeline list
- [ ] Effect shows "SYM" category badge (not "???")
- [ ] Enabling effect adds it to pipeline
- [ ] UI sliders modify effect in real-time
- [ ] Preset save/load preserves settings
- [ ] Modulation routes to rotationSpeed, twistSpeed, smoothing
- [ ] segments=3 produces Y-shaped patterns
- [ ] segments=6 produces snowflake patterns
- [ ] offset parameter controls branch separation
- [ ] smoothing softens wedge edges
