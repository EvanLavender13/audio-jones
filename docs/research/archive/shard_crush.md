# Shard Crush

Shatters the input image into noise-rotated angular blocks with prismatic chromatic aberration. Each iteration samples the input texture at a slightly offset position and tints with a spectral color, accumulating a rainbow-fringed mosaic of displaced fragments. Distinct from the existing VHS-style Glitch (scanline corruption) — this is geometric block shattering with spectral color separation.

## Classification

- **Category**: TRANSFORMS > Retro (section 6)
- **Pipeline Position**: Output stage, reorderable transform chain
- **Dual Module**: Transform counterpart to Digital Shard (generator)

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

Same noise-rotation loop as the generator, but instead of accumulating procedural color, each iteration displaces UV coordinates and samples the input texture with a spectral tint.

Per iteration `i` (from -1 to +1 in `iterations` steps):

1. Compute centered coordinates with per-iteration aberration offset: `(fragTexCoord - center) / (zoom + i * aberrationSpread)`
2. Sample noise texture to get quantized rotation angle: `ceil(N(c) * rotationLevels) * (2*PI / rotationLevels)`
3. Build 2x2 rotation matrix, rotate coordinates
4. Compute displacement mask: `ceil(cos(rotated.x / N(stripeNoise)))` (or softened version)
5. Use the rotated/displaced coordinates to derive a sample UV back in texture space
6. Sample input texture at displaced UV
7. Tint with spectral color: `vec3(1.+i, 2.-abs(2.*i), 1.-i)` (red through green to blue)
8. Accumulate with `1/iterations` weighting

### Adaptation to this codebase

- **Noise source**: Must use procedural cell noise hash, NOT the shared noise texture. The codebase's `NoiseTextureGet()` is 1024x1024 with trilinear filtering — the algorithm requires nearest-filtered 64x64 noise (constant value within each cell, no interpolation). Without this, iterations decorrelate and averaging destroys all hard edges, producing smooth blobs instead of angular blocks. Use:
  ```glsl
  float hash21(vec2 p) {
      return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
  }
  #define N(x) hash21(floor((x) * 64.0 / noiseScale))
  ```
  `floor()` quantizes to integer cells. The `noiseTexture` uniform can be declared but will be unused.
- **Input texture**: `texture0` (standard transform input)
- **Spectral color**: Keep the baked `vec3(1.+i, 2.-abs(2.*i), 1.-i)` ramp — this IS the chromatic aberration, not a palette choice. It produces the prismatic RGB fringing that defines the transform's look.
- **UV recovery**: After the noise-rotation displacement, map coordinates back to UV space for texture sampling. Clamp to [0,1] to avoid edge artifacts.
- **Softness param**: Use `smoothstep(-softness, softness + 0.001, cosVal)` unconditionally. The `+ 0.001` epsilon prevents undefined behavior from equal `smoothstep` edges when softness is exactly 0, while still producing a hard step. No branch needed.
- **Mix param**: `mix(original, accumulated, mixAmount)` for blending with unmodified input
- **Coordinates**: Use `(fragTexCoord - center) * resolution` for centered coordinates per shader conventions

### Key constants (same geometric meaning as generator)

- `0.4` -> `zoom`, `1e-2` -> `aberrationSpread`, `8.0` -> `rotationLevels`, `/64.` -> `noiseScale`

## Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| iterations | int | 20-100 | 100 | Loop count; fewer = faster, less spectral spread |
| zoom | float | 0.1-2.0 | 0.4 | Base coordinate scale |
| aberrationSpread | float | 0.001-0.05 | 0.01 | Per-iteration UV offset; controls prismatic width |
| noiseScale | float | 16.0-256.0 | 64.0 | Noise texture tiling scale |
| rotationLevels | float | 2.0-16.0 | 8.0 | Angle quantization; fewer = chunkier shatter |
| softness | float | 0.0-1.0 | 0.0 | Hard binary snap to smooth gradient |
| speed | float | 0.1-5.0 | 1.0 | Animation rate |
| mix | float | 0.0-1.0 | 1.0 | Blend displaced result with original |

## Modulation Candidates

- **zoom**: Breathing scale shifts the shard pattern
- **aberrationSpread**: Widens/narrows prismatic color fringing
- **rotationLevels**: Snaps between chunky geometric and fine chaotic shattering
- **softness**: Hard digital blocks vs dreamy prismatic smear
- **mix**: Fade the effect in and out

### Interaction Patterns

- **softness vs aberrationSpread** (resonance): High softness + high aberration = dreamy rainbow smear. Low softness + high aberration = sharp spectral layering. The visual character shifts dramatically when both move together.
- **rotationLevels vs zoom** (competing forces): Same density tension as the generator — few large plates vs many small shards, modulated together creates shifting geometric complexity.

## Lessons from Digital Shard Implementation

See archived plan: `docs/plans/archive/digital-shard.md` (Implementation Notes section)

**Critical — noise source**: The shared noise texture (`NoiseTextureGet()`) is 1024x1024 with trilinear filtering. This algorithm requires nearest-filtered 64x64 noise where each cell returns a constant value. The `floor()` in the hash macro is what creates the hard cell boundaries. Without it, the ~100 iterations sample decorrelated noise values and averaging produces smooth ~0.5 everywhere — completely destroying the block structure. This was the single biggest implementation problem.

**Coloring note**: The generator version tried mapping accumulated scalar brightness through a gradient LUT, but since all "on" blocks accumulate to similar values, a rainbow gradient collapsed to gray. The fix was per-cell gradient sampling using `cellId`. This does NOT apply to Shard Crush — the spectral `vec3(1.+i, 2.-abs(2.*i), 1.-i)` ramp is the chromatic aberration itself and should be kept as-is.

**Softness**: `smoothstep(-softness, softness + 0.001, cosVal)` works unconditionally — no need for the `step()`/`smoothstep()` branch originally planned.

## Notes

- Unlike the existing VHS Glitch (scanline-based corruption), this is angular/geometric shattering with prismatic color
- The spectral tint is intentionally hardcoded, not user-configurable — it's the chromatic aberration itself, not a palette
- Shares cell noise hash, iteration structure, and softness logic with Digital Shard generator — shader code can share helper functions or be kept independent (simpler)
