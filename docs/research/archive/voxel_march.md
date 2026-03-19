# Voxel March

Fly through an infinite voxelized 3D structure where rounded sphere shells repeat through domain-folded space, each surface accumulating glowing color as rays pass through. Voxel density ripples across sin-based boundaries creating regions of chunky blocks mixed with fine detail, while configurable layered surfaces sample distinct gradient colors and boundary highlights trace the seams between density zones.

## Classification

- **Category**: GENERATORS > Field
- **Pipeline Position**: Output stage — scratch pass (raymarching) then blend compositor
- **Section Index**: 13

## Attribution

- **Based on**: "Refactoring" by OldEclipse
- **Source**: https://www.shadertoy.com/view/WcycRw
- **License**: CC BY-NC-SA 3.0

Secondary reference (uniform grid / blob variant):
- **Based on**: "Interconnected Blobs" by OldEclipse
- **Source**: https://www.shadertoy.com/view/3XSfRD
- **License**: CC BY-NC-SA 3.0

## References

- [Refactoring (Shadertoy)](https://www.shadertoy.com/view/WcycRw) - Primary reference: sin-boundary voxel quantization with dual shell surfaces
- [Interconnected Blobs (Shadertoy)](https://www.shadertoy.com/view/3XSfRD) - Secondary reference: uniform voxel grid with sinusoidal warping

## Reference Code

### Primary: "Refactoring" by OldEclipse

```glsl
#define S(p) sin(p + iTime)
#define A(u,i) p = mod(p,4.)-2., u = abs(length(p) - 2.8) + .01, c += (vec3(i,1,2-i) - p*.5 + .06/s)/u,

void mainImage( out vec4 O, vec2 I ){
    vec3 p, c;
    for(float i, t=.1, v, w, s; i++<70.;

        // Position
        p = vec3(1.,1.,iTime) + t*normalize(vec3(I+I,0)-iResolution.xyy),

        // Voxels with varying size
        s = 16. - 4.*( step(0., S(p.x)) + step(0., S(p.y+1.)) + step(0., S(p.z+2.)) ),
        p=round(p*s)/s,

        // Highlights on voxel size boundaries
        s = min( min(abs(S(p.x)), abs(S(p.y+1.)) ), abs(S(p.z+2.)) ) + .01,

        // Define blue & red surfaces and add color
        A(v,0)
        A(w,2)

        // March forwards
        t += .3*min(v,w)
    )

    O.rgb = tanh(c/2e3);
}
```

### Secondary: "Interconnected Blobs" by OldEclipse

```glsl
#define M(p) length(mod(p,4.)-2.)

void mainImage( out vec4 O, vec2 I ){
    float i, t=.1, v, w;
    for (O*=i; i++<70.; t += .2*min(v,w*w)){
        // Position
        vec3 p = t*normalize(vec3(I+I,0)-iResolution.xyy);
        p.z += iTime;

        // Voxel effect
        p = round(p*24.)/12.;

        // Transformation
        p += sin(p.zyx);
        p += sin(p.zxy*2.)/2.;

        // Blobs
        v = abs(M(p)-.99) + .02;

        // Lines
        w = min( min( M(p.xy), M(p.xz) ), M(p.yz) ) +.06;

        // Add color
        O.rgb += (1. + cos(p*.8))/v + vec3(3,2,1)/w;
    }

    O = tanh(O*O/1e6);
}
```

## Algorithm

### Substitution Table

Apply these substitutions to the primary reference code ("Refactoring") line by line.

| Original | Replace with | Reason |
|----------|-------------|--------|
| `iTime` in ray origin `vec3(1.,1.,iTime)` | `flyPhase` uniform | CPU-accumulated forward speed |
| `iTime` in `S(p)` macro → `sin(p + iTime)` | `gridPhase` uniform | Separate grid animation rate |
| `iResolution` | `resolution` uniform | Standard generator uniform |
| `70.` iteration count | `marchSteps` uniform (int) | Configurable quality (30-80) |
| `0.3` step multiplier | `stepSize` uniform | Tunable march rate |
| `16.` base voxel scale | `voxelScale` uniform | Tunable grid density |
| `4.` coefficient in `16. - 4.*(...)` | `voxelScale * voxelVariation * 0.25` | When `voxelVariation=0`, grid is uniform |
| `4.` cell size in `mod(p,4.)` | `cellSize` uniform | Domain repetition period |
| `2.` offset in `mod(p,4.)-2.` | `cellSize * 0.5` | Derived — always half the cell size |
| `2.8` shell radius | `shellRadius` uniform | Surface geometry size |
| Fixed 2 calls to `A` macro | Loop `surfaceCount` times (int, 1-3) | Configurable surface layers |
| `vec3(i,1,2-i)` hardcoded color | `texture(gradientLUT, vec2(gradientT, 0.5)).rgb` | Per-surface gradient offset: `gradientT = float(surf) / float(max(surfaceCount-1, 1))` |
| `p*.5` position tint | `p * positionTint` | Configurable position color influence |
| `0.06/s` highlight term | `highlightIntensity / s` | Configurable boundary highlight |
| `tanh(c/2e3)` | `tanh(c * tonemapGain)` | Tunable tonemap exposure |

### Keep Verbatim

- `round(p*s)/s` — voxel quantization
- `step(0., sin(...))` — sin-boundary voxel density classification
- `min(abs(sin(...)), abs(sin(...)), abs(sin(...)))` — boundary highlight distance
- `abs(length(p) - shellRadius) + epsilon` — sphere shell SDF
- Domain fold: `mod(p, cellSize) - cellSize * 0.5` (successive folds per surface create mirrored geometry — this is the key to the dual-surface look)
- `t += stepSize * min(v, w, ...)` — march step based on nearest surface distance
- Ray direction: `normalize(vec3(uv, 1.0))` (adapt from `vec3(I+I,0)-iResolution.xyy`)

### Surface Loop

The original's `A` macro is called twice, each time MODIFYING `p` via the `mod` fold. This means the second surface sees a mirrored version of space. With a configurable surface count, the loop applies the fold iteratively — each surface layer sees a successively re-folded space, creating nested/mirrored geometry:

```
for each surface s in 0..surfaceCount-1:
    p = mod(p, cellSize) - cellSize * 0.5
    dist = abs(length(p) - shellRadius) + epsilon
    gradientT = float(s) / float(max(surfaceCount - 1, 1))
    surfColor = texture(gradientLUT, vec2(gradientT, 0.5)).rgb
    color += (surfColor - p * positionTint + highlightIntensity / boundaryDist) / dist * brightness
    track min distance for march step
```

### FFT Integration

Follow the standard generator FFT frequency spread pattern. Map march step index to frequency band in log space from `baseFreq` to `maxFreq`. Each depth layer gets audio-reactive brightness:

```
t0 = float(step) / float(marchSteps - 1)
t1 = float(step + 1) / float(marchSteps)
freqLo = baseFreq * pow(maxFreq / baseFreq, t0)
freqHi = baseFreq * pow(maxFreq / baseFreq, t1)
→ sample BAND_SAMPLES across [freqLo, freqHi]
→ brightness = baseBright + pow(energy * gain, curve)
```

This creates depth-dependent audio reactivity — near geometry responds to different frequencies than far geometry.

### Voxel Mode Blending

The `voxelVariation` parameter controls how much the sin-boundary density variation is applied:

```
// In the original: s = 16. - 4.*(step(0,sin(x)) + step(0,sin(y)) + step(0,sin(z)))
// Adapted: blend between uniform (sinCount=0) and full variation (sinCount=actual)
float sinCount = step(0., sin(p.x + gridPhase)) + step(0., sin(p.y + 1.0 + gridPhase)) + step(0., sin(p.z + 2.0 + gridPhase));
float s = voxelScale - voxelScale * 0.25 * voxelVariation * sinCount;
```

When `voxelVariation = 0`: uniform grid at `voxelScale` density.
When `voxelVariation = 1`: full sin-boundary variation (faithful to original).

### Camera

Embed `DualLissajousConfig` for camera pan, following the Light Medley pattern. The Lissajous output offsets the ray origin XY.

## Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| marchSteps | int | 30-80 | 60 | Raymarching iterations (quality vs. performance) |
| flySpeed | float | 0.0-5.0 | 1.0 | Forward travel speed through the structure |
| gridAnimSpeed | float | 0.0-3.0 | 1.0 | Sin-boundary animation rate |
| stepSize | float | 0.1-0.5 | 0.3 | March step size multiplier |
| voxelScale | float | 4.0-32.0 | 16.0 | Base voxel grid density |
| voxelVariation | float | 0.0-1.0 | 1.0 | Blend from uniform grid (0) to sin-varying density (1) |
| cellSize | float | 1.0-8.0 | 4.0 | Domain repetition period |
| shellRadius | float | 0.5-4.0 | 2.8 | Sphere shell surface radius |
| surfaceCount | int | 1-3 | 2 | Number of layered shell surfaces |
| highlightIntensity | float | 0.0-0.5 | 0.06 | Boundary seam highlight strength |
| positionTint | float | 0.0-1.0 | 0.5 | How much 3D position shifts surface color |
| tonemapGain | float | 0.0001-0.002 | 0.0005 | Tonemap exposure (maps to `tanh(c * gain)`) |
| baseFreq | float | 27.5-440.0 | 55.0 | FFT low frequency bound |
| maxFreq | float | 1000-16000 | 14000.0 | FFT high frequency bound |
| gain | float | 0.1-10.0 | 2.0 | FFT magnitude gain |
| curve | float | 0.1-3.0 | 1.5 | FFT response curve |
| baseBright | float | 0.0-1.0 | 0.15 | Minimum brightness without audio |

## Modulation Candidates

- **flySpeed**: Travel pace — slow drift vs. rushing forward
- **gridAnimSpeed**: How fast voxel density boundaries shift and ripple
- **voxelScale**: Overall grid coarseness — chunky blocks vs. fine detail
- **voxelVariation**: Uniform crystalline grid vs. organic density variation
- **cellSize**: Domain repetition period — tighter cells vs. wider open chambers
- **shellRadius**: Shell surface size — small scattered orbs vs. large enclosing spheres
- **surfaceCount**: Number of visible overlapping surfaces (1-3)
- **highlightIntensity**: Brightness of boundary seam lines
- **positionTint**: How much spatial position shifts the gradient color
- **tonemapGain**: Overall exposure/brightness of the accumulated scene
- **stepSize**: March rate affects how much detail resolves — smaller steps = denser color

### Interaction Patterns

**Cascading threshold — shellRadius + FFT gain**: At low gain, only the loudest frequencies produce visible brightness. The shell geometry exists everywhere but only "lights up" where FFT energy exceeds the visibility floor set by `baseBright`. Quiet passages show faint shells; loud passages flood the structure with color. Modulating `shellRadius` with a slow LFO while `gain` responds to energy creates structures that breathe into and out of visibility.

**Competing forces — voxelScale + cellSize**: Voxel scale controls quantization granularity while cell size controls domain repetition period. When voxel cells approach the repetition period, geometry breaks into abstract moire-like patterns. When cells are much smaller than the period, shells resolve cleanly. Modulating these against each other creates tension between order and abstraction.

**Resonance — flySpeed + gridAnimSpeed**: Forward motion carries the viewer through sin-boundary density transitions. The grid animation shifts those boundaries independently. When the two rates align, density transitions arrive rhythmically. When misaligned, transitions become unpredictable. Both on beat-reactive modulation creates moments where rhythm locks the visual pulse.

## Notes

- The `A` macro's successive `mod` folds are the key to the dual-surface look — each surface sees progressively re-folded space. With 3 surfaces, the third fold creates a third distinct mirrored geometry.
- `tanh` tonemap handles the unbounded color accumulation naturally — no clamping needed in the loop.
- Performance scales linearly with `marchSteps * surfaceCount`. Worst case (80 steps, 3 surfaces) is 240 SDF evaluations per pixel. Half-res flag available if needed.
- The secondary reference ("Interconnected Blobs") uses `round(p*24)/12` for uniform voxelization — this informs the `voxelVariation = 0` mode where the sin-boundary logic is bypassed in favor of a clean uniform grid.
- CPU-side phase accumulators: `flyPhase += flySpeed * dt`, `gridPhase += gridAnimSpeed * dt`. Both speeds are NEVER accumulated in the shader.
