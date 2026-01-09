# Pixelation

Reorderable transform effect that reduces image resolution to a grid of mosaic cells, with optional Bayer dithering and color posterization for a retro 8-bit aesthetic.

## Current State

Transform effect infrastructure exists and is well-established:
- `src/config/effect_config.h:16-26` - TransformEffectType enum and ordering
- `src/render/shader_setup.cpp:10-32` - GetTransformEffect dispatch table
- `src/render/render_pipeline.cpp:265-272` - Transform chain execution
- `docs/research/pixelation.md` - Algorithm reference with UV quantization, Bayer matrix, posterization

## Technical Implementation

**Source**: `docs/research/pixelation.md`, Bayer matrix from Martins Upitis

### Core UV Quantization

```glsl
// Aspect-corrected cell count
float cellsX = cellCount;
float cellsY = cellCount * resolution.y / resolution.x;

// Quantize UV to cell centers
vec2 pixelatedUV = (floor(uv * vec2(cellsX, cellsY)) + 0.5) / vec2(cellsX, cellsY);
vec4 color = texture(texture0, pixelatedUV);
```

### Bayer Matrix Dithering (when ditherScale > 0)

8x8 ordered dither pattern stored as constant array. Threshold lookup per fragment:

```glsl
// Bayer 8x8 matrix (0-63 values, normalized to 0-1 threshold)
const float bayer8x8[64] = float[64](
     0, 32,  8, 40,  2, 34, 10, 42,
    48, 16, 56, 24, 50, 18, 58, 26,
    12, 44,  4, 36, 14, 46,  6, 38,
    60, 28, 52, 20, 62, 30, 54, 22,
     3, 35, 11, 43,  1, 33,  9, 41,
    51, 19, 59, 27, 49, 17, 57, 25,
    15, 47,  7, 39, 13, 45,  5, 37,
    63, 31, 55, 23, 61, 29, 53, 21
);

// Sample position in dither grid (scaled by ditherScale)
ivec2 ditherPos = ivec2(mod(fragTexCoord * resolution / ditherScale, 8.0));
int idx = ditherPos.y * 8 + ditherPos.x;
float threshold = (bayer8x8[idx] + 0.5) / 64.0;
```

### Color Posterization (when posterizeLevels > 0)

Combine dither noise with quantization for smooth gradients despite limited colors:

```glsl
float levels = float(posterizeLevels);
// Add dither bias before quantizing (-0.5 to +0.5 range, scaled by level step)
float ditherBias = (threshold - 0.5) / levels;
vec3 posterized = floor((color.rgb + ditherBias) * levels + 0.5) / levels;
```

When dithering disabled (ditherScale == 0), use 0.5 as neutral threshold (no dither bias).

### Parameters

| Uniform | Type | Range | Effect |
|---------|------|-------|--------|
| cellCount | float | 4-256 | Cells across width. Lower = blockier. |
| ditherScale | float | 0-8 | Dither pattern size. 0 = disabled. |
| posterizeLevels | int | 0-16 | Color levels per channel. 0 = disabled. |
| resolution | vec2 | - | Screen dimensions (for aspect correction). |

---

## Phase 1: Config and Shader

**Goal**: Create PixelationConfig struct and shader with all three features.

**Build**:
- `src/config/pixelation_config.h` - Config struct with `enabled`, `cellCount`, `ditherScale`, `posterizeLevels`
- `shaders/pixelation.fs` - Fragment shader implementing UV quantization, Bayer dithering, posterization per technical spec above
- `src/config/effect_config.h` - Add `TRANSFORM_PIXELATION` to enum, update `TransformEffectName`, add to `TransformOrderConfig` default

**Done when**: Config compiles, shader file exists with correct algorithm.

---

## Phase 2: Pipeline Integration

**Goal**: Wire shader into transform chain.

**Build**:
- `src/render/post_effect.h` - Add `pixelationShader`, uniform location ints (`pixelationCellCountLoc`, etc.)
- `src/render/post_effect.cpp` - Load shader, get uniform locations, unload in Uninit
- `src/render/shader_setup.h` - Declare `SetupPixelation`
- `src/render/shader_setup.cpp` - Implement `SetupPixelation`, add case to `GetTransformEffect`

**Done when**: Effect renders when enabled (even with hardcoded test values).

---

## Phase 3: UI and Serialization

**Goal**: Add controls and preset support.

**Build**:
- `src/ui/imgui_effects.cpp` - Add Pixelation section with sliders for cellCount (4-256), ditherScale (0-8), posterizeLevels (0-16)
- `src/config/preset.cpp` - Add `to_json`/`from_json` for PixelationConfig

**Done when**: Can enable effect, tweak parameters, save/load presets.

---

## Phase 4: Modulation Registration

**Goal**: Register modulatable parameters.

**Build**:
- `src/automation/param_registry.cpp` - Register `pixelation.cellCount` and `pixelation.ditherScale` with appropriate ranges

**Done when**: Parameters appear in modulation target list and respond to audio routing.

**Added scope**: Created `ModulatableSliderInt` wrapper in `src/ui/ui_units.h` for float-backed parameters that display as integers. Refactored `infiniteZoom.drosteShear` to use this wrapper. Dither slider only visible when posterize > 0.
