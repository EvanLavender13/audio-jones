# FFT Frequency Spread Migration

Migrate generators from the old `numOctaves`-based semitone mapping to the new frequency-spread pattern where layers always cover the full spectrum.

## Problem

The old pattern couples layer count to frequency range:
```glsl
int totalLayers = numOctaves * 12;
float freq = baseFreq * pow(2.0, float(i) / 12.0);
```
- 1 octave at 55 Hz = 12 layers covering 55-110 Hz (bass only)
- 5 octaves at 55 Hz = 60 layers covering 55-1760 Hz (still missing highs)
- More shapes = broader frequency coverage. Can't have few shapes + full spectrum.

## New Pattern

Layers subdivide `baseFreq → maxFreq` evenly in log space:
```glsl
uniform int layers;    // visual density (moved to Geometry section)
uniform float maxFreq; // ceiling frequency (Audio section)

float freq = baseFreq * pow(maxFreq / baseFreq, float(i) / float(layers - 1));
```
- 12 layers = 12 coarse slices across full spectrum
- 60 layers = 60 fine slices of the same spectrum
- Layer count is purely visual density. Frequency coverage is always `baseFreq → maxFreq`.

## Band averaging

The single-point formula above picks one exact frequency per layer. With few layers, most energy lands between sample points and layers go dark. Instead, each layer should capture a **frequency band** and average the energy across it:

```glsl
float t0 = float(i) / float(layers);
float t1 = float(i + 1) / float(layers);
float freqLo = baseFreq * pow(maxFreq / baseFreq, t0);
float freqHi = baseFreq * pow(maxFreq / baseFreq, t1);
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
energy = pow(clamp(energy / float(BAND_SAMPLES) * gain, 0.0, 1.0), curve);
```

This makes every layer an N-band spectrum analyzer slice — 4 wide bands or 60 narrow ones all capture real energy. The single-point formula should only be used for continuous spatial mappings (e.g., pitch spiral where every pixel maps to a unique frequency).

## Color mapping

Old: `pitchClass = semitone % 12` (assumes chromatic steps)
New: `float(i) / float(layers)` (smooth gradient from low to high frequency)

## Parameter changes per effect

### Config struct
- Remove: `int numOctaves`
- Add: `int layers` (default ~24, range 4-96)
- Add: `float maxFreq` (default 14000.0f, range 1000-16000)

### Shader
- Remove: `uniform int numOctaves`
- Add: `uniform int layers`, `uniform float maxFreq`
- Replace frequency calc (see new pattern above)
- Replace color mapping to use normalized position

### UI
- Remove `numOctaves` slider from Audio section
- Add `layers` int slider to Geometry section
- Add `maxFreq` modulatable slider to Audio section
- Audio slider order: Base Freq, Max Freq, Gain, Contrast, Base Bright

### Modulation
- Remove: `<name>.numOctaves` (was not modulatable anyway)
- Add: `ModEngineRegisterParam("<name>.maxFreq", &cfg->maxFreq, 1000.0f, 16000.0f)`

### Preset serialization
- Old `numOctaves` field ignored on load (or convert: `layers = numOctaves * 12`)
- New fields: `layers`, `maxFreq`

## Affected generators

All generators using `numOctaves * 12` semitone loop:
- `signal_frames.fs` (first to migrate)
- `spectral_arcs.fs`
- `motherboard.fs`
- `pitch_spiral.fs`
- `arc_strobe.fs`
- `filaments.fs`
- `slashes.fs`
- `nebula.fs`
