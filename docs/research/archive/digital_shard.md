# Digital Shard

Procedural noise-driven angular shards that shatter the screen into blocky chromatic fragments. Each pixel accumulates color across ~100 iterations with slight positional offsets, creating a spectral aberration effect where blocks of noise-rotated geometry snap to quantized angles. FFT energy drives per-cell brightness through a user-configurable gradient LUT.

## Classification

- **Category**: GENERATORS > Texture (section 12)
- **Pipeline Position**: Output stage, blended onto scene via blend compositor
- **Dual Module**: Generator counterpart to Shard Crush (transform)

## Attribution

- **Based on**: "Gltch" by Xor (@XorDev)
- **Source**: https://www.shadertoy.com/view/mdfGRs
- **License**: CC BY-NC-SA 3.0

## References

- [XorDev tweet](https://twitter.com/XorDev/status/1584603104292769792) - Original 300-char tweetcart version

## Reference Code

```glsl
/*
    "Gltch" by @XorDev

    Tweet: twitter.com/XorDev/status/1584603104292769792
    Twigl : t.co/C60Z76weG4

    <300 chars playlist: https://www.shadertoy.com/playlist/fXlGDN
    -3 Thanks to FabriceNeyret2
*/
void mainImage(out vec4 O, vec2 I)
{
    //Noise macro
    #define N(x) texture(iChannel0,(x)/64.).r

    //Clear fragcolor
    O -= O;
    //Res for scaling, position and scaled coordinates
    vec2 r = iResolution.xy, c;

    //Iterate from -1 to +1 in 100 steps
    for(float i=-1.; i<1.; i+=.02)
        //Compute centered position with aberation scaling
        c=(I+I-r)/r.y/(.4+i/1e2),
        //Generate random glitchy pattern (rotate in 45 degree increments)
        O += ceil(cos((c*mat2(cos(ceil(N(c/=(.1+N(c)))*8.)*.785+vec4(0,33,11,0)))).x/
        //Generate random frequency stripes
        N(N(c)+ceil(c)+iTime)))*
        //Pick aberration color (starting red, ending in blue)
        vec4(1.+i,2.-abs(i+i),1.-i,1)/1e2;
}
```

## Algorithm

The reference shader iterates 100 times, each iteration slightly shifting the sampling position to create chromatic spread. At each step it:

1. Centers and scales coordinates with a per-iteration aberration offset (`0.4 + i/100`)
2. Samples noise texture to get a rotation angle, quantized to 45-degree increments (`ceil(N(...) * 8) * PI/4`)
3. Builds a 2x2 rotation matrix from that quantized angle
4. Rotates the coordinates and takes the x component
5. Divides by another noise lookup (stripe frequency) to create banded patterns
6. Applies `ceil(cos(...))` for hard binary thresholding (0 or 1)
7. Accumulates with spectral color weighting

### Adaptation to this codebase

- **Noise source**: Replace `texture(iChannel0, x/64.)` with `texture(noiseTexture, x / noiseScale)` using `NoiseTextureGet()`. The shared 1024x1024 RGBA noise texture with repeat wrap is a direct replacement.
- **Color**: Replace the baked spectral ramp `vec4(1.+i, 2.-abs(i+i), 1.-i, 1)` with gradient LUT sampling. Accumulate a scalar brightness across iterations, then map through `gradientLUT` at the end.
- **FFT brightness**: After the iteration loop, modulate final brightness per-cell using the standard FFT frequency spread pattern (baseFreq to maxFreq in log space).
- **Softness param**: Replace `ceil(cos(x))` with `smoothstep(threshold - softness, threshold + softness, cos(x))` where `threshold = 0.0`. When `softness = 0`, this collapses to the hard binary snap of the original.
- **Coordinates**: Use `(fragTexCoord - center) * resolution` for centered coordinates per shader conventions.

### Key constants from reference with geometric meaning

- `0.4` = base zoom factor (controls how much of the pattern is visible) -> `zoom` param
- `1e-2` = aberration spread per iteration (chromatic offset range) -> `aberrationSpread` param
- `8.0` = rotation quantization levels (8 * PI/4 = full circle in 45-deg steps) -> `rotationLevels` param
- `0.785` = PI/4 (45 degrees per rotation step) — derived from `rotationLevels`, not independent
- `/64.` = noise texture tiling scale -> `noiseScale` param
- `0.1` = minimum scale floor in noise feedback `(.1 + N(c))` -> keep as constant (prevents division by zero)

## Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| iterations | int | 20-100 | 100 | Loop count; fewer = faster, less chromatic spread |
| zoom | float | 0.1-2.0 | 0.4 | Base coordinate scale; larger = more zoomed in |
| aberrationSpread | float | 0.001-0.05 | 0.01 | Per-iteration position offset; controls chromatic width |
| noiseScale | float | 16.0-256.0 | 64.0 | Noise texture tiling; smaller = finer noise cells |
| rotationLevels | float | 2.0-16.0 | 8.0 | Angle quantization steps; fewer = chunkier blocks |
| softness | float | 0.0-1.0 | 0.0 | Blend from hard binary snap to smooth gradient |
| speed | float | 0.1-5.0 | 1.0 | Animation rate multiplier |
| baseFreq | float | 27.5-440.0 | 55.0 | Lowest FFT frequency (Hz) |
| maxFreq | float | 1000-16000 | 14000.0 | Highest FFT frequency (Hz) |
| gain | float | 0.1-10.0 | 2.0 | FFT magnitude amplifier |
| curve | float | 0.1-3.0 | 1.5 | Contrast exponent on FFT energy |
| baseBright | float | 0.0-1.0 | 0.15 | Baseline brightness for silent cells |
| gradient | ColorConfig | — | gradient mode | Color palette via LUT |
| blendMode | EffectBlendMode | — | SCREEN | Blend compositor mode |
| blendIntensity | float | 0.0-5.0 | 1.0 | Blend strength |

## Modulation Candidates

- **zoom**: Breathing scale changes how much of the shard field is visible
- **aberrationSpread**: Widens/narrows the chromatic fringing dynamically
- **noiseScale**: Shifts the size of noise-driven blocks
- **rotationLevels**: Snaps between chunky (2-3 angles) and fine (16 angles) shattering
- **softness**: Transitions between hard digital and smooth dreamy looks
- **speed**: Varies animation rate

### Interaction Patterns

- **rotationLevels vs zoom** (competing forces): Low rotation levels produce few large angular plates; increasing zoom multiplies them. These push in opposite directions on visual density — modulating both creates shifting tension between sparse geometric and dense chaotic.
- **softness vs aberrationSpread** (resonance): When both spike together, hard digital blocks dissolve into dreamy prismatic smears. When softness is low but aberration is high, you get sharp spectral layering. The combination unlocks states neither parameter reaches alone.
- **noiseScale vs rotationLevels** (cascading threshold): noiseScale controls how many distinct cells exist; rotationLevels controls how many angle options each cell picks from. At low noiseScale + low levels = minimal geometric blocks. Both must increase together to produce a dense chaotic shard field.

## Notes

- 100 iterations with noise texture reads is well within budget for modern GPUs
- The noise texture's repeat wrap and trilinear filtering are important — the reference relies on smooth tiling noise
- The `0.1 + N(c)` floor prevents coordinate collapse when noise returns zero
