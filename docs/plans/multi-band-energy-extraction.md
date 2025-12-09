# Multi-Band Energy Extraction

Implementation plan for Phase 1 of audio-reactive enhancements. Extracts bass, mid, and treble energy bands from FFT magnitude data with attack/release smoothing.

## Goal

Expose three smoothed frequency band values (`bassSmooth`, `midSmooth`, `trebSmooth`) for driving visual parameters. Values follow MilkDrop convention: 1.0 = average, <0.7 = quiet, >1.3 = loud.

## Current State

`src/analysis/beat.cpp:47-61` extracts energy from bins 1-8 (~23-188Hz) for kick detection. The remaining 1017 bins go unused. `src/render/spectrum_bars.cpp` computes 32 display bands but does not expose raw energy values for visualization parameters.

## Frequency Band Definitions

| Band | Frequency Range | FFT Bins (48kHz/2048) | Musical Content |
|------|-----------------|----------------------|-----------------|
| Bass | 20-250 Hz | 1-10 | Kick drum, bass guitar |
| Mid | 250-4000 Hz | 11-170 | Vocals, guitar, snare |
| Treble | 4000-20000 Hz | 171-853 | Hi-hats, cymbals, presence |

Bin calculation: `bin = frequency / (sampleRate / fftSize)` = `frequency / 23.4Hz`

These ranges match MilkDrop's band definitions documented in [Winamp Forums](https://forums.winamp.com/forum/visualizations/milkdrop/milkdrop-presets/301653-frequency-bands-for-bass-mid-treb).

## Algorithm

### RMS Energy Extraction

Compute root-mean-square energy per band from FFT magnitude:

```cpp
float ComputeBandRMS(const float* magnitude, int binStart, int binEnd) {
    float sumSquared = 0.0f;
    int count = binEnd - binStart;
    for (int i = binStart; i < binEnd; i++) {
        sumSquared += magnitude[i] * magnitude[i];
    }
    return sqrtf(sumSquared / count);
}
```

RMS provides perceptually balanced energy measurement. Peak detection responds to transients; RMS tracks sustained energy. Use RMS for bass and mid bands; consider peak for treble where transients dominate. Source: [Home Grail frequency guide](https://homegrail.com/bass-treble-hertz-frequency-chart/).

### Attack/Release Envelope Follower

Asymmetric exponential smoothing captures transients (fast attack) while preventing jitter (slow release):

```cpp
void SmoothEnergy(float* smoothed, float raw, float dt) {
    float tau = (raw > *smoothed) ? ATTACK_TIME : RELEASE_TIME;
    float alpha = 1.0f - expf(-dt / tau);
    *smoothed += alpha * (raw - *smoothed);
}
```

| Parameter | Value | Rationale |
|-----------|-------|-----------|
| Attack time | 10ms | Captures transients within one FFT frame |
| Release time | 150ms | Prevents jitter on sustained notes |

Time constant formula: `alpha = 1 - exp(-dt / tau)` where tau = time to reach 63% of target. Derived from [kferg.dev envelope followers](https://kferg.dev/posts/2020/audio-reactive-programming-envelope-followers/).

Alternative half-life formulation:
```cpp
float halfLifeSamples = sampleRate * (halfLifeMs / 1000.0f);
float a = expf(logf(0.5f) / halfLifeSamples);
```
Half-life of 15ms yields `a ≈ 0.999` at 48kHz, matching TouchDesigner's Lag CHOP defaults.

### Normalization

Raw RMS values vary with source material. Normalize against running average:

```cpp
// Update running average (slow decay)
bassAvg = bassAvg * 0.999f + rawBass * 0.001f;

// Normalize: 1.0 = average level
float bassNormalized = rawBass / (bassAvg + 1e-6f);
```

This produces MilkDrop-style values centered at 1.0.

## Data Structures

Create `src/analysis/bands.h`:

```cpp
#ifndef BANDS_H
#define BANDS_H

#define BAND_BASS_START   1
#define BAND_BASS_END     10    // 20-250 Hz
#define BAND_MID_START    11
#define BAND_MID_END      170   // 250-4000 Hz
#define BAND_TREB_START   171
#define BAND_TREB_END     853   // 4000-20000 Hz

typedef struct BandEnergies {
    // Raw RMS energy per band
    float bass;
    float mid;
    float treb;

    // Attack/release smoothed values
    float bassSmooth;
    float midSmooth;
    float trebSmooth;

    // Running averages for normalization
    float bassAvg;
    float midAvg;
    float trebAvg;
} BandEnergies;

void BandEnergiesInit(BandEnergies* bands);
void BandEnergiesProcess(BandEnergies* bands, const float* magnitude,
                         int binCount, float dt);

#endif
```

## Integration Points

### 1. Add to AppContext (`src/main.cpp:21-40`)

```cpp
typedef struct AppContext {
    // ... existing fields ...
    BandEnergies bands;    // Add after BeatDetector
} AppContext;
```

### 2. Call from Main Loop (`src/main.cpp:149-160`)

Insert after `BeatDetectorProcess()`:

```cpp
if (FFTProcessorUpdate(ctx->fft)) {
    const float* magnitude = FFTProcessorGetMagnitude(ctx->fft);
    int binCount = FFTProcessorGetBinCount(ctx->fft);

    BeatDetectorProcess(&ctx->beat, magnitude, binCount, deltaTime,
                        ctx->postEffect->effects.beatSensitivity);
    BandEnergiesProcess(&ctx->bands, magnitude, binCount, deltaTime);  // NEW
    SpectrumBarsProcess(ctx->spectrumBars, magnitude, binCount, &ctx->spectrum);
}
```

### 3. Expose to UI

Add display panel in `src/ui/panels.cpp` showing:
- Raw values: `bass`, `mid`, `treb`
- Smoothed values: `bassSmooth`, `midSmooth`, `trebSmooth`
- Visual bars or meters for real-time feedback

## File Changes

| File | Change |
|------|--------|
| `src/analysis/bands.h` | Create - struct and function declarations |
| `src/analysis/bands.cpp` | Create - RMS extraction and smoothing |
| `src/main.cpp` | Add `BandEnergies` to AppContext, call `BandEnergiesProcess()` |
| `src/ui/panels.cpp` | Add band energy display panel |
| `CMakeLists.txt` | Add `src/analysis/bands.cpp` to sources |

## Validation

- [ ] `bassSmooth` responds to kick drums within 20ms
- [ ] `midSmooth` tracks vocal/guitar passages
- [ ] `trebSmooth` follows hi-hat patterns
- [ ] Values center around 1.0 during normal playback
- [ ] No audio glitches or frame drops during processing
- [ ] UI displays real-time band energy meters

## Future Extensions

After this phase completes, band energies enable:

| Feature | Audio Input | Visual Output |
|---------|-------------|---------------|
| Waveform scaling | `bassSmooth` | `scale = base + bass * 0.3` |
| Color saturation | `midSmooth` | `saturation = 0.5 + mid * 0.5` |
| Bloom intensity | `trebSmooth` | `bloom = base + treb * 2.0` |

See `docs/plans/audio-reactive-enhancements.md` Phase 4 for mapping details.

## References

- [kferg.dev - Envelope Followers](https://kferg.dev/posts/2020/audio-reactive-programming-envelope-followers/) - Attack/release smoothing formulas
- [Home Grail - Frequency Ranges](https://homegrail.com/bass-treble-hertz-frequency-chart/) - Bass/mid/treble definitions
- [MilkDrop Forums](https://forums.winamp.com/forum/visualizations/milkdrop/milkdrop-presets/301653-frequency-bands-for-bass-mid-treb) - Band range conventions
- [Wikipedia - Spectral Centroid](https://en.wikipedia.org/wiki/Spectral_centroid) - Weighted frequency calculation
- `docs/research/deep-research1.md` - Source research for audio-reactive architecture
