# Constellation Enhancement: Square Points + FFT Reactivity

Two additions to the existing Constellation generator: a square point shape mode and per-point FFT-reactive brightness.

## Classification

- **Category**: GENERATORS > Field (section 13)
- **Pipeline Position**: Existing generator, no pipeline change
- **Scope**: Modifications to `constellation.h`, `constellation.cpp`, `constellation.fs`

## References

- `shaders/galaxy.fs` lines 97-107 - BAND_SAMPLES FFT frequency spread pattern
- `shaders/nebula.fs` lines 100-136 - Per-point FFT reactivity (color == frequency)
- `shaders/constellation.fs` lines 110-117 - Current circular point glow

## Reference Code

### Current circular point (constellation.fs)

```glsl
vec3 Point(vec2 p, vec2 pointPos, vec2 cellID, vec2 cellOffset) {
    vec2 j = (pointPos - p) * (15.0 / pointSize);
    float sparkle = 1.0 / dot(j, j);
    sparkle = clamp(sparkle, 0.0, 1.0);

    vec3 col = textureLod(pointLUT, vec2(N21(cellID), 0.5), 0.0).rgb;
    return col * sparkle * pointBrightness * pointOpacity * WaveBrightness(cellID, cellOffset);
}
```

### BAND_SAMPLES pattern (galaxy.fs)

```glsl
float freqLo = baseFreq * pow(maxFreq / baseFreq, float(i) / float(layers));
float freqHi = baseFreq * pow(maxFreq / baseFreq, float(i + 1) / float(layers));
float binLo = freqLo / (sampleRate * 0.5);
float binHi = freqHi / (sampleRate * 0.5);

float energy = 0.0;
const int BAND_SAMPLES = 4;
for (int s = 0; s < BAND_SAMPLES; s++) {
    float bin = mix(binLo, binHi, (float(s) + 0.5) / float(BAND_SAMPLES));
    if (bin <= 1.0) {
        energy += texture(fftTexture, vec2(bin, 0.5)).r;
    }
}
float mag = pow(clamp(energy / float(BAND_SAMPLES) * gain, 0.0, 1.0), curve);
float brightness = baseBright + mag;
```

## Algorithm

### 1. Square Point Shape

Add a `pointShape` int config field (0 = circle, 1 = square). In the shader `Point()` function:

- **Circle (existing)**: `float sparkle = 1.0 / dot(j, j);`
- **Square**: Use Chebyshev distance for a soft square glow:
  ```glsl
  float d = max(abs(j.x), abs(j.y));
  float sparkle = 1.0 / (d * d);
  ```
  Same `clamp(sparkle, 0.0, 1.0)` and energy level. The `15.0 / pointSize` scaling stays the same.

Branch on a `pointShape` uniform int in the shader.

### 2. FFT-Reactive Point Brightness

Each point already has a unique hash via `N21(cellID)`. Use this hash to assign a frequency band index, then sample FFT magnitude to modulate brightness.

**Shader changes:**
- Add uniforms: `fftTexture` (sampler2D), `sampleRate` (float), `baseFreq` (float), `maxFreq` (float), `gain` (float), `curve` (float), `baseBright` (float), `starBins` (int)
- In `Point()`, compute band index from hash: `int semi = int(N21(cellID) * float(starBins))`
- Compute frequency band lo/hi from log-space spread using `semi` and `starBins`
- Multi-sample with `BAND_SAMPLES = 4`
- Apply `gain`, `curve`, `baseBright` to get final magnitude
- Multiply point brightness by this magnitude
- Replace LUT lookup coordinate: use `float(semi) / float(starBins)` instead of `N21(cellID)` so color == frequency

**C++ changes:**
- Add to `ConstellationConfig`: `baseFreq`, `maxFreq`, `gain`, `curve`, `baseBright`, `starBins` with standard Audio defaults (baseFreq 55, maxFreq 14000, gain 2.0, curve 1.5, baseBright 0.15)
- Add corresponding uniform locations to `ConstellationEffect`
- Change `ConstellationEffectSetup()` signature to accept `Texture2D fftTexture` (same as nebula)
- Update `SetupConstellation()` bridge to pass `pe->fftTexture`
- Add Audio section to UI, register new params

### What to keep verbatim from reference
- The `BAND_SAMPLES` loop and log-space frequency spread from galaxy.fs
- The `baseBright + mag` brightness formula
- The `pow(clamp(energy / float(BAND_SAMPLES) * gain, 0.0, 1.0), curve)` contrast curve

### What to replace
- Nebula's single-bin `texture(fftTexture, vec2(sBin, 0.5)).r` with multi-sampled `BAND_SAMPLES` pattern
- Current `N21(cellID)` LUT coordinate with `float(semi) / float(starBins)` so color tracks frequency

## Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| pointShape | int | 0-1 | 0 | Point shape: 0 = circle, 1 = square |
| baseFreq | float | 27.5-440.0 | 55.0 | Lowest mapped frequency (Hz) |
| maxFreq | float | 1000-16000 | 14000.0 | Highest mapped frequency (Hz) |
| gain | float | 0.1-10.0 | 2.0 | FFT sensitivity |
| curve | float | 0.1-3.0 | 1.5 | Contrast exponent on FFT magnitudes |
| baseBright | float | 0.0-1.0 | 0.15 | Point brightness when frequency is silent |
| starBins | int | 12-120 | 60 | Number of frequency bands for point mapping |

## Modulation Candidates

- **gain**: overall FFT sensitivity, controls how reactive points are
- **curve**: contrast — low values make all points respond, high values only loud frequencies show
- **baseBright**: floor brightness for silent points
- **baseFreq**: shifts the frequency range being visualized

### Interaction Patterns

- **baseBright vs gain (cascading threshold)**: With low baseBright, silent points disappear entirely. Gain determines how much audio is needed to bring them back. During quiet passages the field thins out; loud sections fill in. This creates verse/chorus contrast without any modulation routing.
- **waveInfluence vs FFT brightness (competing forces)**: The existing wave system dims points far from the wave center. FFT pushes brightness up. A point with a loud frequency bin but in a wave trough shows tension — the two systems compete for control of visibility.
- **fillThreshold vs FFT (gating)**: Triangle fill uses perimeter threshold. If FFT-bright points draw the eye to certain regions, the triangles in those regions create a secondary visual layer that only matters when the points are active enough to notice.

## Notes

- The `pointShape` uniform is an int, not bool, for shader compatibility (same pattern as `interpolateLineColor`, `fillEnabled`)
- UI: Add a `Combo("Point Shape", ...)` with "Circle\0Square\0" in the Points section
- UI: Add standard Audio section with the 5 standard sliders (Base Freq, Max Freq, Gain, Contrast, Base Bright) plus `SliderInt` for Star Bins
- `starBins` goes in the Audio section (controls frequency subdivision, not geometry)
- The depth layers loop means FFT sampling happens per-layer — each layer's points react independently since they have different cell IDs
