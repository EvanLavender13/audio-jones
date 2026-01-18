# Hierarchical Voronoi

Nested cellular pattern where Voronoi cells recursively subdivide into smaller cells, creating fractal-like structures that mimic natural hierarchies like leaf vein networks, cracked mud with micro-fractures, or nested bubble clusters.

## Classification

- **Category**: TRANSFORMS → Cellular
- **Core Operation**: Standard Voronoi with conditional recursive subdivision based on per-cell random level assignment
- **Pipeline Position**: Transform stage (user-ordered), samples input texture

## References

- [Understanding Cellular Noise Variations](https://sangillee.com/2025-04-18-cellular-noises/) - Hierarchical Voronoi algorithm with GLSL approach
- [The Book of Shaders - Cellular Noise](https://thebookofshaders.com/12/) - Foundation for tile-based Voronoi
- [Shadertoy - Hierarchical Voronoi](https://www.shadertoy.com/view/Xll3zX) - Reference implementation (user must paste code)

## Algorithm

### Core Concept

Standard Voronoi assigns each pixel to its nearest seed point. Hierarchical Voronoi extends this by:
1. Randomly assigning each cell a "level" (0, 1, or 2)
2. Level 0 cells compute distance normally
3. Level 1+ cells subdivide their grid into 2×2 sub-grids, each with its own seed
4. Higher levels subdivide further (4×4, etc.)

### Level Assignment

```glsl
float level = rand3(gridPos).z;  // Random per grid cell
// level < 0.5: no subdivision (level 0)
// level < 0.75: subdivide once (level 1)
// level >= 0.75: subdivide twice (level 2)
```

### Subdivision Logic

GLSL lacks recursion, so subdivision requires nested loops:

```glsl
vec2 hierarchicalVoronoi(vec2 uv, float scale) {
    vec2 p = uv * scale;
    vec2 gridPos = floor(p);
    vec2 fracPos = fract(p);

    float minDist = 1e10;
    vec2 closestCell = vec2(0.0);

    // Check 3x3 neighborhood
    for (int y = -1; y <= 1; y++) {
        for (int x = -1; x <= 1; x++) {
            vec2 neighbor = vec2(x, y);
            vec2 cell = gridPos + neighbor;
            vec3 cellRand = hash3(cell);
            float level = cellRand.z;

            if (level < 0.5) {
                // Level 0: single seed per cell
                vec2 seed = neighbor + cellRand.xy - fracPos;
                float d = dot(seed, seed);
                if (d < minDist) {
                    minDist = d;
                    closestCell = cell;
                }
            } else {
                // Level 1+: subdivide into 2x2
                for (int sy = 0; sy < 2; sy++) {
                    for (int sx = 0; sx < 2; sx++) {
                        vec2 subCell = cell * 2.0 + vec2(sx, sy);
                        vec3 subRand = hash3(subCell);
                        vec2 subOffset = (neighbor * 2.0 + vec2(sx, sy) + subRand.xy) / 2.0;
                        vec2 seed = subOffset - fracPos;
                        float d = dot(seed, seed);
                        if (d < minDist) {
                            minDist = d;
                            closestCell = subCell;
                        }
                    }
                }
            }
        }
    }

    return vec2(sqrt(minDist), hash3(closestCell).x);
}
```

### UV Transform Application

```glsl
// Sample input texture based on cell assignment
vec2 cellCenter = getCellCenter(closestCell);
vec2 warpedUV = mix(uv, cellCenter, warpStrength);
vec4 color = texture(inputTexture, warpedUV);
```

## Parameters

| Parameter | Type | Range | Effect |
|-----------|------|-------|--------|
| Scale | float | 2.0 - 64.0 | Base grid density (cells across screen) |
| Max Levels | int | 1 - 3 | Maximum subdivision depth |
| Subdivision Threshold | float | 0.0 - 1.0 | Probability of cell subdividing (lower = more uniform) |
| Warp Strength | float | 0.0 - 1.0 | How much UV pulls toward cell center |
| Edge Width | float | 0.0 - 0.1 | Width of cell boundary lines |
| Edge Color | vec3 | RGB | Color of cell boundaries |

## Modulation Candidates

| Parameter | Audio Feature | Behavior |
|-----------|---------------|----------|
| Scale | Bass energy | Cells grow/shrink with bass |
| Subdivision Threshold | Mid energy | More subdivisions during busy sections |
| Warp Strength | Beat | Pulse warp on beat |
| Edge Width | High energy | Edges thicken with treble |

## Notes

- GLSL lacks recursion, so max levels must be hardcoded via nested loops
- Each additional level quadruples the loop iterations (3×3 → 3×3×4 → 3×3×4×4)
- Consider limiting to 2 levels for real-time performance
- Hash function must be high-quality to avoid visible grid artifacts at subdivision boundaries
- Cell center calculation requires tracking which level the closest seed came from
