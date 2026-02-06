# Multi-Scale Grid

Overlapping wavy grids at multiple scales creating a glowing cell/tile pattern. Each grid layer samples the input texture at a different zoom level with independent scroll speeds. Cell edges darken to create boundaries. Produces a "stage floor," "LED wall," or cyberpunk tile aesthetic.

## Classification

- **Type**: Generator or Transform (can sample input texture or generate standalone)
- **Category**: GENERATORS (if standalone) or TRANSFORMS > Graphic (if sampling input)
- **Pipeline Position**: Generator stage or transform stage depending on mode
- **Chosen Approach**: Transform that samples input at multiple scales with grid overlay

## References

- [Shadertoy: On Stage by ocb](https://www.shadertoy.com/view/...) - Original multi-scale grid with light rays

## Algorithm

### Grid Distortion

Creates a wavy/distorted grid by applying sine warp:

```glsl
vec2 grid(vec2 uv, float scale) {
    uv *= scale;
    return uv - warpAmount * 0.5 * sin(uv);
}
```

The sine subtraction creates organic waviness in the grid lines.

### Edge Detection

Find cell boundaries using fractional distance from cell center:

```glsl
vec2 edge(vec2 x) {
    return abs(fract(x) - 0.5);  // 0 at edges, 0.5 at centers
}
```

### Multi-Scale Composition

Three grid layers at different scales (e.g., 10, 25, 50):

```glsl
vec2 a = grid(uv, scale1);  // Coarse grid
vec2 b = grid(uv, scale2);  // Medium grid
vec2 c = grid(uv, scale3);  // Fine grid

// Cell coordinates for texture sampling (with time-based scroll)
vec2 cellA = floor(a) / scale1 + time * scrollSpeed1;
vec2 cellB = floor(b) / scale2 + time * scrollSpeed2;
vec2 cellC = floor(c) / scale3 + time * scrollSpeed3;

// Combined edge pattern
vec2 edges = (edge(a) + edge(b) + edge(c)) / 1.5;
edges = pow(edges, vec2(edgePower));  // Sharpen (edgePower=3 typical)
```

### Texture Sampling and Blending

Sample input texture at each grid scale with different weights:

```glsl
vec4 tex = weight1 * texture(input, cellA)
         + weight2 * texture(input, cellB)
         + weight3 * texture(input, cellC);

// Darken at edges
tex -= edgeContrast * (edges.x + edges.y);

// Optional glow threshold (creates bright cell effect)
tex *= glowAmount * smoothstep(glowThreshold, glowThreshold + 0.8, tex.r) * tex;
```

### Full Shader

```glsl
uniform sampler2D texture0;
uniform float time;
uniform float scale1, scale2, scale3;
uniform float scrollSpeed;
uniform float warpAmount;
uniform float edgeContrast;
uniform float edgePower;
uniform float glowThreshold;
uniform float glowAmount;

vec2 grid(vec2 uv, float s) {
    uv *= s;
    return uv - warpAmount * 0.5 * sin(uv);
}

vec2 edge(vec2 x) {
    return abs(fract(x) - 0.5);
}

void main() {
    vec2 uv = fragTexCoord;
    float t = time * scrollSpeed;

    vec2 a = grid(uv, scale1);
    vec2 b = grid(uv, scale2);
    vec2 c = grid(uv, scale3);

    vec2 cellA = floor(a) / scale1 + t;
    vec2 cellB = floor(b) / scale2 + t * 0.7;
    vec2 cellC = floor(c) / scale3 + t * 1.3;

    vec2 edges = (edge(a) + edge(b) + edge(c)) / 1.5;
    edges = pow(edges, vec2(edgePower));

    vec4 tex = 0.8 * texture(texture0, cellA)
             + 0.6 * texture(texture0, cellB)
             + 0.4 * texture(texture0, cellC);

    tex -= edgeContrast * (edges.x + edges.y);
    tex *= glowAmount * smoothstep(glowThreshold, glowThreshold + 0.8, tex.r) * tex;

    finalColor = tex;
}
```

## Parameters

| Parameter | Type | Range | Default | Visual Effect |
|-----------|------|-------|---------|---------------|
| scale1 | float | 5-20 | 10.0 | Coarse grid density |
| scale2 | float | 15-40 | 25.0 | Medium grid density |
| scale3 | float | 30-80 | 50.0 | Fine grid density |
| scrollSpeed | float | 0.0-0.1 | 0.005 | Base drift rate |
| warpAmount | float | 0.0-1.0 | 0.5 | Sine distortion on grid lines |
| edgeContrast | float | 0.0-0.5 | 0.2 | Cell boundary darkness |
| edgePower | float | 1.0-5.0 | 3.0 | Edge sharpness |
| glowThreshold | float | 0.5-1.5 | 1.0 | Brightness cutoff for glow |
| glowAmount | float | 1.0-3.0 | 2.0 | Glow intensity multiplier |

## Modulation Candidates

- **scrollSpeed**: Vary drift rate with energy
- **warpAmount**: Increase distortion dynamically
- **glowThreshold**: Pulse glow with bass
- **edgeContrast**: Sharpen/soften grid boundaries

## Implementation Notes

- Samples input texture, so requires content from accumulation or previous stage
- The three scroll speed multipliers (1.0, 0.7, 1.3) create parallax depth
- Edge power of 3 (`edges = pow(edges, 3)`) creates sharp cell boundaries
- Glow formula `tex *= smoothstep(...) * tex` squares the result, creating contrast
- Works well combined with god rays (radial streak with movable center) for stage lighting effect
- Consider adding blend mode parameter to composite over input vs replace

## Visual Characteristics

- Cyberpunk / synthwave aesthetic
- Glowing cells on dark background
- Multi-layer parallax from different scroll speeds
- Organic waviness from sine-warped grid
- Works best with colorful, high-contrast input
