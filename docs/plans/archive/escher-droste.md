# Escher Droste

New transform effect implementing the true self-tiling Droste warp (distinct from the existing `droste_zoom`, which is a spiral smear). Samples the input texture in tiled log-polar UV space so the frame recurses into itself at log-spaced scales along an Escher spiral. Lives in the Motion (`MOT`) category next to `droste_zoom`.

**Research**: `docs/research/escher_droste.md`

## Design

### Types

Placed in `src/effects/escher_droste.h`.

```cpp
#include "config/dual_lissajous_config.h"
#include "raylib.h"
#include <stdbool.h>

struct EscherDrosteConfig {
  bool enabled = false;
  float scale = 256.0f;          // Tile scale factor k (4.0 - 1024.0, log slider)
  float zoomSpeed = 0.5f;        // Log-radial zoom rate, signed (-2.0 - 2.0)
  float spiralStrength = 1.0f;   // 0 = zoom-only Droste, 1 = Escher spiral (-2.0 - 2.0)
  float rotationOffset = 0.0f;   // Static rotation of tiling pattern, radians (-PI - PI)
  DualLissajousConfig center = {.amplitude = 0.0f};  // 2D drift of vanishing point
  float innerRadius = 0.05f;     // Fade mask around singular center (0.0 - 0.5)
};

#define ESCHER_DROSTE_CONFIG_FIELDS                                            \
  enabled, scale, zoomSpeed, spiralStrength, rotationOffset, center, innerRadius

typedef struct EscherDrosteEffect {
  Shader shader;
  int resolutionLoc;
  int centerLoc;
  int scaleLoc;
  int zoomPhaseLoc;
  int spiralStrengthLoc;
  int rotationOffsetLoc;
  int innerRadiusLoc;
  float zoomPhase;  // CPU-accumulated phase, replaces iTime*0.5 from reference
} EscherDrosteEffect;

bool EscherDrosteEffectInit(EscherDrosteEffect *e);
void EscherDrosteEffectSetup(EscherDrosteEffect *e, EscherDrosteConfig *cfg,
                             float deltaTime);
void EscherDrosteEffectUninit(const EscherDrosteEffect *e);
void EscherDrosteRegisterParams(EscherDrosteConfig *cfg);
```

Note: `Setup` takes a non-const `EscherDrosteConfig*` because `DualLissajousUpdate` mutates the embedded `center.phase`.

### Algorithm

The reference shader (perlinw, Shadertoy `sfBSWm`) is transcribed line-by-line using the substitution table below. The Algorithm section IS the source of truth; implementing agents must match this shader byte-for-byte, no reinterpretation.

#### Substitution table (reference -> this effect)

| Reference | Replacement | Notes |
|-----------|-------------|-------|
| `fragCoord - 0.5 * iResolution.xy` | `fragTexCoord * resolution - 0.5 * resolution - center` | `center` is in pixel space: `{0,0}` = screen center, positive = right/up |
| `/ iResolution.y` | `/ resolution.y` | Aspect-correct normalization, verbatim |
| `iResolution.xy` | `resolution` (vec2 uniform) | |
| `iTime * 0.5` | `zoomPhase` (float uniform) | CPU-accumulated: `e->zoomPhase += cfg->zoomSpeed * deltaTime` (speed-accumulation rule) |
| `k = 256.0` | `scale` (float uniform) | Modulatable param |
| `v * L / (TWO_PI)` in warpU | `v * (L / TWO_PI) * spiralStrength` | `spiralStrength = 0` collapses to zoom-only tiling, `= 1` reproduces reference |
| `u * L / (TWO_PI)` in warpV | `u * (L / TWO_PI) * spiralStrength` | same factor applied symmetrically |
| (not in reference) | `warpV += rotationOffset` inserted after the `warpU -= zoomPhase` line | Static rotation of tiled pattern, radians |
| `iChannel0` | `texture0` | Input stage texture |
| commented `col *= smoothstep(0.0, 0.01, r)` | `col *= (innerRadius > 0.0) ? smoothstep(0.0, innerRadius, r) : 1.0` | Fade mask, parameterized; skip when `innerRadius == 0` |

#### Shader

`shaders/escher_droste.fs`:

```glsl
// Based on "Escher's Droste warp" by perlinw
// https://www.shadertoy.com/view/sfBSWm
// License: CC BY-NC-SA 3.0 Unported
// Modified: parameterized spiralStrength, rotationOffset, innerRadius, CPU-accumulated zoom phase, configurable center

#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;
uniform vec2 resolution;
uniform vec2 center;           // Pixel-space drift from screen center (0,0 = no drift)
uniform float scale;           // Tile scale factor k
uniform float zoomPhase;       // CPU-accumulated phase (replaces iTime * 0.5)
uniform float spiralStrength;  // 0 = zoom-only tiling, 1 = Escher spiral
uniform float rotationOffset;  // Static angle added to warpV (radians)
uniform float innerRadius;     // Fade mask radius (0 disables)

const float TWO_PI = 6.28318530718;

void main() {
    vec2 uv = (fragTexCoord * resolution - 0.5 * resolution - center) / resolution.y;

    float L = log(scale);

    // log-polar space transform
    float r = length(uv);
    float a = atan(uv.y, uv.x);
    float u = log(r);
    float v = a;

    // Escher warp with parameterized strength
    // c = 0: warpU/V collapse to u/v (pure zoom-only tiling)
    // c = L/TWO_PI: reference Escher spiral
    float c = (L / TWO_PI) * spiralStrength;
    float warpU = u + v * c;
    float warpV = v - u * c;

    // continuous zoom along log-radius (signed)
    warpU -= zoomPhase;

    // static rotation of tiled pattern
    warpV += rotationOffset;

    // map back to tile UV space and tile
    vec2 texCoord = vec2(warpU / L, warpV / TWO_PI);

    // seam-corrected derivatives to hide atan2 branch cut at deep zoom
    vec2 dx = dFdx(texCoord);
    vec2 dy = dFdy(texCoord);
    dx -= floor(dx + 0.5);
    dy -= floor(dy + 0.5);

    vec3 col = textureGrad(texture0, texCoord, dx, dy).rgb;

    // fade the singular center where log-polar math diverges
    float fade = (innerRadius > 0.0) ? smoothstep(0.0, innerRadius, r) : 1.0;

    finalColor = vec4(col * fade, 1.0);
}
```

### CPU Setup

`EscherDrosteEffectSetup(e, cfg, deltaTime)` does, in order:

1. Accumulate `e->zoomPhase += cfg->zoomSpeed * deltaTime` (speed-accumulation rule).
2. Compute screen dimensions: `const float w = (float)GetScreenWidth(); const float h = (float)GetScreenHeight();`
3. Call `DualLissajousUpdate(&cfg->center, deltaTime, 0.0f, &cx, &cy)` to get drift offsets in `[-amplitude, +amplitude]` UV-fraction units.
4. Convert to the pixel-space `center` uniform by scaling by screen height so horizontal and vertical drift have equal visual magnitude under the shader's `/ resolution.y` normalization:
   ```cpp
   const float centerPix[2] = {cx * h, cy * h};
   ```
5. Bind `resolution` as `{w, h}` (same pattern as `infinite_zoom.cpp:95-97`).
6. Bind `center` (vec2), `scale`, `zoomPhase`, `spiralStrength`, `rotationOffset`, `innerRadius`.

No resize callback needed - shader derives all spatial geometry from `resolution` uniform each frame.

### UI

`static void DrawEscherDrosteParams(EffectConfig *e, const ModSources *ms, ImU32 glow)` uses the Signal Stack section order:

```cpp
ImGui::SeparatorText("Geometry");
ModulatableSliderLog("Scale##escherDroste", &cfg->scale,
                     "escherDroste.scale", "%.0f", ms);

ImGui::SeparatorText("Warp");
ModulatableSlider("Spiral Strength##escherDroste", &cfg->spiralStrength,
                  "escherDroste.spiralStrength", "%.2f", ms);
ModulatableSliderAngleDeg("Rotation##escherDroste", &cfg->rotationOffset,
                          "escherDroste.rotationOffset", ms);

ImGui::SeparatorText("Animation");
ModulatableSlider("Zoom Speed##escherDroste", &cfg->zoomSpeed,
                  "escherDroste.zoomSpeed", "%.2f", ms);

ImGui::SeparatorText("Center");
DrawLissajousControls(&cfg->center, "escherDroste_center",
                      "escherDroste.center", ms, 5.0f);

ImGui::SeparatorText("Masking");
ModulatableSlider("Inner Radius##escherDroste", &cfg->innerRadius,
                  "escherDroste.innerRadius", "%.2f", ms);
```

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| `scale` | float | 4.0 - 1024.0 (log) | 256.0 | Yes | `Scale##escherDroste` |
| `spiralStrength` | float | -2.0 - 2.0 | 1.0 | Yes | `Spiral Strength##escherDroste` |
| `rotationOffset` | float (rad) | -ROTATION_OFFSET_MAX - +ROTATION_OFFSET_MAX | 0.0 | Yes (AngleDeg) | `Rotation##escherDroste` |
| `zoomSpeed` | float | -2.0 - 2.0 | 0.5 | Yes | `Zoom Speed##escherDroste` |
| `center.amplitude` | float | 0.0 - 0.5 | 0.0 | Yes (via DrawLissajousControls) | via widget |
| `center.motionSpeed` | float | 0.0 - 10.0 | 1.0 | Yes (via DrawLissajousControls) | via widget |
| `innerRadius` | float | 0.0 - 0.5 | 0.05 | Yes | `Inner Radius##escherDroste` |

Params registered with `ModEngineRegisterParam` in `EscherDrosteRegisterParams`:

```cpp
ModEngineRegisterParam("escherDroste.scale", &cfg->scale, 4.0f, 1024.0f);
ModEngineRegisterParam("escherDroste.zoomSpeed", &cfg->zoomSpeed, -2.0f, 2.0f);
ModEngineRegisterParam("escherDroste.spiralStrength", &cfg->spiralStrength, -2.0f, 2.0f);
ModEngineRegisterParam("escherDroste.rotationOffset", &cfg->rotationOffset,
                       -ROTATION_OFFSET_MAX, ROTATION_OFFSET_MAX);
ModEngineRegisterParam("escherDroste.center.amplitude", &cfg->center.amplitude, 0.0f, 0.5f);
ModEngineRegisterParam("escherDroste.center.motionSpeed", &cfg->center.motionSpeed, 0.0f, 10.0f);
ModEngineRegisterParam("escherDroste.innerRadius", &cfg->innerRadius, 0.0f, 0.5f);
```

The two `center.*` registrations satisfy the shared-widget rule: `DrawLissajousControls` references `<prefix>.amplitude` and `<prefix>.motionSpeed`, and both IDs must exist even when `amplitude = 0`.

### Constants

- Enum name: `TRANSFORM_ESCHER_DROSTE` (inserted in `TransformEffectType` next to `TRANSFORM_DROSTE_ZOOM`)
- Display name: `"Escher Droste"`
- Field name: `escherDroste`
- Badge: `"MOT"`
- Section index: `3` (Motion)
- Param prefix: `"escherDroste."`
- Flags: `EFFECT_FLAG_NONE`

---

## Tasks

### Wave 1: Header

#### Task 1.1: Create `src/effects/escher_droste.h`

**Files**: `src/effects/escher_droste.h`
**Creates**: `EscherDrosteConfig`, `EscherDrosteEffect`, `ESCHER_DROSTE_CONFIG_FIELDS` - consumed by every Wave 2 task.

**Do**: Write the header exactly matching the Design > Types section. Include `config/dual_lissajous_config.h` for the embedded `DualLissajousConfig`, `raylib.h` for `Shader`, and `<stdbool.h>`. Use `DROSTE_ZOOM_H` / `droste_zoom.h` as a structural template, but with the fields listed in the Design section (not `droste_zoom`'s fields).

**Verify**: Header compiles on its own. `cmake.exe --build build` succeeds after Wave 2 tasks land.

---

### Wave 2: Implementation (all parallel, no file overlap)

#### Task 2.1: Create `src/effects/escher_droste.cpp`

**Files**: `src/effects/escher_droste.cpp`
**Depends on**: Task 1.1 complete

**Do**: Follow `src/effects/droste_zoom.cpp` as structural template. Implement five functions + colocated UI + registration macro:

1. `EscherDrosteEffectInit(e)` - load `shaders/escher_droste.fs`, cache all uniform locations from Design > Types, zero `zoomPhase`. Same shader-id-check-and-fail pattern as `droste_zoom.cpp:13-29`.
2. `EscherDrosteEffectSetup(e, cfg, deltaTime)` - perform the steps in Design > CPU Setup, in order.
3. `EscherDrosteEffectUninit(e)` - `UnloadShader(e->shader)`.
4. `EscherDrosteRegisterParams(cfg)` - seven `ModEngineRegisterParam` calls as listed in Design > Parameters.
5. `// === UI ===` section with `static void DrawEscherDrosteParams(EffectConfig*, const ModSources*, ImU32)` matching Design > UI.
6. Non-static `void SetupEscherDroste(PostEffect *pe)` bridge: delegates to `EscherDrosteEffectSetup(&pe->escherDroste, &pe->effects.escherDroste, pe->currentDeltaTime)`.
7. Bottom: `REGISTER_EFFECT(TRANSFORM_ESCHER_DROSTE, EscherDroste, escherDroste, "Escher Droste", "MOT", 3, EFFECT_FLAG_NONE, SetupEscherDroste, NULL, DrawEscherDrosteParams)` between `// clang-format off` / `// clang-format on`.

Includes at top (clang-format sorts alphabetically within groups):
- Own header: `"escher_droste.h"`
- Project: `"automation/mod_sources.h"`, `"automation/modulation_engine.h"`, `"config/constants.h"`, `"config/dual_lissajous_config.h"`, `"config/effect_descriptor.h"`, `"render/post_effect.h"`
- UI: `"imgui.h"`, `"ui/modulatable_slider.h"`, `"ui/ui_units.h"`
- System: `<stddef.h>`

**Verify**: `cmake.exe --build build` succeeds. Effect shows up in the Motion category of the effects panel with badge `MOT`.

---

#### Task 2.2: Create `shaders/escher_droste.fs`

**Files**: `shaders/escher_droste.fs`
**Depends on**: none (parallel with all other Wave 2 tasks)

**Do**: Create the shader exactly as written in Design > Algorithm. The attribution comment block MUST precede `#version 330` and match the Design section's header verbatim. Do not omit, reorder, or rewrite lines. The reference-derived block (log-polar transform, Escher warp, seam correction, `textureGrad` sampling) must match the Design section exactly. If any line looks ambiguous, consult the Design > Algorithm > Substitution table above, not the research doc - the plan is the source of truth.

**Verify**: Shader compiles at runtime (check raylib log for `SHADER: [ID] Fragment shader compiled successfully`). Enabling the effect in-app yields a recognizable spiral tiling of the incoming frame, not a solid color or checkerboard.

---

#### Task 2.3: Modify `src/config/effect_config.h`

**Files**: `src/config/effect_config.h`
**Depends on**: Task 1.1 complete

**Do**: Three edits (no existing field or enum member is removed):

1. Add `#include "effects/escher_droste.h"` alongside the other effect includes near the top (clang-format sorts).
2. Add `TRANSFORM_ESCHER_DROSTE,` to the `TransformEffectType` enum, inserted directly after `TRANSFORM_DROSTE_ZOOM,` on line 169.
3. Add `EscherDrosteConfig escherDroste;` inside `struct EffectConfig`, immediately after the `DrosteZoomConfig drosteZoom;` member on line 496. Precede with a one-line comment: `// Escher Droste (log-polar recursive tiling, distinct from droste zoom)`.

Do NOT edit `TransformOrderConfig::TransformOrderConfig()` - it fills the order array by iteration.

**Verify**: `cmake.exe --build build` succeeds. Enabling the effect in the UI adds it to the live pipeline list.

---

#### Task 2.4: Modify `src/render/post_effect.h`

**Files**: `src/render/post_effect.h`
**Depends on**: Task 1.1 complete

**Do**: Two edits:

1. Add `#include "effects/escher_droste.h"` alongside the other effect includes near the top of the file (clang-format sorts).
2. Add `EscherDrosteEffect escherDroste;` inside `struct PostEffect`, immediately after the `DrosteZoomEffect drosteZoom;` member on line 259.

**Verify**: `cmake.exe --build build` succeeds.

---

#### Task 2.5: Modify `src/config/effect_serialization.cpp`

**Files**: `src/config/effect_serialization.cpp`
**Depends on**: Task 1.1 complete

**Do**: Three edits:

1. Add `#include "effects/escher_droste.h"` alongside the other per-effect includes near the top of the file (clang-format sorts).
2. Add the JSON macro next to the other D-effect macros (see `effect_serialization.cpp:364-367` for the `DrosteZoomConfig` / `DrekkerPaintConfig` pair):
   ```cpp
   NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(EscherDrosteConfig,
                                                   ESCHER_DROSTE_CONFIG_FIELDS)
   ```
3. Add `X(escherDroste)` to the `EFFECT_CONFIG_FIELDS(X)` X-macro table on `effect_serialization.cpp:707`, immediately after `X(drosteZoom)`.

`DualLissajousConfig` already has a serialization macro on line 283, so the embedded `center` field serializes automatically.

**Verify**: `cmake.exe --build build` succeeds. Toggling the effect on, saving a preset, and reloading it restores all params including `center` Lissajous fields.

---

## Final Verification

- [ ] `cmake.exe --build build` succeeds with no warnings.
- [ ] Effect appears in the Motion category with badge `MOT`, labeled `Escher Droste`.
- [ ] Effect can be reordered via drag-drop in the transform pipeline.
- [ ] At `spiralStrength = 0` the output tiles without spiral (straight log-radial tiling).
- [ ] At `spiralStrength = 1` the output matches the visual character of the reference Shadertoy (infinite spiral recursion with seamless deep zoom).
- [ ] `zoomSpeed = 0` freezes the animation; flipping sign reverses direction.
- [ ] `rotationOffset` slider rotates the tile pattern; at +180deg the pattern returns to a visually equivalent configuration.
- [ ] `Inner Radius = 0` shows the raw center (expected visible artifact near the singularity); increasing it hides the center cleanly.
- [ ] Center Lissajous with `amplitude > 0` drifts the vanishing point around screen center.
- [ ] All seven modulation routes listed in Design > Parameters appear in the modulation engine's target list.
- [ ] Preset save then reload preserves every param, including embedded `center.*` Lissajous fields.
- [ ] Existing `droste_zoom` effect is unaffected (both effects can be enabled simultaneously without interaction).
