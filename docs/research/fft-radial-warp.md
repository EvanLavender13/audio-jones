# FFT Radial Warp

Radial UV displacement driven by real-time FFT magnitude data. Bass frequencies warp the center, treble warps the edges. Bidirectional push/pull creates complex interference patterns that breathe with the music.

## Classification

- **Category**: TRANSFORMS > Warp
- **Pipeline Position**: Transform stage (user-ordered with other transforms)
- **Chosen Approach**: Balanced - FFT throttling infrastructure + dedicated effect with audio-native parameters

## Infrastructure: FFT Texture Throttling

The FFT texture currently updates every frame (~60-144hz), causing jittery visuals. Drawables update at 20hz for smooth animation. The FFT texture needs the same treatment.

### Current Flow (render_pipeline.cpp:234-239)

```cpp
float normalizedFFT[FFT_BIN_COUNT];
for (int i = 0; i < FFT_BIN_COUNT; i++) {
  normalizedFFT[i] = fftMagnitude[i] / maxMag;
}
UpdateTexture(pe->fftTexture, normalizedFFT);
```

### Required Change

Move FFT texture update into the 20hz-gated `UpdateVisuals()` path in `main.cpp`, or add a dedicated accumulator. The FFT magnitude array (`ctx->analysis.fft.magnitude`) already exists - just gate when `UpdateTexture` gets called.

Options:
1. **Move to UpdateVisuals**: Add FFT texture update call alongside drawable waveform processing
2. **Separate accumulator**: Own timer in PostEffect, checked in RenderPipelineExecute

Option 1 is simpler - the 20hz rate already works for drawables.

## Effect: FFT Radial Warp

### Algorithm

1. Convert fragment UV to polar coordinates (radius, angle)
2. Map radius [0, maxRadius] to FFT bin index [0, binCount-1]
3. Sample FFT texture at that bin to get magnitude
4. Compute displacement direction (radial outward or inward)
5. Apply bidirectional logic: alternate push/pull based on configurable pattern
6. Offset UV by displacement, sample input texture

### Core Shader Logic

```glsl
vec2 delta = fragTexCoord - center;
float radius = length(delta);
float angle = atan(delta.y, delta.x);

// Map radius to FFT bin (0-1 range for texture lookup)
float freqCoord = clamp(radius / maxRadius, 0.0, 1.0);
freqCoord = mix(freqStart, freqEnd, freqCoord);  // Configurable frequency range
float magnitude = texture(fftTexture, vec2(freqCoord, 0.5)).r;

// Bidirectional: use angle or radius to determine push vs pull
float pushPull = mix(-1.0, 1.0, step(0.5, fract(angle * segments / TAU + pushPullPhase)));
// Or simpler: based on magnitude threshold
// float pushPull = magnitude > threshold ? 1.0 : -1.0;

vec2 radialDir = normalize(delta);
vec2 displacement = radialDir * magnitude * intensity * pushPull;

vec2 sampleUV = fragTexCoord + displacement;
finalColor = texture(texture0, sampleUV);
```

### Bidirectional Modes

Several options for push/pull behavior:

1. **Angular segments**: Divide circle into N segments, alternate push/pull
2. **Frequency-based**: Low frequencies push, high frequencies pull (or vice versa)
3. **Threshold**: Above threshold pushes, below pulls
4. **Phase-shifted**: Two FFT samples at different phases, difference determines direction

Recommend starting with angular segments (most visually interesting, matches user's request for "bidirectional").

## References

- [ISF Audio Visualizers](https://docs.isf.video/primer_chapter_8.html) - FFT texture sampling patterns
- [WebGL Fundamentals: Audio in Shaders](https://webglfundamentals.org/webgl/lessons/webgl-qna-how-to-get-audio-data-into-a-shader.html) - FFT as 1D texture approach
- Existing `shaders/radial_pulse.fs` - polar coordinate displacement pattern

## Parameters

| Parameter | Type | Range | Default | Visual Effect |
|-----------|------|-------|---------|---------------|
| intensity | float | 0.0 - 0.5 | 0.1 | Overall displacement strength |
| freqStart | float | 0.0 - 1.0 | 0.0 | Which FFT bin maps to center (0 = bass) |
| freqEnd | float | 0.0 - 1.0 | 0.5 | Which FFT bin maps to edge (0.5 = mid frequencies) |
| maxRadius | float | 0.1 - 1.0 | 0.7 | Screen radius that maps to freqEnd |
| segments | int | 1 - 16 | 4 | Angular divisions for push/pull alternation |
| pushPullPhase | float | 0.0 - 1.0 | 0.0 | Rotates the push/pull pattern |
| centerX | float | 0.0 - 1.0 | 0.5 | Horizontal center position |
| centerY | float | 0.0 - 1.0 | 0.5 | Vertical center position |

## Modulation Candidates

- **intensity**: Scales overall warp strength
- **freqStart/freqEnd**: Shifts which frequencies affect which screen regions
- **segments**: Changes complexity of push/pull pattern
- **pushPullPhase**: Rotates the bidirectional pattern
- **centerX/centerY**: Moves the warp origin

## Notes

- FFT texture is 1D (FFT_BIN_COUNT x 1), sample with `vec2(freqCoord, 0.5)`
- The 20hz update rate may still show some stepping - consider optional per-frame lerp toward target values in shader if needed
- `maxRadius` of 0.7 covers most of the screen; corners at ~0.707 from center
- Segments=1 gives pure outward or inward motion; higher values create flower-like interference
- Effect pairs well with feedback/blur to smooth the motion further
