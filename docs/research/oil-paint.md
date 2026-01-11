# Oil Paint (Kuwahara Filter)

Transforms images into a painterly style by smoothing flat regions while preserving hard edges. Creates the characteristic look of oil paint brush strokes with visible color banding.

## Classification

- **Category**: TRANSFORMS → Style
- **Core Operation**: Divide pixel neighborhood into sectors, output average color of lowest-variance sector
- **Pipeline Position**: Output stage transforms (user-ordered with other Style effects)

## References

- [Maxime Heckel's Painterly Shaders](https://blog.maximeheckel.com/posts/on-crafting-painterly-shaders/) - Comprehensive Kuwahara variants with GLSL
- [LYGIA Kuwahara](https://lygia.xyz/v1.1.4/filter/kuwahara) - Clean shader library implementation
- [Kyprianidis et al. "Anisotropic Kuwahara Filtering on the GPU"](https://www.researchgate.net/publication/334689545_Oil_Painting_Style_Rendering_Based_on_Kuwahara_Filter) - Academic foundation

## Algorithm

### Basic Kuwahara (4 Sectors)

Divide a square kernel around each pixel into four overlapping quadrants:

```
+---+---+
| 0 | 1 |
+---X---+  X = current pixel
| 2 | 3 |
+---+---+
```

For each sector, compute mean color and variance:

```glsl
vec3 calculateSector(vec2 uv, vec2 offset, int radius) {
    vec3 colorSum = vec3(0.0);
    vec3 colorSqSum = vec3(0.0);
    float count = 0.0;

    for (int x = 0; x <= radius; x++) {
        for (int y = 0; y <= radius; y++) {
            vec2 sampleUV = uv + (offset + vec2(x, y)) * pixelSize;
            vec3 c = texture(inputTex, sampleUV).rgb;
            colorSum += c;
            colorSqSum += c * c;
            count += 1.0;
        }
    }

    vec3 mean = colorSum / count;
    vec3 variance = (colorSqSum / count) - (mean * mean);
    float v = dot(variance, vec3(0.299, 0.587, 0.114));

    return vec3(mean.r, mean.g, mean.b); // store variance separately
}
```

Select sector with minimum variance:

```glsl
void main() {
    // Calculate all 4 sectors
    vec4 s0 = calcSector(uv, vec2(-radius, -radius), radius);
    vec4 s1 = calcSector(uv, vec2(0, -radius), radius);
    vec4 s2 = calcSector(uv, vec2(-radius, 0), radius);
    vec4 s3 = calcSector(uv, vec2(0, 0), radius);

    // Find minimum variance
    vec4 result = s0;
    if (s1.a < result.a) result = s1;
    if (s2.a < result.a) result = s2;
    if (s3.a < result.a) result = s3;

    gl_FragColor = vec4(result.rgb, 1.0);
}
```

### Generalized Kuwahara (8 Circular Sectors)

Replace square quadrants with 8 circular sectors using Gaussian weighting:

```glsl
float gaussianWeight(float dist, float sigma) {
    return exp(-(dist * dist) / (2.0 * sigma * sigma));
}
```

Sectors overlap smoothly, reducing block artifacts at sector boundaries.

### Anisotropic Kuwahara (Advanced)

Compute structure tensor from Sobel derivatives to detect local edge orientation:

```glsl
// Structure tensor components
float Jxx = dot(Sx, Sx);  // Sx = horizontal Sobel
float Jyy = dot(Sy, Sy);  // Sy = vertical Sobel
float Jxy = dot(Sx, Sy);

// Eigenvalue-based anisotropy
float lambda1 = 0.5 * (Jxx + Jyy + sqrt((Jxx - Jyy) * (Jxx - Jyy) + 4.0 * Jxy * Jxy));
float lambda2 = 0.5 * (Jxx + Jyy - sqrt((Jxx - Jyy) * (Jxx - Jyy) + 4.0 * Jxy * Jxy));
float anisotropy = (lambda1 - lambda2) / (lambda1 + lambda2 + 1e-7);
```

Stretch the kernel along the dominant edge direction for sharper edges.

## Parameters

| Parameter | Type | Range | Effect |
|-----------|------|-------|--------|
| radius | int | 2-12 | Kernel radius (brush stroke size) |
| sectors | enum | 4/8 | Number of kernel sectors |
| sharpness | float | 0.1-8.0 | Gaussian sigma for sector weighting |
| anisotropic | bool | - | Enable edge-aligned kernel stretching |
| alpha | float | 1.0-50.0 | Anisotropy strength (if enabled) |

## Audio Mapping Ideas

- **radius** ← bass intensity (larger strokes on heavy bass)
- **sharpness** ← high frequency energy (crisper edges on bright sounds)
- **anisotropy strength** ← beat envelope

## Notes

- Performance scales with radius² — radius 10+ may drop frames
- Basic 4-sector at radius 4-6 provides good real-time performance
- Anisotropic version requires pre-pass for structure tensor (two-pass shader)
- Consider combining with edge overlay for painterly outlines
- Pairs well with color quantization for poster-paint look
