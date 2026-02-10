# Moiré Interference

Self-interference effect that samples the input texture multiple times with slight rotations or scale differences, then blends the samples to produce large-scale wave interference patterns. The interference fringes emerge automatically from the texture's own content—no procedural pattern generation required.

## Classification

- **Category**: TRANSFORMS → Symmetry (or new "Interference" subcategory)
- **Core Operation**: Multi-sample UV transform with multiplicative blending
- **Pipeline Position**: Transform stage (user-reorderable)

## References

- [Wikipedia - Moiré pattern](https://en.wikipedia.org/wiki/Moir%C3%A9_pattern) - ✓ User provided: Mathematical formulas for parallel and rotated pattern interference
- [Observable - Graphene Moiré Shader](https://observablehq.com/@pamacha/graphene-moire-fragment-shader) - ✓ Fetched: GLSL implementation using hexagonal lattice with rotation
- [Wolfram MathWorld - Moiré Pattern](https://mathworld.wolfram.com/MoirePattern.html) - ✓ Fetched: Conceptual overview

## Algorithm

### Core Principle

When two similar patterns overlay with slight displacement, rotation, or scale difference, interference fringes appear at a spatial frequency much lower than either original pattern. The moiré acts as a "beat frequency" in 2D space.

### Mathematical Foundation

**Parallel patterns with different spatial frequencies:**

For two sinusoidal patterns with spatial frequencies k₁ and k₂:

```
f₁ = (1 + sin(k₁·x)) / 2
f₂ = (1 + sin(k₂·x)) / 2

Combined: f₃ = (1 + sin(A·x)·cos(B·x)) / 2

Where:
  A = (k₁ + k₂) / 2    // carrier frequency (high, from original patterns)
  B = (k₁ - k₂) / 2    // envelope frequency (low, the visible moiré)
```

The `cos(B·x)` term creates the visible interference fringes—a "beat" pattern at half the frequency difference.

**Rotated patterns (same spatial frequency):**

For two identical patterns rotated by angle α:

```
Fringe spacing: D = p / (2·sin(α/2))

For small angles: D ≈ p / α
```

Where p is the pattern's period. Smaller rotation angles produce wider, more spread-out fringes.

**Fringe orientation:** The interference fringes bisect the angle between the two pattern orientations, appearing at angle α/2 relative to each pattern.

### Shader Implementation

```glsl
// Rotation matrix
mat2 rotate2d(float angle) {
    float c = cos(angle);
    float s = sin(angle);
    return mat2(c, -s, s, c);
}

vec4 moireEffect(sampler2D tex, vec2 uv, float angle, float scale, int layers) {
    vec2 center = vec2(0.5);
    vec2 centered = uv - center;

    vec4 result = texture(tex, uv);

    for (int i = 1; i < layers; i++) {
        float layerAngle = angle * float(i);
        float layerScale = 1.0 + (scale - 1.0) * float(i);

        vec2 rotated = rotate2d(layerAngle) * centered;
        vec2 scaled = rotated * layerScale + center;

        vec4 sample = texture(tex, scaled);

        // Multiplicative blend reveals interference
        result *= sample;
    }

    // Normalize to prevent excessive darkening
    result = pow(result, vec4(1.0 / float(layers)));

    return result;
}
```

### Blend Modes

Different blend operations produce different interference characteristics:

| Mode | Formula | Visual Result |
|------|---------|---------------|
| Multiply | `a * b` | Dark fringes where patterns misalign, bright where aligned |
| Min | `min(a, b)` | Sharp dark interference lines |
| Average | `(a + b) / 2` | Subtle, softer fringes |
| Difference | `abs(a - b)` | Highlights misalignment regions |

Multiply is the most physically accurate for overlaid semi-transparent layers.

## Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| rotationAngle | float | 0 - 30° | 5° | Angle between layers; smaller = wider fringes |
| scaleDiff | float | 0.95 - 1.05 | 1.02 | Scale ratio between layers; closer to 1 = wider fringes |
| layers | int | 2 - 4 | 2 | Number of overlaid samples |
| blendMode | enum | multiply/min/average/difference | multiply | How layers combine |
| centerX | float | 0 - 1 | 0.5 | Rotation/scale center X |
| centerY | float | 0 - 1 | 0.5 | Rotation/scale center Y |
| animationSpeed | float | 0 - 10°/s | 1°/s | Rate of rotation animation |

## Modulation Candidates

- **rotationAngle**: Modulating creates flowing, morphing interference fringes. Small changes produce dramatic fringe movement.
- **scaleDiff**: Modulating creates "breathing" or pulsing fringes that expand/contract from center.
- **animationSpeed**: Modulating creates acceleration/deceleration of fringe flow.
- **centerX/centerY**: Modulating shifts the interference pattern's focal point across the image.

## Notes

### Visual Characteristics

- Works best with content that has fine detail or high-frequency patterns (waveforms, particle trails)
- Solid colors or smooth gradients produce minimal interference
- The effect amplifies existing texture detail into large-scale patterns

### Implementation Considerations

- Requires multiple texture samples per pixel (2-4 typically)
- Edge handling: Use `GL_REPEAT` or `GL_MIRRORED_REPEAT` to avoid hard edges at texture boundaries
- The rotation center should be configurable (defaults to screen center)

### Audio Reactivity Potential

- Bass → rotation speed (slow, flowing fringes)
- Mids → scale oscillation (breathing effect)
- Treble → number of layers or blend mode switching

### Relationship to Existing Effects

- **Different from Kaleidoscope**: Kaleidoscope mirrors wedges; Moiré overlays rotated copies and multiplies
- **Different from KIFS**: KIFS uses recursive folding; Moiré uses simple rotation with interference
- **Complementary to waveform content**: The fine lines in waveforms create strong interference patterns
