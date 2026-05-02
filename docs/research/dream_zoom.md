# Dream Zoom

Seamless infinite zoom into a per-pixel iterated complex-map fractal. The viewer continuously dives toward (or away from) a recursive fractal core; the same visual structure tiles endlessly through Jacobi-elliptic-function periodicity in log-polar coordinates. Unlike Droste-style infinite-zoom *transforms* (which recurse a captured image), this generates the fractal procedurally per frame, so the zoom never repeats and every depth reveals new orbit-trap detail.

## Classification

- **Category**: GENERATORS > Texture
- **Pipeline Position**: Output stage > Generators (sibling to Byzantine, Infinity Matrix, Color Stretch, Subdivide)

## Attribution

- **Inspired by**: "Dream Zoom" by NR4 (Alexander Kraus). https://www.shadertoy.com/view/NfBXDG
- **Reference license**: GPL-3.0-or-later (incompatible with this project's CC BY-NC-SA 3.0 license).
- **Approach**: This is NOT a port. The technique - log-polar coordinate transport, Jacobi elliptic function tiling, complex-map iteration with orbit-trap coloring - is reimplemented from public-domain mathematical sources cited below. NR4's shader is the visual target for matching; the GPL code is intentionally not reproduced anywhere in this repository.

## References

- [DLMF Chapter 22 - Jacobian Elliptic Functions](https://dlmf.nist.gov/22) - NIST Digital Library of Mathematical Functions, US gov work, public domain. Definitions, addition theorems, and computation methods.
- [DLMF Section 22.20 - Methods of Computation](https://dlmf.nist.gov/22.20) - descending Landen / AGM iteration for sn, cn, dn.
- [Wikipedia: Jacobi elliptic functions](https://en.wikipedia.org/wiki/Jacobi_elliptic_functions) - addition formulas and periodicity.
- [Roy van der Wegen - Implementing the Droste effect in WebGL](http://roy.red/posts/droste/) - log-polar mapping in shaders.
- [Reinder Nijhoff - Escher and the Droste effect (WebGL)](https://reindernijhoff.net/2014/05/escher-droste-effect-webgl-fragment-shader/) - shader-side complex math primitives.
- [Inigo Quilez - Smooth iteration count for fractals](https://iquilezles.org/articles/msetsmooth/) - orbit-trap coloring conventions.

## Why this is "truly" infinite zoom

Three mechanisms cooperate to make the zoom seamless and unbounded:

1. **Log-polar transport** maps `(x, y)` to `(log(r), theta)`. In this coordinate system, a continuous radial zoom in screen space becomes a continuous translation along the real axis - geometric scaling is now linear motion.
2. **Modulo wrap on the translation** (`mod(t * speed, 1.0) * 3.7`) makes that translation cyclic. The wrap is invisible because:
3. **The Jacobi cn function is doubly periodic** over the complex plane. A single fundamental cell of cn(z, m) tiles the entire complex plane with no seams. The wrap distance `3.7` is calibrated to be an integer number of cn periods along the translation axis, so the wrap lands on a phase-equivalent point in the lattice.

The fractal content (`formula(z)` iteration with orbit trap) is then generated *after* the cn map, so it inherits the seamless zoom rather than recursing a captured image.

## Algorithm

This section is the complete implementation specification. The implementing agent should derive the shader from this, not from any GPL source. Constants and formulas are cited.

### 1. Per-pixel coordinate transport

```glsl
// Centered, isotropic UV in [-aspect, aspect] x [-1, 1].
vec2 uv = (fragTexCoord - 0.5) * resolution / resolution.y;

// Optional global rotation of the sampling plane (slow drift).
float gphi = globalRotationAccum;  // CPU-accumulated angle
mat2 grot = mat2(cos(gphi), -sin(gphi), sin(gphi), cos(gphi));
uv = grot * uv;

// Cartesian -> log-polar:  clog(z) = (log|z|, atan(z.y, z.x))
// Standard complex logarithm; Wikipedia "Complex logarithm".
vec2 z = vec2(log(length(uv) + 1e-20), atan(uv.y, uv.x));

// Pre-scale to align with the cn fundamental period.
// Constant 1.1803 = 2*K(0.5)/pi, where K(m) is the complete elliptic
// integral of the first kind. Calibrates one log-radius cycle to one
// real-period of cn(z, 0.5).  Source: DLMF 19.2.8 / Wikipedia.
const float CN_PERIOD_CALIB = 1.18034;
z *= CN_PERIOD_CALIB * 0.5 * jacobiRepeats;

// Continuous translation along the real axis = continuous radial zoom in
// screen space. The mod() wrap is seamless because 3.7 is calibrated to
// match the cn period along this axis.
const float CN_WRAP_DISTANCE = 3.7;
z.x -= mod(zoomTimeAccum, 1.0) * CN_WRAP_DISTANCE;

// 45-degree shear places the doubly-periodic cn lattice diagonally so
// successive zoom steps don't alias into vertical bands.
z = mat2(1.0, 1.0, -1.0, 1.0) * z;

// Map through Jacobi cn at m = 0.5 (lemniscatic modulus).
z = cn_complex(z, 0.5);
```

### 2. Jacobi cn implementation (descending Landen / AGM)

Derived from DLMF 22.20.1-2 (descending Landen sequence) and DLMF 22.6.1 (addition theorem for complex argument). Public domain math.

```glsl
// Computes Jacobi sn, cn, dn for real argument u and parameter m = k^2,
// using descending Landen / AGM iteration. 4 iterations is sufficient
// for shader precision in our parameter range.
//   Source: DLMF 22.20.1.
void jacobi_sncndn(float u, float m, out float sn, out float cn, out float dn) {
    const int N = 4;
    float emc = 1.0 - m;
    float a = 1.0;
    float em[4];
    float en[4];
    float c = 1.0;

    for (int i = 0; i < N; i++) {
        em[i] = a;
        emc = sqrt(emc);
        en[i] = emc;
        c = 0.5 * (a + emc);
        emc = a * emc;
        a = c;
    }

    u = c * u;
    sn = sin(u);
    cn = cos(u);
    dn = 1.0;

    if (sn != 0.0) {
        float ac = cn / sn;
        c = ac * c;
        for (int i = N - 1; i >= 0; i--) {
            float b = em[i];
            ac = c * ac;
            c = dn * c;
            dn = (en[i] + ac) / (b + ac);
            ac = c / b;
        }
        float a2 = inversesqrt(c * c + 1.0);
        sn = (sn < 0.0) ? -a2 : a2;
        cn = c * sn;
    }
}

// Complex Jacobi cn via the addition theorem:
//   cn(x + iy | m) = (cn(x|m) cn(y|1-m) - i sn(x|m) dn(x|m) sn(y|1-m) dn(y|1-m))
//                    / (1 - dn^2(x|m) sn^2(y|1-m))
//   Source: DLMF 22.6.1.
vec2 cn_complex(vec2 z, float m) {
    float snu, cnu, dnu;
    float snv, cnv, dnv;
    jacobi_sncndn(z.x, m, snu, cnu, dnu);
    jacobi_sncndn(z.y, 1.0 - m, snv, cnv, dnv);
    float invDenom = 1.0 / (1.0 - dnu * dnu * snv * snv);
    return invDenom * vec2(cnu * cnv, -snu * dnu * snv * dnv);
}
```

### 3. Per-pixel fractal iteration with orbit trap

The cn-mapped coordinate `z` is the entry point to a complex-map iteration. The map blends two well-known dynamical systems (complex exponential and squaring) and tracks the minimum distance to a fixed trap point across the orbit.

```glsl
// Pre-iteration coordinate offset (drifts the whole fractal).
vec2 zf = z - 0.01 * origin;

float tmin = 1e9;
for (int i = 0; i < iterations; i++) {
    if (dot(zf, zf) > 1e10) { break; }

    // exp(zf) for complex zf, with x clamped to prevent overflow.
    //   exp(x + iy) = exp(x) * (cos(y), sin(y))
    vec2 expz = vec2(cos(zf.y), sin(zf.y)) * min(exp(zf.x), 2.0e4);

    // zf * zf for complex zf:  (a + bi)^2 = (a*a - b*b) + 2ab i
    vec2 sqz = vec2(zf.x * zf.x - zf.y * zf.y, 2.0 * zf.x * zf.y);

    // Map: f(z) = mix(exp(z), z*z, formulaMix) + 0.01*constantOffset + z
    zf = mix(expz, sqz, formulaMix) + 0.01 * constantOffset + zf;

    // Moebius-style orbit trap: distance from z to a fixed point,
    // normalized by the distance to a singularity at trapOffset.
    //   trap(z) = |z / (z - trapOffset)|
    float t = length(zf / (zf - trapOffset));
    tmin = min(tmin, t * cmapScale * 0.01);
}

// Per-pixel depth coordinate. Equal-tmin pixels lie on the same depth
// ring of the fractal regardless of where they appear on screen.
float tLUT = fract(cmapOffset - log(mix(exp(0.001), exp(1.0), tmin)));
```

### 4. FFT-reactive depth modulation

Because `tLUT` is a fractal *depth* coordinate (not a screen coordinate), sampling FFT at `tLUT` makes individual zoom rings respond to specific frequency bands. Inner rings glow on bass; outer rings glow on treble (or any user-routed mapping).

```glsl
// Standard generator FFT pattern (see memory/generator_patterns.md).
// Maps tLUT in [0,1] to log-spaced FFT frequency in [baseFreq, maxFreq].
float fLog = mix(log(baseFreq), log(maxFreq), tLUT);
float fNorm = (exp(fLog) - baseFreq) / (maxFreq - baseFreq);
float amp = pow(texture(fftTexture, vec2(fNorm, 0.5)).r * gain, contrast);
float bright = baseBright + amp;

vec3 col = texture(gradientLUT, vec2(tLUT, 0.5)).rgb * bright;
```

### 5. CPU-side accumulation (per `feedback_speed_accumulation.md`)

```cpp
// In <Name>EffectSetup():
e->zoomTimeAccum += cfg->zoomSpeed * deltaTime;
// Wrap to keep float precision bounded (the shader does its own mod).
if (e->zoomTimeAccum > 1024.0f) { e->zoomTimeAccum -= 1024.0f; }
if (e->zoomTimeAccum < -1024.0f) { e->zoomTimeAccum += 1024.0f; }

e->globalRotationAccum += cfg->globalRotationSpeed * deltaTime;
// Wrap angle to [-PI, PI].
e->globalRotationAccum = fmodf(e->globalRotationAccum + PI_F, 2.0f * PI_F) - PI_F;
```

### 6. Variant constants

A `variant` int uniform selects between two preset pre-iteration affine transformations applied to z after cn-tiling and before the formula loop. This is the only structural difference between NR4's two source shaders ("Dream Zoom" and "Jacobi Zoomer"); all other parameters remain user-editable sliders regardless of variant.

```glsl
// Selected at the start of main(). The compiler folds these into constants.
float coordinateScale; vec2 preOffset;
if (variant == 0) {        // VARIANT_DREAM
    coordinateScale = 1.0;
    preOffset = vec2(0.0);
} else {                    // VARIANT_JACOBI
    coordinateScale = 1.063;
    preOffset = vec2(11.88, -23.33);
}
```

The constants apply in step 3, between cn-tiling and the iteration loop:

```glsl
// After z = cn_complex(z, 0.5), before the iteration loop:
z -= preOffset * 0.001;
z *= exp(mix(log(1e-4), log(1.0), coordinateScale));

// Pre-iteration coordinate offset (drifts the whole fractal).
vec2 zf = z - 0.01 * origin;
```

`VARIANT_DREAM` is the identity case (no extra scale, no extra offset) and reproduces NR4's "Dream Zoom" parameter region. `VARIANT_JACOBI` reproduces NR4's "Jacobi Zoomer" sibling shader by injecting his hardcoded `iCoordinateScale` and `iOffset` values.

## Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| variant | int (combo) | DREAM, JACOBI | DREAM | Selects baked-in pre-iteration affine constants. Dropdown only - not a continuous parameter and not modulatable. See section 6 of Algorithm. |
| zoomSpeed | float | -1.0 .. 1.0 | 0.1 | Continuous zoom rate. Sign sets direction (positive = zoom out, negative = zoom in). |
| jacobiRepeats | float | 0.5 .. 4.0 | 1.0 | Number of cn-tile cycles per zoom revolution. Higher = denser fractal stack. |
| formulaMix | float | 0.0 .. 1.0 | 0.25 | Per-pixel map blend: 0 = pure complex exp (organic, smooth), 1 = pure squaring (Mandelbrot-like, harsh). |
| iterations | int | 16 .. 256 | 64 | Per-pixel iteration cap. Higher = more detail, more cost. |
| cmapScale | float | 1.0 .. 200.0 | 85.0 | LUT remap amplitude on the trap minimum. |
| cmapOffset | float | 0.0 .. 10.0 | 5.4 | LUT remap phase shift (rotates the gradient through depth rings). |
| globalRotationSpeed | float | -ROTATION_SPEED_MAX .. +ROTATION_SPEED_MAX | 0.1 | Slow rotation of the sampling plane. |
| rotationSpeed | float | -ROTATION_SPEED_MAX .. +ROTATION_SPEED_MAX | 0.0 | Inner rotation applied to the fractal coords. |
| trapOffset | DualLissajousConfig | - | amp 0.05 | Position of the orbit-trap point. Small motion = pulsing local detail. |
| origin | DualLissajousConfig | - | amp 0.3 | Pre-iteration translation of the fractal coords. Drifts the whole structure. |
| constantOffset | DualLissajousConfig | - | amp 0.03 | Per-iteration constant added to z. Shifts the fractal "constant" continuously. |
| baseFreq | float | 27.5 .. 440.0 | 55.0 | Lowest FFT frequency mapped to inner depth. (Standard Audio.) |
| maxFreq | float | 1000 .. 16000 | 14000 | Highest FFT frequency mapped to outer depth. (Standard Audio.) |
| gain | float | 0.1 .. 10.0 | 2.0 | FFT amplitude gain. (Standard Audio.) |
| contrast | float | 0.1 .. 3.0 | 1.5 | FFT amplitude exponent. (Standard Audio.) |
| baseBright | float | 0.0 .. 1.0 | 0.15 | Brightness floor when FFT is silent. (Standard Audio.) |

Plus standard generator output: gradient LUT (color), blend intensity, blend mode.

## Modulation Candidates

- **zoomSpeed**: pulse-driven zoom bursts; LFO for breathing in/out cycles. Sign flip = direction flip.
- **jacobiRepeats**: stepped values create snapping changes in fractal density.
- **formulaMix**: morphs the fractal character between two distinct visual languages (smooth exp <-> harsh squaring).
- **cmapOffset**: phase-rotates the gradient through depth rings; loops cleanly with a sawtooth LFO.
- **rotationSpeed / globalRotationSpeed**: independent rotation rates.
- **trapOffset / origin / constantOffset amplitudes**: distort the fractal subtly without breaking the zoom continuity.

### Interaction Patterns

- **Cascading thresholds (depth-band gating)**: `tLUT` is both the gradient lookup AND the FFT lookup. FFT energy in a given band only brightens that band's depth ring. Bass-rich passages illuminate the inner core; treble-rich passages illuminate the outer rings. Quiet bands sit at `baseBright` and never fight active bands for visual attention - the fractal's depth axis becomes a frequency-organized stack.

- **Competing forces (zoomSpeed vs cmapOffset)**: Modulate both with the same envelope but opposite signs. The gradient slips backward through the depth rings at exactly the rate the zoom advances them, so the structure appears frozen against the zoom. Modulating only one breaks the standoff and the structure visibly accelerates one way. Visualizes as "the fractal stops moving on the drop, then suddenly tears free".

- **Resonance (origin + constantOffset)**: Both vec2 offsets perturb z at different stages of the iteration. When their LFOs phase-align, displacements compound and the fractal jolts; when they phase-cancel, it snaps back to rest. Two LFOs at slightly different rates produce a slow "beating" alignment that creates rare bright punctuation.

## Notes

- **Loop seam preservation**: The constants `CN_PERIOD_CALIB = 1.18034` and `CN_WRAP_DISTANCE = 3.7` are tied to the cn fundamental period at m = 0.5. They MUST stay coupled. `jacobiRepeats` is safe to expose because it scales `clog(z) * CN_PERIOD_CALIB * 0.5 * jacobiRepeats` linearly; the wrap distance still lands on a phase-equivalent lattice point. Do NOT expose the modulus `m` - changing it would require recomputing both calibration constants.

- **Performance**: Inner iteration loop has an early exit on `dot(z,z) > 1e10`. Worst case is `iterations` (capped at 256). Each step is ~10 ops including one `exp` and one `length`. Expect roughly 1.5x the cost of equivalent Mandelbrot iteration count due to the `exp(z)` branch.

- **GLSL 330 dynamic loops**: Per `memory/feedback_glsl_loops.md`, use `for (int i = 0; i < iterations; i++)` directly with a uniform int. Do not hardcode max + break.

- **Coordinate centering**: All spatial operations use `(fragTexCoord - 0.5) * resolution / resolution.y` style centering per `memory/feedback_effect_implementation.md`. Do NOT use raw `fragTexCoord` for noise / distance / radial math.

- **Clamp `exp` for stability**: Keep `min(exp(zf.x), 2.0e4)` inside the iteration. Removing the clamp produces NaN cascades when zf escapes upward.

- **No tonemap**: Output the LUT-sampled color multiplied by brightness directly per `memory/feedback_no_reinhard.md`. Blend compositor handles final mixing.

- **No debug overlay** per `memory/feedback_no_debug_overlay.md`.

- **Use shared configs for 2D positions**: `trapOffset`, `origin`, `constantOffset` are all 2D position parameters - embed each as `DualLissajousConfig` per `memory/feedback_lissajous_for_2d_positions.md`. Do not expose raw X/Y sliders.

- **DROPPED from the reference**: Vogel-disk DOF, hash grain, temporal AA, palette switcher, beat-synced `bpm` / `nbeats` / `syncTime` machinery, `iSampleCount`, `iFeedbackAmount`, `iTrapParam`, `iTrapRadius`, `iCoordinates`, `iFormula`, `iBlendTime`, `iLastSwitchTime`, `iPumpAmont`. Either dead in our kept code path, replaced by AudioJones infrastructure (FFT, modulation engine, blend compositor, separate Bokeh/Film Grain transforms), or hardcoded sync to a single fixed BPM that we don't want.

- **`iCoordinateScale` and `iOffset`**: kept as variant-baked constants only (see Algorithm step 6), not exposed as sliders. They differ between NR4's two source shaders and are the sole reason the variant dropdown exists.
