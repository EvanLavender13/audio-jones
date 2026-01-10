# Pixelation

Reduces image resolution by snapping UV coordinates to a grid, creating a mosaic/pixel-art aesthetic. Each output "pixel" samples a single point from the input texture.

## Classification

- **Category**: UV Transform
- **Core Operation**: Quantize UV coordinates to grid cells, sample input at cell centers
- **Pipeline Position**: Reorderable transform (like other warp effects)

## References

- [fade_blur.frag](https://github.com/EvanLavender13/AudioThing/blob/main/fade_blur.frag) - UV quantization with optional anti-aliased sampling
- [Bayer matrix dithering](http://devlog-martinsh.blogspot.com/2011/03/glsl-8x8-bayer-matrix-dithering.html) by Martins Upitis - 8x8 ordered dithering
- [The Art of Dithering](https://blog.maximeheckel.com/posts/the-art-of-dithering-and-retro-shading-web/) - Bayer matrix theory and GLSL implementation

## Algorithm

### Core UV Quantization

```glsl
vec2 pixelatedUV = (floor(uv * resolution / pixelSize) + 0.5) * pixelSize / resolution;
vec4 color = texture(inputTexture, pixelatedUV);
```

Where:
- `resolution` = texture dimensions in pixels
- `pixelSize` = size of each mosaic cell in pixels
- `+ 0.5` centers the sample within each cell

### Simplified Version (resolution-independent)

```glsl
float cells = 64.0; // number of cells across width
vec2 pixelatedUV = floor(uv * cells) / cells;
```

### Optional: Anti-aliased Sampling

For smoother block edges, jitter samples within each cell:

```glsl
vec4 color = vec4(0.0);
const int SAMPLES = 4;
for (int i = 0; i < SAMPLES; i++) {
    float angle = float(i) * 6.28318 / float(SAMPLES);
    vec2 offset = vec2(cos(angle), sin(angle)) * (0.5 / cells);
    color += texture(inputTexture, pixelatedUV + offset);
}
color /= float(SAMPLES);
```

### Optional: Ordered Dithering (Bayer Matrix)

Simulates limited color palette by thresholding against a repeating pattern:

```glsl
// 8x8 Bayer matrix (values 0-63, divide by 64 for threshold)
const int bayer8x8[8][8] = {
    { 0, 32,  8, 40,  2, 34, 10, 42},
    {48, 16, 56, 24, 50, 18, 58, 26},
    {12, 44,  4, 36, 14, 46,  6, 38},
    {60, 28, 52, 20, 62, 30, 54, 22},
    { 3, 35, 11, 43,  1, 33,  9, 41},
    {51, 19, 59, 27, 49, 17, 57, 25},
    {15, 47,  7, 39, 13, 45,  5, 37},
    {63, 31, 55, 23, 61, 29, 53, 21}
};

float dither(int x, int y, float brightness) {
    float threshold = float(bayer8x8[x % 8][y % 8] + 1) / 64.0;
    return brightness < threshold ? 0.0 : 1.0;
}

// Per-channel dithering (preserves color)
vec2 xy = uv * resolution * ditherScale;
int x = int(mod(xy.x, 8.0));
int y = int(mod(xy.y, 8.0));
vec3 dithered;
dithered.r = dither(x, y, color.r);
dithered.g = dither(x, y, color.g);
dithered.b = dither(x, y, color.b);
```

### Optional: Color Posterization

Reduces color depth to simulate limited palettes:

```glsl
float levels = 4.0; // number of color levels per channel
vec3 posterized = floor(color * levels + 0.5) / levels;
```

Combine with dithering for smooth gradients despite limited colors:

```glsl
// Add dither noise before quantizing
float bayerNoise = float(bayer8x8[x % 8][y % 8]) / 64.0 - 0.5;
vec3 ditheredColor = color + bayerNoise / levels;
vec3 posterized = floor(ditheredColor * levels + 0.5) / levels;
```

## Parameters

| Parameter | Type | Range | Effect |
|-----------|------|-------|--------|
| cellCount | float | 4-256 | Number of cells across width. Lower = blockier |
| aspectCorrect | bool | - | Maintain square cells regardless of aspect ratio |
| sampleMode | int | 0-1 | 0=center sample, 1=anti-aliased (multi-sample) |
| ditherEnabled | bool | - | Enable Bayer matrix dithering |
| ditherScale | float | 1-8 | Size of dither pattern relative to pixels |
| posterizeLevels | int | 2-16 | Color levels per channel (0=disabled) |

## Audio Mapping Ideas

- **cellCount** <- bass energy: loud bass = blockier pixels
- **cellCount** <- beat trigger: snap to low resolution on beat, lerp back
- Animate cell offset for "scrolling pixel grid" effect

## Notes

- Very cheap effect (single texture lookup per fragment in basic mode)
- Aspect correction: multiply Y cell count by `resolution.x / resolution.y`
- Combines well with color quantization (posterization) for retro game aesthetic
- Consider edge detection overlay for pixel-art outline effect
