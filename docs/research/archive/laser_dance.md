# Laser Dance

Glowing laser-like ribbons weaving through 3D space, rendered via volumetric raymarching. Cosine interference at two frequencies creates sharp creases where the beams appear — the viewer floats inside an ambient, slowly morphing light structure. FFT drives brightness so the lasers breathe with the music. Gradient LUT colors by ray depth.

## Classification

- **Category**: GENERATORS > Filament
- **Pipeline Position**: Generator (blend compositor)
- **Compute Model**: Single fragment shader, no ping-pong or compute

## Attribution

- **Based on**: "Laser Dance" by @XorDev
- **Source**: https://www.shadertoy.com/view/tct3Rf
- **License**: CC BY-NC-SA 3.0

## References

- [Volumetric Raymarching - GM Shaders (XorDev)](https://mini.gmshaders.com/p/volumetric) - Same core technique: `dot(sin(p), cos(p*0.618).yzx)` density field with tanh tonemapping. Laser Dance swaps `dot` for `length(min(...))` to get sharp creases instead of smooth clouds.
- [Raymarching Distance Fields - Inigo Quilez](https://iquilezles.org/articles/raymarchingdf/) - Domain repetition with periodic functions, step size adaptation.

## Reference Code

```glsl
/*
    "Laser Dance" by @XorDev

    https://x.com/XorDev/status/1923037860485075114
*/
void mainImage(out vec4 O, vec2 I)
{
    //Raymarch iterator, step distance, depth
    float i, d, z;

    //Clear fragcolor and raymarch 100 steps
    for(O *= i; i++ < 1e2;
    //Pick colors and attenuate
    O += (cos(z + vec4(0,2,3,0)) + 1.5) / d / z)
    {
        //Raymarch sample point
        vec3 p = z * normalize(vec3(I+I,0) - iResolution.xyy) + .8;
        //Reflection distance
        d = max(-p.y, 0.);
        //Reflect y-axis
        p.y += d+d - 1.;
        //Step forward slowly with a bias for soft reflections
        z += d = .3 * (.01 + .1*d +
        //Lazer distance field
        length(min( p = cos(p + iTime) + cos(p / .6).yzx, p.zxy))
        / (1.+d+d+d*d));
    }
    //Tanh tonemapping
    O = tanh(O / 7e2);
}
```

## Algorithm

### Core distance field (keep verbatim)

The laser structure is two cosine fields at different frequencies combined with a crease operation:

```glsl
p = cos(p + time) + cos(p / freqRatio).yzx;
float dist = length(min(p, p.zxy));
```

- `cos(p + time)` — primary wave field, animated
- `cos(p / freqRatio).yzx` — secondary field at lower frequency, swizzled for asymmetry
- `min(p, p.zxy)` — component-wise minimum creates sharp edges (the "laser lines")
- `length(...)` — converts to scalar distance

The `freqRatio` (0.6 in original) controls density. Lower = more interference = denser web. Higher = fewer, more distinct beams.

### Adaptations for AudioJones

1. **Remove floor reflection**: Drop `d = max(-p.y, 0.)`, the `p.y += d+d - 1.` reflection, and all `d`-dependent terms in the step size and color accumulation. Replace with simple volumetric accumulation: `color += sampleColor / dist / z`.

2. **Replace rainbow with gradient LUT**: Instead of `cos(z + vec4(0,2,3,0))`, sample `gradientLUT` using normalized depth: `texture(gradientLUT, vec2(z / maxDepth, 0.5))`.

3. **Add FFT brightness**: Follow standard FFT audio pattern (baseFreq, maxFreq, gain, curve, baseBright). Sample FFT texture to modulate overall brightness, applied as a multiplier on the accumulated color before tonemapping.

4. **Camera offset uniform**: The `+0.8` offset becomes a `cameraOffset` parameter controlling viewing angle into the structure.

5. **Step count**: The 100-step loop is fixed (not a parameter) — controls quality vs. performance. Can be reduced to ~64 if needed.

6. **Tonemapping divisor**: The `700.0` in `tanh(O / 7e2)` becomes tied to a `brightness` parameter.

## Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| speed | float | 0.1-5.0 | 1.0 | Time accumulation rate for animation |
| freqRatio | float | 0.3-1.5 | 0.6 | Ratio between the two cosine fields — lower = denser laser web |
| cameraOffset | float | 0.2-2.0 | 0.8 | Camera position offset into the structure |
| brightness | float | 0.5-3.0 | 1.0 | Overall output brightness (tonemapping divisor) |
| baseFreq | float | 27.5-440.0 | 55.0 | Lowest FFT frequency mapped |
| maxFreq | float | 1000-16000 | 14000.0 | Highest FFT frequency mapped |
| gain | float | 0.1-10.0 | 2.0 | FFT sensitivity |
| curve | float | 0.1-3.0 | 1.5 | Contrast exponent on FFT magnitudes |
| baseBright | float | 0.0-1.0 | 0.15 | Minimum brightness when audio is silent |

## Modulation Candidates

- **speed**: animation pace — slow for ambient drift, fast for frenetic motion
- **freqRatio**: density of the laser web — shifts between sparse beams and dense mesh
- **cameraOffset**: viewpoint into the structure — creates zoom/dolly feel
- **brightness**: overall intensity — pulse with audio energy

### Interaction Patterns

**freqRatio + speed (resonance)**: At certain frequency ratios, the two cosine fields align periodically, creating moments where the laser structure briefly simplifies into clean geometric shapes before dissolving back into complexity. Speed controls how fast these alignment moments pass. When both are modulated by related audio bands, you get rare convergence flashes during specific harmonic content.

## Notes

- The 100-step raymarch loop is the main performance cost. At 1080p this should be fine; at 4K may need to reduce steps or use half-res flag.
- The `.yzx` swizzle is load-bearing — it breaks what would otherwise be cubic symmetry. Changing the swizzle pattern changes the character of the laser structure entirely.
- `freqRatio` near golden ratio (0.618) produces maximum visual complexity due to irrational frequency relationship (no exact periodicity). This is what XorDev uses in both Laser Dance (0.6) and the volumetric tutorial (0.618).
