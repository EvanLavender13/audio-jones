# Duotone Effect + Color Category

Adds duotone color transform (luminance → two-color gradient), creates a "Color" effect category, and fixes NeonGlow's RGB sliders to use ColorEdit3.

## Current State

- `src/config/color_grade_config.h:1-18` - Example color transform config
- `src/ui/imgui_effects.cpp:26-63` - `GetTransformCategory()` with existing categories (SYM, WARP, CELL, MOT, STY)
- `src/ui/imgui_effects_transforms.cpp:529-555` - NeonGlow UI with separate R/G/B sliders
- `src/ui/imgui_effects_transforms.cpp:582-607` - ColorGrade UI in DrawStyleCategory
- `docs/research/duotone.md` - Algorithm reference

## Technical Implementation

### Duotone Shader

**Source**: [Agate Dragon: Duotone Gradient Shader](https://agatedragon.blog/2024/01/06/duotone-gradient-shader/)

**Luminance extraction** (BT.601 weights):
```glsl
const vec3 LUMINANCE_WEIGHTS = vec3(0.299, 0.587, 0.114);
float lum = dot(color.rgb, LUMINANCE_WEIGHTS);
```

**Two-color mapping**:
```glsl
vec3 toned = mix(shadowColor, highlightColor, lum);
vec3 result = mix(color.rgb, toned, intensity);
```

**Parameters**:
| Uniform | Type | Range | Default |
|---------|------|-------|---------|
| shadowColor | vec3 | 0-1 | (0.1, 0.0, 0.2) |
| highlightColor | vec3 | 0-1 | (1.0, 0.9, 0.6) |
| intensity | float | 0-1 | 0.0 |

---

## Phase 1: Duotone Config & Shader

**Goal**: Create config struct and fragment shader.

**Build**:
- `src/config/duotone_config.h` - DuotoneConfig struct with enabled, shadowColor[3], highlightColor[3], intensity
- `shaders/duotone.fs` - Fragment shader implementing luminance → mix → intensity blend

**Done when**: Files compile and shader loads without error.

---

## Phase 2: Effect Registration

**Goal**: Register duotone in the effect system.

**Build** (all in `src/config/effect_config.h`):
- Add `#include "duotone_config.h"`
- Add `TRANSFORM_DUOTONE` to enum before COUNT
- Add "Duotone" case to `TransformEffectName()`
- Add `TRANSFORM_DUOTONE` to `TransformOrderConfig::order` array
- Add `DuotoneConfig duotone;` member to EffectConfig
- Add `TRANSFORM_DUOTONE` case to `IsTransformEnabled()`

**Done when**: Build succeeds with duotone in effect enum.

---

## Phase 3: PostEffect Integration

**Goal**: Load shader and wire up uniforms.

**Build**:
- `src/render/post_effect.h` - Add `Shader duotoneShader;` and uniform location ints (duotoneShadowColorLoc, duotoneHighlightColorLoc, duotoneIntensityLoc)
- `src/render/post_effect.cpp` - Load shader, validate, get locations, unload

**Build** (shader setup):
- `src/render/shader_setup.h` - Declare `SetupDuotone()`
- `src/render/shader_setup.cpp` - Add dispatch case and implement SetupDuotone

**Done when**: Shader loads and uniforms bind without warnings.

---

## Phase 4: Add Color Category

**Goal**: Create new "Color" category (COL) for ColorGrade and Duotone.

**Build** (`src/ui/imgui_effects.cpp`):
- In `GetTransformCategory()`: add `case TRANSFORM_DUOTONE:` returning `{"COL", 5}`
- Move `TRANSFORM_COLOR_GRADE` case from STY (section 4) to COL (section 5)

**Build** (`src/ui/imgui_effects_transforms.cpp`):
- Add section state: `static bool sectionDuotone = false;`
- Create `DrawColorDuotone()` helper with ColorEdit3 pickers for shadow/highlight, ModulatableSlider for intensity
- Create `DrawColorCategory()` orchestrator calling DrawColorDuotone and DrawStyleColorGrade (renamed to DrawColorColorGrade)
- Remove ColorGrade call from `DrawStyleCategory()`

**Build** (`src/ui/imgui_effects_transforms.h`):
- Declare `DrawColorCategory()`

**Build** (`src/ui/imgui_effects.cpp` panel function):
- Add `DrawColorCategory()` call after DrawStyleCategory

**Done when**: Color category appears in UI with ColorGrade and Duotone effects.

---

## Phase 5: NeonGlow ColorEdit Fix

**Goal**: Replace three RGB sliders with single ColorEdit3.

**Build** (`src/ui/imgui_effects_transforms.cpp`):
- In `DrawStyleNeonGlow()`: replace the three `SliderFloat` calls for R/G/B with:
  ```cpp
  float glowCol[3] = { ng->glowR, ng->glowG, ng->glowB };
  if (ImGui::ColorEdit3("Glow Color##neonglow", glowCol)) {
      ng->glowR = glowCol[0];
      ng->glowG = glowCol[1];
      ng->glowB = glowCol[2];
  }
  ```
- Remove the `ImGui::Text("Glow Color");` line

**Done when**: NeonGlow shows single color picker instead of three sliders.

---

## Phase 6: Preset & Modulation

**Goal**: Save/load presets and wire intensity for modulation.

**Build** (`src/config/preset.cpp`):
- Add JSON macro for DuotoneConfig
- Add to_json and from_json entries

**Build** (`src/automation/param_registry.cpp`):
- Add `{"duotone.intensity", {0.0f, 1.0f}}` to PARAM_TABLE
- Add `&effects->duotone.intensity` to targets array at matching index

**Done when**: Duotone settings save/load correctly and intensity responds to modulation.

---

## Phase 7: Verification

**Goal**: Full integration test.

**Verification checklist**:
- [ ] Duotone appears in transform order pipeline with "COL" badge
- [ ] ColorGrade shows "COL" badge (moved from "STY")
- [ ] Duotone can be reordered via drag-drop
- [ ] Color pickers work for shadow/highlight colors
- [ ] Intensity slider responds to LFO/audio modulation
- [ ] NeonGlow shows single color picker (no RGB sliders)
- [ ] Preset save/load preserves all duotone settings
- [ ] Build succeeds with no warnings

**Done when**: All checklist items pass.
