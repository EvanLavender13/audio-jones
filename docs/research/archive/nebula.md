# Nebula

Parallax nebula generator with two kaliset fractal gas layers at different depths and per-semitone star twinkling. Front and back gas layers breathe independently with broad-band audio energy. Stars flare individually when their mapped semitone activates, creating a living star field where constellations trace the melody against slowly drifting cosmic gas.

## Classification

- **Category**: GENERATORS
- **Pipeline Position**: Output stage, alongside other generators
- **Chosen Approach**: Balanced — two fractal layers + star overlay matches the CBS "Simplicity Galaxy" structure, adapted to the codebase's FFT-driven semitone pattern and ColorLUT system

## References

- [Kali's original formula](http://www.fractalforums.com/new-theories-and-research/very-simple-formula-for-fractal-patterns/) — the `abs(p)/mag + c` iteration that all nebula shaders derive from
- [Kalisets and Hybrid Ducks (Softology, 2011)](https://softologyblog.wordpress.com/2011/05/04/kalisets-and-hybrid-ducks/) — background on `z = abs(z^-1) + c` and parameter sensitivity
- [Kaliset Explorer (defgsus)](https://defgsus.github.io/blog/2021/10/12/kaliset-explorer.html) — accumulation methods and the folding mechanism
- CBS "Simplicity Galaxy" (user-provided shader code) — parallax two-layer structure with audio band mapping
- Berghold "Simplicity Galaxy" variant (user-provided shader code) — refined iteration count control and star twinkling function

## Algorithm

### Kaliset Field Function

The core fractal evaluates nebula density at a 3D point. Each iteration folds space via sphere inversion (`abs(p) / dot(p,p)`) and offsets by a constant, accumulating an exponentially-weighted energy metric that peaks where successive magnitudes nearly match — producing luminous filaments.

```glsl
// Kaliset nebula density at point p, seeded by s
float field(vec3 p, float s, int iter) {
    float accum = s / 4.0;
    float prev = 0.0;
    float tw = 0.0;
    for (int i = 0; i < iter; i++) {
        float mag = dot(p, p);
        p = abs(p) / mag + FOLD_OFFSET;  // sphere-inversion fold
        float w = exp(-float(i) / DECAY); // exponential weight decay
        accum += w * exp(-STRENGTH * pow(abs(mag - prev), POWER));
        tw += w;
        prev = mag;
    }
    return max(0.0, SCALE * accum / tw - THRESHOLD);
}
```

Key constants from the CBS reference:
- `FOLD_OFFSET = vec3(-0.5, -0.4, -1.5)` — controls the fractal attractor shape
- `STRENGTH ≈ 7.0` — sharpness of the energy peaks
- `DECAY = 7.0` — how fast iteration weight falls off
- `POWER = 2.2` — contrast exponent on magnitude delta
- `SCALE = 5.0`, `THRESHOLD = 0.7` — output normalization

### Two-Layer Parallax

Each layer evaluates the field at a different z-depth with a different UV scale, producing parallax:

**Foreground layer**: UV divided by ~2.5-4.0, z ≈ 0.0, higher iteration count (13). Fills the screen with large-scale gas structure.

**Background layer**: UV divided by ~4.0-5.0, z ≈ 1.5-4.0, lower iteration count (18 in CBS, but fewer iterations = softer). Smaller apparent scale, drifts at a different rate — perceived as distant.

Both layers drift via slow sinusoidal offsets:
```glsl
p += driftScale * vec3(sin(time / 16.0), sin(time / 12.0), sin(time / 128.0));
```
Background uses a smaller drift multiplier than foreground (0.16 vs 0.45 in Berghold) to reinforce depth.

### Audio-Reactive Gas Brightness

Each layer's field output scales by a summed band energy. The shader samples the FFT texture over a range of bins:

- **Foreground**: Sums lower-octave semitone magnitudes → bass/low-mid energy pumps the front gas layer
- **Background**: Sums upper-octave semitone magnitudes → mid/treble energy brightens the rear layer

The band sum uses the standard semitone lookup:
```glsl
float bandEnergy = 0.0;
for (int s = startSemitone; s < endSemitone; s++) {
    float freq = baseFreq * pow(2.0, float(s) / 12.0);
    float bin = freq / (sampleRate * 0.5);
    if (bin <= 1.0)
        bandEnergy += texture(fftTexture, vec2(bin, 0.5)).r;
}
bandEnergy = pow(clamp(bandEnergy * gain / float(endSemitone - startSemitone), 0.0, 1.0), curve);
```

### Per-Semitone Star Overlay

Stars use a grid-cell hash approach (from CBS/Berghold `nrand3`). Each parallax layer contributes its own star field at the layer's UV coordinates:

```glsl
vec2 seed = floor(layerUV * starDensity);
vec3 rnd = nrand3(seed);
float starBright = pow(rnd.y, starSharpness);  // Only ~1% of cells produce visible stars
```

To add semitone reactivity, each star cell maps to a semitone via hash:
```glsl
int semitone = int(floor(rnd.z * float(numOctaves * 12)));
float freq = baseFreq * pow(2.0, float(semitone) / 12.0);
float mag = texture(fftTexture, vec2(freq / (sampleRate * 0.5), 0.5)).r;
starBright *= baseBright + mag * gain;
```

Star color samples the gradient LUT by pitch class: `fract(float(semitone) / 12.0)`.

### Color Composition

Gas layer colors derive from the field value cubed/squared for nonlinear contrast, tinted by LUT samples:
```glsl
vec3 frontColor = frontField * texture(gradientLUT, vec2(frontColorIndex, 0.5)).rgb;
vec3 backColor  = backField  * texture(gradientLUT, vec2(backColorIndex, 0.5)).rgb;
```

Front and back use offset LUT coordinates (e.g., 0.2 and 0.8) so they naturally take different colors from the same gradient.

Edge vignette fades the result toward black at screen borders:
```glsl
float v = (1.0 - exp((abs(uv.x) - 1.0) * 6.0)) * (1.0 - exp((abs(uv.y) - 1.0) * 6.0));
```

Final output: `mix(backLayer, 1.0, v) * frontColor + backColor + starColor`

## Parameters

| Parameter | Type | Range | Default | Visual Effect |
|-----------|------|-------|---------|---------------|
| baseFreq | float | 27.5-440.0 | 55.0 | Lowest mapped pitch (Hz) |
| numOctaves | int | 2-8 | 5 | Semitone range for star mapping |
| gain | float | 1.0-20.0 | 5.0 | FFT sensitivity |
| curve | float | 0.5-3.0 | 1.5 | Contrast exponent on FFT magnitudes |
| baseBright | float | 0.0-1.0 | 0.15 | Star glow when semitone is silent |
| driftSpeed | float | 0.01-0.2 | 0.06 | Sinusoidal wander rate through nebula space |
| frontScale | float | 1.0-8.0 | 3.0 | UV divisor for foreground layer (larger = more zoomed in) |
| backScale | float | 2.0-12.0 | 5.0 | UV divisor for background layer |
| frontIter | int | 6-20 | 13 | Fractal iterations for front layer (detail vs. cost) |
| backIter | int | 6-20 | 10 | Fractal iterations for back layer |
| starDensity | float | 100.0-800.0 | 400.0 | Grid resolution for star placement |
| starSharpness | float | 10.0-60.0 | 35.0 | Hash power exponent — higher = fewer, brighter stars |
| brightness | float | 0.5-3.0 | 1.0 | Overall output multiplier |
| vignetteStrength | float | 0.0-10.0 | 6.0 | Edge fade steepness |
| gradient | ColorConfig | — | cyan→magenta | Color palette for gas and stars |

## Modulation Candidates

- **gain**: Overall audio sensitivity — louder music reveals more gas and star activity
- **frontScale / backScale**: Zooming layers in/out changes parallax separation
- **starDensity**: Modulating shifts the star field grid, creating a "twinkling swarm" effect as cells re-hash
- **brightness**: Master intensity pump
- **driftSpeed**: Surge the camera drift on drops
- **starSharpness**: Lower values flood the screen with faint stars; higher values isolate bright points
- **vignetteStrength**: Open or close the visible aperture

### Interaction Patterns

- **Cascading threshold**: `gain` gates star visibility — at low gain, only `baseBright` shows. Stars only flare when gain pushes FFT magnitudes past the `baseBright` floor. Gas layers similarly need enough band energy to cross the field threshold. Quiet passages show only dim ambient stars and faint gas; loud passages flood both layers with light.
- **Competing forces**: `frontScale` vs `backScale` control relative parallax depth. When both move toward the same value, layers merge into a flat image. When they diverge, parallax intensifies. Modulating them in opposite directions creates a breathing depth effect.
- **Resonance**: `starSharpness` and `gain` amplify each other — high sharpness means only the brightest hash values produce stars, but high gain makes more semitones cross the brightness threshold. Both peaking together creates rare "star burst" moments where many sharp points appear simultaneously.

## Notes

- Front layer iteration count (13) dominates GPU cost. Exposing `frontIter`/`backIter` as config lets users trade detail for performance.
- The kaliset `FOLD_OFFSET` constant (`vec3(-0.5, -0.4, -1.5)`) controls the fractal attractor's shape. Exposing this as a parameter creates wildly different nebula topologies but risks producing visual noise — keep as a constant initially, consider exposing later.
- The `STRENGTH` constant (~7.0) includes a tiny time-varying perturbation in the CBS shader (`+ 0.03 * log(fract(sin(time)*4373))`) to prevent perfect frame-to-frame repetition. Include this for temporal shimmer.
- Star grid uses integer floor of UV coordinates — resolution-dependent in the CBS shader. Normalizing by `max(resolution.x, 600)` (as Berghold does) keeps star density consistent across display sizes.
- Edge vignette reuses the standard `(1 - exp((|x|-1)*k))` pattern found in CBS — steepness controlled by `vignetteStrength`.
