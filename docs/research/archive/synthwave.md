# Synthwave

Transforms input visuals into an 80s retrofuturism aesthetic with neon-on-dark color treatment, perspective grid overlay, and horizontal sun stripe bands. The effect adapts existing content rather than generating a generic scene - bright areas take on the striped sun character while dark regions reveal the laser grid.

## Classification

- **Category**: TRANSFORMS > Graphic
- **Pipeline Position**: After symmetry/warp effects, before output stage

## References

- [Inigo Quilez - Palettes](https://iquilezles.org/articles/palettes/) - Cosine palette formula for color remapping
- [Inigo Quilez - Filterable Procedurals](https://iquilezles.org/articles/filterableprocedurals/) - Anti-aliased grid rendering
- [Godot Shaders - SynthWave Grid](https://godotshaders.com/shader/synthwave-grid-shader/) - Perspective grid with glow
- [glsl-cos-palette](https://github.com/Erkaman/glsl-cos-palette) - Cosine palette implementation

## Algorithm

The effect combines three layered techniques, each modulated by input content.

### 1. Cosine Palette Color Remap

Extract luminance from input and map through a synthwave-tuned cosine palette:

```glsl
vec3 synthwavePalette(float t) {
    // Tuned for cyan -> magenta -> orange gradient
    vec3 a = vec3(0.5, 0.5, 0.5);
    vec3 b = vec3(0.5, 0.5, 0.5);
    vec3 c = vec3(1.0, 1.0, 1.0);
    vec3 d = vec3(0.0, 0.1, 0.2);  // Sunset-ish phase
    return a + b * cos(6.283185 * (c * t + d));
}

float luma = dot(inputColor.rgb, vec3(0.299, 0.587, 0.114));
vec3 remapped = synthwavePalette(luma);
```

The `d` parameter shifts the palette. Values around `(0.0, 0.1, 0.2)` produce warm sunset tones; `(0.5, 0.6, 0.7)` shifts toward cool cyan/magenta.

### 2. Perspective Grid Overlay

Screen-space grid with vanishing point at horizon:

```glsl
float perspectiveGrid(vec2 uv, float horizon, float spacing, float thickness) {
    // Transform UV to perspective space (y below horizon stretches toward infinity)
    float depth = (uv.y - horizon) / (1.0 - horizon);  // 0 at horizon, 1 at bottom
    if (depth <= 0.0) return 0.0;  // Above horizon: no grid

    // Perspective-correct coordinates
    float z = 1.0 / depth;
    float x = (uv.x - 0.5) * z;

    // Filtered grid lines (horizontal and vertical)
    vec2 gridUV = vec2(x, z) * spacing;
    vec2 fw = fwidth(gridUV);
    vec2 grid = smoothstep(thickness + fw, thickness - fw,
                           abs(fract(gridUV) - 0.5));

    return max(grid.x, grid.y);
}
```

Grid intensity blends stronger on dark input areas:

```glsl
float gridMask = perspectiveGrid(uv, horizonY, gridSpacing, lineThickness);
float darknessMask = 1.0 - luma;  // Grid shows through dark areas
gridMask *= darknessMask * gridOpacity;
```

### 3. Horizontal Sun Stripes

Bands in upper screen portion that appear on bright areas:

```glsl
float sunStripes(vec2 uv, float horizon, float stripeCount, float softness) {
    if (uv.y > horizon) return 0.0;  // Below horizon: no stripes

    // Normalized position in sky region (0 at top, 1 at horizon)
    float skyPos = uv.y / horizon;

    // Horizontal stripes with spacing that increases toward horizon
    float stripePhase = skyPos * stripeCount;
    float stripe = smoothstep(0.4 - softness, 0.4 + softness,
                              abs(fract(stripePhase) - 0.5));

    // Fade out near top and near horizon
    float fade = smoothstep(0.0, 0.3, skyPos) * smoothstep(1.0, 0.7, skyPos);

    return stripe * fade;
}
```

Stripes blend onto bright areas:

```glsl
float stripeMask = sunStripes(uv, horizonY, stripeCount, stripeSoftness);
stripeMask *= luma * stripeIntensity;  // Brighter input = more stripes
```

### 4. Final Composition

```glsl
// Start with palette-remapped color
vec3 result = mix(inputColor.rgb, remapped, colorMix);

// Add grid glow on dark areas
result += gridColor * gridMask * gridGlow;

// Blend sun stripes on bright areas
result = mix(result, sunColor, stripeMask);

// Optional: horizon glow gradient
float horizonGlow = exp(-abs(uv.y - horizonY) * horizonFalloff);
result += horizonColor * horizonGlow * horizonIntensity;
```

## Parameters

| Parameter | Type | Range | Default | Visual Effect |
|-----------|------|-------|---------|---------------|
| horizonY | float | 0.3-0.7 | 0.5 | Vertical position of horizon line |
| colorMix | float | 0-1 | 0.7 | Blend between original and palette colors |
| palettePhase | vec3 | 0-1 each | (0, 0.1, 0.2) | Cosine palette d parameter (color character) |
| gridSpacing | float | 2-20 | 8.0 | Distance between grid lines |
| gridThickness | float | 0.01-0.1 | 0.03 | Width of grid lines |
| gridOpacity | float | 0-1 | 0.5 | Overall grid visibility |
| gridGlow | float | 1-3 | 1.5 | Neon bloom intensity on grid |
| gridColor | vec3 | color | (0, 0.8, 1) | Grid line color (cyan default) |
| stripeCount | float | 4-20 | 8.0 | Number of horizontal sun bands |
| stripeSoftness | float | 0-0.3 | 0.1 | Edge softness of stripes |
| stripeIntensity | float | 0-1 | 0.6 | Overall stripe visibility |
| sunColor | vec3 | color | (1, 0.4, 0.8) | Sun stripe color (magenta-pink default) |
| horizonIntensity | float | 0-1 | 0.3 | Glow at horizon line |
| horizonColor | vec3 | color | (1, 0.6, 0) | Horizon glow color (orange default) |

## Modulation Candidates

- **palettePhase**: Shifts color character, cycling through warm/cool tones
- **horizonY**: Moves horizon up/down with audio energy
- **gridSpacing**: Pulses grid density
- **stripeCount**: Changes stripe frequency
- **gridGlow / stripeIntensity**: React to bass/treble

## Notes

- The grid uses `fwidth()` for anti-aliasing; requires derivatives (standard in fragment shaders)
- Grid vanishing point is centered horizontally; could add `vanishX` parameter for offset
- Stripe spacing increases toward horizon for perspective consistency
- Works best with existing content that has contrast; flat inputs produce subtle results
- Consider pairing with Bloom effect for enhanced neon glow
- The cosine palette portion could be factored out once Cosine Palette is implemented as a shared utility
