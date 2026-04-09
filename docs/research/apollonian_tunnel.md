# Apollonian Tunnel

Raymarched flythrough of an Apollonian gasket fractal - infinite recursive sphere-packing foam carved into a sinusoidal tunnel. The camera snakes through the tube while volumetric glow accumulates depth-tinted color from the fractal surface proximity. A blend parameter transitions between the carved tunnel and the open infinite lattice. FFT brightness is mapped along the tunnel depth axis so the viewer flies through frequency space.

## Classification

- **Category**: GENERATORS > Geometric
- **Pipeline Position**: Generator (renders to own texture, composited via blend)
- **Compute Model**: Single-pass fragment shader raymarch (no ping-pong, no compute)

## Attribution

- **Based on**: "Fractal Tunnel Flight" by diatribes
- **Source**: https://www.shadertoy.com/view/WXyfWh
- **License**: CC BY-NC-SA 3.0

The Apollonian gasket distance estimator technique originates from Knighty (Abdelaziz Nait Merzouk), first published on Fractal Forums (2010). The fold-into-fundamental-domain approach is documented in Mikael Hvidtfeldt Christensen's Syntopia blog series on distance-estimated 3D fractals.

## References

- [Fractal Tunnel Flight](https://www.shadertoy.com/view/WXyfWh) - Primary reference shader by diatribes (user-provided code)
- [Distance Estimated 3D Fractals III: Folding Space](http://blog.hvidtfeldts.net/index.php/2011/08/distance-estimated-3d-fractals-iii-folding-space/) - Explains sphere inversion distance estimation and derivative tracking for fold-based fractals
- [Apollonian Fractal snippet](https://www.bellaard.com/snippets/Apollonian%20Fractal/) - Concise description of the mod-fold + sphere-inversion algorithm
- [Kaleidoscopic IFS - Fractal Forums](http://www.fractalforums.com/sierpinski-gasket/kaleidoscopic-(escape-time-ifs)/) - Knighty's original thread introducing the technique

## Reference Code

From "Fractal Tunnel Flight" by diatribes (CC BY-NC-SA 3.0):

```glsl
#define T (iTime*1.5 + 5. + 1.5*sin(iTime*.5))
#define P(z) (vec3(cos((z) * .07) * 16., \
                   0, (z)))
#define R(a) mat2(cos(a+vec4(0,33,11,0)))
#define N normalize

float apollonian(vec3 p) {
    float i, s, w = 1.;
    vec3 b = vec3(.5, 1., 1.5);
    p.y -= 2.;
    p.yz = p.zy;
    p /= 16.;
    for(; i++ < 6.;) {
        p = mod(p + b, 2. * b) - b;
        s =2. / dot(p, p);
        p *= s;
        w *= s;
    }
    return length(p) / w * 6.;
}

float map(vec3 p) {
    return max(2. - length((p - P(p.z)).xy), apollonian(p));
}

void mainImage(out vec4 o, in vec2 u) {
    float s, d, i,a;
    vec3  r = iResolution;
    u = (u-r.xy/2.)/r.y;

    vec3 p = P(T*2.),
         Z = N( P(T*2.+7.) - p),
         X = N(vec3(Z.z,0,-Z)),
         D = N(vec3(R(sin(iTime*.15)*.3)*u, 1) * mat3(-X, cross(X, Z), Z));

    for(o = vec4(0);i++ <128.;)
        p += D * s,
        d += s = map(p)*.8,
        o += (1.+cos(.05*i+.5*p.z+vec4(6,4,2,0)))/max(s, .0003);

    o = tanh(o/d/7e4*exp(vec4(3,2,1,0)*d/4e1));
}
```

## Algorithm

### What the reference does

1. **Apollonian SDF** (`apollonian`): shifts Y by -2, swaps YZ, scales down by 16, then 6 iterations of box-fold (`mod(p+b, 2b)-b`) followed by sphere inversion (`p *= 2/dot(p,p)`). Returns `length(p) / w * 6` where `w` tracks cumulative inversion scale.

2. **Tunnel carving** (`map`): `max(tunnelSDF, apollonianSDF)`. The tunnel is a cylinder along a sinusoidal path `P(z) = vec3(cos(z*0.07)*16, 0, z)` with radius 2. The `max` intersection confines the fractal to the tube interior.

3. **Camera**: position at `P(T*2)`, look-at `P(T*2+7)`, with a gentle roll `R(sin(iTime*0.15)*0.3)`. The `T` macro adds sinusoidal speed variation.

4. **Volumetric accumulation**: each raymarch step adds `(1 + cos(0.05*i + 0.5*p.z + vec4(6,4,2,0))) / max(s, 0.0003)`. This creates depth-cycling rainbow glow weighted by surface proximity (small `s` = more glow).

5. **Tone mapping**: `tanh(o / d / 7e4 * exp(vec4(3,2,1,0) * d / 40))`. Depth-dependent exponential with per-channel separation creates warm-near, cool-far depth fog.

### Substitutions for AudioJones

| Reference | Ours | Notes |
|-----------|------|-------|
| `iTime` | `flyPhase` (CPU-accumulated) | `flyPhase += flySpeed * deltaTime` |
| `iResolution` | `resolution` uniform | Standard |
| Cosine palette `cos(0.05*i + 0.5*p.z + ...)` | `texture(gradientLUT, vec2(t, 0.5)).rgb` | `t` derived from depth `p.z` normalized to visible range |
| Fixed brightness | FFT-modulated brightness | `brightness = baseBright + mag` where mag comes from FFT sampled at `freq = baseFreq * pow(maxFreq/baseFreq, t)` |
| Hardcoded 6 iterations | `fractalIters` uniform (int) | Config range 3-10, default 6 |
| Hardcoded `p /= 16.` | `preScale` uniform (float) | Config range 4.0-32.0, default 16.0 |
| Hardcoded `p.y -= 2.` | `verticalOffset` uniform (float) | Config range -4.0 to 4.0, default -2.0 |
| Hardcoded cylinder radius 2.0 | `tunnelRadius` uniform (float) | Config range 0.5-4.0, default 2.0 |
| No open-field mode | `tunnelBlend` uniform (float) | 0.0 = full tunnel, 1.0 = open lattice. Interpolate: `mix(max(tunnel, fractal), fractal, tunnelBlend)` |
| Hardcoded path curvature `cos(z*0.07)*16` | `pathAmplitude` + `pathFreq` uniforms | amplitude default 16.0 (4.0-32.0), freq default 0.07 (0.01-0.2) |
| Hardcoded roll `sin(iTime*0.15)*0.3` | `rollPhase` (CPU-accumulated) + `rollAmount` | `rollPhase += rollSpeed * deltaTime`, rollAmount default 0.3 (0.0-1.0) |
| `tanh(o/d/7e4*exp(...))` | Replace `7e4` with `glowIntensity`, `d/40` denominator with `fogDensity` | glowIntensity: 5e3-1e6 log scale. fogDensity: 10.0-100.0 |
| 128 march steps | `marchSteps` uniform (int) | Config range 48-128, default 96 |

### Depth-to-t mapping

The `t` index for gradient LUT and FFT must be derived from `p.z` in the raymarch loop. Normalize relative to the ray origin's Z position:

```
float t = fract((p.z - camPos.z) / depthRange);
```

Where `depthRange` is derived from the max ray travel distance. This maps near-field to t=0 (bass/left of gradient) and far-field to t=1 (highs/right of gradient). The same `t` drives both `texture(gradientLUT, vec2(t, 0.5))` and the FFT frequency lookup.

### Bridge function pattern

Follows Dream Fractal / Voxel March: `SetupApollonianTunnel(PostEffect* pe)` bridge calls the typed setup. `REGISTER_GENERATOR_FULL` macro for sized init + custom render support.

## Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| marchSteps | int | 48-128 | 96 | Raymarch iterations per pixel |
| fractalIters | int | 3-10 | 6 | Apollonian fold-invert iterations (detail level) |
| preScale | float | 4.0-32.0 | 16.0 | Fractal feature size relative to world |
| verticalOffset | float | -4.0 to 4.0 | -2.0 | Vertical shift through the lattice |
| tunnelRadius | float | 0.5-4.0 | 2.0 | Carved tunnel cylinder radius |
| tunnelBlend | float | 0.0-1.0 | 0.0 | 0 = tunnel, 1 = open infinite lattice |
| flySpeed | float | 0.0-5.0 | 1.5 | Forward travel speed (CPU-accumulated) |
| pathAmplitude | float | 4.0-32.0 | 16.0 | Lateral weave amplitude |
| pathFreq | float | 0.01-0.2 | 0.07 | Lateral weave frequency along Z |
| rollSpeed | float | -2.0 to 2.0 | 0.15 | Camera roll rate rad/s (CPU-accumulated) |
| rollAmount | float | 0.0-1.0 | 0.3 | Camera roll intensity |
| glowIntensity | float | 5e3-1e6 | 7e4 | Volumetric glow brightness (log scale) |
| fogDensity | float | 10.0-100.0 | 40.0 | Depth fog falloff rate |
| baseFreq | float | 27.5-440 | 55.0 | FFT low frequency bound Hz |
| maxFreq | float | 1000-16000 | 14000.0 | FFT high frequency bound Hz |
| gain | float | 0.1-10 | 2.0 | FFT amplitude multiplier |
| curve | float | 0.1-3.0 | 1.5 | FFT response curve exponent |
| baseBright | float | 0.0-1.0 | 0.15 | Minimum brightness when silent |

## Modulation Candidates

- **flySpeed**: audio-driven speed bursts create surges through the tunnel
- **tunnelBlend**: morph between claustrophobic tunnel and cosmic open lattice
- **preScale**: shifts fractal feature size, breathing the geometry larger/smaller
- **verticalOffset**: slides viewpoint through different cross-sections of the lattice
- **tunnelRadius**: widens/narrows the carved tube
- **rollAmount**: intensifies or dampens camera roll
- **glowIntensity**: brightens/dims the volumetric fog
- **fogDensity**: reveals or hides distant structure
- **fractalIters**: changes detail level (dramatic visual jumps between integer values)

### Interaction Patterns

- **preScale x tunnelBlend** (competing forces): in tunnel mode (blend=0), large preScale makes bubbles fill the tube - claustrophobic, dense. In open mode (blend=1), the same value creates massive floating spheres with empty space between them. The tunnel constrains what preScale means visually - these two parameters define opposite ends of a density spectrum.

- **flySpeed x glowIntensity** (competing forces): faster flight means less glow accumulates per feature (fewer march steps per surface encounter), dimming the image. Higher glowIntensity compensates. If bass drives speed while mids drive intensity, loud sections stay bright despite rushing forward, while quiet sections glow softly in place.

- **fractalIters x fogDensity** (cascading threshold): more iterations add fine detail visible only at close range. High fog hides distant detail anyway, so extra iterations only matter in the near-field. Low fog + high iterations = infinite recursive depth visible. High fog + low iterations = simple blobs fading fast. Iterations unlock detail that fog gates the visibility of.

## Notes

- The fold box `b = vec3(0.5, 1.0, 1.5)` is intentionally NOT exposed. These three values define the Apollonian gasket character. Changing them produces different IFS fractals that lose the sphere-packing look.
- The 0.8 step multiplier in `map(p) * 0.8` is a safety factor for the non-Euclidean distance field produced by sphere inversion. Do not remove or the ray will overshoot surfaces.
- `glowIntensity` should use a log-scale slider (`ModulatableSliderLog`) due to its wide range.
- The `tanh` tone mapping in the reference is kept -- it soft-clips the volumetric accumulation. This is NOT Reinhard tonemap.
- The per-channel depth separation `exp(vec4(3,2,1,0) * d/fogDensity)` creates warm-near/cool-far coloring in the reference. With gradient LUT replacing the cosine palette, this channel separation is removed -- the gradient handles all coloring.
