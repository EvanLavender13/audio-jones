# Phyllotaxis

Cell-based effect using sunflower seed spiral patterns. Finds nearest seed/floret in a phyllotaxis arrangement, applies Voronoi-style effects (UV distortion, fill, iso rings, glow) to create organic flower-like visuals.

## Classification

- **Category**: TRANSFORMS → Cellular
- **Core Operation**: Find nearest phyllotaxis seed, apply effects based on distance/direction to seed center
- **Pipeline Position**: Reorderable transform (alongside Voronoi, Lattice Fold)

## References

- [Jason Davies: Sunflower Phyllotaxis](https://www.jasondavies.com/sunflower-phyllotaxis/) - Interactive visualization of Vogel's model: `r = c√n`, `θ = n × 137.5°`. Shows how angle changes dramatically alter the pattern.
- [GM Shaders Mini: Phi](https://mini.gmshaders.com/p/phi) - GLSL implementation: `vec2 dir = cos(i * G_A + vec2(0, HPI)); float rad = sqrt(i/n) * r;`
- [Wikipedia: Golden Angle](https://en.wikipedia.org/wiki/Golden_angle) - Mathematical definition: 137.5077...° or 2.39996... radians.
- [Shadertoy: Phyllotaxis visualization](https://www.shadertoy.com/view/4d2Xzw) - Loop-based nearest-seed finding with per-seed coloring (code provided by user).

## Core Algorithm

### Phyllotaxis Fundamentals

Sunflower seeds follow Vogel's model (1979):
- **Radius**: `r = c * sqrt(n)` — seeds spread outward as square root of index
- **Angle**: `θ = n * goldenAngle` — each seed rotated by ≈137.5° from previous

The golden angle ensures no two seeds align radially, creating optimal packing. The visible spiral "arms" are emergent — seeds with indices differing by Fibonacci numbers (13, 21, 34) visually align.

### Constants

```glsl
#define PHI 1.618033988749
#define GOLDEN_ANGLE 2.39996322972865  // radians, ≈137.5°
#define PI 3.14159265359
#define TWO_PI 6.28318530718
```

### Seed Position

```glsl
vec2 seedPosition(float i, float scale, float angle) {
    float r = scale * sqrt(i);
    float theta = i * angle;
    return r * vec2(cos(theta), sin(theta));
}
```

### Finding the Nearest Seed

Unlike Voronoi (grid-based lookup), phyllotaxis requires searching seed positions. Optimization: estimate starting index from radius, search a narrow range.

```glsl
void findNearestSeed(vec2 p, float scale, float angle, int maxSeeds,
                     out float nearestDist, out float nearestIndex, out vec2 nearestPos) {
    nearestDist = 999.0;
    nearestIndex = 0.0;
    nearestPos = vec2(0.0);

    // Estimate starting index from radius: n ≈ (r/scale)²
    float r = length(p);
    float estimatedN = (r / scale) * (r / scale);

    // Search window around estimated index
    int searchRadius = 20;
    int startIdx = max(0, int(estimatedN) - searchRadius);
    int endIdx = min(maxSeeds, int(estimatedN) + searchRadius);

    for (int i = startIdx; i <= endIdx; i++) {
        vec2 seedPos = seedPosition(float(i), scale, angle);
        float dist = length(p - seedPos);
        if (dist < nearestDist) {
            nearestDist = dist;
            nearestIndex = float(i);
            nearestPos = seedPos;
        }
    }
}
```

### Cell Geometry

```glsl
struct PhylloCell {
    float dist;       // Distance to seed center
    float index;      // Seed index (for animation phase, coloring)
    vec2 pos;         // Seed position
    vec2 toCenter;    // Vector from pixel to seed center (normalized)
};

PhylloCell getCell(vec2 uv, float scale, float angle, int maxSeeds) {
    PhylloCell cell;
    vec2 centered = uv - 0.5;

    findNearestSeed(centered, scale, angle, maxSeeds,
                    cell.dist, cell.index, cell.pos);

    cell.toCenter = normalize(cell.pos - centered);
    return cell;
}
```

## Shared Parameters

| Parameter | Type | Range | Effect |
|-----------|------|-------|--------|
| `scale` | float | [0.02, 0.15] | Seed spacing. Smaller = more seeds, finer detail. |
| `divergenceAngle` | float | [2.0, 2.8] | Angle between seeds. 2.4 = golden angle. |
| `maxSeeds` | int | [50, 500] | Maximum seeds to generate |
| `animationSpeed` | float | [0.0, 2.0] | Per-cell animation rate |

### Divergence Angle Reference

| Angle (radians) | Angle (degrees) | Visual Result |
|-----------------|-----------------|---------------|
| 2.39996 | 137.5° | Golden angle: classic sunflower, optimal packing |
| 2.22 | 127° | 8 visible spokes |
| 2.51 | 144° | 5 visible spokes |
| 1.57 | 90° | 4 radial spokes, cross pattern |
| 3.14 | 180° | 2 spokes, bilateral split |

## Sub-Effects

Modeled after Voronoi's multi-effect structure. Compute geometry once, apply blendable effects.

### UV Distortion

Displace UV toward or away from seed center.

```glsl
if (uvDistortIntensity > 0.0) {
    vec2 displacement = cell.toCenter * uvDistortIntensity * 0.1;
    finalUV = uv + displacement;
}
```

| Parameter | Type | Range | Effect |
|-----------|------|-------|--------|
| `uvDistortIntensity` | float | [0.0, 1.0] | Displacement magnitude toward seed centers |

### Flat Fill (Stained Glass)

Sample color from seed center, fill cell.

```glsl
if (flatFillIntensity > 0.0) {
    vec2 seedUV = cell.pos + 0.5;
    vec3 seedColor = texture(input, seedUV).rgb;
    float fillMask = smoothstep(cellRadius, cellRadius * 0.5, cell.dist);
    color = mix(color, seedColor, fillMask * flatFillIntensity);
}
```

| Parameter | Type | Range | Effect |
|-----------|------|-------|--------|
| `flatFillIntensity` | float | [0.0, 1.0] | Stained glass cell fill amount |
| `cellRadius` | float | [0.01, 0.1] | Size of filled region per cell |

### Center Iso Rings

Concentric rings radiating from seed centers.

```glsl
if (centerIsoIntensity > 0.0) {
    float phase = cell.dist * isoFrequency;
    float rings = abs(sin(phase * TWO_PI * 0.5));
    vec3 seedColor = texture(input, cell.pos + 0.5).rgb;
    color = mix(color, seedColor, rings * centerIsoIntensity);
}
```

| Parameter | Type | Range | Effect |
|-----------|------|-------|--------|
| `centerIsoIntensity` | float | [0.0, 1.0] | Ring visibility |
| `isoFrequency` | float | [2.0, 20.0] | Ring density |

### Edge Glow

Glow at cell boundaries (where distance from center is large).

```glsl
if (edgeGlowIntensity > 0.0) {
    float edgeness = smoothstep(cellRadius * 0.3, cellRadius, cell.dist);
    color += edgeness * edgeGlowIntensity;
}
```

| Parameter | Type | Range | Effect |
|-----------|------|-------|--------|
| `edgeGlowIntensity` | float | [0.0, 1.0] | Glow brightness at cell boundaries |

### Index-Based Animation

Use seed index to vary animation phase per cell.

```glsl
float phase = time + cell.index * 0.1;
float pulse = 0.5 + 0.5 * sin(phase);
// Apply pulse to any effect intensity
```

## Modulation Candidates

- **divergenceAngle**: At golden angle (2.4 rad), optimal packing with no visible spokes. Slightly off creates Fibonacci-number spoke patterns (5, 8, 13, 21 spokes). Dramatically changes the overall structure.
- **scale**: Controls seed density. Smaller = more cells = finer flower-like detail.
- **uvDistortIntensity**: Creates organic flowing distortion toward seed centers.
- **Effect intensities**: Blend between clean image, filled cells, ringed patterns, glowing edges.

## Notes

### Comparison to Voronoi

| Aspect | Voronoi | Phyllotaxis |
|--------|---------|-------------|
| Cell centers | Random (hash-based) | Deterministic (formula) |
| Cell shapes | Irregular polygons | Roughly circular, size varies with radius |
| Lookup | 3×3 grid search (fast) | Index-range search (slower) |
| Visual character | Cracked mud, cells | Sunflower, flower head, organic spiral |

### Performance Considerations

- **Search radius**: The optimized search checks ~40 seeds per pixel. Increase `searchRadius` if cells near edges look wrong.
- **Max seeds**: 200-300 covers most of the screen at typical scales. Increase for zoomed-out views.
- **Skip center**: Seeds near origin are densely packed. Consider starting loop at index 1 or higher.

### Effects NOT Ported from Voronoi

These require computing cell borders (second-nearest seed), adding significant cost:
- `edgeIsoIntensity` — iso rings from edges
- `determinantIntensity` — 2D cross product shading
- `ratioIntensity` — edge/center distance ratio
- `edgeDetectIntensity` — sharp edge detection

Could be added later if the simpler center-based effects prove useful.

### Edge Cases

- **Center singularity**: At r≈0, many seeds overlap. Mask center or start search at index > 0.
- **Screen edges**: Seeds only exist up to `maxSeeds`. Outer regions may have no nearby seed — clamp or extend.
