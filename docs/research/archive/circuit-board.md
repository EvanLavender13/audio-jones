# Circuit Board Warp

Warps input imagery along iteratively-folded triangle wave patterns, producing circuit-trace or maze-like distortion. The effect channels 90s PCB aesthetics through recursive UV folding.

## Classification

- **Category**: TRANSFORMS > Warp
- **Pipeline Position**: Transform stage (user-ordered with other transforms)

## References

- User-provided Shadertoy code (FabriceNeyret2's "Circuit Diagram" variants)
- [Domain Warping](https://iquilezles.org/articles/warp/) - Inigo Quilez (f(p + h(p)) theory)
- [Triangle Waves](https://thndl.com/triangle-waves.html) - Triangle wave shader technique

## Algorithm

### Core Triangle Wave Function

The building block is a clamped triangle wave oscillating 0 → 0.5 → 0:

```glsl
vec2 triangleWave(vec2 p, vec2 patternConst, float scale) {
    return abs(fract((p + patternConst) * scale) - 0.5);
}
```

Unlike `sin()`, this produces hard corners at peaks/troughs, creating the circuit-board aesthetic.

### Iterative Folding

Each iteration folds the UV space, compounds the folding, and shrinks the scale:

```glsl
vec2 computeWarp(vec2 uv, vec2 c, int iterations, float scale, float offset, float scaleDecay) {
    for (int i = 0; i < iterations; i++) {
        // Fold both axes, combine x and y
        uv = triangleWave(uv + offset, c, scale)
           + triangleWave(uv.yx, c, scale);

        // Scale decays each iteration (finer detail)
        scale /= scaleDecay;
        offset /= scaleDecay;

        // Optional: flip y for interlocking pattern
        uv.y = -uv.y;
    }
    return uv;
}
```

**Key behaviors:**
- `uv.yx` swap creates orthogonal interference → branching traces
- Scale decay compounds detail at each level
- Y-flip creates alternating interlocking

### Pattern Constant Effects

The `patternConst` (c) value dramatically changes output character:

| c value | Visual style |
|---------|--------------|
| (7.0, 5.0) | Circuit traces, branching paths |
| (7.0, 9.0) | Denser trace network |
| (2.0, 2.5) | Maze corridors, blocky |
| (1.7, 1.9) | Organic veins, less geometric |

### Adapting to UV Warp

The original shaders generate color from UV. For image warping:

```glsl
// Compute warp displacement
vec2 warpUV = computeWarp(uv, patternConst, iterations, scale, offset, scaleDecay);

// Use as offset to sample input
float displacement = fract(warpUV.x - warpUV.y);
vec2 finalUV = uv + strength * (warpUV - 0.5);

vec4 color = texture(inputTex, finalUV);
```

### Animation

Add time-based scroll to create flowing circuit effect:

```glsl
vec2 scrollUV = uv + vec2(time * 0.5, time * 0.33) * scrollSpeed;
vec2 warpUV = computeWarp(scrollUV, ...);
```

### Per-Channel Chromatic Mode

Run iterations separately per RGB channel with different starting conditions for chromatic aberration:

```glsl
vec3 color;
for (int c = 0; c < 3; c++) {
    vec2 warp = computeWarp(uv, patternConst, iterations, scale + float(c) * 0.1, offset, scaleDecay);
    color[c] = texture(inputTex, uv + strength * warp).r; // Sample single channel
}
```

## Parameters

| Parameter | Type | Range | Default | Visual Effect |
|-----------|------|-------|---------|---------------|
| patternX | float | 1.0 - 10.0 | 7.0 | X component of pattern constant |
| patternY | float | 1.0 - 10.0 | 5.0 | Y component of pattern constant |
| iterations | int | 3 - 12 | 6 | Recursion depth, more = finer detail |
| scale | float | 0.5 - 3.0 | 1.4 | Initial folding frequency |
| offset | float | 0.05 - 0.5 | 0.16 | Initial offset between folds |
| scaleDecay | float | 1.01 - 1.2 | 1.05 | Scale reduction per iteration |
| strength | float | 0.0 - 1.0 | 0.5 | Warp intensity |
| scrollSpeed | float | 0.0 - 2.0 | 0.0 | Animation scroll rate |
| chromatic | bool | - | false | Per-channel iteration for RGB separation |

## Modulation Candidates

- **strength**: warp intensity pulses with beat
- **scrollSpeed**: circuit flow accelerates
- **scale**: folding frequency shifts
- **patternX/Y**: pattern morphs between circuit and maze styles
- **iterations**: detail level breathes

## Notes

- **Performance**: 6-9 iterations is the sweet spot. Above 12 iterations adds diminishing visual return for GPU cost.
- **Pattern presets**: Consider exposing named presets ("Circuit", "Maze", "Veins") that set patternX/Y to known-good values.
- **Combination**: Works well layered with Glitch (circuit + VHS corruption) or before Neon Glow (glowing traces).
- **Edge behavior**: High strength values will sample outside [0,1] UV range; may need clamp or wrap mode consideration.
