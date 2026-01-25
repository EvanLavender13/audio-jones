# Superformula Shape

A parametric polar curve that generates an enormous variety of 2D shapes — circles, stars, rounded polygons, organic blobs, spiky forms — from a single equation with 6 parameters. Replaces fixed-sided polygons with a continuously morphable supershape whose outline renders as a polyline via ThickLine.

## Classification

- **Category**: DRAWABLE (new type: `DRAWABLE_SUPERFORMULA`)
- **Core Operation**: CPU-side polar curve evaluation → vertex polyline
- **Pipeline Position**: Drawable stage (same as Shape/Waveform/Spectrum)

## References

- [Paul Bourke: Supershapes](https://paulbourke.net/geometry/supershape/) — C implementation, parameter effects, special cases
- [Wikipedia: Superformula](https://en.wikipedia.org/wiki/Superformula) — Equation, generalized form, GNU Octave code
- [SuperformulaSVG (Jason Webb)](https://github.com/jasonwebb/SuperformulaSVG-for-web) — Interactive parameter exploration, SVG generation
- [Supershape 2D GLSL](https://machinesdontcare.wordpress.com/2008/03/12/supershape-2d-glsl-rewrite/) — Fragment shader distance-field approach (reference only; we use CPU polyline)
- [gpfault.net: Superformula](https://gpfault.net/posts/superformula.txt.html) — GLSL implementation notes, generalized m1/m2 variant

## Algorithm

### Core Equation

In polar coordinates, the radius at angle φ:

```
r(φ) = ( |cos(m·φ/4) / a|^n2 + |sin(m·φ/4) / b|^n3 ) ^ (-1/n1)
```

Convert to Cartesian:
```
x = r(φ) · cos(φ)
y = r(φ) · sin(φ)
```

### Generalized Form (asymmetric)

Replace single `m` with separate `m1`, `m2` for the cos/sin terms:

```
r(φ) = ( |cos(m1·φ/4) / a|^n2 + |sin(m2·φ/4) / b|^n3 ) ^ (-1/n1)
```

This produces rotationally asymmetric and nested structures when m1 ≠ m2.

### Vertex Generation (C implementation)

```c
// Generate NP vertices around the supershape
for (int i = 0; i <= NP; i++) {
    float phi = i * TWO_PI / NP;

    float t1 = cosf(m * phi / 4.0f) / a;
    t1 = fabsf(t1);
    t1 = powf(t1, n2);

    float t2 = sinf(m * phi / 4.0f) / b;
    t2 = fabsf(t2);
    t2 = powf(t2, n3);

    float r = powf(t1 + t2, 1.0f / n1);
    if (r == 0.0f) {
        x[i] = 0.0f;
        y[i] = 0.0f;
    } else {
        r = 1.0f / r;
        x[i] = r * cosf(phi);
        y[i] = r * sinf(phi);
    }
}
```

### Resolution

Use 128-512 vertices (NP) depending on shape complexity. Higher m values need more vertices to avoid aliasing angular features. A reasonable heuristic: `NP = max(128, m * 32)`.

### Special Cases

| Condition | Result |
|-----------|--------|
| m = 0 | Circle (r = 1 everywhere) |
| n1 = n2 = n3 = 2, a = b = 1 | Circle |
| m = 4, n1 = n2 = n3 = 100 | Approaches a square |
| m = 4, n1 = 2, n2 = n3 = 2 | Rounded diamond |
| Small n1 (< 1) | Pinched/star forms |
| Large n1 (> 10) | Bloated/convex forms |
| n2 ≠ n3 | Asymmetric angular features |

### Numerical Safety

- Guard `r == 0` (division by zero when result is infinite)
- Use `fabsf()` before `powf()` to avoid NaN from negative bases with fractional exponents
- Clamp output radius to a maximum to prevent degenerate vertices

## Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| m | float | 0 – 20 | 4.0 | Rotational symmetry (petal/spoke count) |
| n1 | float | 0.1 – 40 | 1.0 | Overall convexity: <1 pinched/star, >1 bloated/round |
| n2 | float | 0.1 – 40 | 1.0 | Cos-term curvature (spike sharpness) |
| n3 | float | 0.1 – 40 | 1.0 | Sin-term curvature (spike sharpness) |
| a | float | 0.1 – 4.0 | 1.0 | X-axis scale |
| b | float | 0.1 – 4.0 | 1.0 | Y-axis scale |
| resolution | int | 64 – 512 | 256 | Vertex count for polyline |
| size | float | 0.05 – 1.0 | 0.3 | Radius as fraction of minDim |
| thickness | int | 1 – 20 | 3 | ThickLine pixel width |
| filled | bool | — | false | Fill interior with triangulated mesh |
| textured | bool | — | false | Sample accumTexture through shape silhouette (like Shape drawable) |
| texZoom | float | 0.1 – 4.0 | 1.0 | Texture zoom (textured mode only) |
| texBrightness | float | 0.0 – 1.0 | 0.9 | Per-frame decay for textured feedback (textured mode only) |

## Modulation Candidates

- **m**: Changes spoke/petal count — morphs symmetry in real time. Integer snapping produces discrete jumps between shapes; float values produce intermediate open curves.
- **n1**: Sweeps between star/pinched forms and bloated/round forms — dramatic silhouette change.
- **n2, n3**: Sharpens or softens individual spikes. Modulating one independently creates asymmetric breathing.
- **a, b**: Stretches the shape directionally — squash-and-stretch animation feel.
- **size**: Pulsing the overall radius with beat energy.
- **rotationSpeed**: Standard drawable rotation, compounds with the shape's own angular features.

## Notes

- The shape is generated as a closed polyline, so it renders identically to circular waveforms via ThickLine. No shader needed.
- For `filled` mode, triangulate from center (fan triangulation) since the supershape is always star-convex when parameters are reasonable.
- For `textured` mode, use the same fan triangles with UV coordinates derived from vertex screen positions (normalized to 0-1). Samples accumTexture through the shape silhouette — identical to `ShapeDrawTextured()`. Reuses the existing shape texture shader.
- Deep concavities (n1 < 0.3 with high m) cause fan triangle overlap in filled/textured mode. Clamp n1 minimum when fill is active, or accept double-draw brightening.
- Non-integer m values produce open curves (the path doesn't close after one revolution). Either clamp m to integers, or extend φ range to `2π · lcm` for closure. Simplest approach: allow non-integer m and accept the visual gap, or snap to nearest 0.5.
- The generalized m1/m2 form is a potential advanced mode but adds UI complexity. Start with single m.
- CPU cost is trivial: 256 iterations of trig + pow per frame. No GPU compute needed.
- Consider caching vertices and only recomputing when parameters change (or every frame if modulated).
