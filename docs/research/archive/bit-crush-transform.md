# Bit Crush Transform

A retro transform effect that applies the lattice walk algorithm to the input image. Pixels cluster into irregular staircase-shaped blocks that constantly reorganize, each block sampling one input color. Like pixelation but with chaotic, shifting cell boundaries instead of a regular grid.

## Category

Retro transform (alongside Pixelation, Glitch, ASCII Art).

## Algorithm

Same lattice walk as the generator, but samples `texture0` instead of a gradient LUT.

```glsl
// Lattice walk (identical to generator)
vec2 p = (fragTexCoord - center) * resolution * scale;
for (int i = 0; i < iterations; i++) {
    vec2 ceilCell = ceil(p / cellSize);
    vec2 floorCell = floor(p / cellSize);
    p = ceil(p + r(ceilCell, time) + r(floorCell, time) * vec2(-1.0, 1.0));
}

// Map final position back to UV and sample input
vec2 sampleUV = p / (resolution * scale) + center;
vec3 color = texture(texture0, sampleUV).rgb;

finalColor = vec4(color, 1.0);
```

No FFT, no gradient LUT, no glow. The input image provides all color.

## Parameters

| Parameter | Type | Range | Default | Description |
|-----------|------|-------|---------|-------------|
| scale | float | 0.05–1.0 | 0.3 | Coordinate zoom — larger = more cells |
| cellSize | float | 2.0–32.0 | 8.0 | Grid quantization coarseness |
| iterations | int | 4–64 | 32 | Walk steps — more = more chaotic |
| speed | float | 0.1–5.0 | 1.0 | Animation rate |

Four params total. Much simpler than the generator.

## Differences from Generator

| | Generator (current) | Transform (this) |
|-|---------------------|-------------------|
| Input | None (generates from nothing) | Samples `texture0` |
| Color | Gradient LUT | Input image |
| FFT | Per-cell brightness modulation | None |
| Blend | Screen/Add via blend compositor | Direct transform (no blend) |
| Category | Generator / Texture | Retro transform |
| Params | 12 | 4 |

## Open Questions

- Should `sampleUV` clamp or wrap when the walk pushes it out of bounds? Clamp would smear edges, wrap would tile.
- Could add an optional mix slider to blend between the crushed and original image for subtlety.
- Walk mode variants from `bit-crush-walk-modes.md` would apply here too.
