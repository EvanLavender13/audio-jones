# Phi Blur

Standalone blur transform with two phi-based sampling modes: a rectangular kernel with free angle and aspect ratio, and a circular disc kernel with radius control. Both distribute samples using golden-ratio low-discrepancy sequences for artifact-free coverage at any sample count. Gamma-correct blending linearizes samples before averaging to prevent midtone darkening.

## Classification

- **Category**: TRANSFORMS > Optical (alongside Bloom, Bokeh, Heightfield Relief)
- **Pipeline Position**: Reorderable in the transform chain
- **Chosen Approach**: Full — both kernel shapes with gamma-correct blending. Single shader with mode switch, no multi-pass complexity.

## References

- [GM Shaders: Phi Tutorial](https://mini.gmshaders.com/p/phi) — XorDev's original tutorial covering rect and disc blur kernels with golden-ratio sampling. Source of both algorithms and GLSL snippets.
- [Bart Wronski: Golden Ratio Sequence](https://bartwronski.com/2016/10/30/dithering-part-two-golden-ratio-sequence-blue-noise-and-highpass-and-remap/) — Analysis of golden-ratio low-discrepancy properties: uniform coverage, frequency characteristics, comparison to blue noise.

## Algorithm

### Core Principle

Phi (1.618034) is the "most irrational" number — hardest to approximate with ratios. Multiplying successive integers by phi and taking the fractional part produces a 1D sequence that fills the unit interval with minimal gaps at any sample count. The golden angle (tau * (2 - phi) = 2.399963 rad) extends this to circular distributions.

### Rect Blur

Distributes N samples across a rectangular area using phi for the second dimension:

```glsl
#define PHI 1.618034

for (float i = 0.0; i < samples; i++) {
    vec2 offset = vec2(i / samples, fract(i * PHI)) - 0.5;
    // offset is in [-0.5, 0.5] x [-0.5, 0.5]
    // rotate by angle, scale by (radius * aspectRatio, radius)
}
```

The rotation applies a 2x2 matrix from the `angle` parameter. Aspect ratio stretches one axis relative to the other before rotation.

### Disc Blur

Distributes N samples in a circular disk using golden-angle rotation with sqrt-radius scaling for uniform area coverage:

```glsl
#define GOLDEN_ANGLE 2.399963
#define HALF_PI 1.570796

for (float i = 0.0; i < samples; i++) {
    vec2 dir = cos(i * GOLDEN_ANGLE + vec2(0.0, HALF_PI));
    float r = sqrt(i / samples) * radius;
    vec2 offset = dir * r;
}
```

`sqrt(i/N)` prevents over-sampling the center — without it, sample density clusters near the origin because area scales with r-squared.

### Gamma-Correct Blending

Both modes linearize before averaging and re-gamma after:

```glsl
blur += pow(textureSample, vec4(gamma));
// ...
result = pow(blur / samples, vec4(1.0 / gamma));
```

Default gamma = 2.2 (perceptually accurate). Higher values (4.0-6.0) exaggerate bright-region preservation for a more stylized look.

## Parameters

| Parameter | Type | Range | Default | Visual Effect |
|-----------|------|-------|---------|---------------|
| mode | int | 0-1 | 0 | 0 = Rect, 1 = Disc |
| radius | float | 0.0-50.0 | 5.0 | Blur extent in pixels |
| angle | float | 0-2pi | 0.0 | Rect kernel rotation (radians, displayed as degrees) |
| aspectRatio | float | 0.1-10.0 | 1.0 | Rect kernel width/height ratio. 1.0 = square, >1 = horizontal stretch |
| samples | int | 8-128 | 32 | Sample count. Higher = smoother, slower |
| gamma | float | 1.0-6.0 | 2.2 | Blending gamma. 1.0 = linear average, 2.2 = perceptual, 6.0 = bright-preserving |

### Mode-Specific Visibility

- `angle` and `aspectRatio` only apply in Rect mode. Hidden or grayed in Disc mode.

## Modulation Candidates

- **radius**: Pulsing blur intensity
- **angle**: Rotating directional blur streaks (rect mode)
- **aspectRatio**: Morphing between circular and directional blur (rect mode)
- **gamma**: Shifting between flat averaging and bright-preserving blend

## Notes

- **Performance**: Single-pass, no ping-pong textures. Cost scales linearly with sample count. At 32 samples the inner loop runs 32 texture fetches per pixel — comparable to a 5-tap separable Gaussian in total bandwidth but without the multi-pass overhead.
- **Quality vs Speed**: 16 samples produces visible grain on large radii. 32 is the sweet spot. 64+ for production-quality large blurs.
- **No separable shortcut**: Phi-based sampling is inherently 2D. Cannot split into H/V passes like Gaussian. The tradeoff is flexibility (arbitrary kernel shape) for raw throughput.
- **Edge handling**: Clamp UVs to [0,1] to prevent texture wrapping at screen borders.
