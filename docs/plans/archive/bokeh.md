# Bokeh

Simulates out-of-focus camera blur where bright spots bloom into soft circular highlights. Uses golden-angle Vogel disc sampling weighted by brightness to create characteristic "popping" bokeh orbs.

**Research**: `docs/research/bokeh.md` - Contains complete algorithm, ShaderToy references, parameter ranges.

## Current State

No existing bokeh implementation. Hook into the transform effect pipeline:
- `src/config/effect_config.h:69` - Last enum entry is `TRANSFORM_PALETTE_QUANTIZATION`
- `src/render/post_effect.h:55` - Last shader is `paletteQuantizationShader`
- `src/ui/imgui_effects_transforms.cpp:751-866` - Style category contains Oil Paint, Watercolor, etc.

## Technical Implementation

**Source**: [David Hoskins - Bokeh Disc](https://www.shadertoy.com/view/4d2Xzw), [GM Shaders - Phi/Golden Angle](https://mini.gmshaders.com/p/phi)

### Constants
```glsl
#define GOLDEN_ANGLE 2.39996323  // tau * (2 - phi)
#define HALF_PI 1.5707963
```

### Core Algorithm
```glsl
vec3 BokehBlur(sampler2D tex, vec2 uv, vec2 resolution, float radius, int iterations, float brightPower)
{
    vec3 acc = vec3(0.0);
    vec3 div = vec3(0.0);
    float aspect = resolution.y / resolution.x;

    for (int i = 0; i < iterations; i++)
    {
        // Golden angle spiral direction
        vec2 dir = cos(float(i) * GOLDEN_ANGLE + vec2(0.0, HALF_PI));

        // Square root distributes samples uniformly across disc area
        float r = sqrt(float(i) / float(iterations)) * radius;

        // Aspect-corrected offset
        vec2 offset = dir * r;
        offset.x *= aspect;

        vec3 col = texture(tex, uv + offset).rgb;

        // Brightness weighting - higher power = more "pop"
        vec3 weight = pow(col, vec3(brightPower));

        acc += col * weight;
        div += weight;
    }

    return acc / max(div, vec3(0.001));
}
```

### Parameters

| Uniform | Type | Range | Default | Effect |
|---------|------|-------|---------|--------|
| radius | float | 0.0 - 0.1 | 0.02 | Blur disc size in UV space |
| iterations | int | 16 - 150 | 64 | Sample count (quality vs performance) |
| brightnessPower | float | 1.0 - 8.0 | 4.0 | Brightness weighting exponent |

---

## Phase 1: Config and Registration

**Goal**: Create config struct and register effect in the transform pipeline.

**Build**:
- Create `src/config/bokeh_config.h` with BokehConfig struct (enabled, radius, iterations, brightnessPower)
- Modify `src/config/effect_config.h`:
  - Add `#include "bokeh_config.h"`
  - Add `TRANSFORM_BOKEH` to enum before `TRANSFORM_EFFECT_COUNT`
  - Add case in `TransformEffectName()` returning "Bokeh"
  - Add `TRANSFORM_BOKEH` to `TransformOrderConfig::order` array (commonly missed)
  - Add `BokehConfig bokeh;` member to `EffectConfig`
  - Add case in `IsTransformEnabled()`

**Done when**: Project compiles with new enum and config struct.

---

## Phase 2: Shader

**Goal**: Implement the golden-angle disc sampling shader.

**Build**:
- Create `shaders/bokeh.fs` with:
  - Uniforms: `resolution`, `radius`, `iterations`, `brightnessPower`
  - Golden angle constant and HALF_PI
  - Vogel disc sampling loop with brightness weighting
  - Aspect ratio correction on sample offsets

**Done when**: Shader file exists with complete algorithm from Technical Implementation section.

---

## Phase 3: PostEffect Integration

**Goal**: Load shader and cache uniform locations.

**Build**:
- Modify `src/render/post_effect.h`:
  - Add `Shader bokehShader;`
  - Add uniform locations: `bokehResolutionLoc`, `bokehRadiusLoc`, `bokehIterationsLoc`, `bokehBrightnessPowerLoc`
- Modify `src/render/post_effect.cpp`:
  - Load shader in `LoadPostEffectShaders()`
  - Add to success check
  - Get uniform locations in `GetShaderUniformLocations()`
  - Add to `SetResolutionUniforms()`
  - Unload in `PostEffectUninit()`

**Done when**: Shader loads without errors, uniform locations cached.

---

## Phase 4: Shader Setup and Dispatch

**Goal**: Wire effect into transform pipeline.

**Build**:
- Modify `src/render/shader_setup.h`:
  - Declare `void SetupBokeh(PostEffect* pe);`
- Modify `src/render/shader_setup.cpp`:
  - Add `TRANSFORM_BOKEH` case in `GetTransformEffect()` returning shader, SetupBokeh, enabled pointer
  - Implement `SetupBokeh()` binding radius, iterations, brightnessPower uniforms

**Done when**: Enabling effect renders (even without UI controls).

---

## Phase 5: UI Controls

**Goal**: Add controls to Style category.

**Build**:
- Modify `src/ui/imgui_effects.cpp`:
  - Add `TRANSFORM_BOKEH` case in `GetTransformCategory()` returning style category (commonly missed)
- Modify `src/ui/imgui_effects_transforms.cpp`:
  - Add `static bool sectionBokeh = false;` at top
  - Add `DrawStyleBokeh()` helper with:
    - Enable checkbox with `MoveTransformToEnd` on enable
    - `ModulatableSlider` for radius (range 0.0-0.1)
    - `ImGui::SliderInt` for iterations (range 16-150)
    - `ModulatableSlider` for brightnessPower (range 1.0-8.0)
  - Call helper in `DrawStyleCategory()` after existing effects

**Done when**: Effect appears in Style category with working controls.

---

## Phase 6: Serialization and Modulation

**Goal**: Enable preset save/load and audio modulation.

**Build**:
- Modify `src/config/preset.cpp`:
  - Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(BokehConfig, enabled, radius, iterations, brightnessPower)`
  - Add `to_json` entry for bokeh
  - Add `from_json` entry for bokeh
- Modify `src/automation/param_registry.cpp`:
  - Add PARAM_TABLE entries for `bokeh.radius`, `bokeh.iterations`, `bokeh.brightnessPower` with ranges
  - Add corresponding `&effects->bokeh.*` pointers to targets array at matching indices (commonly missed)

**Done when**: Presets persist bokeh settings, LFO/audio routes to all three parameters.

---

## Verification Checklist

- [ ] Effect appears in transform order pipeline
- [ ] Effect shows "Style" category badge (not "???")
- [ ] Effect can be reordered via drag-drop
- [ ] Enabling effect adds it to pipeline list
- [ ] UI controls modify effect in real-time
- [ ] Bright areas bloom into soft circles
- [ ] Aspect ratio maintained (circular, not elliptical)
- [ ] Preset save/load preserves settings
- [ ] Modulation routes to radius, iterations, brightnessPower
- [ ] Build succeeds with no warnings
