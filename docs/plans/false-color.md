# False Color

Replaces Duotone with a more versatile luminance-to-gradient color mapper. Maps image luminance to a user-defined color gradient via 1D LUT texture. Supports Solid, Rainbow, and custom multi-stop gradients through the existing ColorConfig system.

## Current State

Files to **remove** (Duotone):
- `src/config/duotone_config.h` - config struct
- `shaders/duotone.fs` - fragment shader

Files to **modify**:
- `src/config/effect_config.h:33,69,106,150,309,364` - duotone includes, enum, name, order, member, enabled check
- `src/render/post_effect.h:51,259-262` - duotoneShader, uniform locations
- `src/render/post_effect.cpp:95,134,549` - shader load, check, unload
- `src/render/shader_setup.h` - SetupDuotone declaration
- `src/render/shader_setup.cpp:62,658-669` - dispatch case, SetupDuotone function
- `src/ui/imgui_effects.cpp` - GetTransformCategory case
- `src/ui/imgui_effects_transforms.cpp:46,732-758,814` - section state, DrawColorDuotone, DrawColorCategory call
- `src/config/preset.cpp` - duotone JSON serialization
- `src/automation/param_registry.cpp:131,306` - duotone.intensity entries

Files to **create**:
- `src/config/false_color_config.h` - new config struct with ColorConfig + intensity
- `shaders/false_color.fs` - luminance-to-LUT shader

## Technical Implementation

**Source**: `docs/research/false-color.md`

### Core Algorithm

The shader extracts luminance using BT.601 weights, then uses that value as the U-coordinate to sample a 1D gradient texture.

```glsl
// BT.601 luminance weights
const vec3 LUMINANCE_WEIGHTS = vec3(0.299, 0.587, 0.114);

float luma = dot(color.rgb, LUMINANCE_WEIGHTS);

// Sample gradient LUT (256x1 texture)
// V=0.5 samples the center of the 1-pixel-tall strip
vec3 mappedColor = texture(gradientLUT, vec2(luma, 0.5)).rgb;

// Blend with original based on intensity
vec3 result = mix(color.rgb, mappedColor, intensity);
```

### LUT Texture Binding

False Color is the first transform effect requiring a runtime-generated texture. The pattern exists in simulations (Cymatics binds ColorLUT to texture unit 3 via glActiveTexture/glBindTexture).

For transforms, use raylib's `SetShaderValueTexture()` to bind the LUT as `texture1`:

```cpp
// In SetupFalseColor():
SetShaderValueTexture(pe->falseColorShader, pe->falseColorGradientLUTLoc,
                      ColorLUTGetTexture(pe->falseColorLUT));
```

The shader declares:
```glsl
uniform sampler2D texture0;  // Input image (automatic)
uniform sampler2D texture1;  // Gradient LUT (bound by SetShaderValueTexture)
```

### Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| gradient | ColorConfig | N/A | 2-stop cyan→magenta | Defines the color ramp |
| intensity | float | 0.0-1.0 | 1.0 | Blend: 0 = original, 1 = full false color |

---

## Phase 1: Config and Enum

**Goal**: Define FalseColorConfig and register in effect system.

**Build**:
- Create `src/config/false_color_config.h`:
  - `FalseColorConfig` struct with `enabled`, `ColorConfig gradient`, `float intensity`
  - Default gradient: 2-stop cyan (0,255,255) at 0.0, magenta (255,0,255) at 1.0
  - Default intensity: 1.0

- Modify `src/config/effect_config.h`:
  - Add `#include "false_color_config.h"` (replace duotone include)
  - Rename `TRANSFORM_DUOTONE` → `TRANSFORM_FALSE_COLOR`
  - Update `TransformEffectName()`: return "False Color"
  - Update `TransformOrderConfig::order` array entry
  - Replace `DuotoneConfig duotone` → `FalseColorConfig falseColor`
  - Update `IsTransformEnabled()` case

**Done when**: Build succeeds with FalseColorConfig in EffectConfig.

---

## Phase 2: Shader

**Goal**: Create false color fragment shader.

**Build**:
- Create `shaders/false_color.fs`:

```glsl
#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;  // Input image
uniform sampler2D texture1;  // 1D Gradient LUT (256x1)
uniform float intensity;     // Blend: 0 = original, 1 = full false color

const vec3 LUMINANCE_WEIGHTS = vec3(0.299, 0.587, 0.114);

void main()
{
    vec4 color = texture(texture0, fragTexCoord);

    // Extract luminance
    float luma = dot(color.rgb, LUMINANCE_WEIGHTS);

    // Sample gradient LUT
    vec3 mappedColor = texture(texture1, vec2(luma, 0.5)).rgb;

    // Blend with original
    vec3 result = mix(color.rgb, mappedColor, intensity);

    finalColor = vec4(result, color.a);
}
```

- Delete `shaders/duotone.fs`

**Done when**: Shader compiles (verified at runtime in Phase 3).

---

## Phase 3: PostEffect Integration

**Goal**: Load shader, create ColorLUT, wire uniform locations.

**Build**:
- Modify `src/render/post_effect.h`:
  - Add `#include "color_lut.h"` forward declaration (already exists as `typedef struct ColorLUT ColorLUT;`)
  - Replace `Shader duotoneShader` → `Shader falseColorShader`
  - Replace uniform locations: `falseColorIntensityLoc`, `falseColorGradientLUTLoc`
  - Add `ColorLUT* falseColorLUT` member

- Modify `src/render/post_effect.cpp`:
  - Add `#include "color_lut.h"`
  - In `LoadPostEffectShaders()`: load `shaders/false_color.fs` (replace duotone)
  - In success check: update shader name
  - In `GetShaderUniformLocations()`:
    - `pe->falseColorIntensityLoc = GetShaderLocation(pe->falseColorShader, "intensity")`
    - `pe->falseColorGradientLUTLoc = GetShaderLocation(pe->falseColorShader, "texture1")`
  - In `PostEffectInit()`: `pe->falseColorLUT = ColorLUTInit(&pe->effects.falseColor.gradient)`
  - In `PostEffectUninit()`: `ColorLUTUninit(pe->falseColorLUT)`, unload shader

**Done when**: App launches without shader errors.

---

## Phase 4: Shader Setup

**Goal**: Wire uniform values and LUT texture binding.

**Build**:
- Modify `src/render/shader_setup.h`:
  - Replace `SetupDuotone` → `SetupFalseColor`

- Modify `src/render/shader_setup.cpp`:
  - Update `GetTransformEffect()` case: `TRANSFORM_FALSE_COLOR` → `falseColorShader`, `SetupFalseColor`, `falseColor.enabled`
  - Replace `SetupDuotone` → `SetupFalseColor`:

```cpp
void SetupFalseColor(PostEffect* pe)
{
    const FalseColorConfig* fc = &pe->effects.falseColor;

    // Update LUT if gradient changed
    ColorLUTUpdate(pe->falseColorLUT, &fc->gradient);

    SetShaderValue(pe->falseColorShader, pe->falseColorIntensityLoc,
                   &fc->intensity, SHADER_UNIFORM_FLOAT);
    SetShaderValueTexture(pe->falseColorShader, pe->falseColorGradientLUTLoc,
                          ColorLUTGetTexture(pe->falseColorLUT));
}
```

**Done when**: Enabling False Color applies gradient mapping to the image.

---

## Phase 5: UI Panel

**Goal**: Create gradient picker and intensity slider UI.

**Build**:
- Modify `src/ui/imgui_effects.cpp`:
  - Update `GetTransformCategory()` case: `TRANSFORM_FALSE_COLOR` → same category index as duotone (Color)

- Modify `src/ui/imgui_effects_transforms.cpp`:
  - Rename `sectionDuotone` → `sectionFalseColor`
  - Replace `DrawColorDuotone` → `DrawColorFalseColor`:

```cpp
static void DrawColorFalseColor(EffectConfig* e, const ModSources* modSources, const ImU32 categoryGlow)
{
    if (DrawSectionBegin("False Color", categoryGlow, &sectionFalseColor)) {
        const bool wasEnabled = e->falseColor.enabled;
        ImGui::Checkbox("Enabled##falsecolor", &e->falseColor.enabled);
        if (!wasEnabled && e->falseColor.enabled) { MoveTransformToEnd(&e->transformOrder, TRANSFORM_FALSE_COLOR); }
        if (e->falseColor.enabled) {
            FalseColorConfig* fc = &e->falseColor;

            ImGuiDrawColorMode(&fc->gradient);

            ModulatableSlider("Intensity##falsecolor", &fc->intensity,
                              "falseColor.intensity", "%.2f", modSources);
        }
        DrawSectionEnd();
    }
}
```

  - Update `DrawColorCategory()` call

**Done when**: False Color panel shows gradient picker (Solid/Rainbow/Gradient modes) and intensity slider.

---

## Phase 6: Serialization and Modulation

**Goal**: Persist settings and enable audio-reactive intensity.

**Build**:
- Modify `src/config/preset.cpp`:
  - Remove duotone JSON macro
  - Add false color serialization (ColorConfig has custom to_json/from_json, so use manual approach):

```cpp
// In to_json:
if (e.falseColor.enabled) {
    j["falseColor"]["enabled"] = e.falseColor.enabled;
    j["falseColor"]["gradient"] = e.falseColor.gradient;
    j["falseColor"]["intensity"] = e.falseColor.intensity;
}

// In from_json:
if (j.contains("falseColor")) {
    auto& fc = j["falseColor"];
    e.falseColor.enabled = fc.value("enabled", false);
    e.falseColor.gradient = fc.value("gradient", e.falseColor.gradient);
    e.falseColor.intensity = fc.value("intensity", 1.0f);
}
```

- Modify `src/automation/param_registry.cpp`:
  - Replace `{"duotone.intensity", {0.0f, 1.0f}}` → `{"falseColor.intensity", {0.0f, 1.0f}}`
  - Update corresponding pointer: `&effects->falseColor.intensity`

**Done when**: Preset save/load works, intensity responds to modulation routes.

---

## Phase 7: Cleanup

**Goal**: Remove duotone remnants.

**Build**:
- Delete `src/config/duotone_config.h`
- Verify no remaining references to "duotone" in codebase (grep check)
- Update CMakeLists.txt if it explicitly lists duotone_config.h (unlikely, headers auto-discovered)

**Done when**: Clean build with no duotone references. Full verification:
- [ ] Effect appears in transform order pipeline with "False Color" name
- [ ] Effect shows correct category badge (Color)
- [ ] Gradient picker cycles through Solid/Rainbow/Gradient modes
- [ ] Intensity slider modulates blend amount
- [ ] Preset save/load preserves gradient and intensity
- [ ] Modulation routes to intensity parameter
