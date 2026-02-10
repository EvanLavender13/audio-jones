# Cross-Hatching

A non-photorealistic rendering (NPR) effect that simulates hand-drawn shading using procedural strokes. It replaces smooth gradients with distinct directional lines (hatching) of varying density based on input luminance, often combined with edge detection for a comic-book or sketch aesthetic.

## Classification

- **Category**: STYLE
- **Core Operation**: Maps input luminance to procedural line density/orientation + Sobel edge detection.
- **Pipeline Position**: **Output Stage** (alongside *Toon*, *Pixelation*, *ASCII*). It should likely run *after* Transforms to ensure the hatch lines themselves aren't warped, maintaining the "drawn on top" look, OR *before* Transforms if the goal is to warp the paper itself. Given the "Style" designation, **after Transforms** is standard.

## References

- **User Provided Code (Shadertoy/Matchbox port)**:
    - Demonstrates 4-layer thresholding (`hatch_1` to `hatch_4`).
    - Uses `mod(x + y, density)` for diagonal lines.
    - Includes Sobel edge detection for outlines.
- **Reference 2 (Raymarcher extraction)**:
    - Shows noise-based/organic hatching.
    - Uses `sin` functions combined with coordinate perturbations.
- **Reference 3 ("Sketchy")**:
    - Introduces UV jitter/distortion to simulate hand tremors or rough paper.

## Algorithm

The core algorithm consists of three stages: **Preparation**, **Hatching**, and **Compositing**.

### 1. Preparation
Convert input color to luminance (brightness).
```glsl
// Rec. 709 luma
float get_luma(vec3 c) {
    return dot(c, vec3(0.2126, 0.7152, 0.0722));
}
```

### 2. Procedural Hatching
Instead of sampling textures, we generate lines mathematically using screen coordinates. This ensures infinite resolution and avoids repeating texture artifacts.

**Basic Diagonal Line:**
`if (mod(gl_FragCoord.x + gl_FragCoord.y, density) < width) -> draw_line`

**Stacked Layers:**
We define threshold levels (e.g., 0.8, 0.6, 0.3, 0.15).
- **Layer 1 (Light Shading)**: `mod(x + y, ...)` (Diagonal /)
- **Layer 2 (Medium Shading)**: `mod(x - y, ...)` (Diagonal \)
- **Layer 3 (Dark Shading)**: `mod(x + y - offset, ...)` (Offset /)
- **Layer 4 (Deep Shadow)**: `mod(x - y - offset, ...)` (Offset \)

**Sketchy Jitter (Optional):**
Perturb `gl_FragCoord` before calculating the mod patterns using simple noise or trig functions to simulate a shaky hand.
```glsl
vec2 jitter = vec2(
    sin(uv.y * freq + time), 
    cos(uv.x * freq + time)
) * amount;
```

### 3. Edge Detection (Sobel)
Calculate gradients `gx` and `gy` using a 3x3 kernel on the luminance channel.
`edge_strength = 1.0 - (gx*gx + gy*gy)`
Multiply the hatch result by `edge_strength` to draw black outlines.

## Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| `density` | float | 1.0 - 50.0 | 10.0 | Spacing between hatch lines (pixels). |
| `width` | float | 0.1 - 5.0 | 1.0 | Thickness of individual hatch lines. |
| `threshold`| float | 0.0 - 1.0 | 1.0 | Global sensitivity scaling for the luminance layers. |
| `jitter` | float | 0.0 - 1.0 | 0.0 | Amount of "sketchy" displacement/waviness. |
| `outline` | float | 0.0 - 1.0 | 0.5 | Strength of the Sobel black outlines. |
| `blend` | float | 0.0 - 1.0 | 1.0 | Mix between Original Color (0.0) and Pure Ink (1.0). |

## Audio Mapping Ideas

- **Bass -> Density/Width**: Heavy bass strokes could make the pen press harder (thicker lines) or make the shading denser.
- **High Mids -> Jitter**: High energy causes the "hand" to shake more, creating a chaotic, scribbled look during drops.
- **Volume -> Threshold**: Louder audio lowers the black threshold, filling the screen with ink darkness.

## Notes

- **Anti-aliasing**: The simple `mod` approach creates hard aliased lines. For smoother lines, use `smoothstep` around the edges of the line width instead of a hard `if`.
- **Paper Texture**: Can be combined with a background paper texture (multiply blend) for authenticity, though purely procedural "paper noise" is also possible.
- **Color Mode**: "Colored Pencil" look can be achieved by multiplying the hatch mask with the original image color instead of constant black/grey.
