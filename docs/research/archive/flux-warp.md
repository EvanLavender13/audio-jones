# Flux Warp

Evolving cellular distortion where sharp-edged cells split, merge, and restructure over time. The pattern flows organically at high coupling or snaps to geometric grids at low coupling — like watching tectonic plates rearrange in fast-forward. The core trick is `mod()` on coupled oscillators with an animated divisor: smooth trig inputs create sharp cell boundaries, and the divisor morphing between product-cells and radial-cells causes the cell topology itself to continuously restructure.

## Classification

- **Category**: TRANSFORMS > Warp
- **Pipeline Position**: User-ordered transform chain (alongside Domain Warp, Interference Warp, etc.)

## References

- User-provided Shadertoy shader "Trippy patterns" (pasted in conversation)

## Reference Code

```glsl
float myLength(vec2 uv, vec2 fragCoord) {
    float x = cos(iResolution.x * abs(uv.x) + 0.2 * iTime);
    float y = sin(abs(uv.y) + x - 0.5 * iTime);
    float b = 0.5 * (1. + cos(0.3 * iTime));
    float b2 = 0.5 * (1. + cos(0.15 * iTime));
    return mod(x + y, (1.-b) * x * y + b * 0.4 * length(uv)) * ( b2 + (1.-b2)* 0.5 * (0.2 + sin(2. * uv.x * uv.y + iTime)));
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    float t = iTime;

    // Normalized pixel coordinates (from 0 to 1)
    vec2 uv = 6. * (fragCoord - 0.5 * iResolution.xy) /iResolution.y;
    float theta = atan(uv.y, uv.x);
    float k = 0.5 * (1. + cos(0.25 * t));
    float s = 0.5 * (1. + cos(t));
    s = (1.-k) * s + k * (1.-s);

    float d = myLength(uv, fragCoord);
    float p = 0.2;
    float a = 0.2;

    float r = 0.4 + 0.1 * (s * cos(3. * theta + t) + (1.-s) * sin(7. * theta + t));
    float r2 =  0.43 + 0.1 * (s * cos(3. * theta + t + p) + (1.-s) * sin(7. * theta + t + p));
    t -= a;
    float g = 0.4 + 0.1 * (s * cos(3. * theta + t) + (1.-s) * sin(7. * theta + t));
    float g2 =  0.43 + 0.1 * (s * cos(3. * theta + t + p) + (1.-s) * sin(7. * theta + t + p));
    t += 2. * a;
    float b = 0.4 + 0.1 * (s * cos(3. * theta + t) + (1.-s) * sin(7. * theta + t));
    float b2 =  0.43 + 0.1 * (s * cos(3. * theta + t + p) + (1.-s) * sin(7. * theta + t + p));

    r = 1.- smoothstep(d, r2, r);
    g = 1.- smoothstep(d, g2, g);
    b = 1.- smoothstep(d, b2, b);
    r = 16. * r * r * (1.-r) * (1.-r);
    g = 16. * g * g * (1.-g) * (1.-g);
    b = 16. * b * b * (1.-b) * (1.-b);

    float c1 = 0.5 * (1. + cos(t));
    float c2 = 0.5 * (1. + cos(t + 3.14159 / 3.));
    float c3 = 0.5 * (1. + cos(t - 3.14159 / 3.));

    fragColor = vec4(g + c1 * r,r + c2 * b,b + c3 * g,1.);
}
```

## Algorithm

**What to keep from the reference:**
- The `myLength` core: `mod(x + y, animated_divisor) * amplitude_gate` — this is the entire visual character
- Coupled oscillators: `y = sin(abs(uv.y) + x * coupling - 0.5 * t)` where x feeds into y
- Animated divisor blend: `(1-b) * x*y + b * 0.4 * length(uv)` morphing between product-cells and radial-cells
- Amplitude modulation gate: `b2` blend that suppresses/reveals regions over time

**What to change:**
- Discard the entire `mainImage` color/flower logic — that's cosmetic, not relevant to the warp
- Convert scalar field to 2D displacement: evaluate `myLength` twice with different phase offsets (e.g., +1.7 and +3.1) to get independent x and y displacement channels
- Replace `iResolution.x` frequency multiplier with a user-controlled `waveFreq` parameter (the original uses screen width as a frequency, which is resolution-dependent and not intentional design)
- Replace hardcoded time multipliers (0.2, 0.5, 0.3, 0.15) with `animSpeed` scaling
- Add `coupling` parameter to control x→y dependency: `y = sin(abs(uv.y) + coupling * x - ...)` where 0 = independent grid, 1 = full organic coupling
- Add `warpStrength` to scale final displacement amplitude
- Add `divisorBias` to allow static control of the product↔radial cell blend (the reference only animates it)

**Displacement approach:**
```
float fx = fluxField(uv, time, phaseX);
float fy = fluxField(uv, time, phaseY);
vec2 displaced = texCoord + vec2(fx, fy) * warpStrength;
color = texture(inputTexture, displaced);
```

## Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| warpStrength | float | 0.0-0.5 | 0.15 | Displacement amplitude — how far pixels shift |
| cellScale | float | 1.0-20.0 | 6.0 | UV multiplier — higher = more/smaller cells |
| coupling | float | 0.0-1.0 | 0.7 | X→Y wave dependency (0 = axis-aligned grid, 1 = organic flow) |
| waveFreq | float | 1.0-50.0 | 15.0 | Trig oscillation frequency — higher = finer cell detail |
| animSpeed | float | 0.0-2.0 | 1.0 | Overall time multiplier for all animation |
| divisorSpeed | float | 0.0-1.0 | 0.3 | How fast the cell geometry morphs between product and radial modes |
| divisorBias | float | 0.0-1.0 | 0.5 | Static blend between product-cells (0, diagonal ridges) and radial-cells (1, concentric rings) |
| gateSpeed | float | 0.0-0.5 | 0.15 | Amplitude modulation rate — how fast regions fade in/out |

## Modulation Candidates

- **warpStrength**: Intensity of distortion — low values give subtle shimmer, high values shatter the image
- **coupling**: Morphs cell geometry between rigid grids and organic tangles
- **cellScale**: Zoom in/out on the cell structure
- **waveFreq**: Fine detail density — higher adds more internal cell texture
- **divisorBias**: Shifts between diagonal ridge cells and radial ring cells
- **animSpeed**: Overall animation rate

### Interaction Patterns

- **coupling + cellScale (cascading threshold)**: At low coupling, increasing cellScale just makes a finer grid. At high coupling, the same increase creates dense organic tangles where small cells interact with their neighbors. Coupling "unlocks" cellScale's visual complexity.
- **warpStrength + animSpeed (competing forces)**: High strength + slow speed = languid, viscous morphing. High strength + fast speed = chaotic jittering. The ratio controls perceived viscosity — modulating both in opposite directions creates tension between "trying to move" and "being held back."
- **divisorBias + coupling (resonance)**: When divisor morphs between modes AND coupling is high, cell topology restructures dramatically — cells split and merge. With low coupling, the same divisor animation just oscillates between two stable grid types. Coupling amplifies the visual impact of divisor changes.

## Notes

- The `mod()` function creates C0 discontinuities (sharp edges) in an otherwise smooth field — this is what gives the "cellular" look without computing actual Voronoi cells. Much cheaper than distance-based tessellation.
- The reference shader's `iResolution.x` in the cosine is almost certainly unintentional (it makes the pattern resolution-dependent). Replacing with a user parameter is an improvement.
- Performance: single texture lookup per pixel with two scalar field evaluations. Very cheap — no iterative loops or multi-tap sampling.
- Edge case: at `warpStrength` near 0.5, displacement can sample far outside the original UV range. Clamp or wrap UVs to avoid artifacts.
- Quasiperiodic animation: the incommensurate frequency ratios (0.2, 0.3, 0.5 relative to each other) mean the pattern never exactly repeats. Preserving irrational ratios between the internal time multipliers is important for the "never repeats" quality.
