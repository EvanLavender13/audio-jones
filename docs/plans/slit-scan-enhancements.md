# Slit Scan Enhancements

Rename "Slit Scan Corridor" to "Slit Scan", add a flat (single-wall) mode, and add a center glow parameter. All changes enhance the existing effect — no new pipeline slot, no new files in the descriptor system.

**Research**: `docs/research/slit_scan_enhancements.md`

## Design

### Types

**SlitScanConfig** (renamed from `SlitScanCorridorConfig`):

```cpp
struct SlitScanConfig {
  bool enabled = false;

  // Slit sampling
  float slitPosition = 0.5f; // Horizontal UV to sample (0.0-1.0)
  float slitWidth = 0.005f;  // Slit width in UV space (0.001-1.0)

  // Mode
  int mode = 0; // 0 = Corridor, 1 = Flat (not modulatable)

  // Corridor dynamics
  float speed = 2.0f;       // Outward advance rate (0.1-10.0)
  float pushAccel = 0.3f;   // Edge push acceleration (0.0-10.0)
  float perspective = 0.3f; // Vertical wall spread at edges (0.0-10.0)
  float fogStrength = 1.0f; // Depth brightness falloff (0.1-5.0)
  float brightness = 1.0f;  // Fresh slit brightness (0.1-3.0)
  float glow = 0.0f;        // Center brightness falloff steepness (0.0-5.0)

  // Rotation (display-time only, NOT inside ping-pong)
  float rotationAngle = 0.0f; // Static rotation radians (-PI..PI)
  float rotationSpeed = 0.0f; // Rotation rate rad/s (-PI..PI)
};
```

Fields macro:

```cpp
#define SLIT_SCAN_CONFIG_FIELDS                                                \
  enabled, slitPosition, slitWidth, mode, speed, pushAccel, perspective,       \
      fogStrength, brightness, glow, rotationAngle, rotationSpeed
```

**SlitScanEffect** (renamed from `SlitScanCorridorEffect`):

```cpp
typedef struct SlitScanEffect {
  Shader shader;        // Accumulation (ping-pong)
  Shader displayShader; // Perspective + rotation pass
  RenderTexture2D pingPong[2];
  int readIdx;
  float rotationAccum;

  // Accumulation shader locations
  int resolutionLoc;
  int sceneTextureLoc;
  int slitPositionLoc;
  int speedDtLoc;
  int pushAccelLoc;
  int slitWidthLoc;
  int brightnessLoc;
  int centerLoc;

  // Display shader locations
  int dispRotationLoc;
  int dispPerspectiveLoc;
  int dispFogLoc;
  int dispCenterLoc;
  int dispGlowLoc;
} SlitScanEffect;
```

### Algorithm

**Accumulation shader (`slit_scan.fs`)** — replace hardcoded `0.5` center with `center` uniform:

```glsl
#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;
uniform sampler2D sceneTexture;
uniform vec2 resolution;
uniform float slitPosition;
uniform float speedDt;
uniform float pushAccel;
uniform float slitWidth;
uniform float brightness;
uniform float center;

void main() {
    vec2 uv = fragTexCoord;
    float dx = uv.x - center;
    float d = abs(dx);
    float signDx = sign(dx);

    float halfPixel = 0.5 / resolution.x;
    float shift = speedDt * (1.0 + d * pushAccel);
    float sampleD = max(halfPixel, d - shift);

    vec2 sampleUV = vec2(center + signDx * sampleD, uv.y);
    vec3 old = texture(texture0, sampleUV).rgb;

    float injectionWidth = 2.0 / resolution.x;
    float slitMask = smoothstep(injectionWidth, 0.0, d);

    float halfSlit = slitWidth * 0.5;
    float t = clamp(dx / max(injectionWidth, 0.001), -1.0, 1.0);
    float slitU = fract(slitPosition + t * halfSlit);
    vec3 slitColor = texture(sceneTexture, vec2(slitU, uv.y)).rgb * brightness;

    finalColor = vec4(mix(old, slitColor, slitMask), 1.0);
}
```

Changes from current: `0.5` → `center` uniform in `dx` calculation and `sampleUV` reconstruction. Everything else unchanged.

**Display shader (`slit_scan_display.fs`)** — add `center` and `glow` uniforms, normalize distance:

```glsl
#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;
uniform float rotation;
uniform float perspective;
uniform float fogStrength;
uniform float center;
uniform float glow;

void main() {
    vec2 uv = fragTexCoord;

    // Rotation centered on slit position
    vec2 centered = uv - vec2(center, 0.5);
    float c = cos(rotation), s = sin(rotation);
    uv = vec2(c * centered.x + s * centered.y,
              -s * centered.x + c * centered.y) + vec2(center, 0.5);

    // Horizontal distance from slit
    float dist = max(abs(uv.x - center), 0.001);

    // Normalize distance: 0 at slit, 1 at farthest edge
    float maxDist = max(center, 1.0 - center);
    float normDist = clamp(dist / maxDist, 0.0, 1.0);

    // Vertical wall spread
    float scale = 1.0 + perspective * normDist;
    float wallY = (uv.y - 0.5) / scale + 0.5;

    vec2 bufferUV = vec2(uv.x, clamp(wallY, 0.0, 1.0));
    vec3 color = texture(texture0, bufferUV).rgb;

    // Depth fog
    float fog = pow(normDist, fogStrength);
    color *= fog;

    // Center glow
    if (glow > 0.0) {
        float g = pow(1.0 - normDist, glow);
        color *= 1.0 + g;
    }

    finalColor = vec4(color, 1.0);
}
```

Changes from current: rotation pivot uses `(center, 0.5)` instead of `(0.5, 0.5)`. Distance normalized via `maxDist`. Fog uses `normDist` instead of `dist * 2.0`. Perspective uses `normDist` instead of `dist * 2.0`. Glow block added after fog. For corridor mode (center=0.5): `maxDist=0.5`, so `normDist = dist/0.5 = dist*2.0` — identical to current behavior.

**CPU-side center derivation** (in Setup):

```cpp
float center = (cfg->mode == 0) ? 0.5f : 0.0f;
```

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| slitPosition | float | 0.0-1.0 | 0.5 | yes | Slit Position |
| slitWidth | float | 0.001-1.0 | 0.005 | yes | Slit Width |
| mode | int | 0-1 | 0 | no | Mode |
| speed | float | 0.1-10.0 | 2.0 | yes | Speed |
| pushAccel | float | 0.0-10.0 | 0.3 | yes | Push Accel |
| perspective | float | 0.0-10.0 | 0.3 | yes | Perspective |
| fogStrength | float | 0.1-5.0 | 1.0 | yes | Fog Strength |
| brightness | float | 0.1-3.0 | 1.0 | yes | Brightness |
| glow | float | 0.0-5.0 | 0.0 | yes | Glow |
| rotationAngle | float | -PI..PI | 0.0 | yes | Rotation |
| rotationSpeed | float | -PI..PI | 0.0 | yes | Rotation Speed |

### Constants

- Enum name: `TRANSFORM_SLIT_SCAN` (was `TRANSFORM_SLIT_SCAN_CORRIDOR`)
- Display name: `"Slit Scan"` (was `"Slit Scan Corridor"`)
- Category: `"MOT"`, section 3 (unchanged)
- Flags: `EFFECT_FLAG_NEEDS_RESIZE` (unchanged)
- Config field: `slitScan` (was `slitScanCorridor`)
- Param ID prefix: `"slitScan."` (was `"slitScanCorridor."`)

---

## Tasks

### Wave 1: Header

#### Task 1.1: Create slit_scan.h

**Files**: `src/effects/slit_scan.h` (create), `src/effects/slit_scan_corridor.h` (delete)
**Creates**: `SlitScanConfig`, `SlitScanEffect` types, function declarations

**Do**: Create `src/effects/slit_scan.h` using the Design section's `SlitScanConfig` and `SlitScanEffect` structs verbatim. Rename all public functions from `SlitScanCorridor*` to `SlitScan*`:
- `SlitScanEffectInit(SlitScanEffect*, const SlitScanConfig*, int, int)`
- `SlitScanEffectSetup(SlitScanEffect*, const SlitScanConfig*, float)`
- `SlitScanEffectRender(SlitScanEffect*, const SlitScanConfig*, PostEffect*)`
- `SlitScanEffectResize(SlitScanEffect*, int, int)`
- `SlitScanEffectUninit(SlitScanEffect*)`
- `SlitScanConfigDefault(void)`
- `SlitScanRegisterParams(SlitScanConfig*)`

Include guard: `SLIT_SCAN_EFFECT_H`. Forward-declare `PostEffect`. Same structure as the old header but with new names, `mode` and `glow` fields added, `SLIT_SCAN_CONFIG_FIELDS` macro updated, and `centerLoc`/`dispCenterLoc`/`dispGlowLoc` added to the effect struct.

Delete `src/effects/slit_scan_corridor.h`.

**Verify**: `cmake.exe --build build` compiles (will fail until Wave 2 completes — that's expected).

---

### Wave 2: Parallel Implementation

#### Task 2.1: Create slit_scan.cpp

**Files**: `src/effects/slit_scan.cpp` (create), `src/effects/slit_scan_corridor.cpp` (delete)
**Depends on**: Wave 1 complete

**Do**: Create `src/effects/slit_scan.cpp` by renaming all identifiers from the old file. Follow the same structure: `CacheLocations`, `InitPingPong`, `UnloadPingPong`, lifecycle functions, UI section, bridge functions, manual registration.

Key changes beyond renaming:
- `#include "slit_scan.h"` (not `slit_scan_corridor.h`)
- `CacheLocations`: add `e->centerLoc = GetShaderLocation(e->shader, "center")`, `e->dispCenterLoc = GetShaderLocation(e->displayShader, "center")`, `e->dispGlowLoc = GetShaderLocation(e->displayShader, "glow")`
- `InitPingPong`: label string `"SLIT_SCAN"` (not `"SLIT_SCAN_CORRIDOR"`)
- `SlitScanEffectInit`: shader paths `"shaders/slit_scan.fs"` and `"shaders/slit_scan_display.fs"`
- `SlitScanEffectSetup`: compute `float center = (cfg->mode == 0) ? 0.5f : 0.0f;` and bind to `e->centerLoc` and `e->dispCenterLoc`. Bind `cfg->glow` to `e->dispGlowLoc`.
- `SlitScanRegisterParams`: add `ModEngineRegisterParam("slitScan.glow", &cfg->glow, 0.0f, 5.0f)`. All param IDs use `"slitScan."` prefix.
- UI: add `ImGui::Combo("Mode##slitscan", &cfg->mode, "Corridor\0Flat\0")` before the speed slider. Add `ModulatableSlider("Glow##slitscan", &cfg->glow, "slitScan.glow", "%.2f", ms)` after the brightness slider. All field references use `e->slitScan` (not `e->slitScanCorridor`), all param IDs use `"slitScan."` prefix.
- Registration: `TRANSFORM_SLIT_SCAN`, display name `"Slit Scan"`, field `slitScan`, `offsetof(EffectConfig, slitScan.enabled)`, prefix `"slitScan."`
- Bridge functions: `SetupSlitScan`, `RenderSlitScan`, `Init_slitScan`, `Uninit_slitScan`, `Resize_slitScan`, `Register_slitScan`, `GetShader_slitScan`

Delete `src/effects/slit_scan_corridor.cpp`.

**Verify**: Compiles after all Wave 2 tasks complete.

---

#### Task 2.2: Create shaders

**Files**: `shaders/slit_scan.fs` (create), `shaders/slit_scan_display.fs` (create), `shaders/slit_scan_corridor.fs` (delete), `shaders/slit_scan_corridor_display.fs` (delete)
**Depends on**: Wave 1 complete

**Do**: Create both shader files using the Algorithm section verbatim.

`slit_scan.fs`: Copy from the Algorithm section's accumulation shader. The only change from the old shader is `0.5` → `center` uniform in the `dx` calculation and `sampleUV` reconstruction.

`slit_scan_display.fs`: Copy from the Algorithm section's display shader. Changes: rotation pivot `(center, 0.5)`, normalized distance via `maxDist`, fog/perspective use `normDist`, glow block after fog, `center` and `glow` uniforms added.

Delete `shaders/slit_scan_corridor.fs` and `shaders/slit_scan_corridor_display.fs`.

**Verify**: Files exist with correct uniform declarations.

---

#### Task 2.3: Update effect_config.h

**Files**: `src/config/effect_config.h`
**Depends on**: Wave 1 complete

**Do**:
- Change `#include "effects/slit_scan_corridor.h"` → `#include "effects/slit_scan.h"`
- Change `TRANSFORM_SLIT_SCAN_CORRIDOR` → `TRANSFORM_SLIT_SCAN` in the enum
- Change `SlitScanCorridorConfig slitScanCorridor;` → `SlitScanConfig slitScan;` in the `EffectConfig` struct
- In `TransformOrderConfig::order` array, change `TRANSFORM_SLIT_SCAN_CORRIDOR` → `TRANSFORM_SLIT_SCAN`

**Verify**: Compiles after all Wave 2 tasks complete.

---

#### Task 2.4: Update post_effect.h

**Files**: `src/render/post_effect.h`
**Depends on**: Wave 1 complete

**Do**:
- Change `#include "effects/slit_scan_corridor.h"` → `#include "effects/slit_scan.h"`
- Change `SlitScanCorridorEffect slitScanCorridor;` → `SlitScanEffect slitScan;` in `PostEffect`

**Verify**: Compiles after all Wave 2 tasks complete.

---

#### Task 2.5: Update effect_serialization.cpp

**Files**: `src/config/effect_serialization.cpp`
**Depends on**: Wave 1 complete

**Do**:
- Change `#include "effects/slit_scan_corridor.h"` → `#include "effects/slit_scan.h"`
- Change `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(SlitScanCorridorConfig, SLIT_SCAN_CORRIDOR_CONFIG_FIELDS)` → `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(SlitScanConfig, SLIT_SCAN_CONFIG_FIELDS)`
- In the `EFFECT_CONFIG_FIELDS(X)` macro, change `X(slitScanCorridor)` → `X(slitScan)`

**Verify**: Compiles after all Wave 2 tasks complete.

---

#### Task 2.6: Update CMakeLists.txt

**Files**: `CMakeLists.txt`
**Depends on**: Wave 1 complete

**Do**: Change `src/effects/slit_scan_corridor.cpp` → `src/effects/slit_scan.cpp` in `EFFECTS_SOURCES`.

**Verify**: Compiles after all Wave 2 tasks complete.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] No references to `slitScanCorridor`, `SlitScanCorridor`, `SLIT_SCAN_CORRIDOR`, or `slit_scan_corridor` remain in source
- [ ] Old files deleted: `slit_scan_corridor.h`, `slit_scan_corridor.cpp`, `slit_scan_corridor.fs`, `slit_scan_corridor_display.fs`
- [ ] Mode combo toggles between Corridor and Flat in UI
- [ ] Glow slider appears and is modulatable
- [ ] Corridor mode (mode=0) produces identical output to the old effect
- [ ] Flat mode (mode=1) pushes from left edge rightward
- [ ] Glow=0 produces no change; glow=5 produces intense center brightness
