# Vignette

Edge darkening effect using SDF-based shape control and smooth color blending. Combines GPUImage's `mix(source, color, percent)` blend with TyLindberg's `sdSquare` + roundness SDF for configurable circular-to-rectangular shape.

**Research**: `docs/research/vignette.md`

## Design

### Types

**VignetteConfig** (`src/effects/vignette.h`):

```
bool enabled = false
float intensity = 0.5f   // How strongly edges blend toward vignette color (0.0-1.0)
float radius = 0.5f      // Distance from center where darkening begins (0.0-1.0)
float softness = 0.4f    // Width of falloff gradient (0.01-1.0)
float roundness = 1.0f   // Shape: 0=rectangular, 1=circular (0.0-1.0)
float colorR = 0.0f      // Vignette color red (0.0-1.0)
float colorG = 0.0f      // Vignette color green (0.0-1.0)
float colorB = 0.0f      // Vignette color blue (0.0-1.0)
```

**VignetteEffect** (`src/effects/vignette.h`):

```
Shader shader
int intensityLoc
int radiusLoc
int softnessLoc
int roundnessLoc
int colorLoc
```

No time accumulator - purely spatial, no animation state.

### Algorithm

SDF distance function (TyLindberg, from iq):

```glsl
float sdSquare(vec2 point, float width) {
    vec2 d = abs(point) - width;
    return min(max(d.x, d.y), 0.0) + length(max(d, 0.0));
}
```

Main vignette computation:

```glsl
void main() {
    vec4 color = texture(texture0, fragTexCoord);

    // Center UV to (-0.5, 0.5) range
    vec2 uv = fragTexCoord - 0.5;

    // SDF shape - symmetric size simplifies TyLindberg UV clamping away
    // roundness=1: boxSize=0, dist=length(uv)-radius -> circle
    // roundness=0: boxSize=radius, dist=sdSquare(uv,radius) -> rectangle
    float boxSize = radius * (1.0 - roundness);
    float dist = sdSquare(uv, boxSize) - (radius * roundness);

    // Falloff and blend (GPUImage approach)
    float percent = smoothstep(0.0, softness, dist) * intensity;
    finalColor = vec4(mix(color.rgb, vignetteColor, percent), color.a);
}
```

All distance computation in UV space (0-1). No `resolution` uniform needed.

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Widget | UI Label | Format |
|-----------|------|-------|---------|-------------|-----------|----------|--------|
| intensity | float | 0.0-1.0 | 0.5 | Yes | ModulatableSlider | "Intensity" | "%.2f" |
| radius | float | 0.0-1.0 | 0.5 | Yes | ModulatableSlider | "Radius" | "%.2f" |
| softness | float | 0.01-1.0 | 0.4 | Yes | ModulatableSliderLog | "Softness" | "%.2f" |
| roundness | float | 0.0-1.0 | 1.0 | Yes | ModulatableSlider | "Roundness" | "%.2f" |
| colorR/G/B | float | 0.0-1.0 | 0.0 | No | ColorEdit3 | "Color" | - |

5 params + color - flat UI, no SeparatorText subsections needed.

### Constants

- Enum: `TRANSFORM_VIGNETTE`
- Field name: `vignette`
- Display name: `"Vignette"`
- Category badge: `"OPT"`
- Section index: `7` (Optical)
- Flags: `EFFECT_FLAG_NONE`
- Param prefix: `"vignette."`

---

## Tasks

### Wave 1: Header

#### Task 1.1: VignetteConfig and VignetteEffect structs

**Files**: `src/effects/vignette.h` (CREATE)
**Creates**: Config struct, Effect struct, function declarations

**Do**: Follow `chromatic_aberration.h` structure. Define `VignetteConfig` with fields from Design section (4 float params + 3 color floats). Define `VignetteEffect` with shader + 5 uniform location ints (4 float + 1 vec3 color). Define `VIGNETTE_CONFIG_FIELDS` macro listing all 7 floats + enabled. Declare `VignetteEffectInit`, `VignetteEffectSetup` (const Effect*, const Config*), `VignetteEffectUninit`, `VignetteRegisterParams`.

**Verify**: Header parses with no errors.

---

### Wave 2: Implementation (all parallel)

#### Task 2.1: Effect module implementation

**Files**: `src/effects/vignette.cpp` (CREATE)
**Depends on**: Wave 1 complete

**Do**: Follow `chromatic_aberration.cpp` as pattern. Includes: `vignette.h`, `automation/mod_sources.h`, `automation/modulation_engine.h`, `config/constants.h`, `config/effect_descriptor.h`, `render/post_effect.h`, `imgui.h`, `ui/modulatable_slider.h`, `<stddef.h>`.

- `VignetteEffectInit`: Load `shaders/vignette.fs`, cache 5 uniform locations (intensity, radius, softness, roundness, vignetteColor). Return false if shader.id == 0.
- `VignetteEffectSetup`: Bind 4 float uniforms as SHADER_UNIFORM_FLOAT. Pack `colorR/G/B` into a `float[3]` and bind as SHADER_UNIFORM_VEC3 to `colorLoc`. No deltaTime parameter - no animation state. Follow `anamorphic_streak.cpp` pattern for color binding.
- `VignetteEffectUninit`: UnloadShader.
- `VignetteRegisterParams`: Register 4 modulatable params (intensity, radius, softness, roundness) with `"vignette."` prefix. Bounds must match the range column in Design. Color is not modulatable - skip registration.
- `SetupVignette(PostEffect* pe)` bridge: non-static, delegates to `VignetteEffectSetup(&pe->vignette, &pe->effects.vignette)`.
- `// === UI ===` section: `static void DrawVignetteParams(EffectConfig*, const ModSources*, ImU32)`. Use `ModulatableSlider` for intensity, radius, roundness. Use `ModulatableSliderLog` for softness. All `"%.2f"` format. All labels suffixed with `##vignette`. All param IDs prefixed `"vignette."`. Add `ImGui::ColorEdit3("Color##vignette", &cfg->colorR)` for the color picker.
- Registration macro at bottom: `REGISTER_EFFECT(TRANSFORM_VIGNETTE, Vignette, vignette, "Vignette", "OPT", 7, EFFECT_FLAG_NONE, SetupVignette, NULL, DrawVignetteParams)` wrapped in clang-format off/on.

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 2.2: Fragment shader

**Files**: `shaders/vignette.fs` (CREATE)
**Depends on**: Wave 1 complete

**Do**: Implement the Algorithm section from Design. Standard header: `#version 330`, `in vec2 fragTexCoord`, `out vec4 finalColor`, `uniform sampler2D texture0`. Four float uniforms: intensity, radius, softness, roundness. One vec3 uniform: vignetteColor. Include `sdSquare` function. Main function implements the vignette computation exactly as shown in the Algorithm section.

**Verify**: Shader file exists with correct uniforms matching the loc names in the header.

---

#### Task 2.3: Integration edits

**Files**: `src/config/effect_config.h` (MODIFY), `src/render/post_effect.h` (MODIFY), `CMakeLists.txt` (MODIFY), `src/config/effect_serialization.cpp` (MODIFY)
**Depends on**: Wave 1 complete

**Do**:

`src/config/effect_config.h`:
1. Add `#include "effects/vignette.h"` with other effect includes
2. Add `TRANSFORM_VIGNETTE` to `TransformEffectType` enum before `TRANSFORM_ACCUM_COMPOSITE`
3. Add `VignetteConfig vignette;` to `EffectConfig` struct

`src/render/post_effect.h`:
1. Add `#include "effects/vignette.h"` with other effect includes
2. Add `VignetteEffect vignette;` to `PostEffect` struct

`CMakeLists.txt`:
1. Add `src/effects/vignette.cpp` to `EFFECTS_SOURCES` (alphabetical)

`src/config/effect_serialization.cpp`:
1. Add `#include "effects/vignette.h"` with other effect includes
2. Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(VignetteConfig, VIGNETTE_CONFIG_FIELDS)`
3. Add `X(vignette) \` to the `EFFECT_CONFIG_FIELDS(X)` X-macro table

**Verify**: `cmake.exe --build build` compiles with no warnings.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] Effect appears in Effects window under Optical section with "OPT" badge
- [ ] Enabling effect shows darkened edges (default: circular, 50% intensity, black)
- [ ] Roundness slider morphs shape from circular to rectangular
- [ ] Softness slider changes falloff sharpness
- [ ] Radius slider moves the vignette boundary inward/outward
- [ ] Color picker changes vignette color (white for light leak, colored for tint)
- [ ] Preset save/load preserves all parameters including color
- [ ] 4 modulatable params appear in modulation routing dropdown
