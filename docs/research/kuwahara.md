# Kuwahara Filter

Edge-preserving smoothing that flattens regions of similar color while maintaining hard boundaries. Converts soft gradients into distinct cell-like shapes with defined edges — visually distinct from the geometry-based Oil Paint effect.

## Classification

- **Category**: TRANSFORMS → Style
- **Core Operation**: Divide pixel neighborhood into sectors, output mean color of lowest-variance sector
- **Pipeline Position**: Output stage transforms (user-ordered with other Style effects)

## References

- [lettier/3d-game-shaders-for-beginners](https://github.com/lettier/3d-game-shaders-for-beginners/blob/master/demonstration/shaders/fragment/kuwahara-filter.frag) — Clean 4-sector GLSL implementation
- [Generalized Kuwahara (Godot Shaders)](https://godotshaders.com/shader/generalized-kuwahara/) — 8-sector implementation with Gaussian weighting
- [Kyprianidis et al. "Anisotropic Kuwahara Filtering with Polynomial Weighting"](https://diglib.eg.org/bitstreams/3309663a-3134-44bc-9297-2fa33554277d/download) — Academic foundation for N-sector generalization
- Git commit `2bfc7bb` — Previously working 4-sector shader in this codebase

## Algorithm

### Basic Mode (4 Sectors)

Divide a square kernel into four overlapping quadrants centered on the current pixel:

```
+---+---+
| 0 | 1 |
+---X---+  X = current pixel (shared by all sectors)
| 2 | 3 |
+---+---+
```

For each sector, compute mean color and luminance variance:

```glsl
vec3 mean[4];
float variance[4];

for (int s = 0; s < 4; s++) {
    vec3 colorSum = vec3(0.0);
    vec3 colorSqSum = vec3(0.0);
    float count = 0.0;

    int xStart = (s == 0 || s == 2) ? -radius : 0;
    int xEnd   = (s == 0 || s == 2) ? 0 : radius;
    int yStart = (s == 0 || s == 1) ? -radius : 0;
    int yEnd   = (s == 0 || s == 1) ? 0 : radius;

    for (int x = xStart; x <= xEnd; x++) {
        for (int y = yStart; y <= yEnd; y++) {
            vec3 c = texture(source, uv + vec2(x, y) * texel).rgb;
            colorSum += c;
            colorSqSum += c * c;
            count += 1.0;
        }
    }

    mean[s] = colorSum / count;
    vec3 v = (colorSqSum / count) - (mean[s] * mean[s]);
    variance[s] = dot(v, vec3(0.299, 0.587, 0.114));
}

// Select sector with minimum variance
int minIdx = 0;
float minVar = variance[0];
for (int s = 1; s < 4; s++) {
    if (variance[s] < minVar) {
        minVar = variance[s];
        minIdx = s;
    }
}
fragColor = vec4(mean[minIdx], 1.0);
```

### Generalized Mode (8 Circular Sectors)

Replace square quadrants with 8 overlapping circular sectors using Gaussian-weighted sampling. Eliminates blocky artifacts at sector boundaries.

Each sector spans a 45-degree wedge. Samples are weighted by a Gaussian falloff from the sector center direction:

```glsl
const int N = 8;
const float PI = 3.14159265;

for (int k = 0; k < N; k++) {
    // Sector center angle
    float angle = float(k) * PI * 2.0 / float(N);
    vec2 dir = vec2(cos(angle), sin(angle));

    vec3 colorSum = vec3(0.0);
    vec3 colorSqSum = vec3(0.0);
    float weightSum = 0.0;

    for (int x = -radius; x <= radius; x++) {
        for (int y = -radius; y <= radius; y++) {
            vec2 offset = vec2(x, y);
            float dist = length(offset);
            if (dist > float(radius)) continue;

            // Gaussian spatial weight
            float g = exp(-3.125 * dot(offset / float(radius), offset / float(radius)));

            // Sector membership: dot product with sector direction
            float cosAngle = dot(normalize(offset + 0.001), dir);
            float sectorWeight = pow(max(0.0, cosAngle), sharpness);

            float w = g * sectorWeight;
            vec3 c = texture(source, uv + offset * texel).rgb;
            colorSum += c * w;
            colorSqSum += c * c * w;
            weightSum += w;
        }
    }

    vec3 sectorMean = colorSum / weightSum;
    vec3 sectorVar = (colorSqSum / weightSum) - (sectorMean * sectorMean);
    float v = dot(sectorVar, vec3(0.299, 0.587, 0.114));

    // Hardness controls how aggressively minimum variance wins
    float weight = 1.0 / (1.0 + pow(v * 1000.0, 0.5 * hardness));

    totalColor += sectorMean * weight;
    totalWeight += weight;
}

fragColor = vec4(totalColor / totalWeight, 1.0);
```

The generalized version uses soft blending between sectors (weighted by inverse variance) rather than hard minimum selection, producing smoother results.

## Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| radius | int | 2-12 | 4 | Kernel radius — larger creates broader flat regions |
| quality | enum | basic/generalized | basic | 4-sector (fast) or 8-sector Gaussian (smooth) |
| sharpness | float | 1.0-18.0 | 8.0 | Sector directional selectivity (generalized mode only) |
| hardness | float | 1.0-100.0 | 8.0 | How aggressively it picks uniform sectors (generalized mode only) |

## Modulation Candidates

- **radius**: Larger values create more abstract, posterized look with bigger color cells. Smaller preserves more detail.
- **sharpness**: Higher values create more defined sector boundaries; lower creates softer blending between regions.
- **hardness**: Higher values produce harder edges between color regions; lower creates smoother transitions.

## Notes

- Performance scales with radius². Radius 4-6 provides good real-time results. Radius 10+ may need frame-time monitoring.
- The 4-sector basic mode runs ~4x fewer samples than generalized for the same radius — use as the default.
- Generalized mode eliminates the cross-shaped artifacts visible in basic mode at high radii.
- Distinct from Oil Paint: Kuwahara produces flat posterized cells; Oil Paint produces textured directional brush strokes with relief lighting.
- Pairs well with Bokeh (creates diatom-like cell boundaries around blur circles) and color effects.
