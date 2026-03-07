# Infinite Zoom v2

Extends the existing Infinite Zoom effect with per-layer UV warping (sine or noise), movable zoom center with lissajous motion, per-layer parallax offset with lissajous drift, and selectable layer blend modes. Addresses the "muddying" problem of weighted-average-only blending and adds organic motion and depth.

**Research**: `docs/research/infinite_zoom_v2.md`

## Design

### Types

**InfiniteZoomConfig** (extend existing struct):

```
// New fields appended after existing layerRotate:
int warpType = 0;             // 0=None, 1=Sine, 2=Noise
float warpStrength = 0.0f;    // Distortion amplitude (0.0-1.0)
float warpFreq = 3.0f;        // Spatial frequency of warp (1.0-10.0)
float warpSpeed = 0.5f;       // Warp animation rate (-2.0 to 2.0)
float centerX = 0.5f;         // Zoom focal point X (0.0-1.0)
float centerY = 0.5f;         // Zoom focal point Y (0.0-1.0)
DualLissajousConfig centerLissajous = {.amplitude = 0.0f};
float offsetX = 0.0f;         // Per-layer X drift (-0.1 to 0.1)
float offsetY = 0.0f;         // Per-layer Y drift (-0.1 to 0.1)
DualLissajousConfig offsetLissajous = {.amplitude = 0.0f};
int blendMode = 0;            // 0=Weighted Avg, 1=Additive, 2=Screen
```

**InfiniteZoomEffect** (extend existing struct):

```
// New uniform locations appended:
int centerLoc;
int offsetLoc;
int warpTypeLoc;
int warpStrengthLoc;
int warpFreqLoc;
int warpTimeLoc;
int blendModeLoc;

// New animation accumulator:
float warpTime;
```

**CONFIG_FIELDS macro** — append all new fields:
```
enabled, speed, zoomDepth, layers, spiralAngle, spiralTwist, layerRotate,
warpType, warpStrength, warpFreq, warpSpeed, centerX, centerY, centerLissajous,
offsetX, offsetY, offsetLissajous, blendMode
```

### Algorithm

#### Shader (infinite_zoom.fs)

**New uniforms:**
```glsl
uniform vec2 center;        // Replaces hardcoded vec2(0.5)
uniform vec2 offset;        // Parallax direction per layer
uniform int warpType;       // 0=None, 1=Sine, 2=Noise
uniform float warpStrength; // Distortion amplitude
uniform float warpFreq;     // Spatial frequency
uniform float warpTime;     // Animation phase
uniform int blendMode;      // 0=WeightedAvg, 1=Additive, 2=Screen
```

**Noise primitives** (add before main, copy from texture_warp.fs):
```glsl
float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

float noise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    float a = hash(i);
    float b = hash(i + vec2(1.0, 0.0));
    float c = hash(i + vec2(0.0, 1.0));
    float d = hash(i + vec2(1.0, 1.0));
    return mix(mix(a, b, f.x), mix(c, d, f.x), f.y);
}

float fbm(vec2 p) {
    float value = 0.0;
    float amplitude = 0.5;
    float frequency = 1.0;
    for (int i = 0; i < 4; i++) {
        value += amplitude * noise(p * frequency);
        frequency *= 2.0;
        amplitude *= 0.5;
    }
    return value;
}
```

**Layer loop changes:**

1. **Parallax center** — replace `vec2 center = vec2(0.5)` with the `center` uniform. Before the existing `uv = fragTexCoord - center` line, shift per layer:
```glsl
vec2 layerCenter = center + offset * float(i);
vec2 uv = fragTexCoord - layerCenter;
```

2. **UV warp** — after scale division (`uv = uv / scale`) and spiral twist, but before aspect correction back to texture space:
```glsl
if (warpType == 1) {
    // Sine warp, attenuated by depth
    uv.x += sin(uv.y * warpFreq + warpTime) * warpStrength / scale;
    uv.y += sin(uv.x * warpFreq * 1.3 + warpTime * 0.7) * warpStrength / scale;
} else if (warpType == 2) {
    // Noise warp via fBM, attenuated by depth
    vec2 warpOffset = vec2(
        fbm(uv * warpFreq + warpTime),
        fbm(uv * warpFreq + warpTime + vec2(5.2, 1.3))
    ) - 0.5;
    uv += warpOffset * warpStrength / scale;
}
```

The `/scale` attenuation is essential: deeper (zoomed-out) layers get less warp, preserving depth illusion.

3. **Blend mode accumulation** — replace the single accumulation line with a switch:
```glsl
if (blendMode == 0) {
    // Weighted Average (current behavior)
    colorAccum += sampleColor * weight;
    weightAccum += weight;
} else if (blendMode == 1) {
    // Additive
    colorAccum += sampleColor * weight;
} else {
    // Screen
    colorAccum = 1.0 - (1.0 - colorAccum) * (1.0 - sampleColor * weight);
}
```

4. **Finalization** — replace the normalize-and-output with:
```glsl
if (blendMode == 0) {
    finalColor = (weightAccum > 0.0)
        ? vec4(colorAccum / weightAccum, 1.0)
        : vec4(0.0, 0.0, 0.0, 1.0);
} else if (blendMode == 1) {
    finalColor = vec4(clamp(colorAccum, 0.0, 1.0), 1.0);
} else {
    finalColor = vec4(colorAccum, 1.0);
}
```

#### CPU changes

**Setup function** — extend `InfiniteZoomEffectSetup`:
- Accumulate `warpTime += cfg->warpSpeed * deltaTime`
- Compute center lissajous: `DualLissajousUpdate(&cfg->centerLissajous, deltaTime, 0.0f, &offsetX, &offsetY)` then `center = {cfg->centerX + offsetX, cfg->centerY + offsetY}`
- Compute offset lissajous: `DualLissajousUpdate(&cfg->offsetLissajous, deltaTime, 0.0f, &offsetX, &offsetY)` then `offset = {cfg->offsetX + offsetX, cfg->offsetY + offsetY}`
- Set all new uniforms as vec2/float/int

### Parameters

#### Existing (unchanged)

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| `speed` | float | -2.0 to 2.0 | 1.0 | No | Speed |
| `zoomDepth` | float | 1.0-5.0 | 3.0 | No | Zoom Depth |
| `layers` | int | 2-8 | 6 | No | Layers |
| `spiralAngle` | float | +-pi | 0.0 | Yes | Spiral Angle |
| `spiralTwist` | float | +-pi | 0.0 | Yes | Twist |
| `layerRotate` | float | +-pi | 0.0 | Yes | Layer Rotate |

#### New: Warp

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| `warpType` | int | 0-2 | 0 | No | Warp Type |
| `warpStrength` | float | 0.0-1.0 | 0.0 | Yes | Warp Strength |
| `warpFreq` | float | 1.0-10.0 | 3.0 | Yes | Warp Freq |
| `warpSpeed` | float | -2.0 to 2.0 | 0.5 | No | Warp Speed |

#### New: Center

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| `centerX` | float | 0.0-1.0 | 0.5 | Yes | Center X |
| `centerY` | float | 0.0-1.0 | 0.5 | Yes | Center Y |
| `centerLissajous` | DualLissajousConfig | — | amplitude=0.0 | Yes (amp, speed) | Center Lissajous |

#### New: Parallax

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| `offsetX` | float | -0.1 to 0.1 | 0.0 | Yes | Offset X |
| `offsetY` | float | -0.1 to 0.1 | 0.0 | Yes | Offset Y |
| `offsetLissajous` | DualLissajousConfig | — | amplitude=0.0 | Yes (amp, speed) | Offset Lissajous |

#### New: Blending

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| `blendMode` | int | 0-2 | 0 | No | Blend Mode |

### Constants

No new enum entries — this extends the existing `TRANSFORM_INFINITE_ZOOM` effect.

---

## Tasks

### Wave 1: Header

#### Task 1.1: Extend InfiniteZoomConfig and InfiniteZoomEffect

**Files**: `src/effects/infinite_zoom.h`
**Creates**: Updated config struct and effect struct that Wave 2 tasks depend on

**Do**:
- Add `#include "config/dual_lissajous_config.h"` after existing includes
- Add new config fields after `layerRotate` (see Design > Types for exact fields, types, defaults)
- Update `INFINITE_ZOOM_CONFIG_FIELDS` macro to include all new fields
- Add new uniform location `int` fields to `InfiniteZoomEffect`: `centerLoc`, `offsetLoc`, `warpTypeLoc`, `warpStrengthLoc`, `warpFreqLoc`, `warpTimeLoc`, `blendModeLoc`
- Add `float warpTime` accumulator to `InfiniteZoomEffect`
- Change `InfiniteZoomEffectSetup` declaration: `const InfiniteZoomConfig *cfg` → `InfiniteZoomConfig *cfg` (DualLissajousUpdate mutates phase)

**Verify**: `cmake.exe --build build` compiles.

---

### Wave 2: Parallel Implementation

#### Task 2.1: Extend shader with warp, center, parallax, blend modes

**Files**: `shaders/infinite_zoom.fs`
**Depends on**: Wave 1 complete

**Do**:
- Add new uniforms: `center` (vec2), `offset` (vec2), `warpType` (int), `warpStrength` (float), `warpFreq` (float), `warpTime` (float), `blendMode` (int)
- Add noise primitives (`hash`, `noise`, `fbm`) before `main()` — implement the Algorithm section
- Remove `vec2 center = vec2(0.5)` local variable (now a uniform)
- Replace `uv = fragTexCoord - center` with parallax: `vec2 layerCenter = center + offset * float(i)` then `uv = fragTexCoord - layerCenter`
- Add warp block after spiral twist / before aspect correction back — implement the Algorithm section's warp code
- Replace accumulation with blend mode switch — implement the Algorithm section's blend mode code
- Replace finalization with blend mode switch — implement the Algorithm section's finalization code

**Verify**: Shader parses (build succeeds, effect loads without error).

---

#### Task 2.2: Extend CPU setup, registration, and UI

**Files**: `src/effects/infinite_zoom.cpp`
**Depends on**: Wave 1 complete

**Do**:

*Init* — add `GetShaderLocation` calls for all 7 new uniform names: `"center"`, `"offset"`, `"warpType"`, `"warpStrength"`, `"warpFreq"`, `"warpTime"`, `"blendMode"`. Initialize `warpTime = 0.0f`.

*Setup* — extend `InfiniteZoomEffectSetup`:
1. Accumulate `e->warpTime += cfg->warpSpeed * deltaTime`
2. Compute center with lissajous (follow `wave_ripple.cpp` pattern): call `DualLissajousUpdate` on `cfg->centerLissajous`, add offsets to `cfg->centerX`/`cfg->centerY`, pass as vec2 uniform
3. Compute offset with lissajous: same pattern with `cfg->offsetLissajous`, add to `cfg->offsetX`/`cfg->offsetY`, pass as vec2 uniform
4. Set `warpType` (int), `warpStrength` (float), `warpFreq` (float), `warpTime` (float), `blendMode` (int) uniforms
- Note: `DualLissajousUpdate` mutates `phase` inside the lissajous config. The setup function signature must change from `const InfiniteZoomConfig*` to `InfiniteZoomConfig*` (matching `wave_ripple.cpp` pattern). Update both the declaration in the header and the definition in the `.cpp`.

*RegisterParams* — add registrations:
- `"infiniteZoom.warpStrength"` — 0.0f, 1.0f
- `"infiniteZoom.warpFreq"` — 1.0f, 10.0f
- `"infiniteZoom.centerX"` — 0.0f, 1.0f
- `"infiniteZoom.centerY"` — 0.0f, 1.0f
- `"infiniteZoom.offsetX"` — -0.1f, 0.1f
- `"infiniteZoom.offsetY"` — -0.1f, 0.1f
- Center lissajous: `"infiniteZoom.centerLissajous.amplitude"` and `"infiniteZoom.centerLissajous.motionSpeed"`
- Offset lissajous: `"infiniteZoom.offsetLissajous.amplitude"` and `"infiniteZoom.offsetLissajous.motionSpeed"`

*UI* — extend `DrawInfiniteZoomParams`, adding sections after existing sliders:
1. **Warp section** (`SeparatorText("Warp")`):
   - `ImGui::Combo("Warp Type##infzoom", &cfg->warpType, "None\0Sine\0Noise\0")`
   - `ModulatableSlider` for warpStrength (0.0-1.0, "%.2f")
   - `ModulatableSlider` for warpFreq (1.0-10.0, "%.1f")
   - `ImGui::SliderFloat` for warpSpeed (-2.0 to 2.0, "%.2f")
2. **Center section** (`SeparatorText("Center")`):
   - `ModulatableSlider` for centerX (0.0-1.0, "%.2f")
   - `ModulatableSlider` for centerY (0.0-1.0, "%.2f")
   - `DrawLissajousControls` for centerLissajous (idSuffix: `"infzoom_center"`, paramPrefix: `"infiniteZoom.centerLissajous"`)
3. **Parallax section** (`SeparatorText("Parallax")`):
   - `ModulatableSlider` for offsetX (-0.1 to 0.1, "%.3f")
   - `ModulatableSlider` for offsetY (-0.1 to 0.1, "%.3f")
   - `DrawLissajousControls` for offsetLissajous (idSuffix: `"infzoom_offset"`, paramPrefix: `"infiniteZoom.offsetLissajous"`)
4. **Blending section** (`SeparatorText("Blending")`):
   - `ImGui::Combo("Blend Mode##infzoom", &cfg->blendMode, "Weighted Avg\0Additive\0Screen\0")`

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 2.3: Verify serialization (no code changes)

**Files**: `src/config/effect_serialization.cpp` (read-only)
**Depends on**: Wave 1 complete

**Do**: Verify only — no edits needed. The existing `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(InfiniteZoomConfig, INFINITE_ZOOM_CONFIG_FIELDS)` already references the header's macro. Task 1.1 updates that macro, so serialization picks up new fields automatically. `DualLissajousConfig` already has `to_json`/`from_json` registered (line 244).

**Verify**: `cmake.exe --build build` compiles. Preset save/load round-trips new fields.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] Effect loads without shader errors
- [ ] Warp Type combo switches between None/Sine/Noise visually
- [ ] Center X/Y sliders move the zoom focal point
- [ ] Center lissajous creates orbital zoom center motion
- [ ] Offset X/Y creates parallax layer separation
- [ ] Blend mode combo switches between Weighted Avg/Additive/Screen
- [ ] Preset save/load preserves all new settings
- [ ] Modulation routes to registered parameters
