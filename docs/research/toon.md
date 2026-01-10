# Toon

Cartoon-style post-processing that quantizes luminance to discrete flat color bands and renders edge-detected outlines. Optional noise-varied edge thickness creates an organic, brush-stroke appearance.

## Classification

- **Category**: TRANSFORMS → Style (alongside Pixelation and Glitch)
- **Core Operation**: Samples input texture, quantizes luminance to discrete bands, detects edges via Sobel convolution, overlays black outlines
- **Pipeline Position**: Reorderable transform

## References

- [Sobel Edge Detection GLSL](https://gist.github.com/Hebali/6ebfc66106459aacee6a9fac029d0115) - Complete fragment shader with 3×3 kernel sampling
- [Posterization Tutorial](https://lettier.github.io/3d-game-shaders-for-beginners/posterization.html) - Luminance-based color quantization formula
- [Sobel Filter Math](https://jameshfisher.com/2020/08/31/edge-detection-with-sobel-filters/) - Kernel matrices and magnitude calculation
- [Cartoon Effect Shader](https://www.codeproject.com/Articles/94817/Pixel-Shader-for-Edge-Detection-and-Cartoon-Effect) - Combined edge detection + posterization
- [Brush Stroke Rendering](https://gpuhacks.wordpress.com/2012/01/30/brush-strokes/) - Distance field approach for organic edges

## Algorithm

### Luminance Posterization

Quantizes brightness to discrete levels while preserving hue. Uses max RGB as grayscale metric (faster than weighted luminance, similar visual result).

```glsl
// Compute grayscale from max RGB component
float greyscale = max(color.r, max(color.g, color.b));

// Find nearest quantization level
float lower = floor(greyscale * levels) / levels;
float upper = ceil(greyscale * levels) / levels;

float lowerDiff = abs(greyscale - lower);
float upperDiff = abs(upper - greyscale);

float level = lowerDiff <= upperDiff ? lower : upper;

// Scale RGB proportionally to preserve hue
float adjustment = level / max(greyscale, 0.001);  // avoid div by zero
color.rgb *= adjustment;
```

### Sobel Edge Detection

Computes image gradients using 3×3 convolution kernels. Edge magnitude indicates outline strength.

**Horizontal Kernel (Gx):**
```
| -1  0  +1 |
| -2  0  +2 |
| -1  0  +1 |
```

**Vertical Kernel (Gy):**
```
| +1  +2  +1 |
|  0   0   0 |
| -1  -2  -1 |
```

```glsl
// Sample 3x3 neighborhood
vec2 texel = 1.0 / resolution;
vec4 n[9];
n[0] = texture(tex, uv + vec2(-texel.x, -texel.y));
n[1] = texture(tex, uv + vec2(    0.0, -texel.y));
n[2] = texture(tex, uv + vec2( texel.x, -texel.y));
n[3] = texture(tex, uv + vec2(-texel.x,     0.0));
n[4] = texture(tex, uv);
n[5] = texture(tex, uv + vec2( texel.x,     0.0));
n[6] = texture(tex, uv + vec2(-texel.x,  texel.y));
n[7] = texture(tex, uv + vec2(    0.0,  texel.y));
n[8] = texture(tex, uv + vec2( texel.x,  texel.y));

// Apply Sobel kernels
vec4 sobelH = n[2] + 2.0*n[5] + n[8] - (n[0] + 2.0*n[3] + n[6]);
vec4 sobelV = n[0] + 2.0*n[1] + n[2] - (n[6] + 2.0*n[7] + n[8]);

// Edge magnitude
float edge = length(sqrt(sobelH * sobelH + sobelV * sobelV).rgb);
```

### Edge Thresholding and Rendering

Convert continuous edge magnitude to binary outline:

```glsl
// Threshold to binary edge
float outline = smoothstep(threshold - softness, threshold + softness, edge);

// Mix outline color with posterized image
vec3 result = mix(posterizedColor, outlineColor, outline);
```

### Noise-Varied Edge Thickness (Optional Brush Stroke Effect)

Vary the edge detection sample distance using noise to create organic, brush-like outlines:

```glsl
// Noise-based thickness variation
float noise = gnoise(vec3(uv * noiseScale, time * 0.1));
float thicknessMultiplier = 1.0 + noise * thicknessVariation;

vec2 texel = thicknessMultiplier / resolution;
// Use this texel for the 3x3 sampling instead of fixed 1.0/resolution
```

Alternative approach: vary the edge threshold based on noise:

```glsl
float localThreshold = threshold + noise * thresholdVariation;
float outline = smoothstep(localThreshold - softness, localThreshold + softness, edge);
```

This creates thicker lines in some areas and thinner in others, mimicking hand-drawn strokes.

## Parameters

| Parameter | Type | Range | Effect |
|-----------|------|-------|--------|
| **Posterization** ||||
| levels | int | 2-16 | Color quantization bands. Lower = flatter, more cartoon-like |
| **Edge Detection** ||||
| edgeThreshold | float | 0.0-1.0 | Edge sensitivity. Lower = more outlines detected |
| edgeSoftness | float | 0.0-0.2 | Smoothstep transition width. 0 = hard edges |
| outlineColor | vec3 | RGB | Outline color (typically black: 0,0,0) |
| **Brush Stroke (Optional)** ||||
| thicknessVariation | float | 0.0-1.0 | How much noise affects edge sample distance |
| noiseScale | float | 1-20 | Frequency of thickness variation |

## Audio Mapping Ideas

- **levels** <- beat trigger: snap to fewer levels on beat, smoothly increase between beats (dramatic flat shading on hits)
- **edgeThreshold** <- high freq energy: more treble = more detailed outlines
- **thicknessVariation** <- bass energy: bass makes outlines more organic/wobbly
- **outlineColor** <- could cycle hue or pulse brightness with beat

## Notes

- Sobel has slight directional bias: diagonal edges report ~33% stronger than orthogonal. Acceptable for artistic effect.
- Order matters: posterize first, then detect edges. Posterizing after edge detection softens the outlines.
- Edge detection on very dark areas may produce weak outlines. Consider boosting contrast before processing or using luminance-weighted edge detection.
- FXAA pass after this effect smooths jaggy outlines if needed.
- For performance, consider computing edge magnitude from grayscale rather than per-channel.
- The brush stroke effect via noise is a simplified approximation. True painterly strokes require multi-pass distance field computation (see GPU Hacks reference), but the noise approach achieves 80% of the visual effect in a single pass.
