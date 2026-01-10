# Texture Warp Channel Modes

Enhancement to existing Texture Warp effect. Adds parameter to select which color channel combination drives UV displacement, producing different emergent patterns from the same input.

## Classification

- **Category**: Enhancement to TRANSFORMS → Warp → Texture Warp
- **Core Operation**: Parameterized channel selection for displacement vector
- **Pipeline Position**: No change (existing effect)

## References

- `shaders/texture_warp.fs` - Current implementation using RG channels
- `src/config/texture_warp_config.h` - Current config structure

## Current Implementation

```glsl
// texture_warp.fs:14-17
vec3 sample = texture(texture0, warpedUV).rgb;
vec2 offset = (sample.rg - 0.5) * 2.0;  // Maps [0,1] to [-1,1]
warpedUV += offset * strength;
```

The RG channels directly become X/Y displacement. Red-heavy areas push right, green-heavy areas push up.

## Proposed Channel Modes

### Mode 0: RG (Current Default)

```glsl
vec2 offset = (sample.rg - 0.5) * 2.0;
```

- Red → X displacement
- Green → Y displacement
- **Character**: Color-boundary driven, warm colors push one way, cool another

### Mode 1: RB

```glsl
vec2 offset = (sample.rb - 0.5) * 2.0;
```

- Red → X displacement
- Blue → Y displacement
- **Character**: Different color features dominate, green ignored

### Mode 2: GB

```glsl
vec2 offset = (sample.gb - 0.5) * 2.0;
```

- Green → X displacement
- Blue → Y displacement
- **Character**: Red ignored, cyan/yellow boundaries dominate

### Mode 3: Luminance

```glsl
float lum = dot(sample, vec3(0.299, 0.587, 0.114));
vec2 offset = vec2(lum - 0.5) * 2.0;
```

- Brightness → both X and Y (diagonal flow)
- **Character**: Bright areas push toward (+X, +Y), dark toward (-X, -Y)
- Creates diagonal ridge patterns along brightness contours

### Mode 4: Luminance Split

```glsl
float lum = dot(sample, vec3(0.299, 0.587, 0.114));
vec2 offset = vec2(lum - 0.5, 0.5 - lum) * 2.0;
```

- Brightness → X, inverse brightness → Y
- **Character**: Bright pushes right and down, dark pushes left and up
- Anti-diagonal flow pattern

### Mode 5: Chrominance (RG vs B)

```glsl
vec2 offset = vec2(sample.r - sample.b, sample.g - sample.b) * 2.0;
```

- Color difference drives flow, not absolute values
- **Character**: Ignores brightness, reacts to hue/saturation
- Neutral grays produce no displacement

### Mode 6: Polar (Hue → Angle, Saturation → Magnitude)

```glsl
// Simplified hue approximation
vec3 hsv = rgb2hsv(sample);  // Need HSV conversion
float angle = hsv.x * 2.0 * PI;
float magnitude = hsv.y;  // Saturation
vec2 offset = vec2(cos(angle), sin(angle)) * magnitude;
```

- Hue determines direction (red=0°, green=120°, blue=240°)
- Saturation determines strength (gray = no movement)
- **Character**: Colorful areas swirl based on hue, desaturated areas stable

## Implementation Approach

### Config Change

```cpp
// In texture_warp_config.h
enum class TextureWarpChannelMode {
    RG = 0,        // Current default
    RB,
    GB,
    Luminance,
    LuminanceSplit,
    Chrominance,
    Polar
};

struct TextureWarpConfig {
    float strength = 0.02f;
    int iterations = 4;
    TextureWarpChannelMode channelMode = TextureWarpChannelMode::RG;  // New
};
```

### Shader Change

Pass mode as uniform int, branch in shader:

```glsl
uniform int channelMode;

vec2 computeOffset(vec3 sample) {
    if (channelMode == 0) return (sample.rg - 0.5) * 2.0;
    if (channelMode == 1) return (sample.rb - 0.5) * 2.0;
    if (channelMode == 2) return (sample.gb - 0.5) * 2.0;
    if (channelMode == 3) {
        float l = dot(sample, vec3(0.299, 0.587, 0.114));
        return vec2(l - 0.5) * 2.0;
    }
    // ... etc
    return vec2(0.0);
}
```

### UI Change

Add combo box to Texture Warp panel:

```cpp
const char* channelModeNames[] = {
    "RG (Default)", "RB", "GB", "Luminance", "Lum Split", "Chrominance", "Polar"
};
ImGui::Combo("Channel Mode", (int*)&config.channelMode, channelModeNames, 7);
```

## Parameters

| Parameter | Type | Values | Default | Effect |
|-----------|------|--------|---------|--------|
| channelMode | enum | RG, RB, GB, Luminance, LuminanceSplit, Chrominance, Polar | RG | Which channels drive displacement |

## Audio Mapping Ideas

| Parameter | Audio Source | Behavior |
|-----------|--------------|----------|
| channelMode | Beat counter (mod 7) | Cycle through modes on beats |

## Notes

- Polar mode requires HSV conversion - adds ~10 instructions per iteration. Consider whether worth including.
- All modes maintain the self-referential property - no new texture dependencies.
- Some modes (Luminance, Chrominance) may produce very different visual character - good for variety.
- Could expose as "X Channel" and "Y Channel" dropdowns for full flexibility, but enum is simpler.
