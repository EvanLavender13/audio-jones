# Synapse Tree

A raymarched 3D fractal tree built from iterated sphere inversion and abs-fold branching. Organic limbs twist and split outward from a central trunk while electric blue pulses travel along branch tips like synaptic firing. The camera drifts slowly forward into the canopy. Each branching depth responds to a different frequency band — bass illuminates the trunk, treble lights the finest twigs.

## Classification

- **Category**: GENERATORS > Filament
- **Pipeline Position**: Output stage, after trail boost, blended via compositor
- **Compute Model**: Single-pass fragment shader with ping-pong trail buffer (same as Vortex/Muons)

## Attribution

- **Based on**: "Arvore 3b - sinapses" by Elsio
- **Source**: https://www.shadertoy.com/view/XfXcR7
- **License**: CC BY-NC-SA 3.0

## References

- [Arvore 3b - sinapses](https://www.shadertoy.com/view/XfXcR7) - Source shader with sphere inversion IFS fractal tree
- [Distance Estimated 3D Fractals III: Folding Space](http://blog.hvidtfeldts.net/index.php/2011/08/distance-estimated-3d-fractals-iii-folding-space/) - Theory behind sphere inversion and abs-fold fractals

## Reference Code

```glsl
#define rot(a) mat2(cos(a + vec4(0, 11, 33, 0)))
#define t iTime * .4

void mainImage(out vec4 o, vec2 u) {
    vec2  r = iResolution.xy; o *= 0.;
          u = (u - r.xy / 2.) / r.y;

     float j, i, d, s, a, c, cyl, d1;

     while(i ++ < 160.){
        vec3 p = vec3(d * u * 1.8, d * 1.8) + vec3(-.1, .7, -.2);

        j = s = a = 8.45;

        while(j++ < 16.)
            c = dot (p, p),
            p /= c + .005,
            a /= c,

            p.xz = abs(
                       rot(sin(t - 1./c) / a - j) * p.xz
                   ) - .5,

            p.y = 1.78 - p.y,
            cyl = length(p.xz) * 2.5 - .06 / c,
            cyl = max(cyl, p.y) / a,
            s = min(s, cyl) * .9;

        d += abs(s) + 1e-6;

        s < .001 ? o += .000001 / d*i  : o;

        if (cyl > length(p - vec3(0, sin(11.*t) ,0))-.5-.5*cos(7.*t)){
            s < .001 ? o += .000005 / d * i  : o;
            s < .02 ? o.b += .0002 / d  : o.b;
        }
     }
}
```

## Algorithm

### Substitution Table

| Reference | Replacement | Notes |
|-----------|-------------|-------|
| `iResolution` | `resolution` uniform | Standard |
| `iTime * .4` | `animPhase` uniform | CPU-accumulated: `phase += animSpeed * deltaTime` |
| `160.` (ray steps) | `marchSteps` uniform (int) | Configurable quality/performance |
| `8.45` / `16.` (fold loop bounds) | `foldStart` / `foldStart + foldIterations` | `foldStart` fixed at 8, `foldIterations` configurable |
| `vec3(-.1, .7, -.2)` | `vec3(-.1, .7, camZ)` | `camZ` CPU-accumulated: `camZ += driftSpeed * deltaTime` |
| `1.8` (FOV multiplier) | `fov` uniform | |
| `.005` (inversion softening) | Keep verbatim | |
| `.5` (abs-fold offset) | `foldOffset` uniform | Branch spread |
| `1.78` (y-fold height) | `yFold` uniform | Tree proportion |
| `2.5` (cylinder scale) | Keep verbatim | Geometric meaning: maps XZ distance to branch width |
| `.06` (branch thickness) | `branchThickness` uniform | |
| `.9` (min decay) | Keep verbatim | Prevents distance underestimate |
| `.001` (hit threshold) | Keep verbatim | |
| `o += .000001 / d*i` | Accumulate gradient LUT color weighted by FFT energy at fold-depth frequency | See Audio Integration below |
| `sin(11.*t)` / `cos(7.*t)` | `sin(synapseBounceFreq * animPhase)` / `cos(synapsePulseFreq * animPhase)` | Synapse timing |
| `o.b += .0002 / d` | Add synapse accent via `synapseIntensity * gradientLUT color` | Blue channel → configurable accent color from LUT |
| `tanh` tonemap | `color = tanh(color * brightness / divisor)` | Same pattern as Vortex |

### Audio Integration

Map fractal fold iterations (not ray march steps) to frequency bands. Each fold depth represents a branching level — low iterations = trunk (bass), high iterations = twigs (treble):

```
// Inside the fold loop, per iteration j:
float tFreq = float(j - foldStart) / float(foldIterations - 1);
float freq = baseFreq * pow(maxFreq / baseFreq, tFreq);
float bin = freq / (sampleRate * 0.5);
// Standard BAND_SAMPLES=4 lookup
float mag = pow(clamp(energy * gain, 0.0, 1.0), curve);
float layerBright = baseBright + mag;
```

Store per-iteration brightness in an array, then use it when accumulating glow in the outer ray march loop.

### Forward Drift

Camera Z offset accumulated on CPU: `camZ += driftSpeed * deltaTime`. Added to the ray origin's Z component. Creates a continuous forward journey into the fractal canopy.

### Trail Buffer

Same ping-pong trail buffer as Vortex: previous frame decayed and optionally blurred, composited via `max(current, prev * decayFactor)`. Gives branches a glowing persistence.

## Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| marchSteps | int | 40-200 | 120 | Ray budget — quality vs performance |
| foldIterations | int | 4-12 | 8 | Branching depth — sparse tree to dense thicket |
| fov | float | 1.0-3.0 | 1.8 | Field of view multiplier |
| foldOffset | float | 0.2-1.0 | 0.5 | Abs-fold translation — branch spread |
| yFold | float | 1.0-3.0 | 1.78 | Vertical fold height — tree proportion |
| branchThickness | float | 0.01-0.2 | 0.06 | Cylinder radius of branches |
| animSpeed | float | 0.0-2.0 | 0.4 | Fractal self-animation rate (CPU-accumulated) |
| driftSpeed | float | 0.0-1.0 | 0.1 | Forward camera drift rate (CPU-accumulated) |
| synapseIntensity | float | 0.0-2.0 | 0.5 | Brightness of synapse pulse at tips |
| synapseBounceFreq | float | 1.0-20.0 | 11.0 | Synapse vertical bounce speed |
| synapsePulseFreq | float | 1.0-15.0 | 7.0 | Synapse size oscillation speed |
| decayHalfLife | float | 0.1-10.0 | 2.0 | Trail persistence in seconds |
| trailBlur | float | 0.0-1.0 | 0.5 | Trail spatial blur amount |
| baseFreq | float | 27.5-440.0 | 55.0 | Lowest FFT frequency Hz |
| maxFreq | float | 1000-16000 | 14000.0 | Highest FFT frequency Hz |
| gain | float | 0.1-10.0 | 2.0 | FFT sensitivity multiplier |
| curve | float | 0.1-3.0 | 1.5 | FFT contrast exponent |
| baseBright | float | 0.0-1.0 | 0.15 | Minimum brightness when silent |
| colorSpeed | float | 0.0-2.0 | 0.3 | Gradient LUT scroll rate (CPU-accumulated) |
| colorStretch | float | 0.1-5.0 | 1.0 | Spatial color frequency through depth |
| brightness | float | 0.1-5.0 | 1.0 | Intensity multiplier before tonemap |
| blendMode | enum | — | SCREEN | Blend compositor mode |
| blendIntensity | float | 0.0-5.0 | 1.0 | Blend opacity |

## Modulation Candidates

- **foldIterations**: tree fills out or strips back to bare branches
- **foldOffset**: branches spread wide or collapse inward
- **yFold**: tree stretches tall or squashes flat
- **branchThickness**: filaments thicken into trunks or thin to hairlines
- **animSpeed**: fractal morphing accelerates or freezes
- **synapseIntensity**: electric pulses flare bright or go dark
- **driftSpeed**: camera rushes forward or slows to a crawl
- **gain**: overall audio reactivity sensitivity

### Interaction Patterns

**Cascading threshold — foldIterations × gain**: At low iterations only the trunk exists; FFT energy illuminates a few bold limbs. As iterations increase, finer branches appear and higher-frequency bands light up progressively finer structure. Bass-heavy passages illuminate the trunk; treble-heavy passages light the canopy. With both modulated, the tree structurally grows and shrinks with the music's spectral density.

**Competing forces — branchThickness × synapseIntensity**: Thick branches with low synapse intensity show solid dark organic forms. Thin branches with high synapse intensity become pure electric filaments. Modulating these in opposition creates tension between biological structure and neural energy — the tree oscillates between tree and lightning.

**Resonance — animSpeed × driftSpeed**: When fractal morphing and forward drift align, branches reach toward the camera and retract rhythmically. Out of phase, the motion is chaotic and organic. Brief moments of alignment create rhythmic "breathing" during musical peaks.

## Notes

- **Performance**: 120 steps × 8 folds = ~960 inner loops per pixel. Heavy. Default `marchSteps` at 120 (not 160) for reasonable GPU load. Support `EFFECT_FLAG_HALF_RES` for weaker GPUs.
- **Fold iteration FFT array**: GLSL doesn't allow variable-length arrays. Use a fixed `float fftBands[12]` (max foldIterations) populated in the fold loop. If `foldIterations < 12`, unused slots are zero.
- **Forward drift wrap**: `camZ` will grow unbounded. The fractal is self-similar so this is fine visually — no wrap needed. But clamp to prevent float precision issues at extreme values (e.g., `fmod(camZ, 100.0)`).
- **`d1` in reference**: Declared but never used in this version of the shader. Omit.
