# Hue Remap

Replaces the standard HSV hue wheel with a custom color wheel defined by ColorConfig. A "shift" slider rotates through the custom palette exactly like Color Grade's hue shift rotates through the rainbow. Radial blending controls how much the remap applies based on distance from center. Follows the False Color pattern (ColorConfig + LUT).

**Research**: `docs/research/spatial-color-shift.md`

## Design

### Types

**`HueRemapConfig`** (in `src/effects/hue_remap.h`):
```
bool enabled = false
ColorConfig gradient = { .mode = COLOR_MODE_RAINBOW }  // Custom color wheel
float shift = 0.0f      // 0.0-1.0, rotates through palette
float intensity = 1.0f   // 0.0-1.0, global blend strength
float radial = 0.0f      // -1.0 to 1.0, blend varies with distance from center
float cx = 0.5f          // 0.0-1.0, center X
float cy = 0.5f          // 0.0-1.0, center Y
```

**`HueRemapEffect`** (in `src/effects/hue_remap.h`):
```
Shader shader
int shiftLoc
int intensityLoc
int radialLoc
int centerLoc
int resolutionLoc
int gradientLUTLoc
ColorLUT *gradientLUT
```

### Algorithm

Shader (`shaders/hue_remap.fs`):

```glsl
#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;   // Input image
uniform sampler2D texture1;   // 1D gradient LUT (256x1)
uniform vec2 resolution;
uniform float shift;          // Palette rotation offset
uniform float intensity;      // Global blend strength
uniform float radial;         // Radial blend coefficient
uniform vec2 center;          // Radial center (0-1)

// RGB to HSV (same as color_grade.fs)
vec3 rgb2hsv(vec3 c) {
    vec4 K = vec4(0.0, -1.0/3.0, 2.0/3.0, -1.0);
    vec4 p = mix(vec4(c.bg, K.wz), vec4(c.gb, K.xy), step(c.b, c.g));
    vec4 q = mix(vec4(p.xyw, c.r), vec4(c.r, p.yzx), step(p.x, c.r));
    float d = q.x - min(q.w, q.y);
    float e = 1.0e-10;
    return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}

vec3 hsv2rgb(vec3 c) {
    vec4 K = vec4(1.0, 2.0/3.0, 1.0/3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

void main() {
    vec4 color = texture(texture0, fragTexCoord);
    vec3 hsv = rgb2hsv(color.rgb);

    // Sample custom color wheel using pixel hue + shift offset
    float t = fract(hsv.x + shift);
    vec3 remappedRGB = texture(texture1, vec2(t, 0.5)).rgb;
    vec3 remappedHSV = rgb2hsv(remappedRGB);

    // Keep original saturation and value, take remapped hue
    vec3 result = hsv2rgb(vec3(remappedHSV.x, hsv.y, hsv.z));

    // Compute radial blend field
    vec2 uv = fragTexCoord - center;
    float aspect = resolution.x / resolution.y;
    if (aspect > 1.0) { uv.x /= aspect; } else { uv.y *= aspect; }
    float rad = length(uv) * 2.0;
    float blend = clamp(intensity + radial * rad, 0.0, 1.0);

    finalColor = vec4(mix(color.rgb, result, blend), color.a);
}
```

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| gradient | ColorConfig | — | Rainbow | No | (color mode widget) |
| shift | float | 0.0-1.0 | 0.0 | Yes | "Shift" with 360.0 scale |
| intensity | float | 0.0-1.0 | 1.0 | Yes | "Intensity" |
| radial | float | -1.0-1.0 | 0.0 | Yes | "Radial" |
| cx | float | 0.0-1.0 | 0.5 | Yes | "Center X" |
| cy | float | 0.0-1.0 | 0.5 | Yes | "Center Y" |

### Constants

- Enum name: `TRANSFORM_HUE_REMAP`
- Display name: `"Hue Remap"`
- Category badge: `"COL"`, section index `8`
- Descriptor flags: `EFFECT_FLAG_NONE`

---

## Tasks

### Wave 1: Config & Enum

#### Task 1.1: Effect module header and enum registration

**Files**: `src/effects/hue_remap.h` (create), `src/config/effect_config.h` (modify), `src/config/effect_descriptor.h` (modify)

**Creates**: `HueRemapConfig` struct, `HueRemapEffect` struct, `TRANSFORM_HUE_REMAP` enum entry

**Do**:
1. Create `src/effects/hue_remap.h` with `HueRemapConfig` and `HueRemapEffect` structs per the Design section. Follow `false_color.h` as pattern. Init takes `(HueRemapEffect *e, const HueRemapConfig *cfg)` like False Color.
2. In `src/config/effect_config.h`:
   - Add `#include "effects/hue_remap.h"` with other effect includes
   - Add `TRANSFORM_HUE_REMAP,` before `TRANSFORM_EFFECT_COUNT`
   - Add `HueRemapConfig hueRemap;` to `EffectConfig` struct
3. In `src/config/effect_descriptor.h`:
   - Add descriptor row: `[TRANSFORM_HUE_REMAP] = { TRANSFORM_HUE_REMAP, "Hue Remap", "COL", 8, offsetof(EffectConfig, hueRemap.enabled), EFFECT_FLAG_NONE },`

**Verify**: `cmake.exe --build build` compiles.

---

### Wave 2: Implementation (all parallel — no file overlap)

#### Task 2.1: Effect module implementation

**Files**: `src/effects/hue_remap.cpp` (create), `CMakeLists.txt` (modify)

**Depends on**: Wave 1 complete

**Do**: Follow `false_color.cpp` as pattern.
- Init: load `shaders/hue_remap.fs`, cache all uniform locations from `HueRemapEffect`, init LUT via `ColorLUTInit(&cfg->gradient)`. Clean up shader on LUT failure.
- Setup: call `ColorLUTUpdate`, bind all uniforms. Bind center as `vec2{cfg->cx, cfg->cy}`. Bind gradient LUT texture via `SetShaderValueTexture`.
- Uninit: unload shader, `ColorLUTUninit`.
- ConfigDefault: return default-constructed `HueRemapConfig`.
- RegisterParams: register `shift` (0-1), `intensity` (0-1), `radial` (-1 to 1), `cx` (0-1), `cy` (0-1).
- Add `src/effects/hue_remap.cpp` to `EFFECTS_SOURCES` in `CMakeLists.txt`.

**Verify**: Compiles.

#### Task 2.2: Shader

**Files**: `shaders/hue_remap.fs` (create)

**Depends on**: Wave 1 complete

**Do**: Implement the Algorithm section from the Design above. The shader is fully specified there — implement it verbatim.

**Verify**: File exists with correct uniforms matching the effect module locations.

#### Task 2.3: PostEffect integration

**Files**: `src/render/post_effect.h` (modify), `src/render/post_effect.cpp` (modify)

**Depends on**: Wave 1 complete

**Do**: Follow False Color's integration pattern.
- In `post_effect.h`: add `#include "effects/hue_remap.h"`, add `HueRemapEffect hueRemap;` member to `PostEffect`.
- In `post_effect.cpp`:
  - Init: `HueRemapEffectInit(&pe->hueRemap, &pe->effects.hueRemap)` with error handling
  - Uninit: `HueRemapEffectUninit(&pe->hueRemap)`
  - RegisterParams: `HueRemapRegisterParams(&pe->effects.hueRemap)` near other Color effects

**Verify**: Compiles.

#### Task 2.4: Shader setup and dispatch

**Files**: `src/render/shader_setup_color.h` (modify), `src/render/shader_setup_color.cpp` (modify), `src/render/shader_setup.cpp` (modify)

**Depends on**: Wave 1 complete

**Do**: Follow False Color's setup pattern.
- In `shader_setup_color.h`: declare `void SetupHueRemap(PostEffect *pe);`
- In `shader_setup_color.cpp`: implement `SetupHueRemap` — delegates to `HueRemapEffectSetup(&pe->hueRemap, &pe->effects.hueRemap)`
- In `shader_setup.cpp`: add dispatch case `case TRANSFORM_HUE_REMAP: return {&pe->hueRemap.shader, SetupHueRemap, &pe->effects.hueRemap.enabled};`

**Verify**: Compiles.

#### Task 2.5: UI panel

**Files**: `src/ui/imgui_effects_color.cpp` (modify)

**Depends on**: Wave 1 complete

**Do**: Follow `DrawColorFalseColor` as pattern.
- Add `static bool sectionHueRemap = false;` with other section bools
- Add `DrawColorHueRemap` function:
  - `DrawSectionBegin("Hue Remap", ...)`
  - Enabled checkbox with `MoveTransformToEnd` on first enable
  - `ImGuiDrawColorMode(&cfg->gradient)` for the ColorConfig widget
  - `ModulatableSlider("Shift##hueremap", &cfg->shift, "hueRemap.shift", "%.0f °", modSources, 360.0f)` — same scale display as Color Grade hue shift
  - `ModulatableSlider("Intensity##hueremap", &cfg->intensity, "hueRemap.intensity", "%.2f", modSources)`
  - `ModulatableSlider("Radial##hueremap", &cfg->radial, "hueRemap.radial", "%.2f", modSources)`
  - `ModulatableSlider("Center X##hueremap", &cfg->cx, "hueRemap.cx", "%.2f", modSources)`
  - `ModulatableSlider("Center Y##hueremap", &cfg->cy, "hueRemap.cy", "%.2f", modSources)`
- Call `DrawColorHueRemap` in `DrawColorCategory` after Palette Quantization with `ImGui::Spacing()`

**Verify**: Compiles.

#### Task 2.6: Preset serialization

**Files**: `src/config/preset.cpp` (modify)

**Depends on**: Wave 1 complete

**Do**:
- Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(HueRemapConfig, enabled, gradient, shift, intensity, radial, cx, cy)` near the other Color effect macros
- Add to `to_json`: `if (e.hueRemap.enabled) { j["hueRemap"] = e.hueRemap; }`
- Add to `from_json`: `e.hueRemap = j.value("hueRemap", e.hueRemap);`

**Verify**: Compiles.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] Effect appears in Color category with "COL" badge
- [ ] Enabling adds it to transform pipeline
- [ ] Shift slider with Rainbow mode behaves like Color Grade hue shift
- [ ] Changing ColorConfig to gradient/palette produces different color wheel
- [ ] Radial parameter creates spatial variation from center
- [ ] Preset save/load preserves all settings including ColorConfig
- [ ] Modulation routes work for shift, intensity, radial, cx, cy
