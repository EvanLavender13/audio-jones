# Dream Fractal

A raymarched IFS (Iterated Function System) fractal that fills the screen with infinitely recursive carved-sphere tunnels. The camera slowly orbits and drifts forward through the structure while turbulence-based domain warping produces rich spatial color variation, mapped through the gradient LUT. Dense surface detail creates a psychedelic dreamscape that responds to every gradient palette.

## Classification

- **Category**: GENERATORS > Field
- **Pipeline Position**: Generator stage, blended into accumulation buffer
- **Render Model**: Single-pass fragment shader (raymarching)

## Attribution

- **Based on**: "Dream Within A Dream" by OldEclipse
- **Source**: https://www.shadertoy.com/view/fclGWs
- **License**: CC BY-NC-SA 3.0

## References

- [Dream Within A Dream](https://www.shadertoy.com/view/fclGWs) — Source shader by OldEclipse (code pasted below)

## Reference Code

```glsl
// "Dream Within A Dream" by OldEclipse
// https://www.shadertoy.com/view/fclGWs
// License: CC BY-NC-SA 3.0

#define T for(float n=-.2; n++<8.; q+=1.2*sin(q.zxy*n)/n);O+=1.+sin(q.xyzz);

void mainImage( out vec4 O, vec2 I ){
    vec3 p,q,r = normalize(vec3(I+I,0) - iResolution.xyy);
    float i,j,t,l,d,s;
    // Rotate ray direction
    r.xz *= mat2(cos(iTime*.2-vec4(0,11,33,0)));
    // Raymarching loop
    for (O*=i; i++<70.;t+=d){
    p=t*r;
    // Move forward
    p.z-=iTime*.05;
    // Store non-distorted position for color
    q=p*10.;
    // Repeatedly carve spheres out of space
    d=0., s=1.;
    for(j=0.;j++<8.;){
        p*=3.;
        s*=3.;
        d=max(d,(.9-length(abs(mod(p-1.,2.)-1.)))/s);
        p.xz*=.1*mat2(8,6,-6,8);
    }
    // Store distance travelled after 20 iters
    (i==20.)?l=t:l;
    }
    // Use repeated turbulence for color
    T T T
    // Scale by ratio distance travelled to get some outlines
    O*=l/t/6.;
}
```

## Algorithm

### Substitution Table

| Reference | Replace With | Notes |
|-----------|-------------|-------|
| `iResolution` | `resolution` uniform | Standard generator uniform |
| `iTime*.2` (orbit) | `orbitPhase` uniform | CPU-accumulated: `orbitPhase += orbitSpeed * dt` |
| `iTime*.05` (drift) | `driftPhase` uniform | CPU-accumulated: `driftPhase += driftSpeed * dt` |
| `cos(t-vec4(0,11,33,0))` rotation trick | `mat2(cos(orbitPhase), -sin(orbitPhase), sin(orbitPhase), cos(orbitPhase))` | Explicit rotation matrix from accumulated phase |
| `8.` (fractal iters) | `fractalIters` uniform (int) | Parameterized detail level |
| `.9` (sphere radius) | `sphereRadius` uniform | Controls solid vs carved ratio |
| `3.` (scale factor) | `scaleFactor` uniform | Changes fractal ratio |
| `.1*mat2(8,6,-6,8)` | `mat2(cos(twist), sin(twist), -sin(twist), cos(twist))` | Original ≈ 36.87deg rotation; parameterize with `twist` uniform |
| `*10.` (color scale) | `colorScale` uniform | Spatial frequency of turbulence color |
| `1.2` (turbulence amp) | `turbulenceIntensity` uniform | Strength of domain warping |
| `O+=1.+sin(q.xyzz)` | Gradient LUT sample (see below) | Replace direct RGBA with LUT coloring |
| `70.` (march steps) | `MARCH_STEPS` constant (compile-time) | Hardcode at 70; not a runtime param |
| `20.` (outline sample) | `MARCH_STEPS * 0.29` | ~29% of total steps, scales with step count |

### Turbulence → Gradient LUT Coloring

The original accumulates RGBA directly via `1. + sin(q.xyzz)`. Replace with:

```glsl
// After turbulence passes warp q:
float t = 0.5 + 0.5 * sin(length(q));
vec3 color = texture(gradientLUT, vec2(t, 0.5)).rgb;
```

The turbulence passes still warp `q` identically — only the final color mapping changes. Run 3 passes of the turbulence loop to build up the spatial warping, then collapse to a scalar for LUT lookup.

### FFT Integration

Global brightness modulation (no per-layer frequency spread — the fractal has no natural layer decomposition for frequency mapping):

```glsl
float bin = baseFreq / (sampleRate * 0.5);
float energy = texture(fftTexture, vec2(bin, 0.5)).r;
float mag = pow(clamp(energy * gain, 0.0, 1.0), curve);
float brightness = baseBright + mag;
// Apply to final color
color *= brightness;
```

### Phase Accumulation (CPU side)

Per conventions, speed params accumulate on CPU, not in shader:

```c
// In DreamFractalEffectSetup():
e->orbitPhase += cfg->orbitSpeed * deltaTime;
e->driftPhase += cfg->driftSpeed * deltaTime;
// Bind as uniforms
SetShaderValue(shader, locs.orbitPhase, &e->orbitPhase, SHADER_UNIFORM_FLOAT);
SetShaderValue(shader, locs.driftPhase, &e->driftPhase, SHADER_UNIFORM_FLOAT);
```

### Outline Computation

The depth ratio `l/t` creates structural edges. `l` is saved at ~29% of march steps. Where the ray enters deep crevices (large final `t`, similar `l`), the ratio is low → dark. Where the surface is hit early, ratio is high → bright. This naturally emphasizes the fractal's carved edges.

## Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| orbitSpeed | float | -2.0 – 2.0 | 0.2 | Camera orbit rate (rad/s) |
| driftSpeed | float | 0.0 – 0.5 | 0.05 | Forward movement speed into fractal |
| fractalIters | int | 3 – 12 | 8 | Fractal detail levels (more = finer detail, heavier) |
| sphereRadius | float | 0.3 – 1.5 | 0.9 | Carved sphere size — low=mostly void, high=mostly solid |
| scaleFactor | float | 2.0 – 5.0 | 3.0 | Per-iteration scale ratio (changes fractal geometry) |
| twist | float | -PI – PI | 0.6435 | Inter-level rotation angle (~36.87deg default) |
| colorScale | float | 1.0 – 30.0 | 10.0 | Spatial frequency of turbulence color variation |
| turbulenceIntensity | float | 0.1 – 3.0 | 1.2 | Strength of turbulence domain warping |
| baseFreq | float | 27.5 – 440 | 55.0 | FFT base frequency (Hz) |
| maxFreq | float | 1000 – 16000 | 14000 | FFT max frequency (Hz) |
| gain | float | 0.1 – 10 | 2.0 | FFT gain |
| curve | float | 0.1 – 3.0 | 1.5 | FFT contrast curve |
| baseBright | float | 0.0 – 1.0 | 0.15 | Minimum brightness floor |

## Modulation Candidates

- **orbitSpeed**: Camera rotation rate — modulating creates accelerating/decelerating orbit
- **driftSpeed**: Forward movement — bass pulses drive deeper into the fractal
- **sphereRadius**: Pulsing between carved (low) and solid (high) — dramatic structural breathing
- **scaleFactor**: Changes the fractal's fundamental geometry — shifting between tight and open structures
- **twist**: Rotating the inter-level angle — the entire fractal twists and morphs
- **colorScale**: Shifts color spatial frequency — low=smooth gradients, high=psychedelic banding
- **turbulenceIntensity**: Controls color chaos — low=calm, high=swirling

### Interaction Patterns

- **fractalIters × sphereRadius** (cascading threshold): High iterations only reveal visible sub-detail when the sphere radius is large enough to carve meaningful structure at that scale. At small radius, cranking iterations changes almost nothing — radius gates iteration visibility.
- **driftSpeed × orbitSpeed** (competing forces): Their ratio determines whether the experience feels like diving through a tunnel (fast drift, slow orbit) or orbiting a structure (slow drift, fast orbit). Modulating these in opposition creates push-pull between exploration and observation.
- **colorScale × turbulenceIntensity** (resonance): Both high = maximum psychedelic chaos. Both low = calm smooth gradients. They amplify each other, so coinciding peaks create rare moments of extreme color complexity.

## Notes

- **Performance**: 70 march steps × up to 12 fractal iterations = up to 840 SDF evaluations per pixel. At 8 iterations (default) = 560. This is heavier than most generators. The `fractalIters` param gives users a quality/performance knob.
- **Turbulence passes**: Hardcoded at 3. Fewer passes produce noticeably less interesting color. More than 3 gives diminishing returns.
- **The `cos(t-vec4(0,11,33,0))` trick**: The original uses cosine phase shifts to approximate a rotation matrix in one line. We replace with an explicit `mat2` from the accumulated orbit phase for clarity and correctness.
- **Outline sample point**: Saved at step `MARCH_STEPS * 0.29` (step 20 of 70 in the original). This ratio was chosen empirically by the original author. It may benefit from slight tuning.
- **No `layers` param**: Unlike frequency-spread generators, this fractal is a single monolithic SDF with no natural per-layer decomposition. Audio reactivity comes through global FFT brightness and modulation of structural/color params.
