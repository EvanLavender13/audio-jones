# Iris Rings

Concentric colored rings where each ring maps to a frequency band and shows a partial arc proportional to that band's energy. Quiet frequencies shrink to thin slivers, loud ones bloom into full circles. A twist parameter fans successive rings into a pinwheel spiral, and a snap slider transitions the arc gating from smooth proportional opening to hard on/off threshold behavior. Perspective tilt (from Spectral Arcs) allows tilting the disc into 3D.

**Research**: `docs/research/iris-rings.md`

## Design

### Types

**IrisRingsConfig** (in `src/effects/iris_rings.h`):

```
enabled         bool     false
layers          int      24        // Number of concentric rings (4-96)
ringSpacing     float    0.05      // Distance between rings (0.01-0.15)
thickness       float    0.005     // Ring line width (0.001-0.02)
twist           float    0.3       // Per-ring cumulative rotation (-PI to PI)
rotationSpeed   float    0.2       // Global rotation rate rad/s (-PI to PI)
snap            float    0.0       // Proportional arc (0) vs threshold on/off (1) (0-1)
snapThreshold   float    0.5       // Energy cutoff when snap > 0 (0-1)
glowIntensity   float    1.0       // Ring glow brightness (0.1-3.0)
tilt            float    0.0       // Perspective tilt amount (0-3)
tiltAngle       float    0.0       // Tilt direction in radians (-PI to PI)
baseFreq        float    55.0      // Lowest mapped frequency Hz (27.5-440)
maxFreq         float    14000.0   // Highest mapped frequency Hz (1000-16000)
gain            float    2.0       // FFT amplitude multiplier (0.1-10)
curve           float    1.0       // FFT contrast exponent (0.1-3.0)
baseBright      float    0.05      // Minimum ring brightness (0-1)
gradient        ColorConfig  {.mode = COLOR_MODE_GRADIENT}
blendMode       EffectBlendMode  EFFECT_BLEND_SCREEN
blendIntensity  float    1.0       // Blend strength (0-5)
```

**IrisRingsEffect** (runtime state):

```
shader           Shader
gradientLUT      ColorLUT*
rotationAccum    float        // CPU-accumulated rotation angle
[uniform locs]   int          // One per shader uniform
```

### Algorithm

Shader loops over `layers` rings. Each ring gets a per-ring twist rotation, computes an SDF ring distance, gates by angular FFT energy, and accumulates glow.

**Tilt projection** (identical to Spectral Arcs):

```glsl
vec2 centered = fc - r * 0.5;
float ca = cos(tiltAngle), sa = sin(tiltAngle);
vec2 rotated = vec2(centered.x * ca - centered.y * sa,
                    centered.x * sa + centered.y * ca);
mat2 tiltMat = mat2(1.0, -tilt, 2.0 * tilt, 1.0 + tilt);
vec2 p = rotated * tiltMat;
float depth = r.y + r.y - p.y * tilt;
vec2 uv = p / depth;
```

**Ring loop core** (per ring `i` of `layers`):

```glsl
// Per-ring twist rotation
float angle = rotationAccum + twist * float(i) / float(layers);
float c = cos(angle), s = sin(angle);
vec2 ruv = vec2(uv.x * c - uv.y * s, uv.x * s + uv.y * c);

// Ring SDF
float radius = ringSpacing * float(i + 1);
float dist = abs(length(ruv) - radius) - thickness;

// Normalized angle for arc gating
float a = (atan(ruv.y, ruv.x) + PI) / TWO_PI;
```

**FFT energy lookup** (same multi-sample band pattern as Spectral Arcs):

```glsl
float t0 = float(i) / float(layers);
float t1 = float(i + 1) / float(layers);
float freqLo = baseFreq * pow(maxFreq / baseFreq, t0);
float freqHi = baseFreq * pow(maxFreq / baseFreq, t1);
float binLo = freqLo / (sampleRate * 0.5);
float binHi = freqHi / (sampleRate * 0.5);

float mag = 0.0;
const int BAND_SAMPLES = 4;
for (int s = 0; s < BAND_SAMPLES; s++) {
    float bin = mix(binLo, binHi, (float(s) + 0.5) / float(BAND_SAMPLES));
    if (bin <= 1.0) mag += texture(fftTexture, vec2(bin, 0.5)).r;
}
mag = pow(clamp(mag / float(BAND_SAMPLES) * gain, 0.0, 1.0), curve);
```

**Arc gating with snap blend:**

```glsl
float proportionalGate = step(a, mag);          // arc opens from 0 to energy fraction
float snapGate = step(snapThreshold, mag);       // ring fully on or fully off
float gate = mix(proportionalGate, snapGate, snap);
```

**Glow and accumulation:**

```glsl
float glow = smoothstep(thickness, 0.0, dist) * glowIntensity / max(abs(dist), 0.001);
vec3 color = texture(gradientLUT, vec2(float(i) / float(layers), 0.5)).rgb;
result += glow * gate * color * (baseBright + mag);
```

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label | Section |
|-----------|------|-------|---------|-------------|----------|---------|
| layers | int | 4-96 | 24 | No | Layers | Geometry |
| ringSpacing | float | 0.01-0.15 | 0.05 | Yes | Ring Spacing | Geometry |
| thickness | float | 0.001-0.02 | 0.005 | Yes | Thickness | Geometry |
| twist | float | -PI..PI | 0.3 | Yes | Twist | Geometry |
| tilt | float | 0-3 | 0.0 | Yes | Tilt | Tilt |
| tiltAngle | float | -PI..PI | 0.0 | Yes | Tilt Angle | Tilt |
| rotationSpeed | float | -PI..PI | 0.2 | Yes | Rotation Speed | Animation |
| snap | float | 0-1 | 0.0 | Yes | Snap | Arc Gating |
| snapThreshold | float | 0-1 | 0.5 | Yes | Snap Threshold | Arc Gating |
| glowIntensity | float | 0.1-3.0 | 1.0 | Yes | Glow Intensity | Glow |
| baseFreq | float | 27.5-440 | 55.0 | Yes | Base Freq (Hz) | Audio |
| maxFreq | float | 1000-16000 | 14000.0 | Yes | Max Freq (Hz) | Audio |
| gain | float | 0.1-10 | 2.0 | Yes | Gain | Audio |
| curve | float | 0.1-3.0 | 1.0 | Yes | Contrast | Audio |
| baseBright | float | 0-1 | 0.05 | Yes | Base Bright | Audio |
| blendIntensity | float | 0-5 | 1.0 | Yes | Blend Intensity | Output |
| blendMode | enum | - | SCREEN | No | Blend Mode | Output |

### Constants

- Enum: `TRANSFORM_IRIS_RINGS_BLEND`
- Display name: `"Iris Rings Blend"`
- Macro: `REGISTER_GENERATOR`
- Field name on EffectConfig / PostEffect: `irisRings`

---

## Tasks

### Wave 1: Config Header

#### Task 1.1: Create IrisRingsConfig and IrisRingsEffect

**Files**: `src/effects/iris_rings.h` (create)
**Creates**: Config struct and Effect struct that all other files include

**Do**: Follow `spectral_arcs.h` as pattern. Define `IrisRingsConfig` with all fields from the Parameters table (include `ColorConfig gradient` and `EffectBlendMode blendMode` / `float blendIntensity`). Define `IRIS_RINGS_CONFIG_FIELDS` macro. Define `IrisRingsEffect` with shader, `ColorLUT*`, `rotationAccum`, and one `int` location per uniform. Declare the standard 5 functions: Init (takes `IrisRingsEffect*` + `const IrisRingsConfig*`), Setup (takes effect, config, deltaTime, fftTexture), Uninit, ConfigDefault, RegisterParams. Include `"render/color_config.h"`, `"render/blend_mode.h"`, `"raylib.h"`, `<stdbool.h>`.

**Verify**: Header compiles when included.

---

### Wave 2: Parallel Implementation

#### Task 2.1: Effect Module

**Files**: `src/effects/iris_rings.cpp` (create)
**Depends on**: Wave 1

**Do**: Follow `spectral_arcs.cpp` as pattern. Init loads `shaders/iris_rings.fs`, caches all uniform locations, creates ColorLUT. Setup accumulates `rotationAccum += rotationSpeed * deltaTime`, updates LUT, binds all uniforms. Uninit unloads shader and LUT. RegisterParams registers all modulatable fields from the Parameters table. The `twist` and `tiltAngle` fields use `ROTATION_OFFSET_MAX` bounds; `rotationSpeed` uses `ROTATION_SPEED_MAX`. Include `"config/constants.h"` for those. Add the `SetupIrisRings` bridge + `SetupIrisRingsBlend` blend bridge + `REGISTER_GENERATOR` macro at bottom, same structure as spectral_arcs.cpp.

**Verify**: Compiles.

---

#### Task 2.2: Fragment Shader

**Files**: `shaders/iris_rings.fs` (create)
**Depends on**: Wave 1

**Do**: Implement the Algorithm section from this plan. Uniforms: `resolution`, `fftTexture`, `gradientLUT`, `sampleRate`, `layers` (int), `ringSpacing`, `thickness`, `twist`, `rotationAccum`, `snap`, `snapThreshold`, `glowIntensity`, `tilt`, `tiltAngle`, `baseFreq`, `maxFreq`, `gain`, `curve`, `baseBright`. Use `#version 330`. Define `PI` and `TWO_PI` constants. Apply tilt projection, then ring loop with per-ring twist rotation, SDF, FFT lookup, arc gating with snap blend, glow accumulation.

**Verify**: No build step needed (runtime-compiled).

---

#### Task 2.3: Config Registration

**Files**: `src/config/effect_config.h` (modify)
**Depends on**: Wave 1

**Do**:
1. Add `#include "effects/iris_rings.h"` in alphabetical order with other effect includes
2. Add `TRANSFORM_IRIS_RINGS_BLEND,` before `TRANSFORM_EFFECT_COUNT` in the enum
3. Add `IrisRingsConfig irisRings;` to the `EffectConfig` struct with comment

**Verify**: Compiles.

---

#### Task 2.4: PostEffect Struct

**Files**: `src/render/post_effect.h` (modify)
**Depends on**: Wave 1

**Do**: Add `#include "effects/iris_rings.h"` and `IrisRingsEffect irisRings;` member to the PostEffect struct. Follow placement pattern of other generator effects.

**Verify**: Compiles.

---

#### Task 2.5: Build System

**Files**: `CMakeLists.txt` (modify)
**Depends on**: Wave 1

**Do**: Add `src/effects/iris_rings.cpp` to `EFFECTS_SOURCES` in alphabetical order.

**Verify**: CMake configures.

---

#### Task 2.6: UI Panel

**Files**: `src/ui/imgui_effects_gen_geometric.cpp` (modify)
**Depends on**: Wave 1

**Do**: Follow the Spectral Arcs UI pattern in this file.
1. Add `#include "effects/iris_rings.h"` with other effect includes
2. Add `static bool sectionIrisRings = false;`
3. Add `DrawIrisRingsParams()` helper with sections: Audio (standard 5 sliders), Geometry (layers as SliderInt, ringSpacing, thickness as ModulatableSlider, twist as ModulatableSliderAngleDeg), Arc Gating (snap, snapThreshold as ModulatableSlider), Tilt (tilt as ModulatableSlider, tiltAngle as ModulatableSliderAngleDeg), Animation (rotationSpeed as ModulatableSliderSpeedDeg), Glow (glowIntensity as ModulatableSlider)
4. Add `DrawIrisRingsOutput()` with ColorMode + Output section (blendIntensity + blendMode)
5. Add `DrawGeneratorsIrisRings()` orchestrator with enabled checkbox + MoveTransformToEnd
6. Add call in `DrawGeneratorsGeometric()` after Spectral Arcs with `ImGui::Spacing()`

Use `##irisrings` as ImGui ID suffix. Use `"irisRings."` prefix for modulation IDs.

**Verify**: Compiles.

---

#### Task 2.7: Preset Serialization

**Files**: `src/config/effect_serialization.cpp` (modify)
**Depends on**: Wave 1

**Do**:
1. Add `#include "effects/iris_rings.h"`
2. Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(IrisRingsConfig, IRIS_RINGS_CONFIG_FIELDS)`
3. Add `X(irisRings) \` to the `EFFECT_CONFIG_FIELDS(X)` X-macro table

**Verify**: Compiles.

---

## Implementation Notes

Changes from original plan made during implementation:

**Removed parameters:**
- `twist` — replaced by built-in per-ring scramble via `fi * fi` (same technique as spectral arcs). Each ring gets a chaotic fixed angular offset and `sin(fi * fi)` gives each ring a different rotation speed/direction. No user control needed.
- `snap` / `snapThreshold` — threshold gating just produced full concentric circles, not useful.
- `thickness` — removed in favor of auto-scaling ring width tied to layer count (ring-normalized distance space). Rings stay proportional regardless of layer count.
- `glowIntensity` / `glowFalloff` — replaced by a flat `smoothstep(0.5, 0.3, abs(dist))` for clean solid-colored rings matching the reference aesthetic. No inverse-distance glow.

**Changed parameters:**
- `ringSpacing` renamed to `ringScale` (default 0.3, range 0.05-0.8) — controls total radius of outermost ring, not per-ring gap.

**Algorithm changes:**
- Ring radius normalized by layers: `radius = ringScale * fi / fl` — more layers = denser rings, not bigger.
- Per-ring rotation: `rotationAccum * sin(fi * fi) + fi * fi` — differential rotation where each ring spins at a different rate, replacing the linear twist.
- Arc gating capped at half circle: `step(a, mag * mag * 0.5)` — arcs never close into full circles. Squaring `mag` makes quiet bands tiny slivers and only loud bands get long arcs.
- Ring distance in normalized space: `dist = length(ruv) * fl / ringScale - fi` — adjacent rings always 1.0 apart regardless of layer count.

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] Effect appears in Generators > Geometric UI panel
- [ ] Enabling shows correct GEN badge in transform pipeline
- [ ] Rings visible with default settings (no audio needed due to baseBright)
- [ ] Audio input drives arc opening per ring
- [ ] Per-ring scramble creates organic scattered arc positions
- [ ] Tilt tilts the disc into perspective
- [ ] Preset save/load round-trips all settings
- [ ] Modulation routes work for registered parameters
