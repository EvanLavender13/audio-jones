# Gradient Flow Warp

Iteratively displaces UV coordinates perpendicular to image edges, creating streamline patterns that follow the structure of the image. Unlike Texture Warp (which flows based on color values), Gradient Flow follows luminance contours - producing wood-grain, magnetic field, or topographic map aesthetics.

## Classification

- **Category**: TRANSFORMS → Warp
- **Core Operation**: Sobel gradient → rotate 90° → iterative UV displacement
- **Pipeline Position**: Output stage, transforms (user order)

## References

- [Daniel Ilett - Edge Detection Tutorial](https://danielilett.com/2019-05-11-tut1-4-smo-edge-detect/) - Sobel operator and gradient computation
- [Shadertoy - Edge Tangent Flow](https://www.shadertoy.com/view/tdBXR1) - Reference (blocked, but concept documented)
- `shaders/toon.fs:72-89` - Existing Sobel implementation
- `shaders/texture_warp.fs` - Iterative warp pattern to follow

## Algorithm

### Core Concept

The Sobel operator gives you the gradient direction - where luminance changes most rapidly. Flowing **perpendicular** to this gradient means flowing along edges (tangent to contours), not across them.

```
Gradient points "uphill" (toward brighter)
Perpendicular flows "along the contour line"
```

### Step 1: Compute Sobel Gradient

```glsl
vec2 texel = 1.0 / resolution;

// Sample 3x3 neighborhood
float tl = luminance(texture(texture0, uv + vec2(-texel.x, -texel.y)).rgb);
float t  = luminance(texture(texture0, uv + vec2(    0.0, -texel.y)).rgb);
float tr = luminance(texture(texture0, uv + vec2( texel.x, -texel.y)).rgb);
float l  = luminance(texture(texture0, uv + vec2(-texel.x,     0.0)).rgb);
float r  = luminance(texture(texture0, uv + vec2( texel.x,     0.0)).rgb);
float bl = luminance(texture(texture0, uv + vec2(-texel.x,  texel.y)).rgb);
float b  = luminance(texture(texture0, uv + vec2(    0.0,  texel.y)).rgb);
float br = luminance(texture(texture0, uv + vec2( texel.x,  texel.y)).rgb);

// Sobel gradients
float gx = (tr + 2.0*r + br) - (tl + 2.0*l + bl);
float gy = (tl + 2.0*t + tr) - (bl + 2.0*b + br);
```

### Step 2: Rotate to Perpendicular

```glsl
vec2 gradient = vec2(gx, gy);

// Rotate 90 degrees: perpendicular to gradient = tangent to edges
vec2 tangent = vec2(-gradient.y, gradient.x);

// Normalize to get direction only (magnitude controlled by strength)
vec2 flow = length(gradient) > 0.001 ? normalize(tangent) : vec2(0.0);
```

### Step 3: Iterative Displacement

```glsl
vec2 warpedUV = fragTexCoord;

for (int i = 0; i < iterations; i++) {
    // Recompute gradient at current position
    vec2 flow = computeFlowAt(warpedUV);

    // Displace along tangent
    warpedUV += flow * strength;
}

finalColor = texture(texture0, clamp(warpedUV, 0.0, 1.0));
```

### Variation: Gradient-Magnitude Weighting

Flow faster near strong edges:

```glsl
float edgeStrength = length(gradient);
warpedUV += flow * strength * edgeStrength;
```

### Variation: Bidirectional Flow

Add parameter to flow along gradient (across edges) vs perpendicular (along edges):

```glsl
// flowAngle: 0 = along edges, PI/2 = across edges
float s = sin(flowAngle);
float c = cos(flowAngle);
vec2 rotatedFlow = vec2(c * tangent.x - s * gradient.x,
                        c * tangent.y - s * gradient.y);
```

## Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| strength | float | 0.0 - 0.1 | 0.01 | Displacement per iteration |
| iterations | int | 1 - 32 | 8 | Cascade depth |
| flowAngle | float | 0 - π | 0 | 0 = tangent to edges, π/2 = across edges |
| edgeWeighted | bool | - | true | Scale displacement by edge strength |

## Comparison with Texture Warp

| Aspect | Texture Warp | Gradient Flow |
|--------|--------------|---------------|
| Flow direction | RG channel values | Perpendicular to luminance gradient |
| What drives it | Color content | Edge structure |
| Emergent pattern | Terrain-like ridges | Streamlines, wood grain |
| On flat color | No effect | No effect |
| On sharp edges | Crosses edges | Follows edges |

## Audio Mapping Ideas

| Parameter | Audio Source | Behavior |
|-----------|--------------|----------|
| strength | Bass energy | Stronger flow on beats |
| flowAngle | Slow LFO or spectrum centroid | Shift between edge-following and edge-crossing |
| iterations | Overall energy | More iterations = longer trails |

## Notes

- Computationally heavier than Texture Warp: Sobel requires 9 samples per iteration vs 1.
- Consider pre-computing gradient to a texture for performance (but loses self-referential property).
- Combines interestingly with Texture Warp: run both in sequence for complex emergent structures.
- Edge-weighted mode creates "faster flow" near edges, "stagnant" in flat areas.
