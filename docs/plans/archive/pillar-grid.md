# Pillar Grid

A Cellular transform that samples the input scene on a square grid; each cell becomes a 3D rounded-box pillar standing on a floor at y=-3. Pillar height comes from cell luminance, pillar color comes from cell RGB. A static camera with user-controlled pitch looks at the field origin; camera distance auto-derives from `density` so the field always frames. Diffuse + white specular + AO lighting; no cubemap reflection. Single-pass fragment shader doing fixed-step SDF raymarch with a 5x5 neighborhood lookup per ray-step.

**Research**: `docs/research/pillar_grid.md`

## Design

### Types

`src/effects/pillar_grid.h`:

```cpp
#ifndef PILLAR_GRID_EFFECT_H
#define PILLAR_GRID_EFFECT_H

#include "raylib.h"
#include <stdbool.h>

struct PillarGridConfig {
  bool enabled = false;
  float density = 48.0f;       // Cells across the field (16-128, integer-flavored float)
  float pitch = 1.047f;        // Camera pitch in radians (0=horizontal, HALF_PI=overhead); default ~PI/3
  float heightScale = 8.0f;    // Pillar half-extent multiplier on cell luminance (0-20)
  float pillarFill = 0.27f;    // Half-extent of pillar inside its 1-unit cell (0.05-0.49)
  float cornerRadius = 0.3f;   // Rounded-box corner softness (0-0.5)
};

#define PILLAR_GRID_CONFIG_FIELDS                                              \
  enabled, density, pitch, heightScale, pillarFill, cornerRadius

typedef struct PillarGridEffect {
  Shader shader;
  int resolutionLoc;
  int densityLoc;
  int pitchLoc;
  int heightScaleLoc;
  int pillarFillLoc;
  int cornerRadiusLoc;
} PillarGridEffect;

bool PillarGridEffectInit(PillarGridEffect *e);
void PillarGridEffectSetup(const PillarGridEffect *e,
                           const PillarGridConfig *cfg);
void PillarGridEffectUninit(const PillarGridEffect *e);
void PillarGridRegisterParams(PillarGridConfig *cfg);

struct PostEffect;
PillarGridEffect *GetPillarGridEffect(PostEffect *pe);

#endif // PILLAR_GRID_EFFECT_H
```

### Algorithm

This is the complete `shaders/pillar_grid.fs`. Transcribed from the reference shader in `docs/research/pillar_grid.md` by mechanically applying that doc's "Replace from reference" substitutions to the reference code; everything else is verbatim.

```glsl
// Based on "Voxel Pillars webcam Y28" by Yusef28
// https://www.shadertoy.com/view/s3XGWH
// License: CC BY-NC-SA 3.0 Unported
// Modified: parameterized density, camera pitch, and pillar shape; cubemap
// reflection and unused-in-reference paths removed; pillar color sampled from
// input RGB instead of luminance palette.
#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;
uniform vec2 resolution;
uniform float density;
uniform float pitch;
uniform float heightScale;
uniform float pillarFill;
uniform float cornerRadius;

#define FAR 60.0

// Cell id (i, j) -> input UV. Field is `density` cells wide centered on origin;
// the `+0.5` centers each cell on its corresponding input pixel.
vec2 cellUVForId(vec2 id) {
    return (id + density * 0.5 + 0.5) / density;
}

float getGrey(vec3 c) {
    return c.x * 0.299 + c.y * 0.587 + c.z * 0.114;
}

// Rounded box SDF (iq)
float roundBox(vec3 p, vec3 b, float r) {
    return length(max(abs(p) - b, 0.0)) - r;
}

// Heightfield SDF: 5x5 neighborhood of pillars around the current cell.
// Pillar at neighbor n: center (n.x, -3, n.y), half-extents (fill, h, fill),
// where h = grey(input cell) * heightScale.
float tile(vec3 p) {
    const float N = 2.0;
    vec2 id = floor(p.xz);
    p.xz = fract(p.xz) - 0.5;
    float d = 100.0;
    for (float i = -N; i <= N; i++) {
        for (float j = -N; j <= N; j++) {
            vec2 n = vec2(i, j);
            vec2 uv = cellUVForId(id + n);
            float h = getGrey(textureLod(texture0, uv, 2.0).rgb) * heightScale;
            d = min(d, roundBox(p - vec3(n.x, -3.0, n.y),
                                vec3(pillarFill, h, pillarFill),
                                cornerRadius));
        }
    }
    return d;
}

float map(vec3 p) {
    return tile(p);
}

float trace(vec3 ro, vec3 rd) {
    float t = 0.0;
    for (int i = 0; i < 96; i++) {
        float dist = map(ro + rd * t);
        if (dist < 0.0001 || t > FAR) break;
        t += dist * 0.85;
    }
    return t;
}

vec3 normal(vec3 p) {
    mat3 k = mat3(p, p, p) - mat3(0.01);
    return normalize(map(p) - vec3(map(k[0]), map(k[1]), map(k[2])));
}

float calculateAO(vec3 pos, vec3 nor) {
    float sca = 2.0, occ = 0.0;
    for (int i = 0; i < 5; i++) {
        float hr = 0.01 + float(i) * 0.5 / 4.0;
        float dd = map(nor * hr + pos);
        occ += (hr - dd) * sca;
        sca *= 0.7;
    }
    return clamp(1.0 - occ, 0.0, 1.0);
}

vec3 lighting(vec3 sp, vec3 sn, vec3 lp, vec3 rd, vec3 baseColor) {
    vec3 lv = lp - sp;
    float ldist = max(length(lv), 0.001);
    vec3 ldir = lv / ldist;
    float atte = 2.0 / (1.0 + 0.002 * ldist * ldist);
    float diff = dot(ldir, sn);
    float spec = pow(max(dot(reflect(-ldir, sn), -rd), 0.0), 10.0);
    float ao = calculateAO(sp, sn);
    vec3 hotSpec = vec3(1.0);
    vec3 color = (diff * baseColor + spec * hotSpec) * atte;
    return clamp(color * ao, 0.0, 1.0);
}

void main() {
    // Centered NDC coords: y in [-0.5, 0.5], x in [-aspect/2, aspect/2]
    float aspect = resolution.x / resolution.y;
    vec2 uv = (fragTexCoord - 0.5) * vec2(aspect, 1.0);

    // Auto-framed camera: distance R from origin scales with density.
    // Half-FOV ~= 0.785 rad, tan(0.785) ~= 1.0, so visible extent at distance R
    // is ~2R. R = density * 0.7 fits the `density`-wide grid with margin.
    // The -0.25 z offset is transcribed verbatim from the reference's
    // `vec3 ro = lk + vec3(0, 25., -.25)`; it keeps `fwd.z` nonzero so `rgt`
    // is well-defined at pitch = HALF_PI (overhead).
    float R = density * 0.7;
    vec3 lk = vec3(0.0);
    vec3 ro = vec3(0.0, R * sin(pitch), -R * cos(pitch) - 0.25);
    vec3 lp = ro + vec3(0.0, 3.75, 10.0);

    float FOV = 1.57;
    vec3 fwd = normalize(lk - ro);
    vec3 rgt = normalize(vec3(fwd.z, 0.0, -fwd.x));
    vec3 up = cross(fwd, rgt);
    vec3 rd = normalize(fwd + FOV * uv.x * rgt + FOV * uv.y * up);

    float t = trace(ro, rd);
    vec3 sp = ro + rd * t;
    vec3 sn = normal(sp);

    // Color sampled from the input cell that the surface point belongs to.
    vec2 hitUV = cellUVForId(floor(sp.xz));
    vec3 baseColor = texture(texture0, hitUV).rgb;

    vec3 color = lighting(sp, sn, lp, rd, baseColor);

    // Vignette (kicks in only at extreme corners with widescreen aspect)
    float vig = 1.0 - smoothstep(1.0, 3.5, length(uv));
    color *= mix(0.8, 1.0, vig);

    finalColor = vec4(color, 1.0);
}
```

What's verbatim from the reference vs. what's substituted (per research doc):

- `roundBox`, `getGrey`, the 5x5 neighborhood loop body, the `trace` loop (96 steps, 0.85 step scale, FAR=60), the central-difference `normal`, `calculateAO`, the `lighting` body (diffuse, specular `pow(..., 10)`, attenuation `2/(1+0.002*ldist^2)`, `clamp(color*ao, 0, 1)`), and the vignette: verbatim.
- Cell UV formula, pillar half-extents, corner radius, height multiplier, `hotSpec` color, color source: substituted per research doc.
- Camera position substitution `(0, R*sin(pitch), -R*cos(pitch) - 0.25)`: per research doc; the `-0.25` z offset is transcribed from reference's `ro = vec3(0, 25, -0.25)`.
- `helix`, `tileHeight`, `sdBoxFrame`, `rtrace`, `colors[]`, `rnd`/`rndTile`/`rndTileY`, `fres`, `path`, `skyGradient`, `rot`/`rot2`, fisheye, cubemap reflection, second-bounce reflection trace: dropped (all dead in the reference per the brainstorm audit).

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| density | float (int-flavored) | 16-128 | 48 | yes | `Density` |
| pitch | float (radians) | 0 - HALF_PI | PI/3 | yes | `Pitch` (degrees in UI) |
| heightScale | float | 0-20 | 8 | yes | `Height Scale` |
| pillarFill | float | 0.05-0.49 | 0.27 | yes | `Pillar Fill` |
| cornerRadius | float | 0-0.5 | 0.3 | yes | `Corner Radius` |

Param IDs registered with the modulation engine: `pillarGrid.density`, `pillarGrid.pitch`, `pillarGrid.heightScale`, `pillarGrid.pillarFill`, `pillarGrid.cornerRadius`.

Pitch uses `HALF_PI` constant defined locally at the top of `pillar_grid.cpp` as `static const float HALF_PI = PI_F / 2.0f;` (matches `perspective_tilt.cpp`).

### Constants

- Enum name: `TRANSFORM_PILLAR_GRID` (added to `TransformEffectType` in `src/config/effect_config.h` immediately before `TRANSFORM_ACCUM_COMPOSITE` and `TRANSFORM_EFFECT_COUNT`)
- Display name: `"Pillar Grid"`
- Category badge: `"CELL"`
- Section index: `2` (Cellular)
- Flags: `EFFECT_FLAG_HALF_RES` (the 5x5-neighborhood-per-step shader is heavy at 1080p; the research doc anticipates this)
- Macro: `REGISTER_EFFECT` (Init takes only `Effect*`)

### UI Layout

Two `ImGui::SeparatorText()` sections in `DrawPillarGridParams`, in order:

1. `Camera` -- `Pitch` slider (`ModulatableSliderAngleDeg`, register bounds `0` to `HALF_PI`)
2. `Geometry` -- `Density` (`ModulatableSliderInt`, integer display), `Height Scale` (`ModulatableSlider`, `%.2f`), `Pillar Fill` (`ModulatableSlider`, `%.3f`), `Corner Radius` (`ModulatableSlider`, `%.2f`)

Pattern matches `multi_scale_grid.cpp` (sectioned sliders) and `perspective_tilt.cpp` (`ModulatableSliderAngleDeg` for pitch).

---

## Tasks

### Wave 1: Header

#### Task 1.1: Create pillar_grid.h

**Files**: `src/effects/pillar_grid.h`
**Creates**: `PillarGridConfig`, `PillarGridEffect`, `PILLAR_GRID_CONFIG_FIELDS`, function declarations -- needed by Wave 2 tasks.

**Do**: Create the header per the Design > Types section. Use `multi_scale_grid.h` as the structural template (config struct with defaults, `*_CONFIG_FIELDS` macro, runtime struct with shader + uniform locs, four lifecycle decls + `Get*Effect` accessor decl, forward-declared `PostEffect`). Do not introduce any helper structs or extra fields beyond what the Design specifies.

**Verify**: `cmake.exe --build build` -- header should compile clean as part of any unit that includes it (no consumers yet).

---

### Wave 2: Implementation, Shader, Integration (4 tasks in parallel)

All Wave 2 tasks depend on Wave 1 (`pillar_grid.h` exists). They touch four different files and can run in parallel; the build is verified once all four complete.

#### Task 2.1: Create pillar_grid.cpp

**Files**: `src/effects/pillar_grid.cpp`
**Depends on**: Wave 1 complete; Wave 2.3 (effect_config.h) lands `TRANSFORM_PILLAR_GRID` and `EffectConfig::pillarGrid` symbols by build time.

**Do**: Write the implementation in this exact shape, matching the Design and modelling on `multi_scale_grid.cpp` + `perspective_tilt.cpp`:

- Includes (alphabetized within group, per conventions):
  - `"pillar_grid.h"`
  - `"automation/mod_sources.h"`, `"automation/modulation_engine.h"`, `"config/constants.h"`, `"config/effect_descriptor.h"`, `"render/post_effect.h"`
  - `"imgui.h"`, `"ui/modulatable_slider.h"`, `"ui/ui_units.h"`
  - `<stddef.h>`
- File-local: `static const float HALF_PI = PI_F / 2.0f;` after the includes (mirrors `perspective_tilt.cpp:18`).
- `PillarGridEffectInit(PillarGridEffect *e)`: load `shaders/pillar_grid.fs`; if `shader.id == 0` return `false`; cache all 6 uniform locs (`resolution`, `density`, `pitch`, `heightScale`, `pillarFill`, `cornerRadius`); return `true`.
- `PillarGridEffectSetup(const PillarGridEffect *e, const PillarGridConfig *cfg)`: bind `resolution` from `GetScreenWidth()` / `GetScreenHeight()` (vec2 float pattern from `voronoi.cpp`), then bind each of the 5 config float fields with `SHADER_UNIFORM_FLOAT`.
- `PillarGridEffectUninit(const PillarGridEffect *e)`: `UnloadShader(e->shader)`.
- `PillarGridRegisterParams(PillarGridConfig *cfg)`: register all 5 params with the IDs and ranges from the Parameters table. `pitch` registers as `[0, HALF_PI]`. `density` registers as `[16, 128]`.
- `// === UI ===` section: `static void DrawPillarGridParams(EffectConfig *e, const ModSources *ms, ImU32 glow)` -- two `SeparatorText` sections per Design > UI Layout. Use `ModulatableSliderAngleDeg` for pitch, `ModulatableSliderInt` for density, `ModulatableSlider` for the rest.
- `GetPillarGridEffect(PostEffect *pe)` accessor returning `(PillarGridEffect *)pe->effectStates[TRANSFORM_PILLAR_GRID]`.
- Bridge: `void SetupPillarGrid(PostEffect *pe)` (non-static) calling `PillarGridEffectSetup(GetPillarGridEffect(pe), &pe->effects.pillarGrid)`.
- Bottom of file, wrapped in `// clang-format off` / `// clang-format on`:

  ```cpp
  REGISTER_EFFECT(TRANSFORM_PILLAR_GRID, PillarGrid, pillarGrid,
                  "Pillar Grid", "CELL", 2, EFFECT_FLAG_HALF_RES,
                  SetupPillarGrid, NULL, DrawPillarGridParams)
  ```

Reminder: bridge function `SetupPillarGrid` is non-static (referenced by the macro). `DrawPillarGridParams` is static.

**Verify**: `cmake.exe --build build` succeeds with no warnings.

---

#### Task 2.2: Create pillar_grid.fs

**Files**: `shaders/pillar_grid.fs`
**Depends on**: Wave 1 (no compile-time dep, but conceptually downstream of the design's uniform list).

**Do**: Write the shader exactly as in the Design > Algorithm section. Do not deviate. The header attribution comment block (5 lines starting with `// Based on "Voxel Pillars webcam Y28"`) must be present before `#version 330`. ASCII-only comments per conventions.

**Verify**: After Tasks 2.1-2.4 land, `cmake.exe --build build` succeeds; running `./build/AudioJones.exe`, enabling Pillar Grid, the shader compiles at runtime (raylib logs shader compile errors to stderr -- no errors expected).

---

#### Task 2.3: Wire pillar_grid into effect_config.h

**Files**: `src/config/effect_config.h`
**Depends on**: Wave 1 (`pillar_grid.h` must exist for the include).

**Do**: Three small additions, each in alphabetical/logical position:

1. Add `#include "effects/pillar_grid.h"` to the alphabetical include block at the top (between `phyllotaxis.h` and `pitch_spiral.h`).
2. Add `TRANSFORM_PILLAR_GRID,` enum entry inside `TransformEffectType` immediately before `TRANSFORM_ACCUM_COMPOSITE,` (which must remain second-to-last) and `TRANSFORM_EFFECT_COUNT` (which must remain last). Place it at the end of the existing list of transform entries, mirroring how recent additions like `TRANSFORM_ROTOR_GRID_BLEND` were appended.
3. Add `PillarGridConfig pillarGrid;` member to `EffectConfig` struct, with a one-line `// Pillar Grid (...)` comment matching the existing per-effect comment style.

The `TransformOrderConfig::TransformOrderConfig()` constructor already auto-populates `order[i] = (TransformEffectType)i` for all enum values, so no separate order-array entry is required.

**Verify**: `cmake.exe --build build` compiles. `TRANSFORM_PILLAR_GRID` resolves; `EffectConfig::pillarGrid` is accessible from `pillar_grid.cpp`.

---

#### Task 2.4: Wire pillar_grid into effect_serialization.cpp

**Files**: `src/config/effect_serialization.cpp`
**Depends on**: Wave 1 (`pillar_grid.h` must exist for the include and macro).

**Do**: Three additions in this file:

1. Add `#include "effects/pillar_grid.h"` to the alphabetical include block (between `phyllotaxis.h` and `pitch_spiral.h`, mirroring the order in `effect_config.h`).
2. Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(PillarGridConfig, PILLAR_GRID_CONFIG_FIELDS)` alongside the other per-config macros (group with the other Cellular/transform configs near `MULTI_SCALE_GRID_CONFIG_FIELDS`).
3. Add `X(pillarGrid)` to the `EFFECT_CONFIG_FIELDS(X)` X-macro table at the end of the existing list (next to the other recently added entries like `X(rotorGrid)` near the bottom).

**Verify**: `cmake.exe --build build` compiles. Save a preset with Pillar Grid enabled and tweaked params; reload; values restored.

---

## Final Verification

- [ ] `cmake.exe --build build` succeeds with zero warnings.
- [ ] Running `./build/AudioJones.exe`, "Pillar Grid" appears in the Cellular section with badge `CELL`.
- [ ] Enabling Pillar Grid produces the expected look: input scene appears as a 3D voxel city of pillars, viewed at the default pitch (~60 degrees from horizontal).
- [ ] Adjusting `Pitch` smoothly changes camera angle from horizontal-across-field to overhead.
- [ ] Adjusting `Density` smoothly changes the cell count and camera framing (more cells = camera further out, finer resolution).
- [ ] Adjusting `Height Scale` makes pillars rise/fall; effect is dramatic at low pitch, subtle at overhead pitch (the cascading-threshold interaction from the research).
- [ ] Adjusting `Pillar Fill` and `Corner Radius` together shifts the field between "sparse spires" and "merged blobs".
- [ ] Saving and reloading a preset preserves all 5 params and the enabled state.
- [ ] Modulation routes to all 5 params register and respond.

---

## Implementation Notes

These deviations from the original plan landed during implementation while iterating against `image.png`. They are not drift; they are corrections.

**Param surface shrunk from 5 to 4.** `pillarFill` was dropped entirely. Letting users vary fill independent of cornerRadius produced visible gaps between pillars (any combination where `fill + cornerRadius < 0.5`), through which rays reached FAR and exposed the underlying scene. Fill is now derived in the shader as `max(0.57 - cornerRadius, 0)`, preserving the reference's 0.07-per-side overlap that gives sides their differently-colored faces.

**Tightened slider ranges:**
- `density`: `[48, 128]` (was `[16, 128]`). Below 48 the framing math collapses (camera distance clamped, visible area larger than field, edges show passthrough).
- `heightScale`: `[0, 8]` (was `[0, 20]`). Tall pillars at small density tower above the camera position.
- `pitch`: `[1.484, 1.5708]` rad / `[85, 90]` degrees (was `[0, HALF_PI]`). At lower pitches the visible floor extends past the field edge - irreducible without tiling. The user's hard rule is "no fallbacks of any kind" (no tiling, no passthrough, no edge-clamp, no black), so the camera is constrained to where the field always covers the FOV.

**Default pitch is HALF_PI (overhead).** The reference is overhead. A tilted default left visible floor past the field at corners.

**Removed the lighting attenuation term** (`atte = 2.0 / (1.0 + 0.002 * ldist*ldist)`). Reference's coefficient was tuned for its fixed `R=25`; with our R varying with density, atte ranged from 1.3 (boost) at low density to 0.37 (heavy darkening) at high density, producing inconsistent brightness. Lighting is now `diff * baseColor + spec * hotSpec` clamped, uniform across density.

**Pillar height has a small floor.** `h = max(grey * heightScale, 0.05)`. Without the floor, `heightScale = 0` and fully-black input pixels produce zero-thickness flat plates whose SDF is degenerate (raymarch can step past them, normals are undefined). The 0.05 minimum keeps the SDF well-defined while still reading as "flat tile" visually.

**FAR distance scales with density** (`max(60.0, density * 2.0)`) and the trace loop runs 128 steps (was 96). At high density the camera sits far enough that reference's FAR=60 truncated rays before they reached pillars at the far edge, surfacing as passthrough patches.

**Camera framing**: `R = max(density / 3.0 - 3.5, 5.0)`. Looser than the tight overhead fit (`density / 2.8 - 3`) so the field has small margin past the FOV at the constrained pitch range.
