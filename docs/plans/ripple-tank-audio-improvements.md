# Ripple Tank Audio Improvements

Add a spectral synthesis mode that uses FFT magnitudes to drive radial standing waves from each source.

**Research**: `docs/research/ripple-tank-audio-improvements.md`

## Implementation Notes

**Part 1 (buffer resolution) was reverted.** The research proposed increasing `WAVEFORM_HISTORY_SIZE` from 2048 to 16384, replacing the one-sample-per-frame envelope with decimated peak samples at 480Hz, and scaling `waveScale` from 1-50 to 1-400 to compensate. In practice this failed: peak-detected samples at 480Hz have far more inter-sample variation than the original 2Hz envelope, producing noisy/aliased moiré patterns regardless of ALPHA tuning or waveScale. Increasing waveScale to 400 masked the noise at the cost of spreading 8x more noisy detail across the same screen space. More resolution of noisy data is not smoother data. All Part 1 changes (analysis pipeline decimation, buffer sizes, waveScale default/range, texture sizes) were reverted to their original values.

**Part 2 (spectral synthesis mode) was implemented as planned.** New `waveSource == 2` adds FFT-driven radial standing waves using the Faraday band-averaged energy pattern. Audio mode (waveSource 0) and parametric mode (waveSource 1) are unchanged.

## Design

### Types

**Modified: `AnalysisPipeline` (analysis_pipeline.h)**

```
WAVEFORM_HISTORY_SIZE: 2048 → 16384
```

No new fields. `waveformEnvelope` remains (still used as state for decimation smoothing).

**Modified: `RippleTankConfig` (ripple_tank.h)**

```c
// Wave source — add option 2
int waveSource = 0; // 0=audio waveform, 1=parametric, 2=spectral

// Audio mode — updated range
float waveScale = 400.0f; // Audio mode pattern scale (1-400)

// Spectral mode (new fields)
int layers = 6;              // Number of FFT frequency bands (1-16)
float baseFreq = 55.0f;     // Lowest analyzed frequency Hz (27.5-440)
float maxFreq = 14000.0f;   // Highest analyzed frequency Hz (1000-16000)
float gain = 2.0f;          // FFT energy sensitivity (0.1-10.0)
float curve = 1.5f;         // Contrast exponent (0.1-3.0)
float spatialScale = 0.02f; // Frequency to spatial wave number (0.001-0.1)
```

**Modified: `RippleTankEffect` (ripple_tank.h)**

New uniform location fields:
```c
Texture2D currentFftTexture; // Stored in Setup, used in Render

int fftTextureLoc;
int sampleRateLoc;
int layersLoc;
int baseFreqLoc;
int maxFreqLoc;
int gainLoc;
int curveLoc;
int spatialScaleLoc;
```

Updated Setup signature:
```c
void RippleTankEffectSetup(RippleTankEffect *e, RippleTankConfig *cfg,
                           float deltaTime, Texture2D waveformTexture,
                           int waveformWriteIndex, Texture2D fftTexture);
```

**Modified: `WAVEFORM_TEXTURE_SIZE` (post_effect.cpp)**

```
WAVEFORM_TEXTURE_SIZE: 2048 → 16384
```

### Algorithm

#### Part 1: Decimated waveform buffer (analysis_pipeline.cpp)

Replace the one-sample-per-frame envelope logic in `AnalysisPipelineUpdateWaveformHistory()` with a decimation loop:

```
DECIMATION_FACTOR = 100  (48000 Hz / 100 = 480 samples/sec)
ALPHA = 0.3              (~30 Hz envelope response)

if lastFramesRead == 0: return

totalSamples = lastFramesRead
samplesPerChunk = DECIMATION_FACTOR
chunksToWrite = totalSamples / samplesPerChunk

for each chunk (0..chunksToWrite-1):
    startSample = chunk * samplesPerChunk
    endSample = min(startSample + samplesPerChunk, totalSamples)

    // Find peak in chunk
    peakSigned = 0
    peak = 0
    for each frame in [startSample, endSample):
        mono = (left + right) * 0.5
        if abs(mono) > peak:
            peak = abs(mono)
            peakSigned = mono

    // Light smoothing
    waveformEnvelope += ALPHA * (peakSigned - waveformEnvelope)
    if abs(waveformEnvelope) < 0.01: waveformEnvelope = 0

    // Store as [0,1]
    stored = waveformEnvelope * 0.5 + 0.5
    waveformHistory[waveformWriteIndex] = stored
    waveformWriteIndex = (waveformWriteIndex + 1) % WAVEFORM_HISTORY_SIZE
```

Update `AnalysisPipelineInit()` to initialize the larger buffer (same 0.5f fill, same memset pattern — the loop already iterates `WAVEFORM_HISTORY_SIZE`).

#### Part 2: Spectral synthesis shader (ripple_tank.fs)

Add new uniforms:
```glsl
uniform sampler2D fftTexture;
uniform float sampleRate;
uniform int layers;
uniform float baseFreq;
uniform float maxFreq;
uniform float gain;
uniform float curve;
uniform float spatialScale;
```

Add `BAND_SAMPLES` constant:
```glsl
const int BAND_SAMPLES = 4;
```

Add new `waveSource == 2` branch in both `waveFromSource` functions. The standard version:

```glsl
float waveFromSource(vec2 uv, vec2 src, float phase) {
    float dist = length(uv - src);
    float atten = falloff(dist, falloffType, falloffStrength);

    if (waveSource == 0) {
        // Audio: existing code unchanged
        float delay = dist * waveScale;
        float spreading = 1.0 / sqrt(dist + 0.01);
        return fetchWaveform(delay) * spreading * atten;
    } else if (waveSource == 1) {
        // Parametric: existing code unchanged
        return wave(dist * waveFreq - time + phase, waveShape) * atten;
    } else {
        // Spectral: FFT-driven radial standing waves
        float nyquist = sampleRate * 0.5;
        float totalHeight = 0.0;
        float totalWeight = 0.0;

        for (int i = 0; i < layers; i++) {
            float t0 = (layers > 1) ? float(i) / float(layers - 1) : 0.5;
            float t1 = float(i + 1) / float(layers);
            float freq = baseFreq * pow(maxFreq / baseFreq, t0);
            float freqHi = baseFreq * pow(maxFreq / baseFreq, t1);
            float binLo = freq / nyquist;
            float binHi = freqHi / nyquist;

            // Band-averaged FFT energy (same as Faraday)
            float energy = 0.0;
            for (int s = 0; s < BAND_SAMPLES; s++) {
                float bin = mix(binLo, binHi, (float(s) + 0.5) / float(BAND_SAMPLES));
                if (bin <= 1.0) energy += texture(fftTexture, vec2(bin, 0.5)).r;
            }
            float mag = pow(clamp(energy / float(BAND_SAMPLES) * gain, 0.0, 1.0), curve);
            if (mag < 0.001) continue;

            float k = freq * spatialScale;
            // Radial standing wave: dist*k for spatial, time*freq*0.5 for Faraday half-freq oscillation
            float layerHeight = sin(dist * k - time * freq * 0.5 + phase);

            totalHeight += mag * layerHeight;
            totalWeight += mag;
        }

        if (totalWeight > 0.0) totalHeight /= totalWeight;
        return totalHeight * atten;
    }
}
```

The chromatic overload (`waveFromSource` with `freq` parameter) adds the same spectral branch. In this overload, the `freq` parameter is used to scale the spatial wave numbers for chromatic separation — multiply each band's `k` by `(freq / waveFreq)`:

```glsl
float waveFromSource(vec2 uv, vec2 src, float phase, float freq) {
    // ... same structure ...
    } else {
        // Spectral: same FFT loop, but scale k by freq/waveFreq for chromatic separation
        // ...
        float k = bandFreq * spatialScale * (freq / waveFreq);
        float layerHeight = sin(dist * k - time * bandFreq * 0.5 + phase);
        // ...
    }
}
```

#### Part 3: CPU binding (ripple_tank.cpp)

In `BindWaveSource()`, add `waveSource == 2` branch:
- Accumulate `e->time += cfg->waveSpeed * deltaTime` (same as sine mode)
- Bind `time`, `sampleRate` (48000.0f), `layers`, `baseFreq`, `maxFreq`, `gain`, `curve`, `spatialScale`
- Store `fftTexture` in `e->currentFftTexture`
- Compute and bind phases (same as sine mode)

In `RippleTankEffectRender()`, bind `fftTexture` when `waveSource == 2`:
```c
if (cfg->waveSource == 2) {
    SetShaderValueTexture(e->shader, e->fftTextureLoc, e->currentFftTexture);
}
```

Update `ComputeBrightness()`: spectral mode returns `1.0f` (same as sine mode).

### Parameters

| Parameter | Type | Range | Default | Modulatable | UI Label |
|-----------|------|-------|---------|-------------|----------|
| waveSource | int | 0-2 | 0 | No | Wave Source |
| waveScale | float | 1-400 | 400 | Yes | Wave Scale |
| layers | int | 1-16 | 6 | No | Layers |
| baseFreq | float | 27.5-440 | 55.0 | Yes | Base Freq (Hz) |
| maxFreq | float | 1000-16000 | 14000.0 | Yes | Max Freq (Hz) |
| gain | float | 0.1-10.0 | 2.0 | Yes | Gain |
| curve | float | 0.1-3.0 | 1.5 | Yes | Contrast |
| spatialScale | float | 0.001-0.1 | 0.02 | Yes | Spatial Scale |

### Constants

No new enum values. Existing `TRANSFORM_RIPPLE_TANK` unchanged.

---

## Tasks

### Wave 1: Headers

#### Task 1.1: Ripple Tank config and effect struct updates

**Files**: `src/effects/ripple_tank.h`
**Creates**: Updated config struct, effect struct, and Setup signature that all Wave 2 tasks depend on

**Do**:
1. Change `waveSource` comment to `// 0=audio waveform, 1=parametric, 2=spectral`
2. Change `waveScale` default from `50.0f` to `400.0f`, range comment from `(1-50)` to `(1-400)`
3. Add spectral mode fields after `waveSpeed`:
   - `int layers = 6;` with comment `// FFT frequency bands (1-16)`
   - `float baseFreq = 55.0f;` with comment `// Lowest analyzed frequency Hz (27.5-440)`
   - `float maxFreq = 14000.0f;` with comment `// Highest analyzed frequency Hz (1000-16000)`
   - `float gain = 2.0f;` with comment `// FFT energy sensitivity (0.1-10.0)`
   - `float curve = 1.5f;` with comment `// Contrast exponent (0.1-3.0)`
   - `float spatialScale = 0.02f;` with comment `// Freq to spatial wave number (0.001-0.1)`
4. Add new fields to `RIPPLE_TANK_CONFIG_FIELDS` macro: `layers, baseFreq, maxFreq, gain, curve, spatialScale`
5. Add to `RippleTankEffect`:
   - `Texture2D currentFftTexture;` after `currentWaveformTexture`
   - Uniform locations: `fftTextureLoc`, `sampleRateLoc`, `layersLoc`, `baseFreqLoc`, `maxFreqLoc`, `gainLoc`, `curveLoc`, `spatialScaleLoc`
6. Update `RippleTankEffectSetup` signature: add `Texture2D fftTexture` as last parameter

**Verify**: `cmake.exe --build build` compiles (will have link errors from .cpp not matching new signature — that's expected for Wave 1).

---

#### Task 1.2: Analysis pipeline buffer size increase

**Files**: `src/analysis/analysis_pipeline.h`
**Creates**: Larger `WAVEFORM_HISTORY_SIZE` constant

**Do**: Change `#define WAVEFORM_HISTORY_SIZE 2048` to `#define WAVEFORM_HISTORY_SIZE 16384`

**Verify**: Header change only — no compilation needed.

---

### Wave 2: Parallel Implementation

#### Task 2.1: Analysis pipeline decimation logic

**Files**: `src/analysis/analysis_pipeline.cpp`
**Depends on**: Wave 1 complete (uses new `WAVEFORM_HISTORY_SIZE`)

**Do**:
1. Replace `AnalysisPipelineUpdateWaveformHistory()` with the decimation loop from the Algorithm section (Part 1). Use `static const int DECIMATION_FACTOR = 100;` and `static const float ALPHA = 0.3f;`.
2. `AnalysisPipelineInit()`: no changes needed — the `for` loop already uses `WAVEFORM_HISTORY_SIZE` and `memset` uses `sizeof(pipeline->waveformHistory)`.

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 2.2: Waveform texture size increase

**Files**: `src/render/post_effect.cpp`
**Depends on**: Wave 1 complete

**Do**: Change `static const int WAVEFORM_TEXTURE_SIZE = 2048;` to `16384`. Update the `TraceLog` comment on line 313 in `post_effect.h` (the `waveformTexture` field comment says `2048x1`) to say `16384x1`.

Wait — the comment is in `post_effect.h` line 313. That's a different file. Only change `post_effect.cpp` here. The comment in `post_effect.h` is acceptable to update in this task since no other task touches `post_effect.h`.

**Files**: `src/render/post_effect.cpp`, `src/render/post_effect.h` (comment only)

**Do**:
1. In `post_effect.cpp`: change `static const int WAVEFORM_TEXTURE_SIZE = 2048;` to `16384`
2. In `post_effect.h`: update the `waveformTexture` field comment from `(2048x1)` to `(16384x1)`

**Verify**: `cmake.exe --build build` compiles.

---

#### Task 2.3: Ripple Tank shader — spectral mode

**Files**: `shaders/ripple_tank.fs`
**Depends on**: Wave 1 complete (uniform names from header)

**Do**:
1. Add new uniforms after existing wave parameters:
   ```glsl
   uniform sampler2D fftTexture;
   uniform float sampleRate;
   uniform int layers;
   uniform float baseFreq;
   uniform float maxFreq;
   uniform float gain;
   uniform float curve;
   uniform float spatialScale;
   ```
2. Add `const int BAND_SAMPLES = 4;` after the existing `TAU` constant
3. Update `waveSource` comment from `// 0=audio, 1=sine` to `// 0=audio, 1=sine, 2=spectral`
4. In the standard `waveFromSource(vec2, vec2, float)`:
   - Change the `else` to `else if (waveSource == 1)`
   - Add `else` block with the spectral FFT loop from the Algorithm section (Part 2, standard version)
5. In the chromatic `waveFromSource(vec2, vec2, float, float)`:
   - Same structural change: split `else` into `else if (waveSource == 1)` + new `else` for spectral
   - In the spectral branch, scale `k` by `(freq / waveFreq)` for chromatic separation

**Verify**: Shader compilation happens at runtime. Verify syntax by inspection.

---

#### Task 2.4: Ripple Tank CPU — spectral binding, UI, registration

**Files**: `src/effects/ripple_tank.cpp`
**Depends on**: Wave 1 complete (new config fields and effect struct)

**Do**:

1. **Init**: Cache new uniform locations (`fftTexture`, `sampleRate`, `layers`, `baseFreq`, `maxFreq`, `gain`, `curve`, `spatialScale`) in `RippleTankEffectInit()`, following existing pattern.

2. **BindWaveSource**: Update signature to accept `Texture2D fftTexture`. Restructure:
   - `waveSource == 0`: existing audio code (unchanged)
   - `waveSource == 1`: existing sine code (unchanged, extracted from current `else`)
   - `waveSource == 2`: new spectral mode:
     - Accumulate `e->time += cfg->waveSpeed * deltaTime`
     - Bind `time`
     - Bind `sampleRate` as `48000.0f` (use `AUDIO_SAMPLE_RATE` from `audio/audio.h` — add include)
     - Bind `layers`, `baseFreq`, `maxFreq`, `gain`, `curve`, `spatialScale`
     - Compute and bind `phases` (same pattern as sine mode)
     - Store `e->currentFftTexture = fftTexture`

3. **Setup**: Update `RippleTankEffectSetup` to accept `Texture2D fftTexture` parameter. Pass it to `BindWaveSource`.

4. **Render**: After existing `SetShaderValueTexture` for waveformTexture, add:
   ```c
   if (cfg->waveSource == 2) {
       SetShaderValueTexture(e->shader, e->fftTextureLoc, e->currentFftTexture);
   }
   ```

5. **ComputeBrightness**: Change `if (cfg->waveSource == 1)` to `if (cfg->waveSource >= 1)` so both sine and spectral return `1.0f`.

6. **Bridge function**: Update `SetupRippleTank` to pass `pe->fftTexture` as the new last argument.

7. **RegisterParams**:
   - Update `waveScale` range from `(1.0f, 50.0f)` to `(1.0f, 400.0f)`
   - Add registrations for: `baseFreq` (27.5, 440), `maxFreq` (1000, 16000), `gain` (0.1, 10.0), `curve` (0.1, 3.0), `spatialScale` (0.001, 0.1)

8. **UI** (`DrawRippleTankParams`): Update the Wave section:
   - Change Combo to 3 options: `"Audio\0Parametric\0Spectral\0"`
   - `waveSource == 0`: show Wave Scale (update range — the `ModulatableSlider` auto-reads from registered bounds)
   - `waveSource == 1`: existing parametric controls (unchanged)
   - `waveSource == 2`: show spectral controls:
     - `ModulatableSlider("Wave Speed##rt", ...)` — reuses existing waveSpeed param
     - `ImGui::SeparatorText("Audio")` — follows FFT Audio UI conventions from conventions.md
     - `ModulatableSlider("Base Freq (Hz)##rt", &e->rippleTank.baseFreq, "rippleTank.baseFreq", "%.1f", ms)`
     - `ModulatableSlider("Max Freq (Hz)##rt", &e->rippleTank.maxFreq, "rippleTank.maxFreq", "%.0f", ms)`
     - `ModulatableSlider("Gain##rt_spec", &e->rippleTank.gain, "rippleTank.gain", "%.1f", ms)` — note `##rt_spec` to avoid ID collision with visual Gain
     - `ModulatableSlider("Contrast##rt", &e->rippleTank.curve, "rippleTank.curve", "%.2f", ms)`
     - `ModulatableSliderLog("Spatial Scale##rt", &e->rippleTank.spatialScale, "rippleTank.spatialScale", "%.4f", ms)`
     - `ImGui::SliderInt("Layers##rt", &e->rippleTank.layers, 1, 16)` — in the Geometry/Wave section, not Audio section per conventions

**Verify**: `cmake.exe --build build` compiles with no warnings.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] Audio mode: waveforms appear smooth (no hard angles at default waveScale=400)
- [ ] Parametric mode: unchanged behavior
- [ ] Spectral mode: radial standing waves respond to FFT audio
- [ ] Spectral mode + chromatic color mode: RGB channel separation works
- [ ] UI: correct controls shown/hidden per wave source mode
- [ ] Preset save/load round-trips new fields
