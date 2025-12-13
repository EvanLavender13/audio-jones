# Voronoi Shader Research

Background research for Voronoi post-processing effect.

## Core Concept

Voronoi diagrams partition a 2D plane into cells. Each cell contains all points closer to its seed than to any other seed. GPU implementations compute this per-pixel by finding the nearest feature point from a set.

The technique falls under "cellular noise" - distance fields computed against multiple feature points rather than continuous noise functions.

## Algorithm Variants

### Brute Force

Iterate all seed points, find minimum distance. O(n) per pixel where n = seed count.

```glsl
float minDist = 100.0;
for (int i = 0; i < NUM_SEEDS; i++) {
    float d = distance(uv, seeds[i]);
    minDist = min(minDist, d);
}
```

**Limitation:** Seed count must be constant (GLSL requires fixed loop bounds). Performance degrades linearly with seed count.

### Tile-Based (Worley Noise)

Subdivide space into tiles. Place one pseudo-random point per tile. Check only 3x3 neighborhood (9 distance calculations regardless of total cell count).

```glsl
vec2 i_st = floor(uv * scale);  // Tile index
vec2 f_st = fract(uv * scale);  // Position within tile

for (int y = -1; y <= 1; y++) {
    for (int x = -1; x <= 1; x++) {
        vec2 neighbor = vec2(float(x), float(y));
        vec2 point = hash2(i_st + neighbor);
        // ... distance check
    }
}
```

**Advantage:** O(1) per pixel - constant 9 iterations regardless of cell count. Scale parameter controls density.

### 2x2 Optimization (Gustavson 2011)

Reduces neighborhood to 2x2 (4 iterations). Creates discontinuity artifacts at tile edges where the closest point lies outside the checked region.

**Trade-off:** 2.25x faster but visible seams. Not recommended for smooth animation.

## Hash Functions

Tile-based approach requires deterministic pseudo-random point positions. Common hash functions:

### Sine-Based Hash

```glsl
vec2 hash2(vec2 p) {
    p = vec2(dot(p, vec2(127.1, 311.7)),
             dot(p, vec2(269.5, 183.3)));
    return fract(sin(p) * 43758.5453);
}
```

Fast but may show patterns on some GPUs due to sin() precision.

### Integer Hash

```glsl
uvec2 hash2(uvec2 p) {
    p = p * 1664525u + 1013904223u;
    p.x += p.y * 1664525u;
    p.y += p.x * 1664525u;
    return p;
}
```

More uniform distribution, requires uint support (GLSL 330+).

## Output Modes

### Distance Field

Output raw minimum distance. Creates gradient from cell centers outward.

```glsl
color = vec3(m_dist);
```

### Cell ID / Coloring

Hash the winning point to generate per-cell color.

```glsl
color = hash3(m_point);  // RGB from point position
```

### Edge Detection

Track both closest and second-closest distances. Difference approaches zero at cell boundaries.

```glsl
float edge = m_dist2 - m_dist;
color = vec3(smoothstep(0.0, 0.05, edge));
```

### Crackle / Inverted Edges

Invert edge detection for dark lines on light background.

```glsl
color = vec3(1.0 - smoothstep(0.0, 0.05, edge));
```

## Animation Techniques

### Point Oscillation

Modulate point positions with sine waves. Each point oscillates on its own phase (determined by hash).

```glsl
point = 0.5 + 0.5 * sin(time * speed + TWO_PI * point);
```

Creates organic flowing movement. Speed controls rate; TWO_PI factor ensures full oscillation range.

### Scale Breathing

Modulate cell scale over time. Cells grow and shrink.

```glsl
float animScale = scale * (1.0 + 0.3 * sin(time * 0.5));
vec2 st = uv * animScale;
```

### Rotation

Rotate UV space before Voronoi calculation. Pattern spins around center.

```glsl
float angle = time * 0.1;
uv = mat2(cos(angle), -sin(angle), sin(angle), cos(angle)) * uv;
```

## Distance Metrics

Standard Euclidean distance produces circular cells. Alternative metrics create different aesthetics:

| Metric | Formula | Cell Shape |
|--------|---------|------------|
| Euclidean | `length(diff)` | Circular |
| Manhattan | `abs(diff.x) + abs(diff.y)` | Diamond |
| Chebyshev | `max(abs(diff.x), abs(diff.y))` | Square |
| Minkowski | `pow(pow(abs(diff.x),p) + pow(abs(diff.y),p), 1/p)` | Parameterized |

Manhattan creates grid-aligned patterns. Chebyshev creates blocky cells.

## Blend Modes for Compositing

When overlaying Voronoi on existing content:

| Mode | Formula | Effect |
|------|---------|--------|
| Replace | `mix(original, voronoi, intensity)` | Crossfade |
| Multiply | `original * voronoi` | Darkens through pattern |
| Screen | `1 - (1-original) * (1-voronoi)` | Brightens without clipping |
| Overlay | Conditional multiply/screen | Preserves midtones |
| Add | `original + voronoi * intensity` | Glowing overlay |

Replace/mix provides most control via intensity parameter.

## Performance Considerations

### Texture Passes

Each Voronoi application requires one full-screen texture pass. Current AudioJones pipeline runs 5 passes; Voronoi adds 1.

### ALU Cost

Per-pixel: 9 hash calculations + 9 distance calculations + 9 comparisons. Comparable to 5-tap Gaussian blur.

### Branching

Avoid dynamic branching in inner loop. Use `min()` instead of `if`:

```glsl
// Avoid
if (dist < m_dist) { m_dist = dist; }

// Prefer (for simple distance field)
m_dist = min(m_dist, dist);
```

Edge detection requires tracking two distances, making conditional unavoidable.

### Resolution Independence

Scale parameter controls cell count. Shader operates in UV space (0-1). Resolution uniform only needed if edge width should be pixel-based rather than UV-based.

## Advanced Variations

### Voro-Noise (Quilez 2014)

Blends Voronoi distance field with value noise. Treats both as special cases of grid-based pattern generation.

### Voronoi Borders (Quilez 2012)

Precise edge rendering using gradient of distance field. More accurate than closest/second-closest difference but more expensive.

### 3D Voronoi

Extends to 3D space with 27-cell neighborhood (3x3x3). UV + time creates animated 2D slice through 3D Voronoi volume.

## Sources

- [The Book of Shaders - Chapter 12: Cellular Noise](https://thebookofshaders.com/12/)
- [Nullprogram - A GPU Approach to Voronoi Diagrams](https://nullprogram.com/blog/2014/06/01/)
- [libretro/glsl-shaders voronoi.glsl](https://github.com/libretro/glsl-shaders/blob/master/borders/resources/voronoi.glsl)
- [CZDomain/glsl-perlin-voronoi](https://github.com/CZDomain/glsl-perlin-voronoi)
- [Shadertoy - Voronoi basic (MslGD8)](https://www.shadertoy.com/view/MslGD8)
- [Shadertoy - Voronoi smooth (ldB3zc)](https://www.shadertoy.com/view/ldB3zc)
- [Hugo Peters - Voronoi Shading the Web](https://hugopeters.me/posts/19/)

## Rejected Approaches

### Depth Buffer Method

Render cones at seed positions; depth buffer naturally selects nearest. Requires geometry pipeline, incompatible with post-processing architecture.

### Jump Flooding Algorithm

GPU-parallel Voronoi via iterative propagation. Requires multiple passes with halving step sizes. Overkill for visual effect where approximate edges suffice.

### Compute Shader

Thread cooperation enables faster 3D Voronoi. AudioJones uses fragment shaders exclusively; adding compute would complicate pipeline.
