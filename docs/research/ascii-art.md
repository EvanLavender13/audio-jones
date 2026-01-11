# ASCII Art

Converts the image into a grid of text characters where darker regions display denser characters (@, #, 8) and lighter regions display sparser ones (., :, space). Creates a retro terminal aesthetic.

## Classification

- **Category**: TRANSFORMS → Style
- **Core Operation**: Sample luminance per cell, render character bitmap based on brightness
- **Pipeline Position**: Output stage transforms (user-ordered with other Style effects)

## References

- [Codrops ASCII Shader](https://tympanus.net/codrops/2024/11/13/creating-an-ascii-shader-using-ogl/) - Luminance thresholds, bit-packed character encoding
- [glsl-ascii-filter](https://github.com/mattdesl/glsl-ascii-filter) - Clean API with pixelSize parameter
- [Ian Parberry ASCII Shader](https://ianparberry.com/art/ascii/shader/) - Texture atlas approach with 8 characters

## Algorithm

### Cell Division

Divide screen into fixed-size cells (typically 8×8 or 16×16 pixels):

```glsl
vec2 cellSize = vec2(8.0) / resolution;
vec2 cellUV = floor(uv / cellSize) * cellSize;
```

### Luminance Calculation

Sample input at cell center, convert to grayscale:

```glsl
vec3 color = texture(inputTex, cellUV + cellSize * 0.5).rgb;
float lum = dot(color, vec3(0.299, 0.587, 0.114));
```

### Approach A: Procedural Characters (Bit-Packed)

Encode 5×5 character bitmaps as integers. Each bit represents one pixel:

```glsl
int getChar(float gray) {
    if (gray > 0.8) return 11512810;  // #
    if (gray > 0.7) return 13195790;  // @
    if (gray > 0.6) return 15252014;  // 8
    if (gray > 0.5) return 13121101;  // &
    if (gray > 0.4) return 15255086;  // o
    if (gray > 0.3) return 163153;    // *
    if (gray > 0.2) return 65600;     // :
    return 0;                          // space
}

float character(int n, vec2 p) {
    p = floor(p * vec2(4.0, 5.0));
    if (clamp(p, 0.0, vec2(3.0, 4.0)) != p) return 0.0;
    int idx = int(p.x) + int(p.y) * 4;
    return float((n >> idx) & 1);
}
```

### Approach B: Texture Atlas

Pre-render characters to a texture strip, index by luminance:

```glsl
// Characters arranged darkest to lightest: M N V F $ I * : .
int charIndex = int(lum * 8.0);
vec2 charUV = fract(uv / cellSize);
charUV.x = (charUV.x + float(charIndex)) / 8.0;
float mask = texture(charAtlas, charUV).r;
```

### Output

Multiply original color by character mask, or use fixed foreground/background:

```glsl
// Colored ASCII
gl_FragColor = vec4(color * mask, 1.0);

// Classic green-on-black
vec3 fg = vec3(0.0, 1.0, 0.0);
vec3 bg = vec3(0.0, 0.05, 0.0);
gl_FragColor = vec4(mix(bg, fg, mask), 1.0);
```

## Parameters

| Parameter | Type | Range | Effect |
|-----------|------|-------|--------|
| cellSize | int | 4-32 | Character cell size in pixels |
| charSet | enum | 8/16/custom | Number of luminance levels |
| colorMode | enum | original/mono/custom | How to color the output |
| foreground | vec3 | RGB | Foreground color (mono mode) |
| background | vec3 | RGB | Background color (mono mode) |
| invert | bool | - | Swap light/dark character mapping |

## Audio Mapping Ideas

- **cellSize** ← bass intensity (larger cells on heavy bass)
- **foreground hue** ← dominant frequency band
- **invert** ← beat trigger toggle

## Notes

- Procedural approach requires no texture loading but limits character fidelity to 5×5
- Texture atlas allows authentic fonts but requires asset management
- Consider CRT scanline overlay for enhanced retro feel
- Cell size affects performance linearly (fewer cells = fewer texture samples)
