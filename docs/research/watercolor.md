# Watercolor

Simulates the appearance of watercolor painting through edge darkening (pigment pooling), paper granulation texture, and soft color bleeding. Creates an organic, hand-painted aesthetic.

## Classification

- **Category**: TRANSFORMS → Style
- **Core Operation**: Apply edge darkening via Sobel, blend paper texture, soften with blur
- **Pipeline Position**: Output stage transforms (user-ordered with other Style effects)

## References

- [Cyan's Watercolour Shader Experiments](https://cyangamedev.wordpress.com/2020/10/06/watercolour-shader-experiments/) - Practical techniques, triplanar noise
- [Maxime Heckel's Painterly Shaders](https://blog.maximeheckel.com/posts/on-crafting-painterly-shaders/) - Kuwahara base for softening
- [Luft & Deussen Watercolor Paper](https://kops.uni-konstanz.de/entities/publication/aee1a598-ae7d-49c3-a471-8d23f7e01e9f) - Academic foundation for granulation

## Algorithm

### Edge Darkening (Pigment Pooling)

Watercolor pigment concentrates at edges as water evaporates. Detect edges via Sobel and darken:

```glsl
// Sobel edge detection
float sobelX = 0.0, sobelY = 0.0;
for (int i = -1; i <= 1; i++) {
    for (int j = -1; j <= 1; j++) {
        float lum = getLuminance(texture(inputTex, uv + vec2(i, j) * pixelSize).rgb);
        sobelX += lum * float(i) * (j == 0 ? 2.0 : 1.0);
        sobelY += lum * float(j) * (i == 0 ? 2.0 : 1.0);
    }
}
float edge = sqrt(sobelX * sobelX + sobelY * sobelY);

// Darken edges
vec3 color = texture(inputTex, uv).rgb;
color *= 1.0 - edge * edgeDarkening;
```

### Paper Granulation

Watercolor pigment settles into paper texture valleys. Sample noise and modulate color:

```glsl
// Paper texture (use tileable noise texture or procedural)
float paper = texture(paperTex, uv * paperScale).r;

// Granulation: pigment settles in darker paper areas
float granulation = mix(1.0, paper, granulationStrength);
color *= granulation;
```

### Color Bleeding / Wet Edge

Blur colors at edges to simulate wet paint spreading:

```glsl
// Directional blur along edge normal
vec2 edgeDir = normalize(vec2(sobelX, sobelY) + 0.001);
vec3 bleed = vec3(0.0);
for (int i = -2; i <= 2; i++) {
    bleed += texture(inputTex, uv + edgeDir * float(i) * bleedRadius * pixelSize).rgb;
}
bleed /= 5.0;

// Blend bleeding at edges
color = mix(color, bleed, edge * bleedStrength);
```

### Soft Color Quantization (Optional)

Watercolors have limited color mixing. Reduce color precision for authenticity:

```glsl
float levels = 8.0;
color = floor(color * levels + 0.5) / levels;
```

### Combined Pipeline

```glsl
void main() {
    vec3 color = texture(inputTex, uv).rgb;

    // 1. Soft blur for paint diffusion
    color = gaussianBlur(inputTex, uv, softness);

    // 2. Edge detection
    float edge = sobelEdge(inputTex, uv);

    // 3. Edge darkening
    color *= 1.0 - edge * edgeDarkening;

    // 4. Paper granulation
    float paper = texture(paperTex, uv * paperScale).r;
    color *= mix(1.0, paper, granulationStrength * (1.0 - edge));

    // 5. Optional: wet edge bleeding
    color = mix(color, bleedColor(uv, edge), edge * bleedStrength);

    gl_FragColor = vec4(color, 1.0);
}
```

## Parameters

| Parameter | Type | Range | Effect |
|-----------|------|-------|--------|
| edgeDarkening | float | 0.0-1.0 | Pigment pooling intensity at edges |
| granulationStrength | float | 0.0-1.0 | Paper texture visibility |
| paperScale | float | 1.0-20.0 | Paper texture tiling frequency |
| softness | float | 0.0-5.0 | Overall paint diffusion blur |
| bleedStrength | float | 0.0-1.0 | Wet edge color spreading |
| bleedRadius | float | 1.0-10.0 | How far wet edges spread |
| colorLevels | int | 4-16 | Color quantization (0 = disabled) |

## Audio Mapping Ideas

- **edgeDarkening** ← mid frequency energy (more definition on vocals/leads)
- **granulationStrength** ← high frequency content (texture on cymbals/hats)
- **softness** ← inverse of transient detection (blur between beats)
- **bleedStrength** ← bass envelope (colors bleed on bass hits)

## Notes

- Requires paper texture asset (tileable noise or scanned paper)
- Can generate procedural paper with layered Perlin noise
- Shares Sobel edge detection with existing Toon effect — consider shared utility
- Edge darkening alone (without granulation) gives "ink wash" look
- Multiple passes may be needed: blur → edge detect → composite
- Consider adding slight color temperature shift toward warm for traditional watercolor look
