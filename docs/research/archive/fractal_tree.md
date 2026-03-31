# Fractal Tree

An endlessly zooming self-similar tree built from a Kaleidoscopic Iterated Function System (KIFS). Golden-ratio branching creates exact self-similarity at every scale, so the camera can fly inward (or outward) forever through recursive forks that never repeat yet always look the same. Branches are colored by depth via gradient LUT -- trunk maps to one end of the ramp, finest twigs to the other -- and each depth level reacts to a different frequency band, so bass lights the trunk while treble sparkles in the canopy.

## Classification

- **Category**: GENERATORS > Geometric
- **Pipeline Position**: Generator (blended into accumulation buffer)
- **Compute Model**: Single-pass fragment shader with 5-sample supersampling

## Attribution

- **Based on**: "Fractal tree" by coffeecake6022
- **Source**: https://www.shadertoy.com/view/scj3Dc
- **License**: CC BY-NC-SA 3.0

## References

- [Shadertoy: Fractal tree](https://www.shadertoy.com/view/scj3Dc) - Original KIFS fractal tree shader (code pasted below)

## Reference Code

```glsl
// "Fractal tree" by coffeecake6022
// https://www.shadertoy.com/view/scj3Dc
// CC BY-NC-SA 3.0

float zoom, ct;
#define THICKNESS .5 / iResolution.y

vec4 samp(vec2 p) {
    // center on limiting point and zoom
    p = (2. * p - iResolution.xy) / iResolution.y;
    p *= .5 * zoom;
    p += vec2(0.70062927, 0.9045085);

    // KIFS magic
    float s = zoom;
    p.x = abs(p.x);
    for (int i = 0; i < 16 + int(ct); i ++) {
        if (p.y - 1. > -.577 * p.x) {
            p.y --;
            p *= mat2(.809015, -1.401257, 1.401257, .809015);
            p.x = abs(p.x);
            s *= 1.618033;
        } else break;
    }

    // check if we are in le branch
    return vec4(min(step(p.x, s * THICKNESS), step(abs(p.y - .5), .5)));
}

void mainImage(out vec4 col, vec2 pos)
{
    ct = 6. * fract(.5 * iTime);
    zoom = pow(.618, ct);

    // sample
    col = samp(pos);
    col += samp(pos - vec2(.5, 0.));
    col += samp(pos + vec2(.5, 0.));
    col += samp(pos - vec2(0., .5));
    col += samp(pos + vec2(0., .5));
    col *= .2;
}
```

## Algorithm

### Substitution Table

| Reference | Ours | Notes |
|-----------|------|-------|
| `iResolution` | `resolution` uniform | Standard generator uniform |
| `iTime` | `time` uniform | CPU-accumulated: `accum += zoomSpeed * deltaTime` |
| `THICKNESS .5 / iResolution.y` | `thickness * 0.5 / resolution.y` | `thickness` uniform (default 1.0) |
| `16 + int(ct)` | `maxIter + int(ct)` | `maxIter` uniform (default 16) |
| `6. * fract(.5 * iTime)` | `6.0 * fract(time)` | Speed baked into CPU accumulation |
| `pow(.618, ct)` | `zoomOut ? pow(.618, 6.0 - ct) : pow(.618, ct)` | `zoomOut` uniform flips direction |
| Monochrome `vec4(min(step(...)))` | Gradient LUT color * FFT brightness | Depth-indexed, see below |

### KIFS Core (Keep Verbatim)

These values are locked to the golden ratio and MUST NOT be parameterized:
- Limit point: `vec2(0.70062927, 0.9045085)`
- Scale factor: `1.618033`
- Zoom base: `.618`
- Rotation matrix: `mat2(.809015, -1.401257, 1.401257, .809015)`
- Branch condition: `p.y - 1. > -.577 * p.x`

### Depth Tracking

Track iteration depth for color/audio indexing. Add a `depth` counter inside the KIFS loop:

```
int depth = 0;
for i in 0..maxIter+int(ct):
    if branch condition:
        apply KIFS transform
        depth = i + 1
    else break

float t = float(depth) / float(maxIter + int(ct));
```

### Color and Audio

The normalized depth `t` is the shared index for both color and audio:
- `texture(gradientLUT, vec2(t, 0.5)).rgb` for branch color
- FFT frequency lookup: `baseFreq * pow(maxFreq / baseFreq, t)` maps trunk to bass, fine branches to treble
- Standard FFT energy computation: bin lookup -> gain -> curve -> baseBright + mag
- Multiply gradient color by brightness. Background stays black (pixels that fail the branch test output vec4(0)).

### Zoom Direction

Outward zoom reverses `ct` within the 6-level cycle:
```
float ct = 6.0 * fract(time);
if (zoomOut) ct = 6.0 - ct;
float zoom = pow(.618, ct);
```

The seamless loop still works because self-similarity is bidirectional at the limit point.

### Supersampling (Keep Verbatim)

The 5-sample pattern (center + 4 cardinal offsets at 0.5px) is essential anti-aliasing for sub-pixel branches. Keep the averaging structure unchanged. Each sample independently computes depth, color, and brightness -- the results average naturally.

## Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| zoomSpeed | float | 0.1-3.0 | 0.5 | Zoom animation rate |
| zoomOut | bool | false/true | false | Zoom direction (false=inward, true=outward) |
| thickness | float | 0.5-4.0 | 1.0 | Branch thickness multiplier |
| maxIterations | int | 8-32 | 16 | Base KIFS iteration count (detail level) |
| baseFreq | float | 27.5-440.0 | 55.0 | Low frequency bound (Hz) |
| maxFreq | float | 1000-16000 | 14000 | High frequency bound (Hz) |
| gain | float | 0.1-10.0 | 2.0 | FFT gain |
| curve | float | 0.1-3.0 | 1.5 | FFT contrast curve |
| baseBright | float | 0.0-1.0 | 0.15 | Minimum branch brightness |

## Modulation Candidates

- **zoomSpeed**: faster zoom during intense passages, slow drift in quiet sections
- **thickness**: branches thicken on beats, thin out between hits
- **gain**: overall audio sensitivity -- low gain shows only loud frequencies
- **baseBright**: floor brightness -- modulating down hides quiet depth layers entirely

### Interaction Patterns

**Cascading threshold (gain x baseBright):** With low baseBright, only frequency bands with enough energy light up their corresponding depth level. Quiet sections reveal only the trunk (bass), while loud sections illuminate the full canopy from root to tips. Different song sections naturally produce different visual densities -- verse might show sparse glowing branches, chorus lights up the entire tree.

**Competing forces (thickness x maxIterations):** High iterations resolve very fine branches, but if thickness is also high, fine branches merge into solid regions. The tension between detail and boldness shifts the visual character between delicate filigree and bold graphic silhouette.

## Notes

- The 5-sample supersampling is a 5x cost per pixel. For a generator this is acceptable, but if performance is a concern, it could be reduced to 3 samples (center + 2 diagonal offsets) at the cost of more aliasing on thin branches.
- `maxIterations` below ~12 produces visibly incomplete trees at deep zoom levels. The `+ int(ct)` term adds up to 5 extra iterations as zoom deepens, so the base count determines minimum quality.
- The golden-ratio constants are NOT tuneable -- changing them breaks the seamless infinite zoom. This is by design per the user's requirement.
