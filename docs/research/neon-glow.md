# Neon Glow

Extracts luminance edges via Sobel detection and renders them as bright, glowing lines. Creates cyberpunk/Tron wireframe aesthetics where shapes are defined by their contours rather than fill.

## Classification

- **Category**: TRANSFORMS → Style
- **Core Operation**: Sobel edge detection on luminance, power-curve shaping, colored glow with optional blur, additive or replace blending
- **Pipeline Position**: Transform stage (user-ordered)

## References

- [GitHub Gist: GLSL Sobel Edge Detection](https://gist.github.com/Hebali/6ebfc66106459aacee6a9fac029d0115) - 3×3 kernel sampling with horizontal/vertical Sobel convolution
- [Shadertoy: edge glow](https://www.shadertoy.com/view/Mdf3zr) - Sobel + additive color, uses g² to suppress noise
- [Shadertoy: GLOW TUTORIAL](https://www.shadertoy.com/view/3s3GDn) - Distance glow with `pow(radius/dist, intensity)` and tonemapping

## Algorithm

### Luminance Extraction

```glsl
float getLuminance(vec3 color) {
    return 0.2126 * color.r + 0.7152 * color.g + 0.0722 * color.b;
}
```

### Sobel Edge Detection

Sample 3×3 neighborhood and apply Sobel kernels:

```glsl
float sobelEdge(sampler2D tex, vec2 uv, vec2 texelSize) {
    // Sample 3x3 neighborhood luminance
    float n[9];
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            vec2 offset = vec2(float(i - 1), float(j - 1)) * texelSize;
            n[i * 3 + j] = getLuminance(texture(tex, uv + offset).rgb);
        }
    }

    // Sobel kernels
    // Horizontal: [-1 0 1]  Vertical: [-1 -2 -1]
    //             [-2 0 2]            [ 0  0  0]
    //             [-1 0 1]            [ 1  2  1]

    float gx = -n[0] - 2.0*n[3] - n[6] + n[2] + 2.0*n[5] + n[8];
    float gy = -n[0] - 2.0*n[1] - n[2] + n[6] + 2.0*n[7] + n[8];

    return sqrt(gx*gx + gy*gy);
}
```

### Edge Shaping

Apply threshold and power curve to control edge appearance:

```glsl
float shapeEdge(float edge, float threshold, float power) {
    float shaped = max(edge - threshold, 0.0);
    return pow(shaped, power);
}
```

Using `edge * edge` (squaring) instead of sqrt suppresses noise while emphasizing strong edges.

### Single-Pass Glow Approximation

For soft glow without multi-pass blur, sample edges in a cross pattern:

```glsl
float glowEdge(sampler2D tex, vec2 uv, vec2 texelSize, float radius, int samples) {
    float total = 0.0;
    float weight = 0.0;

    for (int i = -samples; i <= samples; i++) {
        float w = 1.0 - abs(float(i)) / float(samples + 1);

        // Horizontal
        vec2 offsetH = vec2(float(i) * radius, 0.0) * texelSize;
        total += sobelEdge(tex, uv + offsetH, texelSize) * w;

        // Vertical
        vec2 offsetV = vec2(0.0, float(i) * radius) * texelSize;
        total += sobelEdge(tex, uv + offsetV, texelSize) * w;

        weight += w * 2.0;
    }

    return total / weight;
}
```

### Glow Falloff

For point-based glow (metaball style), use inverse distance with power:

```glsl
float getGlow(float dist, float radius, float intensity) {
    return pow(radius / dist, intensity);
}
```

### Tonemapping

Prevent glow from blowing out to white:

```glsl
vec3 tonemap(vec3 color) {
    return 1.0 - exp(-color);
}
```

### Complete Effect

```glsl
uniform vec3 glowColor;
uniform float edgeThreshold;
uniform float edgePower;
uniform float glowIntensity;
uniform float originalVisibility;  // 0 = edges only, 1 = full original

vec4 neonGlow(sampler2D tex, vec2 uv, vec2 texelSize) {
    vec4 original = texture(tex, uv);

    float edge = sobelEdge(tex, uv, texelSize);
    edge = shapeEdge(edge, edgeThreshold, edgePower);

    vec3 glow = edge * glowColor * glowIntensity;
    glow = tonemap(glow);

    vec3 base = original.rgb * originalVisibility;
    vec3 result = base + glow;  // Additive blend

    return vec4(result, original.a);
}
```

## Parameters

| Parameter | Type | Range | Effect |
|-----------|------|-------|--------|
| glowColor | vec3 | 0–1 per channel | Color of the neon glow |
| edgeThreshold | float | 0–0.5 | Minimum edge strength to glow; suppresses noise |
| edgePower | float | 0.5–3.0 | Edge intensity curve; higher = sharper falloff |
| glowIntensity | float | 0.5–5.0 | Brightness multiplier for glow |
| glowRadius | float | 1–10 | Blur radius in pixels (if using multi-tap glow) |
| originalVisibility | float | 0–1 | How much original image shows through |

## Audio Mapping Ideas

- **glowIntensity** ← Beat detection: Pulse brightness on beats
- **glowColor hue** ← Frequency spectrum: Shift hue based on dominant frequency
- **edgeThreshold** ← Inverse of bass energy: More edges visible on bass hits
- **glowRadius** ← Mid frequencies: Wider glow on mids

## Notes

- Squaring edge magnitude (`g*g` instead of `sqrt(gx*gx + gy*gy)`) suppresses noise while preserving strong edges
- For quality glow, a proper two-pass gaussian blur (horizontal then vertical) on edge texture produces best results, but single-pass cross-tap approximation is acceptable
- Consider a "pulse" mode where glow animates (kernel offset varies with time, as in the Shadertoy example)
- Edge detection kernel size can be scaled for thicker/thinner lines
- Works well after other effects that create high-contrast content
