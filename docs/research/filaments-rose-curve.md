# Filaments Rose Curve Integration

Replace the cumulative rotation endpoint pattern with a Maurer rose curve for smooth, modulatable filament geometry.

## Problem

Current filaments use cumulative rotation to position endpoints:

```glsl
p1 *= rot(stepAngle + raPow * 0.0007);
vec2 p2 = p1 * rot(spread * float(i) - rotationAccum);
```

`stepAngle` and `spread` cause instant visual jumps when modulated because each iteration depends on all previous iterations. Small parameter changes redistribute every filament simultaneously.

## Rose Curve

A polar rose `r = cos(n * theta)` traces petal-like loops. Line segments from origin to points on the curve create the same radial line-segment aesthetic as current filaments, but each endpoint is computed independently from its index.

```glsl
float theta = TWO_PI * float(i) / float(totalFilaments);
float rose = cos(petals * theta);
vec2 endpoint = radius * rose * vec2(cos(theta), sin(theta));
```

**Key property**: `petals` is smoothly modulatable. Fractional values create interesting transitional patterns between clean roses. Integer values give `n` or `2n` petals depending on odd/even.

Source: Shadertoy day-22 sketch (see conversation) + standard Maurer rose mathematics.

## Proposed Parameters

| Replace | With | Type | Default | Range |
|---------|------|------|---------|-------|
| `stepAngle` | `petals` | float | 3.0 | 0.5-12.0 |
| `spread` | (remove) | - | - | - |

`petals` controls the rose petal count. `radius` still controls reach. `rotationSpeed`/`rotationAccum` still spins the whole figure.

## Segment Construction

Each filament is a segment from origin (or a small inset) to the rose curve point:

```glsl
float theta = TWO_PI * float(i) / float(totalFilaments) + rotationAccum;
float rose = cos(petals * theta);
vec2 endpoint = radius * rose * vec2(cos(theta), sin(theta));
float dist = segm(p, vec2(0.0), endpoint);
```

Setting the segment start to `vec2(0.0)` gives a starburst. Could also use a smaller-radius rose as the start point for thicker tangles.

## Visual Character

- `petals = 1.0`: cardioid (single lobe)
- `petals = 2.0`: figure-eight (two petals)
- `petals = 3.0`: three-petal flower
- `petals = 5.5`: complex overlapping star
- High values: dense radial spray approaching the current look
- Modulating `petals` with an LFO creates a continuously morphing figure
