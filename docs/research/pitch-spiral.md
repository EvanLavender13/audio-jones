# Pitch Spiral

Displays current FFT arranged on a logarithmic spiral where position encodes musical pitch. Each full spiral turn equals one octave - musically related notes (octaves, fifths) align geometrically. Shows harmonic structure visually: fundamentals and overtones create radial patterns.

## Classification

- **Type**: Generator (fragment shader)
- **Pipeline Position**: Generator stage (before transforms)
- **Data Source**: FFT texture (current frame only, no history)
- **Chosen Approach**: Archimedean spiral with 12-TET pitch mapping

## References

- [Shadertoy: Spiral Spectrogram](https://www.shadertoy.com/view/WtjSWt) - Original spiral geometry reference
- 12-tone equal temperament (12-TET) for pitch-to-frequency conversion

## Algorithm

### Spiral Geometry

Archimedean spiral where radius increases linearly with angle:

```glsl
vec2 centered = uv - 0.5;
centered.x *= aspect;  // Correct for aspect ratio

float angle = atan(centered.y, centered.x);           // -PI to PI
float radius = length(centered);
float offset = radius + (angle / TAU) * spiralSpacing; // Spiral coordinate
float whichTurn = floor(offset / spiralSpacing);       // Which ring (0, 1, 2...)
```

### Pitch Mapping

Convert spiral position to musical pitch using 12-TET:

```glsl
// Musical constants
const float A_BASE = 220.0;           // A3 = 220 Hz (lowest visible note)
const float TET_ROOT = 1.05946309436; // 2^(1/12), semitone ratio

// Position to cents (1200 cents = 1 octave)
float cents = (whichTurn - (angle / TAU)) * 1200.0;

// Cents to frequency
float freq = A_BASE * pow(TET_ROOT, cents / 100.0);

// Frequency to FFT bin (normalized 0-1)
float bin = freq / (sampleRate * 0.5);  // Nyquist = sampleRate/2
```

### FFT Sampling and Drawing

```glsl
// Sample FFT at computed frequency
float magnitude = texture(fftTexture, vec2(bin, 0.5)).r;

// Apply contrast curve
magnitude = pow(magnitude, curve);

// Draw spiral lines with anti-aliasing
float spiralDist = mod(offset, spiralSpacing);
float lineAlpha = smoothstep(spiralDist - blur, spiralDist, lineWidth)
                - smoothstep(spiralDist, spiralDist + blur, lineWidth);

// Color from LUT by pitch class (angle maps to note within octave)
float pitchClass = fract(cents / 1200.0);  // 0-1 within octave
vec3 noteColor = texture(colorLUT, vec2(pitchClass, 0.5)).rgb;

// Final color
finalColor = vec4(noteColor * magnitude * lineAlpha, 1.0);
```

## Musical Properties

**Why the spiral works musically:**

| Property | Visual Effect |
|----------|---------------|
| Octaves (2:1) | Same angle, adjacent rings |
| Fifths (3:2) | ~7 semitones apart, consistent angular offset |
| Harmonics | Fundamental + overtones align radially |
| Chords | Clustered angular regions light up together |

A sustained note with harmonics creates a radial "spoke" pattern - the fundamental and all its overtones share the same pitch class (modulo octave).

## Parameters

| Parameter | Type | Range | Default | Visual Effect |
|-----------|------|-------|---------|---------------|
| baseFreq | float | 55-440 | 220.0 | Lowest visible frequency (Hz) |
| numTurns | int | 4-12 | 8 | Number of spiral rings (octaves visible) |
| spiralSpacing | float | 0.03-0.1 | 0.05 | Distance between rings |
| lineWidth | float | 0.01-0.04 | 0.02 | Spiral line thickness |
| blur | float | 0.01-0.03 | 0.02 | Anti-aliasing softness |
| curve | float | 0.5-4.0 | 2.0 | Magnitude contrast |

Inherited:
- ColorConfig for LUT-based pitch-class coloring

## Modulation Candidates

- **baseFreq**: Shift the visible frequency range
- **spiralSpacing**: Compress/expand the spiral
- **curve**: Adjust contrast with overall energy

## Implementation Notes

- No history buffer needed - displays current FFT only
- Bins beyond Nyquist (bin > 1) render black
- Line anti-aliasing via smoothstep prevents jagged edges
- Pitch-class coloring: each note name (C, D, E...) gets consistent hue across octaves
- Center of spiral is lowest frequency; edge is highest
- Consider clamping to avoid sampling invalid FFT bins at high frequencies

## Comparison to Spectrogram

| Aspect | Spectrogram | Pitch Spiral |
|--------|-------------|--------------|
| Axes | Time × Frequency | Pitch (log spiral) |
| History | Yes (scrolling) | No (current only) |
| Musical structure | Linear frequency bands | Octave-aligned rings |
| Best for | Seeing evolution over time | Seeing harmonic relationships |
