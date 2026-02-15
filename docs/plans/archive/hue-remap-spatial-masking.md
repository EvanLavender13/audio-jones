# Hue Remap Spatial Masking

Expand Hue Remap's spatial control from a single radial coefficient to five independent masking modes — radial, angular, linear, luminance, and noise — each applied independently to both blend intensity and hue shift. When all spatial coefficients are zero the effect applies uniformly (current behavior). Active modes introduce per-pixel spatial variation via additive field composition.

**Research**: `docs/research/hue-remap-spatial-masking.md`

## Design

### Types

**HueRemapConfig** (expanded — `src/effects/hue_remap.h`):

```
existing fields unchanged:
  enabled, gradient, shift, intensity, cx, cy

renamed field:
  radial → blendRadial  (same semantics, same range -1.0 to 1.0)

new blend spatial coefficients:
  float blendAngular = 0.0f;      // -1.0 to 1.0
  int   blendAngularFreq = 2;     // 1-8
  float blendLinear = 0.0f;       // -1.0 to 1.0
  float blendLinearAngle = 0.0f;  // radians, displayed as degrees
  float blendLuminance = 0.0f;    // -1.0 to 1.0
  float blendNoise = 0.0f;        // -1.0 to 1.0

new shift spatial coefficients:
  float shiftRadial = 0.0f;       // -1.0 to 1.0
  float shiftAngular = 0.0f;      // -1.0 to 1.0
  int   shiftAngularFreq = 2;     // 1-8
  float shiftLinear = 0.0f;       // -1.0 to 1.0
  float shiftLinearAngle = 0.0f;  // radians, displayed as degrees
  float shiftLuminance = 0.0f;    // -1.0 to 1.0
  float shiftNoise = 0.0f;        // -1.0 to 1.0

new shared noise params:
  float noiseScale = 5.0f;        // 1.0-20.0
  float noiseSpeed = 0.5f;        // 0.0-2.0
```

**HueRemapEffect** (expanded — `src/effects/hue_remap.h`):

```
add to existing struct:
  float time;   // accumulated noise drift time

rename existing loc:
  radialLoc → blendRadialLoc

new uniform locations (int):
  blendAngularLoc, blendAngularFreqLoc, blendLinearLoc, blendLinearAngleLoc,
  blendLuminanceLoc, blendNoiseLoc,
  shiftRadialLoc, shiftAngularLoc, shiftAngularFreqLoc, shiftLinearLoc,
  shiftLinearAngleLoc, shiftLuminanceLoc, shiftNoiseLoc,
  noiseScaleLoc, noiseSpeedLoc, timeLoc
```

**HUE_REMAP_CONFIG_FIELDS** macro: Update to list all fields for preset serialization. Rename `radial` → `blendRadial`.

### Preset Compatibility

The `radial` field is renamed to `blendRadial`. No presets currently use this field, so no backward-compat alias needed — a clean rename.

### Algorithm

#### Spatial Coordinate Setup (computed once per pixel)

```glsl
// Aspect-corrected polar coords relative to center (same as current radial code)
vec2 uv = fragTexCoord - center;
float aspect = resolution.x / resolution.y;
if (aspect > 1.0) { uv.x /= aspect; } else { uv.y *= aspect; }
float rad = length(uv) * 2.0;
float ang = atan(uv.y, uv.x);

// Content-aware
float luma = dot(texture(texture0, fragTexCoord).rgb, vec3(0.299, 0.587, 0.114));

// Procedural noise
float n = noise2D(fragTexCoord * noiseScale + time * noiseSpeed);
```

#### Noise Function (inline in shader, ~15 lines)

```glsl
// 2D hash-based smooth noise (no grid artifacts)
vec2 hash22(vec2 p) {
    p = vec2(dot(p, vec2(127.1, 311.7)), dot(p, vec2(269.5, 183.3)));
    return -1.0 + 2.0 * fract(sin(p) * 43758.5453123);
}

float noise2D(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    vec2 u = f * f * (3.0 - 2.0 * f);  // smoothstep interpolant

    return mix(mix(dot(hash22(i + vec2(0.0, 0.0)), f - vec2(0.0, 0.0)),
                   dot(hash22(i + vec2(1.0, 0.0)), f - vec2(1.0, 0.0)), u.x),
               mix(dot(hash22(i + vec2(0.0, 1.0)), f - vec2(0.0, 1.0)),
                   dot(hash22(i + vec2(1.0, 1.0)), f - vec2(1.0, 1.0)), u.x), u.y);
}
```

This is gradient noise (Perlin-style) using a hash function. Output range is approximately -1 to 1.

#### Spatial Field Computation

Each target (blend and shift) computes its own spatial field from 5 modes. The field composition must preserve the "all zeros = uniform (1.0)" invariant.

```glsl
// Per-mode contribution: coefficient * directional_value
// Positive coefficient = normal, negative = inverted

float computeSpatialField(float radialCoeff, float angularCoeff, int angularFreq,
                          float linearCoeff, float linearAngle,
                          float luminanceCoeff, float noiseCoeff,
                          float rad, float ang, float luma, float n,
                          vec2 fragUV, vec2 center) {
    // Start at 1.0 (uniform). Each active mode modulates around 1.0.
    // Same approach as current: mix(1.0, field, abs(coeff))
    // with field = raw_value when coeff > 0, 1-raw_value when coeff < 0

    float field = 1.0;

    // Radial: rad is 0 at center, ~1 at edge
    if (radialCoeff != 0.0) {
        float rv = (radialCoeff > 0.0) ? rad : 1.0 - rad;
        field *= mix(1.0, rv, abs(radialCoeff));
    }

    // Angular: sine wave around polar angle
    if (angularCoeff != 0.0) {
        float av = sin(ang * float(angularFreq)) * 0.5 + 0.5; // 0-1
        if (angularCoeff < 0.0) av = 1.0 - av;
        field *= mix(1.0, av, abs(angularCoeff));
    }

    // Linear: directional gradient
    if (linearCoeff != 0.0) {
        vec2 dir = vec2(cos(linearAngle), sin(linearAngle));
        float lv = dot(fragUV - center, dir) + 0.5; // shift to ~0-1 range
        lv = clamp(lv, 0.0, 1.0);
        if (linearCoeff < 0.0) lv = 1.0 - lv;
        field *= mix(1.0, lv, abs(linearCoeff));
    }

    // Luminance: content-aware
    if (luminanceCoeff != 0.0) {
        float lumv = luma;
        if (luminanceCoeff < 0.0) lumv = 1.0 - lumv;
        field *= mix(1.0, lumv, abs(luminanceCoeff));
    }

    // Noise: procedural organic variation
    if (noiseCoeff != 0.0) {
        float nv = n * 0.5 + 0.5; // remap -1..1 to 0..1
        if (noiseCoeff < 0.0) nv = 1.0 - nv;
        field *= mix(1.0, nv, abs(noiseCoeff));
    }

    return field;
}
```

#### Shift Field (additive composition)

Shift uses additive composition (not multiplicative like blend) — hue shift is a rotation offset, so spatial modes contribute signed additive values that wrap via `fract`. Zero coefficients = zero offset = no spatial shift.

```glsl
float computeShiftField(float radialCoeff, float angularCoeff, int angularFreq,
                         float linearCoeff, float linearAngle,
                         float luminanceCoeff, float noiseCoeff,
                         float rad, float ang, float luma, float n,
                         vec2 fragUV, vec2 center) {
    float field = 0.0;

    field += radialCoeff * rad;
    field += angularCoeff * (sin(ang * float(angularFreq)) * 0.5 + 0.5);

    vec2 dir = vec2(cos(linearAngle), sin(linearAngle));
    float lv = clamp(dot(fragUV - center, dir) + 0.5, 0.0, 1.0);
    field += linearCoeff * lv;

    field += luminanceCoeff * luma;
    field += noiseCoeff * (n * 0.5 + 0.5);

    return field;
}
```

#### Main Shader Logic (replaces current blend computation)

```glsl
// Compute shared spatial coords
vec2 uv = fragTexCoord - center;
float aspect = resolution.x / resolution.y;
if (aspect > 1.0) { uv.x /= aspect; } else { uv.y *= aspect; }
float rad = length(uv) * 2.0;
float ang = atan(uv.y, uv.x);
float luma = dot(color.rgb, vec3(0.299, 0.587, 0.114));
float n = noise2D(fragTexCoord * noiseScale + time * noiseSpeed);

// Blend spatial field (multiplicative)
float blendField = computeSpatialField(
    blendRadial, blendAngular, blendAngularFreq,
    blendLinear, blendLinearAngle,
    blendLuminance, blendNoise,
    rad, ang, luma, n, fragTexCoord, center);
float blend = clamp(intensity * blendField, 0.0, 1.0);

// Shift spatial field (additive)
float shiftField = computeShiftField(
    shiftRadial, shiftAngular, shiftAngularFreq,
    shiftLinear, shiftLinearAngle,
    shiftLuminance, shiftNoise,
    rad, ang, luma, n, fragTexCoord, center);

float t = fract(hsv.x + shift + shiftField);
```

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| `enabled` | bool | — | false | no | Enabled |
| `gradient` | ColorConfig | — | rainbow | no | (color mode widget) |
| `shift` | float | 0.0-1.0 | 0.0 | yes | Shift |
| `intensity` | float | 0.0-1.0 | 1.0 | yes | Intensity |
| `cx` | float | 0.0-1.0 | 0.5 | yes | Center X |
| `cy` | float | 0.0-1.0 | 0.5 | yes | Center Y |
| `blendRadial` | float | -1.0 to 1.0 | 0.0 | yes | Radial |
| `blendAngular` | float | -1.0 to 1.0 | 0.0 | yes | Angular |
| `blendAngularFreq` | int | 1-8 | 2 | no | Angular Freq |
| `blendLinear` | float | -1.0 to 1.0 | 0.0 | yes | Linear |
| `blendLinearAngle` | float | 0-2π | 0.0 | yes | Linear Angle |
| `blendLuminance` | float | -1.0 to 1.0 | 0.0 | yes | Luminance |
| `blendNoise` | float | -1.0 to 1.0 | 0.0 | yes | Noise |
| `shiftRadial` | float | -1.0 to 1.0 | 0.0 | yes | Radial |
| `shiftAngular` | float | -1.0 to 1.0 | 0.0 | yes | Angular |
| `shiftAngularFreq` | int | 1-8 | 2 | no | Angular Freq |
| `shiftLinear` | float | -1.0 to 1.0 | 0.0 | yes | Linear |
| `shiftLinearAngle` | float | 0-2π | 0.0 | yes | Linear Angle |
| `shiftLuminance` | float | -1.0 to 1.0 | 0.0 | yes | Luminance |
| `shiftNoise` | float | -1.0 to 1.0 | 0.0 | yes | Noise |
| `noiseScale` | float | 1.0-20.0 | 5.0 | yes | Scale |
| `noiseSpeed` | float | 0.0-2.0 | 0.5 | yes | Speed |

### UI Layout

Following flow field pattern with `SeparatorText` sub-headers:

```
[Hue Remap] section
  Enabled checkbox
  (color mode widget)
  ── Core ──
  Shift (displayed as degrees × 360)
  Intensity
  Center X, Center Y
  ── Blend Spatial ──
  Radial
  Angular, Angular Freq (int)
  Linear, Linear Angle (degrees)
  Luminance
  Noise
  ── Shift Spatial ──
  Radial, Angular, Angular Freq (int)
  Linear, Linear Angle (degrees)
  Luminance
  Noise
  ── Noise Field ──
  Scale, Speed
```

### Constants

No new enums or descriptor entries — this is an expansion of the existing `TRANSFORM_HUE_REMAP` effect.

---

## Tasks

### Wave 1: Config + Shader

#### Task 1.1: Expand HueRemapConfig and HueRemapEffect

**Files**: `src/effects/hue_remap.h`
**Creates**: Expanded config struct and effect struct that all other tasks depend on

**Do**:
1. Rename existing `radial` → `blendRadial` in `HueRemapConfig`. Add all new config fields per the Parameters table above. Int fields: `blendAngularFreq`, `shiftAngularFreq`.
2. Add `float time = 0.0f;` to `HueRemapEffect` struct.
3. Rename existing `radialLoc` → `blendRadialLoc`. Add uniform location `int` fields to `HueRemapEffect` for all new uniforms: `blendAngularLoc`, `blendAngularFreqLoc`, `blendLinearLoc`, `blendLinearAngleLoc`, `blendLuminanceLoc`, `blendNoiseLoc`, `shiftRadialLoc`, `shiftAngularLoc`, `shiftAngularFreqLoc`, `shiftLinearLoc`, `shiftLinearAngleLoc`, `shiftLuminanceLoc`, `shiftNoiseLoc`, `noiseScaleLoc`, `noiseSpeedLoc`, `timeLoc`.
4. Update `HUE_REMAP_CONFIG_FIELDS` macro to include all new fields.
5. Change `HueRemapEffectSetup` signature to add `float deltaTime` parameter.

**Verify**: `cmake.exe --build build` compiles (will have linker errors from changed signature, that's expected).

#### Task 1.2: Rewrite hue_remap.fs shader

**Files**: `shaders/hue_remap.fs`
**Creates**: Complete shader with spatial masking

**Do**:
1. Keep existing `#version 330`, `rgb2hsv`, `hsv2rgb` functions unchanged.
2. Add new uniforms for all spatial parameters (matching the uniform names from Task 1.1 loc fields, minus the "Loc" suffix).
3. Add `uniform float time;` for noise drift.
4. Implement the `hash22` and `noise2D` functions from the Algorithm section above.
5. Replace the current `main()` radial blend code with the full spatial field computation from the Algorithm section:
   - Compute shared spatial coords (aspect-corrected polar, luma, noise) once
   - Compute blend field using multiplicative composition (`computeSpatialField`)
   - Compute shift field using additive composition (`computeShiftField`)
   - Apply: `float t = fract(hsv.x + shift + shiftField);` then `blend = clamp(intensity * blendField, 0.0, 1.0);`
6. The `computeSpatialField` and `computeShiftField` can be inline in main or separate functions — agent's choice for readability.
7. Use `blendRadial` as the uniform name (replaces old `radial`).

**Verify**: Shader compiles (checked at runtime, but ensure no syntax errors by inspection).

---

### Wave 2: C++ Integration + UI

#### Task 2.1: Expand hue_remap.cpp

**Files**: `src/effects/hue_remap.cpp`
**Depends on**: Wave 1 complete

**Do**:
1. In `HueRemapEffectInit`: rename existing `radial` → `blendRadial` in `GetShaderLocation` call and loc field. Add `GetShaderLocation` calls for all new uniforms. Cache to the new loc fields. Initialize `e->time = 0.0f;`.
2. In `HueRemapEffectSetup`: add `float deltaTime` parameter. Add `e->time += cfg->noiseSpeed * deltaTime;`. Add `SetShaderValue` calls for all new uniforms. Int params (`blendAngularFreq`, `shiftAngularFreq`) use `SHADER_UNIFORM_INT`.
3. In `HueRemapRegisterParams`: rename existing `"hueRemap.radial"` → `"hueRemap.blendRadial"`. Add `ModEngineRegisterParam` calls for all new float params with ranges from the Parameters table. Int params are NOT registered (not modulatable — same as flow field angular freq convention). Angular params (`blendLinearAngle`, `shiftLinearAngle`) use range `0.0f, 6.28318f`.
4. Update `SetupHueRemap` to pass `pe->currentDeltaTime` to `HueRemapEffectSetup`.
5. Update the header declaration in `hue_remap.h` for the new `HueRemapEffectSetup` signature.

**Verify**: `cmake.exe --build build` compiles with no errors.

#### Task 2.2: Expand Hue Remap UI

**Files**: `src/ui/imgui_effects_color.cpp`
**Depends on**: Wave 1 complete

**Do**:
1. Expand `DrawColorHueRemap` following the UI Layout in the Design section.
2. Use `SeparatorText` sub-headers: "Core", "Blend Spatial", "Shift Spatial", "Noise Field".
3. Follow the flow field UI pattern from `imgui_effects.cpp` lines 65-131 as reference.
4. Slider conventions:
   - Float spatial coefficients (-1 to 1): `ModulatableSlider` with `"%.2f"` format
   - Int freq (1-8): `ImGui::SliderInt` (same as flow field angular freq)
   - Angular params: `ModulatableSliderAngleDeg` (from `ui_units.h`)
   - `noiseScale`: `ModulatableSlider` with `"%.1f"` format
   - `noiseSpeed`: `ModulatableSlider` with `"%.2f"` format
   - Existing `shift` slider stays as-is (displayed as degrees × 360)
5. All slider IDs use `##hueremap` suffix for uniqueness.
6. Param registry keys follow `hueRemap.<fieldName>` pattern.

**Verify**: `cmake.exe --build build` compiles with no errors.

---

## Final Verification

- [ ] Build succeeds: `cmake.exe -G Ninja -B build -S . -DCMAKE_BUILD_TYPE=Release && cmake.exe --build build`
- [ ] All spatial coefficients at zero → identical to current behavior (uniform blend at full intensity)
- [ ] `blendRadial` slider works identically to the old `radial` (no behavioral regression)
- [ ] Angular freq sliders are integers 1-8
- [ ] Linear angle displays in degrees
- [ ] Noise drifts smoothly over time when noiseSpeed > 0
