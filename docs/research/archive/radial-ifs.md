# Radial IFS

Iterated polar wedge folding that produces radial snowflake/flower fractals. Each iteration folds UV into a wedge, translates away from center, and scales—creating recursive branching patterns that radiate outward like frost crystals or coral.

## Classification

- **Category**: TRANSFORMS > Symmetry
- **Pipeline Position**: Transforms stage (user-reorderable with other transforms)

## References

- User-provided Shadertoy shader (IFS with `radialFold` function)
- Existing `kifs.fs` and `kaleidoscope.fs` in this codebase

## Algorithm

The core operation combines kaleidoscope's polar fold with KIFS's iteration pattern:

```glsl
vec2 radialFold(vec2 p, int segments, float offset)
{
    float segmentAngle = TWO_PI / float(segments);
    float halfSeg = segmentAngle * 0.5;

    // Convert to polar
    float r = length(p);
    float a = atan(p.y, p.x);

    // Fold angle into wedge (same as kaleidoscope)
    float c = floor((a + halfSeg) / segmentAngle);
    a = mod(a + halfSeg, segmentAngle) - halfSeg;
    a *= mod(c, 2.0) * 2.0 - 1.0;  // mirror alternating segments

    // Back to Cartesian, then translate
    return vec2(cos(a), sin(a)) * r - vec2(offset, 0.0);
}
```

Main iteration loop (mirrors KIFS/Triangle Fold structure):

```glsl
vec2 p = (fragTexCoord - 0.5) * 2.0;
p = rotate2d(p, rotation);

for (int i = 0; i < iterations; i++) {
    p = radialFold(p, segments, offset);
    p *= scale;
    p = rotate2d(p, twistAngle);
}

vec2 newUV = mirror(p * 0.5 + 0.5);
finalColor = texture(texture0, newUV);
```

Key difference from kaleidoscope: the **offset translation** after folding pushes the pattern away from center before the next iteration, creating branching rather than simple mirroring.

Key difference from KIFS: folding happens in **polar space** (wedges) rather than Cartesian space (`abs()`), producing radial symmetry instead of grid-like recursion.

## Parameters

| Parameter | Type | Range | Default | Visual Effect |
|-----------|------|-------|---------|---------------|
| segments | int | 3-12 | 6 | Wedge count per fold; 3=triangular, 6=hexagonal, higher=circular |
| iterations | int | 1-8 | 4 | Recursion depth; more = finer fractal detail |
| scale | float | 1.2-2.5 | 1.8 | Expansion per iteration; affects branch density |
| offset | float | 0.0-2.0 | 0.5 | Translation after fold; controls branch separation |
| rotation | float | 0-2π | 0 | Global pattern rotation |
| twistAngle | float | -π to π | 0 | Per-iteration rotation; creates spiral arms |

## Modulation Candidates

- **rotation**: pattern spins
- **twistAngle**: spiral arms wind/unwind
- **offset**: branches expand/contract from center
- **scale**: fractal detail pulses
- **segments**: wedge count morphs (integer stepping creates discrete jumps)

## Notes

- Segments parameter creates distinct visual families: 3 (triangular/Y-shaped), 4 (cross/X-shaped), 5 (pentagonal), 6 (snowflake/hexagonal)
- Low iteration counts (1-2) produce simple kaleidoscope-like results; the effect diverges from kaleidoscope at 3+ iterations
- High offset values can push content entirely off-screen; clamp or use mirror() wrapping
- Consider optional smoothing parameter (like kaleidoscope's) for soft wedge edges at low segment counts
