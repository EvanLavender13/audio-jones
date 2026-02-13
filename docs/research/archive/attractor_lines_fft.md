# Attractor Lines FFT Reactivity

Add semitone-based audio reactivity to the attractor lines generator. Velocity drives the frequency mapping — fast-moving segments react to treble, slow curves pulse with bass. Color cycles by pitch class (repeating per octave) instead of raw velocity.

## Classification

- **Category**: GENERATORS > Filament (existing effect enhancement)
- **Pipeline Position**: No change — already in generator stage
- **Chosen Approach**: Minimal — adds standard FFT uniform set to existing shader, modifies color/brightness calculation

## References

- Existing codebase: `shaders/filaments.fs`, `shaders/spectral_arcs.fs` — standard FFT semitone glow pattern
- Existing codebase: `shaders/attractor_lines.fs` — current velocity-based coloring

## Reference Code

### Standard FFT semitone glow (from filaments.fs)

```glsl
// Convert index to semitone frequency (equal temperament)
float freq = baseFreq * pow(2.0, float(i) / 12.0);

// Normalize to FFT bin coordinate (0 = DC, 1 = Nyquist)
float bin = freq / (sampleRate * 0.5);

// Sample and process magnitude
float mag = 0.0;
if (bin <= 1.0) {
    mag = texture(fftTexture, vec2(bin, 0.5)).r;
    mag = pow(clamp(mag * gain, 0.0, 1.0), curve);
}

// Color by pitch class (cycles every octave)
float pitchClass = fract(float(i) / 12.0);
vec3 color = texture(gradientLUT, vec2(pitchClass, 0.5)).rgb;

// Brightness: baseline + FFT boost
result += geometricFactor * color * (baseBright + mag);
```

### Current attractor lines color/brightness (from attractor_lines.fs lines 218-222)

```glsl
float c = intensity * smoothstep(focus / resolution.y, 0.0, d);
c += (intensity / 8.5) * exp(-1000.0 * d * d);

float speedNorm = clamp(bestSpeed / maxSpeed, 0.0, 1.0);
vec3 color = texture(gradientLUT, vec2(speedNorm, 0.5)).rgb;
```

## Algorithm

Adapt the standard FFT semitone pattern to the attractor's continuous velocity field:

1. `bestSpeed` already tracks velocity at the nearest segment per pixel
2. Normalize: `speedNorm = clamp(bestSpeed / maxSpeed, 0.0, 1.0)`
3. Map to semitone index: `semitone = speedNorm * float(numOctaves) * 12.0`
4. Convert to frequency: `freq = baseFreq * pow(2.0, semitone / 12.0)`
5. Sample FFT at that frequency's bin position
6. Apply gain/curve as standard
7. Replace gradient sample with pitch-class coloring: `fract(semitone / 12.0)`
8. Multiply brightness by `(baseBright + mag)`

### What changes from existing attractor_lines.fs

- **Replace** gradient LUT sample position from `speedNorm` to `fract(semitone / 12.0)`
- **Multiply** existing brightness `c` by `(baseBright + mag)` before combining with color
- **Add** FFT uniforms: `fftTexture`, `sampleRate`, `baseFreq`, `numOctaves`, `gain`, `curve`, `baseBright`

### What stays the same

- RK4 integration loop and distance field calculation
- `bestSpeed` tracking at nearest segment
- `maxSpeed` normalization (now maps velocity range onto octave span)
- Glow formula (`smoothstep` + `exp` core)
- Decay/trail accumulation with `previousFrame`

## Parameters

New parameters (standard FFT set):

| Parameter | Type | Range | Default | Visual Effect |
|-----------|------|-------|---------|---------------|
| baseFreq | float | 20.0-880.0 | 220.0 | Lowest frequency mapped to slowest velocity |
| numOctaves | int | 1-8 | 5 | Octave span across velocity range |
| gain | float | 0.1-20.0 | 5.0 | FFT magnitude amplifier |
| curve | float | 0.5-4.0 | 1.5 | Contrast exponent on FFT magnitude |
| baseBright | float | 0.0-0.5 | 0.05 | Minimum brightness when semitone is quiet |

Existing `maxSpeed` gains new significance: defines which velocity maps to the highest octave.

## Modulation Candidates

- **gain**: controls how aggressively the attractor reacts to audio
- **baseBright**: fades quiet velocity regions in/out
- **baseFreq**: shifts which musical range the attractor responds to
- **intensity**: overall brightness scaling (existing)

### Interaction Patterns

- **`baseBright` gating**: at 0, quiet semitone regions vanish — only actively-playing notes illuminate their velocity band. Higher values keep the full attractor visible with brightness pulses on top.
- **`maxSpeed` x `numOctaves` tension**: `maxSpeed` controls how much velocity range maps onto the octave span. Low `maxSpeed` compresses all velocities into a narrow frequency band (everything reacts to similar notes). High `maxSpeed` spreads the mapping, giving each velocity region its own distinct pitch response.

## Notes

- No new shader file — modifies existing `attractor_lines.fs`
- No new render pass or pipeline change
- FFT texture already available in generator setup path — follow `shader_setup_generators.cpp` pattern from filaments/spectral arcs
