# Shell

A raymarched hollow sphere rendered as luminous outline contours — each march step rotates the view slightly, spreading the surface into a cloud of near-miss curves that reveal the shape as a wireframe silhouette rather than a solid fill. Turbulence distorts the sphere into organic nautilus-like forms with flowing contour lines. At low outline spread the shape is solid; at high values it dissolves into ghostly traced outlines.

## Classification

- **Category**: GENERATORS > Filament (section 11)
- **Pipeline Position**: Generator pass (ping-pong trail buffer + blend compositor)

## Attribution

- **Based on**: "Speak [470]" by Xor
- **Source**: https://www.shadertoy.com/view/33tSzl
- **License**: CC BY-NC-SA 3.0

## Reference Code

```glsl
// Buffer A — audio history scroll (replaced by our FFT texture)
void mainImage( out vec4 O, vec2 I )
{
    I/=iResolution.xy;
    I.y-=.01;
    O = I.y<0. ? texture(iChannel1, I) : texture(iChannel0, I);
}

// Image
/*
    "Speak" by @XorDev

    Playing with music reactive shaders

    <512 playlist:
    https://www.shadertoy.com/playlist/N3SyzR
*/

void mainImage(out vec4 O, vec2 I)
{
    //Animation time
    float t=iTime,
    //Raymarch depth
    z,
    //Step distance
    d,
    //Signed distance
    s,
    //Raymarch iterator
    i;

    //Clear fragColor and raymarch 60 steps
    for(O*=i; i++<6e1;
        //Coloring and brightness
        O+=(cos(i*.1+t+vec4(6,1,2,0))+1.)/d)
    {
        //Sample point (from ray direction)
        vec3 p = z*normalize(vec3(I+I,0)-iResolution.xyy),
        //Rotation axis
        a = normalize(cos(vec3(0,2,4)+t+.1*i));
        //Move camera back 5 units
        p.z+=9.,
        //Rotated coordinates
        a = a*dot(a,p)-cross(a,p);

        //Turbulence loop
        for(d=.6;d<9.;d+=d)
            a-=cos(a*d+t-.1*i).zxy/d;

        //Distance to hollow, distorted sphere
        z+=d=.1*abs(s=length(a)-3.- sin(texture(iChannel0,vec2(1,s)*.1).r/.1));
    }
    //Tanh tonemap
    O = tanh(O/1e3);
}
```

## Algorithm

### What to keep verbatim
- Ray direction from `(I+I - res.xyy)` normalized
- Rodrigues rotation: `a = a*dot(a,p) - cross(a,p)`
- Per-step rotation axis: `normalize(cos(phase + time + outlineSpread * i))` — this is the key technique that creates the outline/wireframe look
- Turbulence: `a -= cos(a*d + time - outlineSpread*i).zxy / d` with doubling frequency (`d += d`)
- Hollow sphere distance: `abs(length(a) - sphereRadius)`
- Additive per-step coloring with `1/d` glow attenuation
- Tanh tonemapping

### What to replace
- Replace `cos(i*.1+t+vec4(6,1,2,0))+1.` coloring with gradient LUT sampling: `textureLod(gradientLUT, vec2(fract(z * colorStretch + time * colorSpeed), 0.5), 0.0).rgb`
- Replace `iTime` with accumulated `time` uniform
- Replace `iResolution` with `resolution` uniform
- Drop Buffer A (audio history scroll) — our FFT texture provides frequency data directly
- Drop `sin(texture(iChannel0,...).r/.1)` sphere displacement — use per-step FFT glow modulation instead (each step maps to a frequency band, audio scales light contribution)
- Add ping-pong trail buffer with decay/blur (same pattern as Muons)
- Add blend compositor output (same pattern as Muons)

### Parameter mapping from reference constants
- `9.` (p.z offset) -> `cameraDistance`
- `3.` (length(a) - 3.) -> `sphereRadius`
- `0.1` (step scale multiplier) -> `ringThickness`
- `vec3(0,2,4)` (axis phase) -> `phaseX`, `phaseY`, `phaseZ`
- `0.1` in `0.1*i` (per-step rotation) -> `outlineSpread`
- `0.6` (turbulence start freq) -> hardcoded
- `d+=d` (doubling) -> `turbulenceGrowth` (as a multiplier, default 2.0)
- `d<9` (turbulence limit) -> `turbulenceOctaves`
- `6e1` (march iterations) -> `marchSteps`
- `/1e3` (tonemap divisor) -> folded into `brightness`

## Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| marchSteps | int | 4-200 | 60 | Ray budget — more steps = denser contour lines |
| turbulenceOctaves | int | 2-12 | 4 | Distortion layers (reference has ~4: 0.6, 1.2, 2.4, 4.8) |
| turbulenceGrowth | float | 1.2-3.0 | 2.0 | Octave frequency multiplier — higher = more chaotic |
| sphereRadius | float | 0.5-10.0 | 3.0 | Hollow sphere size |
| ringThickness | float | 0.01-0.5 | 0.1 | Step size multiplier — lower = thinner contour lines |
| cameraDistance | float | 3.0-20.0 | 9.0 | Depth into volume |
| phaseX | float | -PI-PI | 0.0 | Rotation axis X phase offset |
| phaseY | float | -PI-PI | 2.0 | Rotation axis Y phase offset |
| phaseZ | float | -PI-PI | 4.0 | Rotation axis Z phase offset |
| outlineSpread | float | 0.0-0.5 | 0.1 | Per-step rotation amount — 0 = solid, higher = wireframe |
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

- **outlineSpread**: morphs between solid shape and wireframe dissolution — the signature parameter
- **sphereRadius**: breathing expansion/contraction of the shell
- **turbulenceGrowth**: shifts between smooth curves and chaotic distortion
- **ringThickness**: thickens/thins the contour lines
- **cameraDistance**: zoom in/out of the volume
- **brightness**: overall intensity pulsing

### Interaction Patterns

- **outlineSpread x marchSteps** (cascading threshold): At low march steps, increasing outline spread quickly dissolves the shape into nothing — there aren't enough steps to form visible contour lines. At high march steps, the same spread creates dense layered outlines. March steps gates how much spread the shape can tolerate before disappearing.
- **sphereRadius x ringThickness** (competing forces): Larger radius means the surface is farther from the camera, so rays need bigger steps to reach it. But smaller ringThickness means tinier steps. The tension determines whether the shape renders as clean outlines or dissolves into noise.

## Notes

- The per-step rotation (`outlineSpread * i`) is applied to both the rotation axis AND the turbulence phase (`time - outlineSpread*i`). Both must use the same parameter to maintain the outline coherence. If only the axis rotates per-step but turbulence doesn't, the outline effect is weaker.
- Turbulence uses subtraction (`a -= cos(...)`) and `.zxy` swizzle, unlike Muons' addition and `.yzx`. This produces different curl directions in the distortion field.
- The reference uses 60 march steps (vs Muons' 100, Vortex's 100). The outline rendering style needs fewer steps since the visual comes from step disagreement, not dense accumulation.
- Tonemap divisor `/1e3` is much smaller than Muons' `/1e5` or Vortex's `/7e3`, reflecting the lower step count and different glow accumulation rate.
