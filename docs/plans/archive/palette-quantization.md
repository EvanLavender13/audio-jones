# Palette Quantization

Reduces image colors to a limited palette with ordered Bayer dithering. Creates retro/pixel-art aesthetics from 8-color to 4096-color palettes without UV quantization (unlike Pixelation effect).

## Current State

Existing infrastructure to hook into:
- `src/config/effect_config.h:68` - TransformEffectType enum ends with TRANSFORM_CROSS_HATCHING
- `src/config/effect_config.h:138` - TransformOrderConfig order array
- `src/ui/imgui_effects_transforms.cpp:712` - DrawColorCategory orchestrates Color effects
- `src/automation/param_registry.cpp:131` - duotone.intensity entry (insert after)

## Technical Implementation

**Source**: [Alex Charlton: Dithering on the GPU](https://alex-charlton.com/posts/Dithering_on_the_GPU/)

### Core Algorithm

Quantize each color channel to N levels, applying Bayer threshold offset before rounding:

```glsl
// Snap value to nearest level
float quantize(float value, float levels) {
    return floor(value * levels + 0.5) / levels;
}

// Dithered quantization
vec3 ditherQuantize(vec3 color, vec2 fragCoord, float levels, float ditherStrength) {
    float threshold = getBayerThreshold(fragCoord);  // 0–1
    float stepSize = 1.0 / levels;

    // Offset color by threshold before quantizing
    vec3 dithered = color + (threshold - 0.5) * stepSize * ditherStrength;
    return vec3(
        quantize(dithered.r, levels),
        quantize(dithered.g, levels),
        quantize(dithered.b, levels)
    );
}
```

### Bayer Matrices

8×8 (default, finer pattern):
```glsl
const float BAYER_8X8[64] = float[64](
     0.0, 32.0,  8.0, 40.0,  2.0, 34.0, 10.0, 42.0,
    48.0, 16.0, 56.0, 24.0, 50.0, 18.0, 58.0, 26.0,
    12.0, 44.0,  4.0, 36.0, 14.0, 46.0,  6.0, 38.0,
    60.0, 28.0, 52.0, 20.0, 62.0, 30.0, 54.0, 22.0,
     3.0, 35.0, 11.0, 43.0,  1.0, 33.0,  9.0, 41.0,
    51.0, 19.0, 59.0, 27.0, 49.0, 17.0, 57.0, 25.0,
    15.0, 47.0,  7.0, 39.0, 13.0, 45.0,  5.0, 37.0,
    63.0, 31.0, 55.0, 23.0, 61.0, 29.0, 53.0, 21.0
);

float getBayerThreshold8(vec2 fragCoord) {
    int x = int(mod(fragCoord.x, 8.0));
    int y = int(mod(fragCoord.y, 8.0));
    return (BAYER_8X8[x + y * 8] + 0.5) / 64.0;
}
```

4×4 (coarser pattern):
```glsl
const float BAYER_4X4[16] = float[16](
     0.0,  8.0,  2.0, 10.0,
    12.0,  4.0, 14.0,  6.0,
     3.0, 11.0,  1.0,  9.0,
    15.0,  7.0, 13.0,  5.0
);

float getBayerThreshold4(vec2 fragCoord) {
    int x = int(mod(fragCoord.x, 4.0));
    int y = int(mod(fragCoord.y, 4.0));
    return (BAYER_4X4[x + y * 4] + 0.5) / 16.0;
}
```

### Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| colorLevels | float | 2–16 | 4 | Quantization levels per channel. 2=8 colors, 4=64, 8=512. |
| ditherStrength | float | 0–1 | 0.5 | Dithering intensity. 0=hard bands, 1=full dither. |
| bayerSize | int | 4 or 8 | 8 | Dither matrix size. 4=coarser pattern, 8=finer. |

---

## Phase 1: Config and Registration

**Goal**: Define configuration struct and register effect in type system.

**Build**:

1. Create `src/config/palette_quantization_config.h`:
   - `PaletteQuantizationConfig` struct with enabled, colorLevels (4.0f), ditherStrength (0.5f), bayerSize (8)

2. Modify `src/config/effect_config.h`:
   - Include `palette_quantization_config.h`
   - Add `TRANSFORM_PALETTE_QUANTIZATION` to enum before `TRANSFORM_EFFECT_COUNT`
   - Add case in `TransformEffectName()` returning "Palette Quantization"
   - Add `TRANSFORM_PALETTE_QUANTIZATION` to `TransformOrderConfig::order` array (at end)
   - Add `PaletteQuantizationConfig paletteQuantization;` member to `EffectConfig`
   - Add case in `IsTransformEnabled()` returning `e->paletteQuantization.enabled`

**Done when**: Project compiles with new enum value accessible.

---

## Phase 2: Shader

**Goal**: Create fragment shader implementing dithered quantization.

**Build**:

1. Create `shaders/palette_quantization.fs`:
   - Uniforms: `texture0`, `colorLevels` (float), `ditherStrength` (float), `bayerSize` (int)
   - Embed both BAYER_8X8 and BAYER_4X4 arrays
   - Branch on bayerSize to select threshold function
   - Apply dithered quantization algorithm from Technical Implementation section

**Done when**: Shader file exists with correct algorithm.

---

## Phase 3: PostEffect Integration

**Goal**: Wire shader into render pipeline.

**Build**:

1. Modify `src/render/post_effect.h`:
   - Add `Shader paletteQuantizationShader;`
   - Add `int paletteQuantizationColorLevelsLoc;`
   - Add `int paletteQuantizationDitherStrengthLoc;`
   - Add `int paletteQuantizationBayerSizeLoc;`

2. Modify `src/render/post_effect.cpp`:
   - In `LoadPostEffectShaders()`: load shader from `shaders/palette_quantization.fs`
   - Add to shader success check
   - In `GetShaderUniformLocations()`: get locations for all three uniforms
   - In `PostEffectUninit()`: unload shader

**Done when**: Shader loads without errors at startup.

---

## Phase 4: Shader Setup and Dispatch

**Goal**: Connect effect to transform chain.

**Build**:

1. Modify `src/render/shader_setup.h`:
   - Declare `void SetupPaletteQuantization(PostEffect* pe);`

2. Modify `src/render/shader_setup.cpp`:
   - Add case in `GetTransformEffect()` for `TRANSFORM_PALETTE_QUANTIZATION`
   - Implement `SetupPaletteQuantization()` binding all three uniforms

**Done when**: Effect renders when enabled (even without UI).

---

## Phase 5: UI Controls

**Goal**: Add user-facing controls in Color category.

**Build**:

1. Modify `src/ui/imgui_effects.cpp`:
   - Add case in `GetTransformCategory()` returning `CATEGORY_COLOR` for `TRANSFORM_PALETTE_QUANTIZATION`

2. Modify `src/ui/imgui_effects_transforms.cpp`:
   - Include `palette_quantization_config.h`
   - Add `static bool sectionPaletteQuantization = false;`
   - Create `DrawColorPaletteQuantization()` with:
     - Enabled checkbox with MoveTransformToEnd on enable
     - ModulatableSlider for colorLevels (2–16 range)
     - ModulatableSlider for ditherStrength (0–1 range)
     - Combo or radio for bayerSize (4×4, 8×8)
   - Call from `DrawColorCategory()` after Halftone with spacing

**Done when**: UI controls appear and modify effect parameters.

---

## Phase 6: Serialization and Modulation

**Goal**: Enable preset save/load and audio reactivity.

**Build**:

1. Modify `src/config/preset.cpp`:
   - Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT` macro for `PaletteQuantizationConfig`
   - Add to_json entry: `if (e.paletteQuantization.enabled) { j["paletteQuantization"] = e.paletteQuantization; }`
   - Add from_json entry: `e.paletteQuantization = j.value("paletteQuantization", e.paletteQuantization);`

2. Modify `src/automation/param_registry.cpp`:
   - Add PARAM_TABLE entries:
     - `{"paletteQuantization.colorLevels", {2.0f, 16.0f}}`
     - `{"paletteQuantization.ditherStrength", {0.0f, 1.0f}}`
   - Add targets entries (in same order):
     - `&effects->paletteQuantization.colorLevels`
     - `&effects->paletteQuantization.ditherStrength`

**Done when**: Preset save/load works, modulation routes drive parameters.

---

## Verification Checklist

- [ ] Effect appears in transform pipeline list
- [ ] Effect shows "Color" category badge (not "???")
- [ ] Effect can be reordered via drag-drop
- [ ] colorLevels slider reduces color palette visibly
- [ ] ditherStrength at 0 shows hard color bands
- [ ] ditherStrength at 1 shows full dithering
- [ ] bayerSize toggle changes dither pattern coarseness
- [ ] Preset save/load preserves all settings
- [ ] Modulation routes affect colorLevels and ditherStrength
- [ ] Build succeeds with no warnings
