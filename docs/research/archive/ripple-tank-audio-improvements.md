# Ripple Tank Audio Improvements

Fix hard angles and straight lines in audio mode by increasing waveform buffer resolution, and add a new spectral synthesis mode that uses FFT magnitudes to drive radial standing waves from each source.

## Classification

- **Category**: GENERATORS > Cymatics (enhancement to existing Ripple Tank)
- **Pipeline Position**: No change — Ripple Tank keeps its current pipeline position
- **Compute Model**: No change — uses existing ping-pong render textures

## References

- Existing Faraday shader `shaders/faraday.fs` — FFT band sampling + lattice superposition pattern (lines 48-80)
- Existing Ripple Tank shader `shaders/ripple_tank.fs` — radial wave interference from point sources (lines 64-92)
- Existing analysis pipeline `src/analysis/analysis_pipeline.cpp` — waveform history buffering (lines 116-154)

## Reference Code

### Current waveform history buffering (analysis_pipeline.cpp:116-154)

```c
void AnalysisPipelineUpdateWaveformHistory(AnalysisPipeline *pipeline) {
  // Find peak amplitude in this update
  float peakSigned = 0.0f;
  if (pipeline->lastFramesRead > 0) {
    const uint32_t frameCount = pipeline->lastFramesRead;
    float peak = 0.0f;
    for (uint32_t i = 0; i < frameCount; i++) {
      const size_t idx = (size_t)i * AUDIO_CHANNELS;
      const float left = pipeline->audioBuffer[idx];
      const float right = pipeline->audioBuffer[idx + 1];
      const float mono = (left + right) * 0.5f;
      if (fabsf(mono) > peak) {
        peak = fabsf(mono);
        peakSigned = mono;
      }
    }
  }

  // Smooth the envelope (~2Hz response)
  const float ALPHA = 0.1f;
  pipeline->waveformEnvelope +=
      ALPHA * (peakSigned - pipeline->waveformEnvelope);

  if (fabsf(pipeline->waveformEnvelope) < 0.01f) {
    pipeline->waveformEnvelope = 0.0f;
  }

  // Store one smoothed value per update
  const float stored = pipeline->waveformEnvelope * 0.5f + 0.5f;
  pipeline->waveformHistory[pipeline->waveformWriteIndex] = stored;
  pipeline->waveformWriteIndex =
      (pipeline->waveformWriteIndex + 1) % WAVEFORM_HISTORY_SIZE;
}
```

### Faraday FFT band sampling (faraday.fs:48-80)

```glsl
for (int i = 0; i < layers; i++) {
    float t0 = (layers > 1) ? float(i) / float(layers - 1) : 0.5;
    float t1 = float(i + 1) / float(layers);
    float freqLo = baseFreq * pow(maxFreq / baseFreq, t0);
    float freqHi = baseFreq * pow(maxFreq / baseFreq, t1);
    float binLo = freqLo / nyquist;
    float binHi = freqHi / nyquist;

    // Band-averaged FFT energy
    float energy = 0.0;
    for (int s = 0; s < BAND_SAMPLES; s++) {
        float bin = mix(binLo, binHi, (float(s) + 0.5) / float(BAND_SAMPLES));
        if (bin <= 1.0) {
            energy += texture(fftTexture, vec2(bin, 0.5)).r;
        }
    }
    float mag = pow(clamp(energy / float(BAND_SAMPLES) * gain, 0.0, 1.0), curve);
    if (mag < 0.001) continue;

    float freq = baseFreq * pow(maxFreq / baseFreq, t0);
    float k = freq * spatialScale;
    float layerHeight = lattice(uv, k, waveCount, rotationOffset);
    layerHeight *= cos(time * freq * 0.5);

    totalHeight += mag * layerHeight;
    totalWeight += mag;
}
```

### Ripple Tank radial wave (ripple_tank.fs:64-77)

```glsl
float waveFromSource(vec2 uv, vec2 src, float phase) {
    float dist = length(uv - src);
    float atten = falloff(dist, falloffType, falloffStrength);

    if (waveSource == 0) {
        float delay = dist * waveScale;
        float spreading = 1.0 / sqrt(dist + 0.01);
        return fetchWaveform(delay) * spreading * atten;
    } else {
        return sin(dist * waveFreq - time + phase) * atten;
    }
}
```

## Algorithm

### Part 1: Improved audio buffer resolution

**Problem**: `AnalysisPipelineUpdateWaveformHistory()` writes one heavily-smoothed envelope sample per frame (~60Hz, 2Hz response). With `waveScale=50`, the visible distance range maps to ~50 discrete values that are nearly constant for long stretches, producing piecewise-constant contours with hard geometric edges.

**Fix**: Write decimated audio samples instead of one envelope value per update.

1. Increase `WAVEFORM_HISTORY_SIZE` from `2048` to `16384`
2. Increase `WAVEFORM_TEXTURE_SIZE` from `2048` to `16384` (64KB — trivial)
3. Replace the one-sample-per-frame logic with a decimation loop:
   - Target write rate: ~480 samples/sec (decimate raw 48kHz audio by ~100)
   - Each decimation chunk: take peak amplitude of ~100 raw samples, store as one history sample
   - Light smoothing: `ALPHA ≈ 0.3` (~30Hz envelope response) — fast enough to track beats, smooth enough to avoid sample-level noise
   - Write `lastFramesRead / DECIMATION_FACTOR` samples per update (~13 at 60fps)
4. Adjust `waveScale` default from `50` to `400` and range from `1-50` to `1-400` to compensate for the 8x higher write rate (same visual speed at default)
5. Texture already uses `TEXTURE_FILTER_BILINEAR` — the higher sample density gives the GPU more points to interpolate between, directly eliminating the hard edges

**Result**: Same 34-second buffer history, same visual speed at default, 8x more waveform detail within the visible range. Bilinear interpolation between dense samples produces smooth curves instead of plateaus.

### Part 2: Spectral synthesis mode (waveSource == 2)

**Concept**: Instead of sampling a time-history buffer, reconstruct radial standing waves from FFT magnitudes. Each frequency band creates a sine wave radiating from each source, weighted by the FFT energy at that frequency. This is the Faraday lattice algorithm applied to radial waves instead of directional lattice vectors.

**Shader changes in `ripple_tank.fs`**:

1. Add new uniforms (spectral mode only):
   - `uniform sampler2D fftTexture;`
   - `uniform float sampleRate;`
   - `uniform float baseFreq;`
   - `uniform float maxFreq;`
   - `uniform float gain;`
   - `uniform float curve;`
   - `uniform float spatialScale;`
   - `uniform int layers;`

2. Add new `waveFromSource` branch for `waveSource == 2`:
   - Reuse Faraday's log-space frequency band loop with band-averaged FFT sampling
   - Replace `lattice(uv, k, waveCount, rotationOffset)` with `sin(dist * k - time * freq * 0.5 + phase)`
   - The `dist * k` term creates radial waves; `time * freq * 0.5` is the Faraday half-frequency oscillation; `phase` is the per-source offset
   - Weight each band's contribution by its FFT magnitude (same `pow(clamp(...), curve)` pattern)
   - Normalize by total magnitude weight (same `totalWeight` pattern as Faraday)
   - Apply existing `falloff()` attenuation

3. Keep `waveSource == 0` (improved audio buffer) and `waveSource == 1` (parametric sine) unchanged

**Config changes in `ripple_tank.h`**:

Add spectral mode params to `RippleTankConfig`:
- `int layers = 6;` — number of FFT frequency bands (1-16)
- `float baseFreq = 55.0f;` — lowest analyzed frequency Hz (27.5-440)
- `float maxFreq = 14000.0f;` — highest analyzed frequency Hz (1000-16000)
- `float gain = 2.0f;` — FFT energy sensitivity (0.1-10.0)
- `float curve = 1.5f;` — contrast exponent (0.1-3.0)
- `float spatialScale = 0.02f;` — frequency to spatial wave number (0.001-0.1)

These params are only visible in the UI when `waveSource == 2`, matching how `waveScale` hides when not in audio mode.

**CPU changes in `ripple_tank.cpp`**:

- `BindWaveSource()`: when `waveSource == 2`, bind FFT texture + spectral uniforms, accumulate `time` via `waveSpeed` (same as sine mode)
- `RippleTankEffectSetup()`: accept `fftTexture` parameter (already passed through render pipeline to Faraday)
- `RippleTankEffectRender()`: bind `fftTexture` as additional texture unit when in spectral mode

### UI layout

Mode selector changes from 2 options to 3:
- 0 = Audio (waveform ring buffer — shows waveScale)
- 1 = Sine (parametric — shows waveFreq, waveSpeed)
- 2 = Spectral (FFT synthesis — shows layers, baseFreq, maxFreq, gain, curve, spatialScale, waveSpeed)

The spectral Audio section follows the standard FFT audio UI conventions from `docs/conventions.md` (same slider order and ranges as Faraday).

## Parameters

### Modified existing

| Parameter | Type | Old Range | New Range | New Default | Change |
|-----------|------|-----------|-----------|-------------|--------|
| waveSource | int | 0-1 | 0-2 | 0 | Add spectral option |
| waveScale | float | 1-50 | 1-400 | 400 | Compensate for 8x buffer write rate |

### New (spectral mode)

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| layers | int | 1-16 | 6 | Number of FFT frequency bands driving the pattern |
| baseFreq | float | 27.5-440 | 55.0 | Lowest analyzed frequency Hz |
| maxFreq | float | 1000-16000 | 14000.0 | Highest analyzed frequency Hz |
| gain | float | 0.1-10.0 | 2.0 | FFT energy sensitivity |
| curve | float | 0.1-3.0 | 1.5 | Contrast exponent on FFT magnitudes |
| spatialScale | float | 0.001-0.1 | 0.02 | Maps frequency (Hz) to spatial wave number (radians/unit) |

### Buffer constants (CPU)

| Constant | Old Value | New Value | Effect |
|----------|-----------|-----------|--------|
| WAVEFORM_HISTORY_SIZE | 2048 | 16384 | 8x more history samples |
| WAVEFORM_TEXTURE_SIZE | 2048 | 16384 | Match buffer size |
| DECIMATION_FACTOR | N/A | ~100 | 48kHz / 100 = 480 samples/sec write rate |

## Modulation Candidates

- **spatialScale**: changes spatial density of standing wave patterns — tight moiré at high values, wide gentle ripples at low
- **layers**: more bands = richer harmonic interference; fewer = cleaner, simpler patterns
- **gain**: controls the FFT energy threshold — low gain shows only the loudest frequencies, high gain reveals the full spectrum
- **waveScale** (audio mode): with the higher-resolution buffer, modulating waveScale now produces smooth zoom rather than quantized jumps

### Interaction Patterns

- **Spectral layers × audio complexity (resonance)**: With many layers, complex audio (full-band chorus) creates dense multi-frequency interference. Simple audio (solo instrument) produces clean, sparse radial patterns. Visual complexity naturally tracks audio complexity.
- **Spectral gain × curve (gating)**: High `curve` exponentiates FFT magnitudes, making quiet frequencies invisible. Combined with `gain`, this creates a dynamic threshold — only the loudest frequency peaks produce visible waves. Verse/chorus transitions create dramatic pattern changes as different frequency bands cross the visibility threshold.
- **Improved audio waveScale × transient detail**: With 8x buffer resolution, fast transients (snare hits, consonants) create sharp, distinct wavefronts propagating outward. Slow sounds (pads, bass) create wide, smooth waves. `waveScale` now smoothly zooms into this detail rather than jumping between quantized plateaus.

## Notes

- The waveform texture at 16384x1 R32 = 64KB. Trivial GPU memory. Full-buffer upload per frame is fine at this size.
- The spectral mode's per-pixel cost scales with `layers`. At layers=6 with 4 band sub-samples each, that's 24 FFT texture lookups + 6 sin() calls per source per pixel. With 5 sources = 120 lookups + 30 sin(). Affordable but not free — if it becomes a concern, reducing `BAND_SAMPLES` from 4 to 2 for spectral mode halves the lookups with minimal quality loss.
- The `waveScale` range change (1-400 from 1-50) is a breaking change for existing presets. The plan should include a one-time migration multiplier in `effect_serialization.cpp` or accept that old presets will look different. Given the project's no-migration convention, old presets will show 8x faster wave propagation — users adjust `waveScale` upward.
- The chromatic color mode path also uses `waveFromSource` — the spectral mode will automatically work with chromatic separation since it goes through the same function.
- `waveSpeed` is shared between sine and spectral modes. In spectral mode it controls the temporal oscillation rate of all frequency bands simultaneously. This is intentional — per-band speed differences come from the `time * freq * 0.5` term (higher frequencies oscillate faster), while `waveSpeed` is a master rate control.
