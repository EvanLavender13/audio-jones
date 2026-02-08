# Spark Web

Glowing line segments connecting points on a 3D Lissajous knot, forming a tangled web of electrical arcs that strobe in sequence. Each segment maps to a semitone frequency — FFT energy drives brightness while a rotating strobe sweep gates which segments fire, creating a musical frequency scanner that reveals spectral structure as it pulses around the knot.

## Classification

- **Category**: GENERATORS
- **Pipeline Position**: Output stage, alongside other generators
- **Chosen Approach**: Balanced — 3D dual-Lissajous knot extending the existing `DualLissajousConfig` pattern to three axes, with per-segment FFT and strobe gating. Rich enough for volumetric depth and musical reactivity without parameter explosion.

## References

- **User-provided Shadertoy shader** — original inspiration. 128 line segments on offset circular orbits with sequential strobe and noise shimmer. Demonstrates `sdSegment` glow, `exp()` strobe envelope, high-frequency noise texture on fade.
- **`src/config/dual_lissajous_config.h`** — existing dual-frequency-per-axis Lissajous config. CPU accumulates phase, evaluates per source with `perSourceOffset`. Pattern to extend to 3 axes.
- **`shaders/filaments.fs`** — established FFT-per-semitone integration in a generator shader. Per-segment frequency bin lookup, gain/curve shaping, gradient LUT color, `baseBright` ember floor.
- **Inigo Quilez `sdSegment`** — standard signed distance to line segment, already used in `filaments.fs`.

## Algorithm

### 3D Dual-Lissajous Curve

Each axis uses two summed sinusoids (matching `DualLissajousConfig` pattern):

```
x(t) = sin(freqX1 * phase + t) + sin(freqX2 * phase + offsetX2 + t)
y(t) = cos(freqY1 * phase + t) + cos(freqY2 * phase + offsetY2 + t)
z(t) = sin(freqZ1 * phase + t) + cos(freqZ2 * phase + offsetZ2 + t)
```

- `phase` accumulates on CPU each frame: `phase += deltaTime * motionSpeed`
- `t = 2*PI * i / totalSegments` distributes points along the curve
- When secondary frequency is 0, that term drops out (single sinusoid per axis)
- Normalize by number of active terms per axis (divide by 2 when both active, like existing config)
- Scale x,y by `amplitudeXY`, z by `amplitudeZ`

### Segment Construction

Each segment is a short chord connecting two nearby points on the Lissajous curve:

- Total segments: `segmentsPerOctave * numOctaves`
- Point P at parameter `t_i = 2*PI * i / totalSegments`
- Point Q at parameter `t_i + orbitOffset` (orbitOffset in radians)
- Project to 2D: use x,y directly. Z reserved for depth modulation.

When the strobe illuminates a range of consecutive segments, their short chords trace out the visible Lissajous curve itself — the knot shape emerges from the connected chords. Small `orbitOffset` = short chords that faithfully trace the knot path. Large `orbitOffset` = long cross-connections that cut across the knot interior, creating a denser web.

### Glow Rendering

Per-pixel, per-segment in the fragment loop:

1. Compute `sdSegment(uv, P.xy, Q.xy)` minus line thickness
2. Glow = `glowIntensity / (dist^(1/falloffExponent) + epsilon)` (matches Filaments pattern)
3. Depth brightness: scale glow by `depthNear + (1 - depthNear) * depthFactor` where depthFactor maps Z from back-to-front into 0..1. Closer segments glow brighter and optionally thicker.

### Sequential Strobe

Each segment has a strobe phase `sc = fract(i / totalSegments)`:

- Strobe envelope: `exp(-strobeDecay * fract(strobeSpeed * time + sc))`
- `strobeSpeed` controls sweep rate (modulatable)
- `strobeDecay` controls how sharp the flash is (high = brief spark, low = lingering glow)
- Low decay: enough consecutive segments glow to reveal long arcs of the Lissajous curve
- High decay: isolated individual chords flash like scattered sparks

### FFT Integration

Frequency maps to position along the knot via `segmentsPerOctave`:

- Each octave occupies `segmentsPerOctave` consecutive segments on the Lissajous curve
- Within an octave, each semitone occupies `segmentsPerOctave / 12` segments
- Multiple segments sharing a semitone sit at adjacent positions on the knot — a note lights up a cluster of nearby chords, creating a wider glowing arc
- Higher `segmentsPerOctave` = fatter bright arcs per active note, denser web overall
- Lower `segmentsPerOctave` = thinner point-like response per note, sparser web

Per-segment lookup (following Filaments pattern):

1. Semitone index for segment `i`: `floor(i * 12 / segmentsPerOctave)`
2. Frequency: `baseFreq * 2^(semitoneIndex / 12)`
3. Normalized bin: `freq / (sampleRate * 0.5)`
4. Sample `fftTexture` at that bin
5. Shape: `mag = pow(clamp(raw * gain, 0, 1), curve)`
6. Final segment brightness: `glow * strobeEnvelope * (baseBright + mag)`

The strobe and FFT multiply together — a segment only fires bright when BOTH the strobe sweep reaches it AND its frequency bin has energy. As the strobe sweeps along the knot, it scans through the frequency spectrum positionally: bass notes light up one region, treble notes another, chords light up multiple regions simultaneously.

### Noise Shimmer

High-frequency noise modulates the strobe fade (from original shader):

- `dot(sin(uv * noiseFreqA), cos(uv.yx * noiseFreqB))` produces screen-space grain
- Mixed into the strobe decay exponent, creating gritty electrical texture on the fade edges
- `noiseStrength` controls how much shimmer affects the envelope (0 = clean, 1 = gritty)

### Color

Gradient LUT indexed by pitch class: `color = texture(gradientLUT, vec2(fract(semitoneIndex / 12), 0.5))` — each semitone gets a consistent color across octaves (same pattern as Filaments and Spectral Arcs).

## Parameters

### Shape

| Parameter | Type | Range | Default | Visual Effect |
|-----------|------|-------|---------|---------------|
| amplitudeXY | float | 0.05–0.8 | 0.3 | Horizontal spread of the knot |
| amplitudeZ | float | 0.0–0.5 | 0.15 | Depth range (0 = flat, higher = more 3D parallax) |
| orbitOffset | float | 0.1–PI | 1.0 | Chord length along curve — small = traces knot path, large = cross-web connections |
| lineThickness | float | 0.001–0.02 | 0.005 | Segment width before glow falloff |

### Lissajous Frequencies

| Parameter | Type | Range | Default | Visual Effect |
|-----------|------|-------|---------|---------------|
| freqX1 | float | 0.01–0.5 | 0.05 | Primary X oscillation rate |
| freqY1 | float | 0.01–0.5 | 0.08 | Primary Y oscillation rate |
| freqZ1 | float | 0.01–0.5 | 0.03 | Primary Z oscillation rate |
| freqX2 | float | 0.0–0.5 | 0.0 | Secondary X frequency (0 = disabled) |
| freqY2 | float | 0.0–0.5 | 0.0 | Secondary Y frequency (0 = disabled) |
| freqZ2 | float | 0.0–0.5 | 0.0 | Secondary Z frequency (0 = disabled) |
| offsetX2 | float | 0–2*PI | 0.3 | Phase offset for secondary X |
| offsetY2 | float | 0–2*PI | 3.48 | Phase offset for secondary Y |
| offsetZ2 | float | 0–2*PI | 1.57 | Phase offset for secondary Z |
| motionSpeed | float | 0.0–5.0 | 1.0 | Phase accumulation rate |

### Glow

| Parameter | Type | Range | Default | Visual Effect |
|-----------|------|-------|---------|---------------|
| glowIntensity | float | 0.001–0.05 | 0.01 | Peak segment brightness |
| falloffExponent | float | 0.8–2.0 | 1.2 | Distance falloff sharpness (higher = tighter glow) |
| depthNear | float | 0.1–1.0 | 0.3 | Minimum brightness for farthest segments (prevents total disappearance) |

### Strobe

| Parameter | Type | Range | Default | Visual Effect |
|-----------|------|-------|---------|---------------|
| strobeSpeed | float | 0.0–10.0 | 1.0 | Sweep rotation rate |
| strobeDecay | float | 5.0–40.0 | 20.0 | Flash sharpness (higher = brief spark, lower = lingering glow) |
| noiseStrength | float | 0.0–1.0 | 0.3 | Electrical shimmer on fade edges |

### FFT

| Parameter | Type | Range | Default | Visual Effect |
|-----------|------|-------|---------|---------------|
| baseFreq | float | 20–880 | 220 | Lowest mapped frequency (Hz) |
| numOctaves | int | 1–8 | 5 | Frequency range covered |
| segmentsPerOctave | int | 4–48 | 12 | Density per octave — 12 = one segment per semitone, higher = fatter arcs per note |
| gain | float | 1–20 | 5.0 | FFT magnitude amplification |
| curve | float | 0.5–4.0 | 2.0 | Contrast exponent on magnitude |
| baseBright | float | 0.0–0.5 | 0.05 | Ember baseline when frequency is quiet |

## Modulation Candidates

- **amplitudeXY / amplitudeZ**: Knot breathes and expands/contracts
- **orbitOffset**: Chord lengths shift — web opens and closes, switches between tracing the knot vs. cross-connecting
- **motionSpeed**: Knot animation pace speeds up or slows
- **strobeSpeed**: Strobe sweep accelerates or decelerates
- **strobeDecay**: Flash duration — brief sparks vs. lingering trails that reveal the knot curve
- **glowIntensity**: Overall brightness pulses
- **gain**: FFT sensitivity — quiet passages dim, loud sections flare
- **noiseStrength**: Shimmer intensity fluctuates

### Interaction Patterns

- **Cascading Threshold**: `strobeEnvelope` gates `FFT magnitude` — a segment only fires bright when the strobe sweep reaches it AND its frequency bin has energy. During sparse spectral moments, the strobe reveals only the active frequencies as it sweeps. During dense moments, most segments fire and the sweep becomes a brightness wave. Song structure (verse vs. chorus) changes which segments the scanner reveals.
- **Competing Forces**: `orbitOffset` vs. `strobeDecay` — small offset with low decay traces clean arcs of the Lissajous curve. Large offset with high decay creates scattered sparks cutting across the interior. Between these extremes, the web transitions from structured knot to chaotic tangle.
- **Resonance**: `amplitudeZ` and `depthNear` — when depth range is high and minimum brightness is low, front segments dramatically overpower back segments. When both values are moderate, the knot reads as a unified volume. Modulating amplitudeZ creates moments of flat-to-volumetric transition.

## Notes

- **Performance**: The fragment loop runs `segmentsPerOctave * numOctaves` iterations per pixel. At 48 segments/octave * 8 octaves = 384 iterations, this is GPU-heavy. Consider a UI warning above 192 total segments.
- **Phase accumulation**: CPU accumulates a single `phase` float and passes it as a uniform. The shader reconstructs all 6 sinusoids per point from this phase + the frequency/offset uniforms. No position arrays needed.
- **DualLissajousConfig reuse**: The shader-side evaluation mirrors the CPU-side `DualLissajousUpdate` logic but runs in GLSL. The config struct pattern (dual freqs + offsets per axis) extends naturally to 3 axes. The CPU config could hold shape params and accumulate phase, even though position evaluation happens in the shader.
- **Shared semitones**: When `segmentsPerOctave > 12`, multiple adjacent segments map to the same semitone bin. This is intentional — a single active note lights a cluster of chords, producing a wider glowing arc on the knot rather than a single point.
- **Tonemap**: Use `tanh(result)` for soft clipping (matches Filaments), preventing blowout from additive accumulation of many overlapping glows.

## Reference Shader (Shadertoy)

Original inspiration — adapt, do not copy verbatim. Key techniques to preserve: `sdSegment` glow, `exp()` strobe envelope, noise shimmer in the decay exponent. Replace the circular orbit with 3D dual-Lissajous evaluation and add FFT per-segment brightness.

```glsl
float sdSegment( in vec2 p, in vec2 a, in vec2 b )
{
    vec2 pa = p-a, ba = b-a;
    float h = clamp( dot(pa,ba)/dot(ba,ba), 0.0, 1.0 );
    return length( pa - ba*h );
}

vec3 erot(vec3 p,vec3 ax,float t){return mix(dot(ax,p)*ax,p,cos(t))+cross(ax,p)*sin(t);}
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    // Normalized pixel coordinates (from 0 to 1)
    vec2 uv = (fragCoord-.5*iResolution.xy)/iResolution.y;

    // Time varying pixel color
 float off =9.;
  vec3 col = vec3(0.);
  for(float i=0.,im=128.;i<im;i++) {
    float sc = fract(i/im);
    vec2 p = vec2(0.2,-.2);
    p = erot(p.xyy,vec3(0.,0.,1.),i).xy;
    p.x += cos(sc*6.28+iTime+sc)*.4;

    vec2 q = vec2(-0.2,.2);
    float qsc = fract(mod(i+off,im)/im);
    q = erot(q.xyy,vec3(0.,0.,1.),mod(i+off,im)).xy;
    q.x += cos(qsc*6.28+iTime+qsc)*.4;
    float d = length(p)-.01;
    d = sdSegment(uv,p,q)-.01;
    d = .001/(.001+abs(d));
    col += vec3(1.,.5,.2)*d*2.*exp(-(20.+tanh(sin(uv.y+iTime+.1*dot(sin(uv*7000.),cos(uv.yx*500.)))*5.)*19.)*fract(iTime+sc));
  }

    // Output to screen
    fragColor = vec4(col,1.0);
}
```

### What each piece does

- **`erot(p.xyy, z_axis, i)`**: 2D rotation by `i` radians — distributes points around orbit. 128 iterations wraps ~20 times.
- **`cos(sc*6.28 + iTime + sc) * 0.4`**: Cosine x-wobble that completes one cycle over all 128 segments. Combined with rotation, creates the Lissajous-ish orbit path.
- **`off = 9`**: Q[i] = P[i+9] — each segment connects two points 9 steps apart on the same orbit. This is the chord offset (`orbitOffset` in our design).
- **`sdSegment(uv, p, q) - 0.01`**: Distance to line segment minus thickness. Each segment is a straight chord.
- **`.001 / (.001 + abs(d))`**: Inverse-distance glow — bright at center, falls off smoothly.
- **`exp(-(...) * fract(iTime + sc))`**: Sequential strobe. `sc = i/128` staggers the phase so segments fire in rotating sequence. `fract(iTime + sc)` creates a sawtooth wave per segment.
- **`20. + tanh(sin(...) * 5.) * 19.`**: Strobe decay ranges 1–39 depending on noise. The `tanh(sin(uv.y + ... + dot(sin(uv*7000), cos(uv.yx*500))))` injects high-frequency screen-space grain into the decay rate, creating the electrical shimmer texture on fade edges.
