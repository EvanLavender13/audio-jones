# Faraday Waves

A vibrating liquid surface where standing wave lattice patterns emerge from parametric resonance. FFT energy excites layers of superimposed plane waves arranged in lattice geometries — stripes, squares, hexagons, or quasicrystal patterns depending on frequency content. Waves oscillate at half the driving frequency, creating the characteristic Faraday "bouncing" effect. The visual is structured, shimmering grids of peaks and troughs — like boiling mercury organized into crystalline order.

## Classification

- **Category**: GENERATORS > Cymatics
- **Pipeline Position**: Generator stage (same as Chladni, Ripple Tank)
- **Compute Model**: Fragment shader with ping-pong trail persistence

## References

- [Pattern formation in Faraday instability — PMC](https://pmc.ncbi.nlm.nih.gov/articles/PMC9968532/) - Experimental validation of theoretical models, pattern selection by damping
- [Amplitude equation and pattern selection in Faraday waves](https://doi.org/10.1103/physreve.60.559) - Standing wave amplitude equations, Lyapunov gradient form, damping-dependent pattern selection
- [Spatial period-multiplying instabilities of hexagonal Faraday waves](https://arxiv.org/abs/nlin/0005066) - Superlattice patterns, hexagonal symmetry breaking
- [Superposition of Waves](https://www.acs.psu.edu/drussell/demos/superposition/superposition.html) - Standing wave fundamentals, constructive/destructive interference

## Reference Code

No existing shader implementation found. The algorithm below is derived from the physics of parametric standing wave superposition on lattices.

### Standing wave superposition (core equation)

The Faraday surface height at position `p` is a sum of standing waves along N lattice directions:

```glsl
// Each wave vector k_i = k * (cos(theta_i), sin(theta_i))
// Standing wave: cos(k_i . p) * cos(omega * t / 2)
// The half-frequency oscillation is the Faraday signature

float height = 0.0;
for (int i = 0; i < N; i++) {
    float theta = float(i) * PI / float(N);  // evenly spaced angles
    vec2 k = waveNumber * vec2(cos(theta), sin(theta));
    height += amplitude * cos(dot(k, p)) * cos(omega * time * 0.5 + phase);
}
```

### Lattice geometries from wave vector count

```
N = 1: cos(kx)                                          → stripes
N = 2: cos(kx) + cos(ky)                                → square grid
N = 3: cos(k1.p) + cos(k2.p) + cos(k3.p)  at 60°       → hexagonal
N = 4: 4 vectors at 45° intervals                       → octagonal quasicrystal
N = 6: 6 vectors at 30° intervals                       → dodecagonal quasicrystal
```

### Pattern selection by damping (from physics literature)

The real Faraday instability selects patterns based on the damping parameter gamma:
- gamma ~ 1 (high damping): stripes (N=1)
- gamma << 1, high frequency (capillary regime): squares (N=2)
- gamma << 1, low frequency (gravity-capillary): hexagons (N=3), then octagonal, ...

## Algorithm

### Multi-layer FFT-driven lattice superposition

The effect stacks multiple "layers" at different spatial scales, each driven by a different FFT frequency band (same log-space spread as Chladni/other generators):

```glsl
float totalHeight = 0.0;
float totalWeight = 0.0;

for (int layer = 0; layer < layerCount; layer++) {
    // Log-space frequency for this layer
    float freq = baseFreq * pow(maxFreq / baseFreq, float(layer) / float(layerCount - 1));
    float bin = freq / nyquist;
    float mag = texture(fftTexture, vec2(bin, 0.5)).r;
    mag = pow(clamp(mag * gain, 0.0, 1.0), curve);

    if (mag < 0.001) continue;

    // Spatial wave number scales with frequency
    float k = freq * spatialScale;

    // Sum N standing waves at evenly spaced angles
    float layerHeight = 0.0;
    for (int i = 0; i < waveCount; i++) {
        float theta = float(i) * PI / float(waveCount) + rotationOffset;
        vec2 dir = vec2(cos(theta), sin(theta));
        layerHeight += cos(dot(dir, uv) * k);
    }
    layerHeight /= float(waveCount);

    // Half-frequency temporal oscillation (Faraday signature)
    layerHeight *= cos(time * freq * 0.5);

    totalHeight += mag * layerHeight;
    totalWeight += mag;
}

if (totalWeight > 0.0) totalHeight /= totalWeight;
```

### Visualization

The raw height field is a signed value. Apply the same visualization pipeline as Chladni:

```glsl
float compressed = tanh(totalHeight * visualGain);
float t = compressed * 0.5 + 0.5;
vec3 color = texture(gradientLUT, vec2(t, 0.5)).rgb;
float brightness = abs(compressed);
```

### Trail persistence

Same ping-pong diffusion + decay as Chladni and Ripple Tank (separable 5-tap Gaussian, exponential decay, max blending).

### What to keep from existing generators

- FFT texture sampling pattern (same as Chladni)
- Gradient LUT color mapping (same as all generators)
- Ping-pong trail system with diffusion + decay (same as Chladni/Ripple Tank)
- Blend compositor output (same infrastructure)
- Standard Audio section UI (baseFreq, maxFreq, gain, curve)

### What is new

- Lattice wave vector superposition instead of eigenmodes or point sources
- `waveCount` parameter controlling lattice geometry (1-6)
- `spatialScale` mapping FFT frequency to spatial wave number
- Half-frequency temporal modulation
- `rotationOffset` for lattice orientation
- `rotationSpeed` for slow lattice rotation over time
- `damping` parameter that crossfades between lattice types (interpolating waveCount)

## Parameters

### Wave

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| waveCount | int | 1-6 | 3 | Lattice vectors: 1=stripes, 2=squares, 3=hexagons, 4+=quasicrystal |
| spatialScale | float | 0.01-1.0 | 0.1 | Maps FFT frequency to spatial wave number density |
| visualGain | float | 0.5-5.0 | 1.5 | Output intensity multiplier |
| rotationSpeed | float | -PI-PI | 0.0 | Lattice rotation rate (radians/second) |
| rotationAngle | float | -PI-PI | 0.0 | Static lattice orientation offset (radians) |

### Audio

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| layers | int | 1-16 | 8 | Number of frequency layers (in Geometry section per convention) |
| baseFreq | float | 27.5-440 | 55.0 | Lowest driven frequency |
| maxFreq | float | 1000-16000 | 14000.0 | Highest driven frequency |
| gain | float | 0.1-10.0 | 2.0 | FFT energy sensitivity |
| curve | float | 0.1-3.0 | 1.5 | Contrast exponent |

### Trail

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| decayHalfLife | float | 0.05-5.0 | 0.3 | Trail persistence seconds |
| diffusionScale | int | 0-8 | 2 | Spatial blur tap spacing |

Plus standard ColorConfig gradient, blendMode, blendIntensity.

## Modulation Candidates

- **spatialScale**: Changes density of the lattice pattern across the whole field
- **visualGain**: Brightness response to audio dynamics
- **rotationSpeed**: Spinning the lattice orientation
- **rotationAngle**: Static lattice tilt
- **waveCount**: Discrete jumps between lattice geometries (integer modulation)

### Interaction Patterns

- **waveCount vs spatialScale (competing forces)**: Low waveCount (stripes) with high spatialScale creates dense parallel lines. High waveCount (hexagons/quasicrystal) with low spatialScale creates sparse crystalline dots. The visual character shifts dramatically between these extremes.
- **layers vs spatialScale (resonance)**: Many layers at a spatialScale that creates harmonic relationships between layer wave numbers produces moire-like interference between the layers. At other scales the layers blur together. Certain spatialScale values create visual "resonance" where the lattice structure suddenly snaps into sharp focus.
- **gain vs waveCount (cascading threshold)**: At low gain, only the loudest FFT bins exceed threshold, so few layers contribute — the lattice is sparse and simple. At high gain, many layers stack, and the waveCount determines whether the stacking produces ordered structure (high N) or chaotic overlap (low N).

## Notes

- The nested loop (layers x waveCount) has worst case 16 x 6 = 96 iterations, each with one FFT texture fetch and one cos+dot. Comparable to Chladni's 20x20 loop with early-out.
- The half-frequency temporal oscillation `cos(time * freq * 0.5)` means each layer "breathes" at a different rate. Bass layers pulse slowly, treble layers flutter. This is physically correct for Faraday waves and creates natural visual rhythm.
- `waveCount` could also be a float with fractional interpolation: at 2.5, blend between square and hexagonal patterns. This would make it continuously modulatable. Implementation: evaluate both floor(N) and ceil(N) lattices and mix by the fractional part.
- The lattice rotation gives the effect visual motion even with static audio, distinguishing it from Chladni's purely audio-reactive pattern.
- Aspect correction required: `uv.x *= aspect` as with all generators.
- Shader coordinates must be centered: `uv = fragTexCoord * 2.0 - 1.0`.
