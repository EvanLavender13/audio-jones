# ASCII Art Effect

Converts rendered frames to a grid of text characters based on luminance. Darker regions display denser characters (@, #, 8), lighter regions display sparser ones (., :, space). Creates a retro terminal aesthetic with optional monochrome coloring.

## Current State

ASCII Art integrates as a Style-category transform effect, following the existing pattern used by Pixelation, Toon, and Glitch.

- `src/config/effect_config.h:26-46` - TransformEffectType enum, add after TRANSFORM_COLOR_GRADE
- `src/config/effect_config.h:109-183` - EffectConfig struct, add AsciiArtConfig member
- `src/render/post_effect.h:13-217` - PostEffect struct, add shader and uniform locations
- `src/render/shader_setup.cpp:10-52` - GetTransformEffect dispatch, add TRANSFORM_ASCII_ART case
- `src/ui/imgui_effects_transforms.cpp:294-435` - DrawStyleCategory, add ASCII Art section

## Technical Implementation

**Source**: [Codrops ASCII Shader](https://tympanus.net/codrops/2024/11/13/creating-an-ascii-shader-using-ogl/)

### Cell Division

Divide screen into fixed-size cells. Sample at cell center:

```glsl
vec2 cellSize = vec2(float(cellPixels)) / resolution;
vec2 cellUV = floor(uv / cellSize) * cellSize + cellSize * 0.5;
vec3 color = texture(texture0, cellUV).rgb;
```

### Luminance Calculation

```glsl
float lum = dot(color, vec3(0.299, 0.587, 0.114));
if (invert == 1) lum = 1.0 - lum;
```

### Bit-Packed Character Lookup

5x5 character bitmaps encoded as 25-bit integers. Lookup table maps luminance to character:

```glsl
// Character set (darkest to lightest): @ # 8 & o * : . (space)
int getCharBits(float lum) {
    if (lum > 0.85) return 11512810;  // @ (densest)
    if (lum > 0.75) return 13195790;  // #
    if (lum > 0.60) return 15252014;  // 8
    if (lum > 0.50) return 13121101;  // &
    if (lum > 0.40) return 15255086;  // o
    if (lum > 0.25) return 163153;    // *
    if (lum > 0.10) return 65600;     // :
    if (lum > 0.02) return 4;         // .
    return 0;                          // space
}
```

### Character Rendering

Decode bit at fragment position within cell:

```glsl
float character(int bits, vec2 p) {
    // Map cell-local UV to 5x4 grid (4 wide, 5 tall for character aspect ratio)
    vec2 pos = floor(p * vec2(4.0, 5.0));
    if (clamp(pos, 0.0, vec2(3.0, 4.0)) != pos) return 0.0;
    int idx = int(pos.x) + int(pos.y) * 4;
    return float((bits >> idx) & 1);
}

// In main():
vec2 cellLocalUV = fract(uv / cellSize);
int charBits = getCharBits(lum);
float mask = character(charBits, cellLocalUV);
```

### Color Modes

```glsl
// colorMode: 0 = original, 1 = mono, 2 = CRT green
vec3 result;
if (colorMode == 0) {
    result = color * mask;
} else if (colorMode == 1) {
    result = mix(background, foreground, mask);
} else {
    // CRT green preset
    result = mix(vec3(0.0, 0.02, 0.0), vec3(0.0, 1.0, 0.0), mask);
}
```

### Uniforms

| Uniform | Type | Description |
|---------|------|-------------|
| `resolution` | vec2 | Screen dimensions (pixels) |
| `cellPixels` | int | Cell size (4-32 pixels) |
| `colorMode` | int | 0=Original, 1=Mono, 2=CRT |
| `foreground` | vec3 | Mono mode foreground RGB |
| `background` | vec3 | Mono mode background RGB |
| `invert` | int | 0/1 swap light/dark mapping |

---

## Phase 1: Config and Enum

**Goal**: Define AsciiArtConfig struct and register in transform effect system.

**Build**:
- Create `src/config/ascii_art_config.h` with AsciiArtConfig struct:
  - `bool enabled = false`
  - `float cellSize = 8.0f` (4.0-32.0 range)
  - `int colorMode = 0` (0=Original, 1=Mono, 2=CRT)
  - `float foregroundR/G/B = 0.0f, 1.0f, 0.0f` (default green)
  - `float backgroundR/G/B = 0.0f, 0.02f, 0.0f` (dark green)
  - `bool invert = false`
- Add `#include "ascii_art_config.h"` to `effect_config.h`
- Add `TRANSFORM_ASCII_ART` to TransformEffectType enum (after TRANSFORM_COLOR_GRADE, before TRANSFORM_EFFECT_COUNT)
- Add case to `TransformEffectName()` returning "ASCII Art"
- Update `TransformOrderConfig::order` default array
- Add `AsciiArtConfig asciiArt;` to EffectConfig struct

**Done when**: Project compiles with new config struct accessible.

---

## Phase 2: Shader

**Goal**: Implement procedural ASCII art fragment shader.

**Build**:
- Create `shaders/ascii_art.fs` implementing:
  - Cell division and center sampling
  - Luminance calculation with invert support
  - Bit-packed character lookup (8 luminance levels)
  - Character rendering via bit decoding
  - Three color modes (original, mono, CRT)
- Use uniforms: resolution, cellPixels, colorMode, foreground, background, invert

**Done when**: Shader compiles without errors (test via LoadShader).

---

## Phase 3: PostEffect Integration

**Goal**: Load shader, cache uniform locations, create setup function.

**Build**:
- Add to `PostEffect` struct in `post_effect.h`:
  - `Shader asciiArtShader;`
  - Uniform locations: `asciiArtResolutionLoc`, `asciiArtCellPixelsLoc`, `asciiArtColorModeLoc`, `asciiArtForegroundLoc`, `asciiArtBackgroundLoc`, `asciiArtInvertLoc`
- Add shader load in `LoadPostEffectShaders()` in `post_effect.cpp`
- Add shader to load success check
- Add uniform location lookups in `GetShaderUniformLocations()`
- Add shader to `SetResolutionUniforms()` if needed
- Add `UnloadShader()` call in `PostEffectUninit()`
- Add `SetupAsciiArt()` function in `shader_setup.cpp` passing all uniforms
- Add `TRANSFORM_ASCII_ART` case to `GetTransformEffect()` dispatch

**Done when**: Effect triggers via transform order (shows black screen if uniforms misconfigured).

---

## Phase 4: UI Panel

**Goal**: Add ASCII Art controls to Style category.

**Build**:
- Add `static bool sectionAsciiArt = false;` at top of `imgui_effects_transforms.cpp`
- Add section in `DrawStyleCategory()` after Color Grade:
  - Enabled checkbox
  - `ModulatableSlider` for Cell Size (4.0-32.0)
  - Combo for Color Mode (Original/Mono/CRT)
  - Conditional color pickers for foreground/background (show when colorMode == 1)
  - Checkbox for Invert

**Done when**: UI controls appear and modify config values.

---

## Phase 5: Serialization and Modulation

**Goal**: Enable preset save/load and audio reactivity.

**Build**:
- Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT` for AsciiArtConfig in `preset.cpp`
- Add `if (e.asciiArt.enabled) { j["asciiArt"] = e.asciiArt; }` in to_json
- Add `e.asciiArt = j.value("asciiArt", e.asciiArt);` in from_json
- Add to `param_registry.cpp`:
  - `{"asciiArt.cellSize", {4.0f, 32.0f}}` in PARAM_TABLE
  - `&effects->asciiArt.cellSize` in PARAM_POINTERS

**Done when**: Effect saves/loads in presets and cellSize responds to modulation.
