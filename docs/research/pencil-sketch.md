# Pencil Sketch

Transforms images into graphite pencil drawings by accumulating stroke darkness along multiple fixed angles. Strokes appear perpendicular to image edges, following object contours like traditional hatching. Includes paper texture simulation and subtle wobble animation.

## Classification

- **Category**: TRANSFORMS → Style
- **Core Operation**: Directional gradient-aligned stroke accumulation
- **Pipeline Position**: Output stage with other transforms (user-reorderable)

## References

- [Shadertoy: Hand Drawing](https://www.shadertoy.com/view/MsSGD1) - Flockaroo (2016), CC BY-NC-SA 3.0. Primary implementation reference.

## Algorithm

### Overview

For each pixel:
1. Compute image gradient via central differences
2. For each of 3 angles, sample 16 points along that direction (both positive and negative)
3. Accumulate stroke darkness where gradient aligns with sample direction
4. Accumulate tinted color from sampled positions
5. Apply paper texture and vignette

### Gradient Computation

Central-difference gradient at position `pos` with epsilon `eps`:

```glsl
vec2 getGrad(vec2 pos, float eps)
{
    vec2 d = vec2(eps, 0);
    return vec2(
        getVal(pos + d.xy) - getVal(pos - d.xy),
        getVal(pos + d.yx) - getVal(pos - d.yx)
    ) / eps / 2.0;
}
```

Where `getVal()` returns luminance (dot with vec3(0.333)).

### Stroke Accumulation Loop

```glsl
for (int i = 0; i < AngleNum; i++)  // AngleNum = 3
{
    float ang = PI2 / float(AngleNum) * (float(i) + 0.8);
    vec2 v = vec2(cos(ang), sin(ang));

    for (int j = 0; j < SampNum; j++)  // SampNum = 16
    {
        // Perpendicular offset (creates stroke width)
        vec2 dpos = v.yx * vec2(1, -1) * float(j) * scale;
        // Parallel offset (curved stroke path)
        vec2 dpos2 = v.xy * float(j * j) / float(SampNum) * 0.5 * scale;

        for (float s = -1.0; s <= 1.0; s += 2.0)  // Both directions
        {
            vec2 pos2 = pos + s * dpos + dpos2;
            vec2 g = getGrad(pos2, 0.4);

            // Stroke visibility: gradient aligned with sample direction
            float fact = dot(g, v) - 0.5 * abs(dot(g, v.yx * vec2(1, -1)));
            fact = clamp(fact, 0.0, 0.05);
            fact *= 1.0 - float(j) / float(SampNum);  // Fade with distance

            col += fact;  // Accumulate stroke darkness
        }
    }
}
```

### Key Insight

`dot(g, v)` is positive when the image gradient points along the sampling direction. Since strokes are drawn perpendicular to the sample direction (`v.yx * vec2(1,-1)`), this means strokes appear **perpendicular to edges**—exactly like pencil hatching that follows contours.

### Paper Texture

Subtle grid pattern via sin waves:

```glsl
vec2 s = sin(pos.xy * 0.1 / sqrt(resolution.y / 400.0));
vec3 karo = vec3(1.0);
karo -= 0.5 * vec3(0.25, 0.1, 0.1) * dot(exp(-s * s * 80.0), vec2(1));
```

### Vignette

Radial cubic falloff:

```glsl
float r = length(pos - resolution.xy * 0.5) / resolution.x;
float vign = 1.0 - r * r * r;
```

### Wobble Animation

Position offset oscillates with time:

```glsl
vec2 pos = fragCoord + 4.0 * sin(iTime * vec2(1, 1.7)) * resolution.y / 400.0;
```

## Parameters

| Parameter | Type | Range | Effect |
|-----------|------|-------|--------|
| angleCount | int | 2-6 | Number of hatching directions (default 3) |
| sampleCount | int | 8-24 | Samples per direction; affects stroke length and cost |
| strokeFalloff | float | 0.0-1.0 | How quickly strokes fade with distance from edge |
| gradientEps | float | 0.2-1.0 | Gradient sample distance; affects edge sensitivity |
| paperStrength | float | 0.0-1.0 | Paper texture visibility |
| vignetteStrength | float | 0.0-1.0 | Edge darkening intensity |
| wobbleSpeed | float | 0.0-2.0 | Animation speed (0 = static) |
| wobbleAmount | float | 0.0-8.0 | Pixel displacement amplitude |

## Modulation Candidates

- **strokeFalloff**: Modulating creates pulsing stroke density—strokes extend/contract from edges
- **wobbleAmount**: Modulating adds rhythmic shake intensity, like nervous hand-drawing
- **vignetteStrength**: Modulating creates breathing tunnel focus effect
- **paperStrength**: Modulating fades paper texture in/out with audio energy

## Notes

### Performance

Original shader samples ~96 textures per pixel (3 angles × 16 samples × 2 directions). For real-time use at 60 FPS:
- Consider reducing to 2 angles × 8 samples (~32 samples)
- Or make sample counts configurable with performance/quality tradeoff

### Color Handling

The original accumulates tinted color (`col2`) from sampled positions via `getColHT()`, which adds noise-based thresholding. This creates subtle color variation in strokes. The final output multiplies stroke darkness by this accumulated color.

### Difference from Cross-Hatching

Cross-hatching uses periodic line patterns activated by luminance thresholds. Pencil Sketch uses gradient-aligned accumulation—strokes only appear where edges exist and only in directions perpendicular to those edges. This creates contour-following rather than blanket coverage.
