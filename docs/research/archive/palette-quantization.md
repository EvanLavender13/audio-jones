# Palette Quantization

Reduces image colors to a limited palette with optional ordered dithering. Creates retro/pixel-art aesthetics ranging from 1-bit monochrome to 256-color palettes.

## Classification

- **Category**: TRANSFORMS → Color
- **Core Operation**: Quantize each color channel to N levels, apply Bayer matrix threshold to select between adjacent levels
- **Pipeline Position**: Transform stage (user-ordered)

## References

- [Alex Charlton: Dithering on the GPU](https://alex-charlton.com/posts/Dithering_on_the_GPU/) - Comprehensive GPU ordered dithering with Bayer matrices

## Algorithm

### Color Quantization

Snap each channel to nearest level in N-level palette:

```glsl
float quantize(float value, float levels) {
    return floor(value * levels + 0.5) / levels;
}

vec3 quantize(vec3 color, float levels) {
    return vec3(
        quantize(color.r, levels),
        quantize(color.g, levels),
        quantize(color.b, levels)
    );
}
```

Total colors = levels^3 (e.g., 4 levels per channel = 64 colors).

### Bayer Matrix (8×8)

Ordered dithering threshold pattern:

```glsl
const int BAYER_8X8[64] = int[64](
     0, 32,  8, 40,  2, 34, 10, 42,
    48, 16, 56, 24, 50, 18, 58, 26,
    12, 44,  4, 36, 14, 46,  6, 38,
    60, 28, 52, 20, 62, 30, 54, 22,
     3, 35, 11, 43,  1, 33,  9, 41,
    51, 19, 59, 27, 49, 17, 57, 25,
    15, 47,  7, 39, 13, 45,  5, 37,
    63, 31, 55, 23, 61, 29, 53, 21
);

float getBayerThreshold(vec2 fragCoord) {
    int x = int(mod(fragCoord.x, 8.0));
    int y = int(mod(fragCoord.y, 8.0));
    return float(BAYER_8X8[x + y * 8]) / 64.0;
}
```

### Dithered Quantization

Add threshold offset before quantizing to create dithered transitions:

```glsl
uniform float colorLevels;      // 2–16
uniform float ditherStrength;   // 0–1

vec3 ditherQuantize(vec3 color, vec2 fragCoord) {
    float threshold = getBayerThreshold(fragCoord);
    float stepSize = 1.0 / colorLevels;

    // Offset color by threshold before quantizing
    vec3 dithered = color + (threshold - 0.5) * stepSize * ditherStrength;
    return quantize(dithered, colorLevels);
}
```

### Lightness-Aware Variant

Quantize lightness separately for more control:

```glsl
uniform float lightnessLevels;  // Separate control for luminance steps

vec3 ditherQuantizeWithLightness(vec3 color, vec2 fragCoord) {
    float threshold = getBayerThreshold(fragCoord);

    // Quantize color channels
    float stepSize = 1.0 / colorLevels;
    vec3 ditheredColor = color + (threshold - 0.5) * stepSize * ditherStrength;
    vec3 quantizedColor = quantize(ditheredColor, colorLevels);

    // Quantize lightness
    float lum = dot(color, vec3(0.299, 0.587, 0.114));
    float lumStep = 1.0 / lightnessLevels;
    float ditheredLum = lum + (threshold - 0.5) * lumStep * ditherStrength;
    float quantizedLum = floor(ditheredLum * lightnessLevels + 0.5) / lightnessLevels;

    // Apply quantized lightness to quantized color
    float currentLum = dot(quantizedColor, vec3(0.299, 0.587, 0.114));
    return quantizedColor * (quantizedLum / max(currentLum, 0.001));
}
```

### 4×4 Bayer (Smaller Pattern)

For more visible dithering pattern:

```glsl
const int BAYER_4X4[16] = int[16](
     0,  8,  2, 10,
    12,  4, 14,  6,
     3, 11,  1,  9,
    15,  7, 13,  5
);

float getBayerThreshold4(vec2 fragCoord) {
    int x = int(mod(fragCoord.x, 4.0));
    int y = int(mod(fragCoord.y, 4.0));
    return float(BAYER_4X4[x + y * 4]) / 16.0;
}
```

## Parameters

| Parameter | Type | Range | Effect |
|-----------|------|-------|--------|
| colorLevels | float | 2–16 | Quantization levels per channel (2=8 colors, 4=64, 8=512) |
| ditherStrength | float | 0–1 | Dithering intensity; 0=hard bands, 1=full dither |
| bayerSize | int | 4 or 8 | Dither matrix size; 4=coarser pattern, 8=finer |
| lightnessLevels | float | 2–16 | Optional separate lightness quantization |

## Audio Mapping Ideas

- **colorLevels** ← Inverse of bass energy: Fewer colors on bass hits for dramatic reduction
- **ditherStrength** ← High frequency energy: More dithering on treble content
- **bayerSize** ← Beat detection: Toggle between 4×4 and 8×8 on beats

## Notes

- Floyd-Steinberg dithering incompatible with fragment shaders (requires sequential error diffusion)
- Ordered dithering works per-pixel independently — fully GPU-parallelizable
- Existing Pixelation effect has basic Bayer dithering; this effect offers dedicated controls without UV quantization
- Consider monochrome mode (single channel quantization) for 1-bit aesthetics
- Pattern scale can be adjusted by dividing fragCoord before mod operation
