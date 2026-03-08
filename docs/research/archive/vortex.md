# Vortex

Smooth, swoopy volumetric spirals that fill the frame with luminous curving ribbons of color. A raymarched distorted hollow sphere where turbulence overwhelms the seed shape, producing flowing tendrils that range from languid smoke curls at low settings to active whirlpool spin at high turbulence. Visually distinct from Muons' chaotic filaments — Vortex is organic, continuous, and ribbon-like.

## Classification

- **Category**: GENERATORS > Filament (section 11)
- **Pipeline Position**: Generator pass (ping-pong trail buffer + blend compositor)

## Attribution

- **Based on**: "Vortex [265]" by Xor
- **Source**: https://www.shadertoy.com/view/wctXWN
- **License**: CC BY-NC-SA 3.0

## References

- [3D Fire](https://www.shadertoy.com/view/3XXSWS) - Predecessor shader by XorDev that Vortex is based on

## Reference Code

```glsl
/*
    "Vortex" by @XorDev

    https://x.com/XorDev/status/1930594981963505793

    An experiment based on my "3D Fire":
    https://www.shadertoy.com/view/3XXSWS
*/
void mainImage(out vec4 O, vec2 I)
{
    //Raymarch iterator
    float i,
    //Raymarch depth
    z = fract(dot(I,sin(I))),
    //Raymarch step size
    d;
    //Raymarch loop (100 iterations)
    for(O *= i; i++<1e2;
        //Sample coloring and glow attenuation
        O+=(sin(z+vec4(6,2,4,0))+1.5)/d)
    {
        //Raymarch sample position
        vec3 p = z * normalize(vec3(I+I,0) - iResolution.xyy);
        //Shift camera back
        p.z += 6.;
        //Distortion (turbulence) loop
        for(d=1.; d<9.; d/=.8)
            //Add distortion waves
            p += cos(p.yzx*d-iTime)/d;
        //Compute distorted distance field of hollow sphere
        z += d = .002+abs(length(p)-.5)/4e1;
    }
    //Tanh tonemapping
    //https://www.shadertoy.com/view/ms3BD7
    O = tanh(O/7e3);
}
```

## Algorithm

### What to keep verbatim
- Raymarch structure: ray direction from `(I+I - res.xyy)` normalized
- Dithered ray start: `z = fract(dot(I, sin(I)))` for banding suppression
- Turbulence loop: `p += cos(p.yzx * d - time) / d` with geometric scaling `d /= growth`
- Hollow sphere distance: `abs(length(p) - radius)`
- Step formula: `floor + distance / detail`
- Additive color accumulation per step (not winner-takes-all)
- Tanh tonemapping

### What to replace
- Replace `sin(z + vec4(6,2,4,0)) + 1.5` coloring with gradient LUT sampling: `textureLod(gradientLUT, vec2(fract(z * colorStretch + time * colorSpeed), 0.5), 0.0).rgb`
- Replace `iTime` with accumulated `time` uniform
- Replace `iResolution` with `resolution` uniform
- Add ping-pong trail buffer with decay/blur (same pattern as Muons)
- Add FFT reactivity: each march step maps to a frequency band in log space, modulates per-step glow contribution
- Add blend compositor output (same pattern as Muons)

### Parameter mapping from reference constants
- `6.` (p.z offset) -> `cameraDistance`
- `0.5` (length(p) - 0.5) -> `sphereRadius`
- `4e1` (distance divisor) -> `surfaceDetail`
- `0.002` (step floor) -> hardcoded
- `0.8` (d /= 0.8) -> `turbulenceGrowth`
- `9.` (d < 9) -> derived from `turbulenceOctaves` (loop until d exceeds limit based on growth^octaves)
- `1e2` (march iterations) -> `marchSteps`
- `/7e3` (tonemap divisor) -> folded into `brightness`

## Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| marchSteps | int | 4-200 | 50 | Ray budget — more steps reveal deeper structure |
| turbulenceOctaves | int | 2-12 | 8 | Distortion layers — fewer = smooth, more = detailed |
| turbulenceGrowth | float | 0.5-0.95 | 0.8 | Octave frequency growth — higher = denser chaos |
| sphereRadius | float | 0.1-3.0 | 0.5 | Seed shape size — larger = more expansive spirals |
| surfaceDetail | float | 5.0-100.0 | 40.0 | Step precision near surface — higher = finer ribbons |
| cameraDistance | float | 3.0-20.0 | 6.0 | Depth into volume |
| colorSpeed | float | 0.0-2.0 | 0.5 | LUT scroll rate over time |
| colorStretch | float | 0.1-5.0 | 1.0 | Spatial color frequency through volume depth |
| brightness | float | 0.1-5.0 | 1.0 | Intensity multiplier before tonemap |
| decayHalfLife | float | 0.1-10.0 | 2.0 | Trail persistence duration in seconds |
| trailBlur | float | 0.0-1.0 | 1.0 | Trail blur amount |
| baseFreq | float | 27.5-440.0 | 55.0 | Lowest FFT frequency Hz |
| maxFreq | float | 1000-16000 | 14000.0 | Highest FFT frequency Hz |
| gain | float | 0.1-10.0 | 2.0 | FFT sensitivity multiplier |
| curve | float | 0.1-3.0 | 1.5 | FFT contrast curve exponent |
| baseBright | float | 0.0-1.0 | 0.15 | Minimum brightness floor when silent |
| blendMode | enum | - | SCREEN | Blend compositing mode |
| blendIntensity | float | 0.0-5.0 | 1.0 | Blend compositing strength |

## Modulation Candidates

- **sphereRadius**: expands/contracts the seed shape, breathing effect
- **turbulenceGrowth**: shifts between smooth drifting and chaotic spinning
- **surfaceDetail**: sharpens/softens ribbon edges
- **cameraDistance**: zoom in/out of the volume
- **colorStretch**: shifts spatial color distribution
- **brightness**: overall intensity pulsing

### Interaction Patterns

- **sphereRadius x turbulenceGrowth** (competing forces): Small radius wants a tight contained shape; high turbulence growth flings it apart. The visual shows their tension — modulating both creates a push-pull between contained orb and screen-filling spirals. Different energy levels in a song shift the balance.
- **surfaceDetail x marchSteps** (cascading threshold): Higher detail means tinier steps near the surface, requiring more march steps to penetrate deep enough. At low march steps, increasing detail past a threshold causes the image to go dark — the rays exhaust their budget before reaching visible structure.

## Notes

- Performance: geometric turbulence scaling (`d /= 0.8`) means more effective octaves than Muons' linear scaling at the same iteration count. At 8 octaves with growth 0.8, max frequency multiplier reaches ~5.96. GPU cost scales linearly with marchSteps * turbulenceOctaves.
- The dithered ray start (`fract(dot(I, sin(I)))`) is critical for avoiding banding artifacts in the smooth gradients. Keep this verbatim.
- Trail buffer decay interacts with the additive accumulation — longer trails blend more frames of the volume rendering, creating the impression of thicker, more persistent ribbons.
