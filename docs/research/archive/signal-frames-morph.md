# Signal Frames — Shape Morphing

Replace the fixed box/triangle alternation with a general polygon SDF that supports fractional side counts. Layers have a gradient of polygon counts across the stack (inner=fewer sides, outer=more). The sweep ratchets each layer's side count forward as it passes, with configurable transition smoothness.

## Classification

- **Category**: Enhancement to existing generator (Signal Frames)
- **Pipeline Position**: No change — same generator slot
- **Scope**: Shader rewrite (replace sdBox/sdEquilateralTriangle with sdNgon), config additions, UI additions

## References

- [Inigo Quilez — 2D Distance Functions](https://iquilezles.org/articles/distfunctions2d/) — SDF reference

## Reference Code

### Polar-fold regular polygon SDF (standard technique)

```glsl
// Regular polygon SDF via polar coordinate folding.
// Supports fractional sides for smooth morphing between shapes.
float sdNgon(vec2 p, float r, float n) {
    float a = atan(p.y, p.x);
    float sector = TAU / n;
    a = mod(a + sector * 0.5, sector) - sector * 0.5;
    return cos(a) * length(p) - r;
}
```

### Current Signal Frames shape selection (to be replaced)

```glsl
// SDF: even layers = box, odd layers = equilateral triangle
float sdf;
if (i % 2 == 0) {
    sdf = sdBox(uv, vec2(size * aspectRatio, size));
} else {
    sdf = sdEquilateralTriangle(uv, size);
}
```

## Algorithm

### Substitution Table

| Current | New | Notes |
|---------|-----|-------|
| `sdBox()` function | Remove | Replaced by `sdNgon()` |
| `sdEquilateralTriangle()` function | Remove | Replaced by `sdNgon()` |
| `if (i % 2 == 0)` shape branch | `sdNgon(uv, size, sides)` | Single SDF call with computed `sides` |
| `aspectRatio` on box | Apply to UV before SDF: `uv.x /= aspectRatio` | Aspect stretches all polygons uniformly |

### Side Count Formula

```glsl
// Layer gradient: inner layers fewer sides, outer layers more (or reversed)
float gradientSides = baseSides + sideSpread * t;

// Sweep ratchet: each sweep pass advances by 1, cycles within morphRange
float sweepAdvance = mod(floor(sweepAccum + t), morphRange);

// Combined raw sides
float rawSides = gradientSides + sweepAdvance;

// Configurable smoothness: 0=discrete snap, 1=continuous morph
float sides = mix(floor(rawSides), rawSides, morphSmooth);

// Clamp minimum to 3 (triangle is lowest valid polygon)
sides = max(sides, 3.0);
```

### Aspect Ratio Adaptation

The current `aspectRatio` param stretches box SDFs via `vec2(size * aspectRatio, size)`. For the general polygon SDF, apply aspect ratio as a UV pre-scale:

```glsl
vec2 sdfUV = vec2(uv.x / aspectRatio, uv.y);
float sdf = sdNgon(sdfUV, size, sides);
```

This stretches any polygon shape horizontally, not just boxes.

## Parameters

### New Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| baseSides | float | 3.0-8.0 | 3.0 | Starting polygon count for innermost layer |
| sideSpread | float | -5.0-5.0 | 3.0 | Side count change across layer stack (inner→outer) |
| morphRange | float | 1.0-6.0 | 4.0 | Shapes before sweep ratchet cycles |
| morphSmooth | float | 0.0-1.0 | 0.5 | Transition smoothness (0=snap, 1=continuous) |

### Removed Parameters

| Parameter | Reason |
|-----------|--------|
| *(none)* | `aspectRatio` is kept but applied differently |

### Unchanged Parameters

All existing Signal Frames parameters (FFT, rotation, orbit, glow, sweep, gradient, blend) remain as-is.

## Modulation Candidates

- **baseSides**: base polygon count shifts with audio — bass makes triangles, treble makes hexagons
- **sideSpread**: range widens on loud passages, collapses to uniform on quiet
- **morphSmooth**: snappy on percussive hits (low), fluid on sustained tones (high)

### Interaction Patterns

- **Cascading threshold — sweep x morphRange**: With large morphRange, the sweep takes many passes to cycle a layer through all shapes. Different sections of a song produce different visual states depending on how many sweep passes have accumulated. Verse and chorus show structurally different geometry.
- **Competing forces — baseSides x sideSpread**: High baseSides with negative sideSpread inverts the gradient (outer=triangle, inner=octagon). Modulating both creates shifting polygon hierarchies across the layer stack.

## Notes

- `sdNgon` with fractional sides is continuous — no visual popping at integer boundaries.
- `morphSmooth = 0` with `morphRange = 2` and `baseSides = 3` recreates the old triangle/square alternation feel, but now sweep-driven.
- `sideSpread = 0` makes all layers the same shape — useful for uniform geometric looks.
- `morphRange = 1` effectively disables sweep ratcheting (mod 1 = 0 always).
- The `sdBox` and `sdEquilateralTriangle` functions can be fully removed from the shader.
