# Lens Space

Recursive crystal-ball warp — rays march through a 3D lens space L(p,q) where each bounce off the spherical boundary rotates by q/p revolutions, multiplying the scene into diminishing kaleidoscopic copies. Morphing P and Q shifts between clean symmetry and chaotic overlap.

**Research**: `docs/research/lens-space.md`

## Design

### Types

**Config** (`LensSpaceConfig`):

```
enabled        bool   false
centerX        float  0.5    // Warp center X (0.0-1.0)
centerY        float  0.5    // Warp center Y (0.0-1.0)
p              float  5.0    // Symmetry order (2.0-12.0)
q              float  2.0    // Rotation fraction (1.0-11.0)
sphereOffsetX  float  0.0    // Sphere center X offset (-0.5 to 0.5)
sphereOffsetY  float  0.0    // Sphere center Y offset (-0.5 to 0.5)
sphereRadius   float  0.3    // Central mirror sphere size (0.05-0.8)
boundaryRadius float  1.0    // Lens space boundary radius (0.5-2.0)
rotationSpeed  float  0.5    // Camera rotation rate rad/s (-ROTATION_SPEED_MAX..+ROTATION_SPEED_MAX)
maxReflections float  12.0   // Reflection depth (2.0-20.0)
dimming        float  0.067  // Per-reflection brightness decay (0.01-0.15)
zoom           float  1.0    // Ray spread / FOV (0.5-3.0)
projScale      float  0.4    // UV projection strength (0.1-1.0)
```

**Effect** (`LensSpaceEffect`):

```
shader           Shader
resolutionLoc    int
centerLoc        int
sphereOffsetLoc  int
pLoc             int
qLoc             int
sphereRadiusLoc  int
boundaryRadiusLoc int
rotAngleLoc      int
maxReflectionsLoc int
dimmingLoc       int
zoomLoc          int
projScaleLoc     int
rotAngle         float   // CPU-accumulated rotation
```

**Config fields macro**: `LENS_SPACE_CONFIG_FIELDS` = `enabled, centerX, centerY, sphereOffsetX, sphereOffsetY, p, q, sphereRadius, boundaryRadius, rotationSpeed, maxReflections, dimming, zoom, projScale`

### Algorithm

The shader raymarches inside a sphere of radius `boundaryRadius`. A smaller mirror sphere sits at `sphereOffset` (independently controllable). When a ray hits the boundary, its position and direction are rotated in XZ by `q*PI/p` and reflected inward. When it hits the mirror sphere, the reflected direction is projected to 2D UV and the input texture is sampled. Rays that exhaust `maxReflections` without hitting the sphere sample the texture using the final ray direction.

**Complete shader** (`shaders/lens_space.fs`):

```glsl
#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;
uniform vec2 resolution;
uniform vec2 center;
uniform vec2 sphereOffset;
uniform float p;
uniform float q;
uniform float sphereRadius;
uniform float boundaryRadius;
uniform float rotAngle;
uniform float maxReflections;
uniform float dimming;
uniform float zoom;
uniform float projScale;

#define PI 3.14159265

mat2 Rot(float th) {
    float c = cos(th);
    float s = sin(th);
    return mat2(vec2(c, s), vec2(-s, c));
}

void main() {
    vec2 uv = (fragTexCoord - center) * resolution / resolution.y;

    vec3 ro = vec3(0.0, 0.0, -0.5);
    vec3 rd = vec3(uv / zoom, 1.0);
    rd.xz *= Rot(rotAngle);

    vec3 sc = vec3(sphereOffset, 0.5);

    float t = 0.0;
    vec3 col = vec3(0.0);
    float refs = 0.0;
    bool hitSphere = false;

    for (int i = 0; i < 300; i++) {
        vec3 pos = ro + t * rd;

        float d = length(pos - sc) - sphereRadius;
        float e = boundaryRadius - length(pos);

        if (d < 0.001) {
            vec3 n = normalize(pos - sc);
            vec3 r = reflect(rd, n);
            col = texture(texture0, 0.5 + r.xy / max(abs(r.z), 0.001) * projScale).rgb;
            hitSphere = true;
            break;
        }

        if (abs(e) < 0.001) {
            mat2 R = Rot(q * PI / p);
            pos.xz = R * pos.xz;
            rd.xz = R * rd.xz;

            vec3 n = normalize(-pos);
            rd = reflect(rd, n);

            ro = pos + 0.01 * n;
            t = 0.0;

            if (refs >= maxReflections) break;
            refs += 1.0;
        }

        t += min(e * 0.8, d * 0.4);
    }

    if (!hitSphere) {
        col = texture(texture0, 0.5 + rd.xy / max(abs(rd.z), 0.001) * projScale).rgb;
    }

    col *= 1.0 - refs * dimming;

    finalColor = vec4(col, 1.0);
}
```

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| centerX | float | 0.0-1.0 | 0.5 | Yes | "Center X" |
| centerY | float | 0.0-1.0 | 0.5 | Yes | "Center Y" |
| sphereOffsetX | float | -0.5-0.5 | 0.0 | Yes | "Sphere X" |
| sphereOffsetY | float | -0.5-0.5 | 0.0 | Yes | "Sphere Y" |
| p | float | 2.0-12.0 | 5.0 | Yes | "P (Symmetry)" |
| q | float | 1.0-11.0 | 2.0 | Yes | "Q (Rotation)" |
| sphereRadius | float | 0.05-0.8 | 0.3 | Yes | "Sphere Radius" |
| boundaryRadius | float | 0.5-2.0 | 1.0 | Yes | "Boundary" |
| rotationSpeed | float | ±ROTATION_SPEED_MAX | 0.5 | Yes | "Rotation" (SliderSpeedDeg) |
| maxReflections | float | 2.0-20.0 | 12.0 | Yes | "Reflections" (SliderInt) |
| dimming | float | 0.01-0.15 | 0.067 | Yes | "Dimming" |
| zoom | float | 0.5-3.0 | 1.0 | Yes | "Zoom" |
| projScale | float | 0.1-1.0 | 0.4 | Yes | "Projection" |

### Constants

- Enum: `TRANSFORM_LENS_SPACE`
- Display name: `"Lens Space"`
- Badge: `"WARP"`
- Section: `1`
- Flags: `EFFECT_FLAG_NONE` (changed from `EFFECT_FLAG_HALF_RES` — performance fine at full res)
- Field name: `lensSpace`

---

## Tasks

### Wave 1: Header

#### Task 1.1: Create effect header

**Files**: `src/effects/lens_space.h` (create)
**Creates**: `LensSpaceConfig`, `LensSpaceEffect` structs and function declarations

**Do**: Create the header following the same structure as `src/effects/surface_warp.h`. Define `LensSpaceConfig` with fields from the Design section (all with defaults). Define `LENS_SPACE_CONFIG_FIELDS` macro. Define `LensSpaceEffect` typedef struct with shader, 12 uniform location ints (resolution, center, sphereOffset, p, q, sphereRadius, boundaryRadius, rotAngle, maxReflections, dimming, zoom, projScale), and `float rotAngle` accumulator. Declare Init, Setup (takes deltaTime + screenWidth + screenHeight like corridor_warp), Uninit, ConfigDefault, RegisterParams.

**Verify**: Header has include guard, includes raylib.h and stdbool.h.

---

### Wave 2: Implementation

#### Task 2.1: Create fragment shader

**Files**: `shaders/lens_space.fs` (create)
**Depends on**: None (pure GLSL)

**Do**: Implement the Algorithm section verbatim. The complete shader is in the Design section above.

**Verify**: File starts with `#version 330`, has all uniforms listed in the Design section.

---

#### Task 2.2: Create effect module

**Files**: `src/effects/lens_space.cpp` (create)
**Depends on**: Wave 1 complete

**Do**: Follow the same structure as `src/effects/corridor_warp.cpp`. Include own header, `automation/modulation_engine.h`, `config/constants.h`, `config/effect_descriptor.h`, `render/post_effect.h`, `<stddef.h>`.

- `LensSpaceEffectInit`: Load `shaders/lens_space.fs`, cache all 12 uniform locations, init `rotAngle = 0.0f`.
- `LensSpaceEffectSetup`: Accumulate `rotAngle += cfg->rotationSpeed * deltaTime`. Pack `centerX`/`centerY` into `float center[2]`, `sphereOffsetX`/`sphereOffsetY` into `float sphereOffset[2]`, and `screenWidth`/`screenHeight` into `float resolution[2]`. Set all uniforms.
- `LensSpaceEffectUninit`: Unload shader.
- `LensSpaceConfigDefault`: Return `LensSpaceConfig{}`.
- `LensSpaceRegisterParams`: Register all 13 modulatable params (all config fields except `enabled`). Use `ROTATION_SPEED_MAX` bounds for `rotationSpeed`.
- `SetupLensSpace(PostEffect* pe)`: Bridge function, delegates to `LensSpaceEffectSetup` passing `pe->currentDeltaTime`, `pe->screenWidth`, `pe->screenHeight`.
- `REGISTER_EFFECT(TRANSFORM_LENS_SPACE, LensSpace, lensSpace, "Lens Space", "WARP", 1, EFFECT_FLAG_HALF_RES, SetupLensSpace, NULL)`

**Verify**: `cmake.exe --build build` compiles (after all Wave 2 tasks complete).

---

#### Task 2.3: Register in effect config

**Files**: `src/config/effect_config.h` (modify)
**Depends on**: Wave 1 complete

**Do**:
1. Add `#include "effects/lens_space.h"` with other effect includes.
2. Add `TRANSFORM_LENS_SPACE,` to `TransformEffectType` enum before `TRANSFORM_EFFECT_COUNT`.
3. Add `LensSpaceConfig lensSpace;` to `EffectConfig` struct, with comment `// Lens Space (recursive L(p,q) crystal-ball raymarching)`.

**Verify**: No syntax errors in the enum or struct.

---

#### Task 2.4: Add effect member to PostEffect

**Files**: `src/render/post_effect.h` (modify)
**Depends on**: Wave 1 complete

**Do**:
1. Add `#include "effects/lens_space.h"` with other effect includes.
2. Add `LensSpaceEffect lensSpace;` to `PostEffect` struct near the other warp effects (near `surfaceWarp`, `corridorWarp`).

**Verify**: Struct has the new member.

---

#### Task 2.5: Add to build system

**Files**: `CMakeLists.txt` (modify)
**Depends on**: Wave 1 complete

**Do**: Add `src/effects/lens_space.cpp` to `EFFECTS_SOURCES`, near `surface_warp.cpp` and `corridor_warp.cpp`.

**Verify**: File appears in EFFECTS_SOURCES list.

---

#### Task 2.6: Add UI panel

**Files**: `src/ui/imgui_effects_warp.cpp` (modify)
**Depends on**: Wave 1 complete

**Do**: Follow the pattern of `DrawWarpCorridorWarp` in the same file.

1. Add `static bool sectionLensSpace = false;` with other section bools.
2. Add `DrawWarpLensSpace` function:
   - `DrawSectionBegin("Lens Space", categoryGlow, &sectionLensSpace, e->lensSpace.enabled)`
   - Enable checkbox with `MoveTransformToEnd(&e->transformOrder, TRANSFORM_LENS_SPACE)` on fresh enable
   - If enabled, show controls:
     - `ModulatableSlider` for p (label "P (Symmetry)", "%.1f"), q ("Q (Rotation)", "%.1f"), sphereRadius ("Sphere Radius", "%.2f"), boundaryRadius ("Boundary", "%.2f"), dimming ("Dimming", "%.3f"), zoom ("Zoom", "%.2f"), projScale ("Projection", "%.2f")
     - `ModulatableSliderSpeedDeg` for rotationSpeed ("Rotation")
     - `ModulatableSliderInt` for maxReflections ("Reflections", treated as int 2-20)
     - `TreeNodeAccented("Center##lensspace")`: centerX ("X", "%.2f"), centerY ("Y", "%.2f")
     - `TreeNodeAccented("Sphere Position##lensspace")`: sphereOffsetX ("X", "%.2f"), sphereOffsetY ("Y", "%.2f")
3. Add call in `DrawWarpCategory`: `ImGui::Spacing(); DrawWarpLensSpace(e, modSources, categoryGlow);` after the last existing warp effect (Flux Warp).

**Verify**: Function compiles, all ImGui IDs use `##lensspace` suffix.

---

#### Task 2.7: Add preset serialization

**Files**: `src/config/effect_serialization.cpp` (modify)
**Depends on**: Wave 1 complete

**Do**:
1. Add `#include "effects/lens_space.h"` with other effect includes.
2. Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(LensSpaceConfig, LENS_SPACE_CONFIG_FIELDS)` with other per-config macros.
3. Add `X(lensSpace) \` to the `EFFECT_CONFIG_FIELDS(X)` X-macro table.

**Verify**: Macro references match config struct field names.

---

## Final Verification

- [ ] `cmake.exe -G Ninja -B build -S . -DCMAKE_BUILD_TYPE=Release && cmake.exe --build build` succeeds with no warnings
- [ ] Effect appears in transform pipeline and reorder UI
- [ ] Category badge shows "WARP"
- [ ] Enabling adds to pipeline, disabling removes
- [ ] All sliders respond and modify visuals in real-time
- [ ] P/Q at integer ratios produce clean symmetry; non-integers produce chaotic overlap
- [ ] Rotation animates smoothly (no jumps on speed change)
- [ ] Half-res rendering active (check with performance overlay)
- [ ] Preset save/load preserves all settings
