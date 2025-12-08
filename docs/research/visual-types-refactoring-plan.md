# Spectrum Bars Feature Plan

Multi-phase plan to add spectrum bar visualization alongside waveforms in AudioJones.

## Scope

**In scope**: Radial spectrum bars with linear/circular modes (matching waveform modes).

**Out of scope**: Spectral rings, visual layer abstraction, preset schema changes. These remain future work if spectrum bars prove valuable.

## Problem Statement

Adding spectrum bars requires two architectural changes:

1. **FFT data locked in BeatDetector** (`src/beat.cpp:45-120`). The magnitude spectrum (1025 bins) remains private. Only beat intensity (float 0-1) reaches other modules. Spectrum bars require direct access to frequency magnitude data.

2. **UI panel saturation**. `UIDrawWaveformPanel()` (`src/ui.cpp`) displays all controls in one panel. Adding spectrum controls increases clutter. Solution: accordion-style collapsible sections using `GuiToggle`.

## Design Decisions

| Question | Decision | Rationale |
|----------|----------|-----------|
| Band count | Fixed 32 bands | Simple; covers audible range at ~1/3 octave resolution |
| Smoothing | Per-band exponential | More responsive than frame-average; matches beat decay behavior |
| Frequency ranges | Hardcoded logarithmic | Easy to change later; matches human perception |
| UI pattern | Accordion via GuiToggle | Native raygui; classic imgui feel |
| Linear/circular | Both modes (like waveforms) | Toggled with SPACE alongside waveforms |

## Data Flow Changes

Current:
```
AudioCapture → BeatDetector (FFT internal) → beatIntensity → Visualizer
            → ProcessWaveform → waveformExtended[] → DrawWaveformCircular
```

Proposed:
```
AudioCapture → SpectralProcessor (FFT exposed) → magnitude[1025]
                    ↓                                  ↓
             BeatDetector (consumes)          SpectrumBars (consumes)
                    ↓                                  ↓
             beatIntensity                    bandLevels[32]
                    ↓                                  ↓
             Visualizer (bloom/chromatic)     DrawSpectrumBars (linear/circular)

            → ProcessWaveform → waveformExtended[] → DrawWaveformCircular
```

## Phase 1: Extract Spectral Processing

**Goal**: Decouple FFT computation from beat detection. Create shared spectral data accessible to multiple consumers.

### 1.1 Create SpectralProcessor Module

New files: `src/spectral.h`, `src/spectral.cpp`

```c
// spectral.h
#define SPECTRAL_FFT_SIZE 2048
#define SPECTRAL_BIN_COUNT 1025  // FFT_SIZE/2 + 1

typedef struct SpectralProcessor SpectralProcessor;

SpectralProcessor* SpectralProcessorInit(void);
void SpectralProcessorUninit(SpectralProcessor* sp);

// Feed audio samples (mono, accumulated internally)
void SpectralProcessorFeed(SpectralProcessor* sp, const float* samples, int frameCount);

// Process FFT when enough samples accumulated (returns true if updated)
bool SpectralProcessorUpdate(SpectralProcessor* sp);

// Access results (valid after Update returns true)
const float* SpectralProcessorGetMagnitude(const SpectralProcessor* sp);  // 1025 bins
const float* SpectralProcessorGetSmoothed(const SpectralProcessor* sp);   // temporally smoothed
int SpectralProcessorGetBinCount(const SpectralProcessor* sp);
float SpectralProcessorGetBinFrequency(const SpectralProcessor* sp, int bin);  // Hz
```

Implementation moves FFT logic from `beat.cpp:45-120` (sample accumulation, Hann window, kiss_fftr call, magnitude computation) into SpectralProcessor.

### 1.2 Refactor BeatDetector as Consumer

Modify `BeatDetector` to receive spectral data instead of raw audio:

```c
// beat.h (modified)
void BeatDetectorProcess(BeatDetector* bd,
                         const float* magnitude,  // from SpectralProcessor
                         int binCount,
                         float deltaTime,
                         float sensitivity);
```

Remove from BeatDetector:
- `kiss_fftr_cfg fftConfig`
- `float sampleBuffer[2048]`
- `float windowedSamples[2048]`
- `kiss_fft_cpx spectrum[1025]`
- FFT execution code

Retain in BeatDetector:
- `float magnitude[1025]` (copy for diff computation)
- `float prevMagnitude[1025]`
- Spectral flux calculation (bass bins 1-8)
- Rolling average/stddev
- Beat detection threshold logic

### 1.3 Update main.cpp Data Flow

```c
// main.cpp (modified update loop)
SpectralProcessorFeed(ctx.spectral, monoBuffer, frameCount);
if (SpectralProcessorUpdate(ctx.spectral)) {
    const float* mag = SpectralProcessorGetMagnitude(ctx.spectral);
    int binCount = SpectralProcessorGetBinCount(ctx.spectral);
    BeatDetectorProcess(&ctx.beat, mag, binCount, deltaTime, sensitivity);
}
```

### Deliverables
- [ ] `src/spectral.h` and `src/spectral.cpp` with FFT processing
- [ ] Refactored `src/beat.cpp` consuming spectral data
- [ ] Updated `src/main.cpp` data flow
- [ ] Existing functionality unchanged (beat detection works)

---

## Phase 2: Extract ColorConfig and Add Spectrum Bars

**Goal**: Extract shared color configuration, then render spectrum bars in linear and circular modes.

### 2.1 Extract ColorConfig

New file: `src/color_config.h`

```c
#ifndef COLOR_CONFIG_H
#define COLOR_CONFIG_H

#include "raylib.h"

typedef enum {
    COLOR_MODE_SOLID,
    COLOR_MODE_RAINBOW
} ColorMode;

struct ColorConfig {
    ColorMode mode = COLOR_MODE_SOLID;
    Color solid = WHITE;
    float rainbowHue = 0.0f;      // Start hue (0-360)
    float rainbowRange = 360.0f;  // Hue span (0-360)
    float rainbowSat = 1.0f;      // Saturation (0-1)
    float rainbowVal = 1.0f;      // Brightness (0-1)
};

#endif
```

Refactor `WaveformConfig` (`src/waveform.h`):

```c
// Before:
struct WaveformConfig {
    // ... geometry fields ...
    Color color = WHITE;
    ColorMode colorMode = COLOR_MODE_SOLID;
    float rainbowHue = 0.0f;
    float rainbowRange = 360.0f;
    float rainbowSat = 1.0f;
    float rainbowVal = 1.0f;
};

// After:
#include "color_config.h"

struct WaveformConfig {
    // ... geometry fields ...
    ColorConfig color;
};
```

Update call sites:
- `waveform.cpp`: Access `config->color.mode`, `config->color.solid`, etc.
- `ui.cpp`: Color picker/rainbow controls reference `config->waveforms[i].color`
- `preset.cpp`: JSON serialization for nested ColorConfig

### 2.2 Define Spectrum Config

New file: `src/spectrum_config.h`

```c
#ifndef SPECTRUM_CONFIG_H
#define SPECTRUM_CONFIG_H

#include "color_config.h"

#define SPECTRUM_BAND_COUNT 32

struct SpectrumConfig {
    bool enabled = false;
    float innerRadius = 0.15f;   // Circular mode: base radius (fraction of minDim)
    float barHeight = 0.25f;     // Max bar height (fraction of minDim)
    float barWidth = 0.8f;       // Bar width (0.5-1.0, fraction of arc/slot)
    float smoothing = 0.8f;      // Per-band smoothing (0.0-0.95, higher = slower decay)
    float minDb = -50.0f;        // Floor threshold (dB)
    float maxDb = -10.0f;        // Ceiling threshold (dB)
    float rotationSpeed = 0.0f;
    float rotationOffset = 0.0f;
    ColorConfig color;           // Shared color config
};

#endif
```

### 2.3 Create Spectrum Module

New files: `src/spectrum.h`, `src/spectrum.cpp`

```c
// spectrum.h
#define SPECTRUM_BAND_COUNT 32

typedef struct SpectrumBars SpectrumBars;

SpectrumBars* SpectrumBarsInit(void);
void SpectrumBarsUninit(SpectrumBars* sb);

// Process magnitude spectrum into 32 display bands (call when FFT updates)
void SpectrumBarsProcess(SpectrumBars* sb,
                         const float* magnitude,
                         int binCount,
                         float smoothing);

// Render to current render target
void SpectrumBarsDrawCircular(const SpectrumBars* sb,
                              const RenderContext* ctx,
                              const SpectrumConfig* config,
                              uint64_t globalTick);

void SpectrumBarsDrawLinear(const SpectrumBars* sb,
                            const RenderContext* ctx,
                            const SpectrumConfig* config,
                            uint64_t globalTick);
```

### 2.4 Band Mapping

Logarithmic frequency mapping (32 bands spanning 20Hz-20kHz):

```c
// Compute bin ranges for each band (done once at init)
// Band 0: 20-32 Hz, Band 31: 12.5k-20k Hz
// Each band ~1/3 octave wide
float minFreq = 20.0f;
float maxFreq = 20000.0f;
float logMin = log2f(minFreq);
float logMax = log2f(maxFreq);

for (int i = 0; i < SPECTRUM_BAND_COUNT; i++) {
    float t0 = (float)i / SPECTRUM_BAND_COUNT;
    float t1 = (float)(i + 1) / SPECTRUM_BAND_COUNT;
    float f0 = exp2f(logMin + t0 * (logMax - logMin));
    float f1 = exp2f(logMin + t1 * (logMax - logMin));
    bands[i].binStart = (int)(f0 / binResolution);
    bands[i].binEnd = (int)(f1 / binResolution);
}
```

### 2.5 Rendering Implementation

**Circular Mode** (`SpectrumBarsDrawCircular`):
- 32 bars radiating from center, evenly spaced around 360 degrees
- Bar base at `innerRadius`, extends outward by `bandLevel * barHeight`
- Each bar: filled triangle or thick line
- Rotation: `angle = (i / 32.0) * 2π + rotationOffset + rotationSpeed * globalTick`
- Color: solid or rainbow gradient by band index

**Linear Mode** (`SpectrumBarsDrawLinear`):
- 32 bars horizontally across screen, centered vertically
- Bar width = screenWidth / 32 * barWidth
- Bar height = bandLevel * barHeight * minDim
- Rotation: apply 2D rotation matrix around screen center
- Color: solid or rainbow gradient by band index

### 2.6 Per-Band Smoothing

Exponential moving average per band:

```c
// On each FFT update
for (int i = 0; i < SPECTRUM_BAND_COUNT; i++) {
    float raw = ComputeBandLevel(magnitude, bands[i]);  // peak or RMS of bins
    float dbValue = 20.0f * log10f(raw + 1e-10f);       // convert to dB
    float normalized = (dbValue - minDb) / (maxDb - minDb);
    normalized = Clamp(normalized, 0.0f, 1.0f);

    // Exponential smoothing: high smoothing = slow decay
    smoothedBands[i] = smoothedBands[i] * smoothing + normalized * (1.0f - smoothing);
}
```

### 2.7 Deliverables
- [ ] `src/color_config.h` with ColorConfig struct and ColorMode enum
- [ ] Refactor WaveformConfig to use ColorConfig
- [ ] Update `waveform.cpp` color access
- [ ] Update `ui.cpp` color controls
- [ ] Update `preset.cpp` JSON serialization for nested ColorConfig
- [ ] `src/spectrum_config.h` with SpectrumConfig struct
- [ ] `src/spectrum.h` and `src/spectrum.cpp`
- [ ] Logarithmic band mapping (32 bands)
- [ ] `SpectrumBarsDrawCircular()` rendering
- [ ] `SpectrumBarsDrawLinear()` rendering
- [ ] Per-band exponential smoothing

---

## Phase 3: UI and Integration

**Goal**: Add spectrum controls to UI with accordion sections, integrate into main loop and presets.

### 3.1 Accordion UI Pattern

Use `GuiToggle` as section headers. Raygui provides this natively.

```c
// ui.cpp pattern
static bool waveformSectionExpanded = true;
static bool spectrumSectionExpanded = true;
static bool effectsSectionExpanded = true;

void UIDrawPanel(...) {
    UILayout layout = UILayoutBegin(x, y, width, padding, spacing);

    // Waveforms section (collapsible)
    GuiToggle(UILayoutRow(&layout, 24), "# Waveforms", &waveformSectionExpanded);
    if (waveformSectionExpanded) {
        UIDrawWaveformControls(&layout, ...);
    }

    // Spectrum section (collapsible)
    GuiToggle(UILayoutRow(&layout, 24), "# Spectrum", &spectrumSectionExpanded);
    if (spectrumSectionExpanded) {
        UIDrawSpectrumControls(&layout, ...);
    }

    // Effects section (collapsible)
    GuiToggle(UILayoutRow(&layout, 24), "# Effects", &effectsSectionExpanded);
    if (effectsSectionExpanded) {
        UIDrawEffectsControls(&layout, ...);
    }
}
```

### 3.2 Spectrum Controls

Add to `src/ui.cpp` (or new `src/ui_spectrum.cpp`):

```c
void UIDrawSpectrumControls(UILayout* layout, SpectrumConfig* config, UIState* state) {
    // Enable toggle
    GuiCheckBox(UILayoutSlot(layout, 1.0f), "Enabled", &config->enabled);

    // Geometry
    GuiSliderBar(..., "Radius", &config->innerRadius, 0.05f, 0.4f);
    GuiSliderBar(..., "Height", &config->barHeight, 0.1f, 0.5f);
    GuiSliderBar(..., "Width", &config->barWidth, 0.3f, 1.0f);

    // Dynamics
    GuiSliderBar(..., "Smoothing", &config->smoothing, 0.0f, 0.95f);
    GuiSliderBar(..., "Min dB", &config->minDb, -80.0f, -20.0f);
    GuiSliderBar(..., "Max dB", &config->maxDb, -30.0f, 0.0f);

    // Rotation
    GuiSliderBar(..., "Rotation", &config->rotationSpeed, -0.05f, 0.05f);

    // Color (reuse existing color controls)
    UIDrawColorControls(layout, &config->color, state);
}
```

### 3.3 Extract Color Controls

Factor out color UI for reuse by both waveform and spectrum:

```c
// ui.cpp
void UIDrawColorControls(UILayout* layout, ColorConfig* color, UIState* state) {
    // Color mode dropdown
    GuiDropdownBox(..., "SOLID;RAINBOW", (int*)&color->mode, state->colorModeDropdownOpen);

    if (color->mode == COLOR_MODE_SOLID) {
        GuiColorPicker(..., &color->solid);
        GuiSliderBar(..., "Alpha", &alpha, 0, 255);
    } else {
        GuiHueRangeSlider(..., &color->rainbowHue, &color->rainbowRange, ...);
        GuiSliderBar(..., "Saturation", &color->rainbowSat, 0.0f, 1.0f);
        GuiSliderBar(..., "Brightness", &color->rainbowVal, 0.0f, 1.0f);
    }
}
```

### 3.4 Main Loop Integration

Update `main.cpp`:

```c
// AppContext additions
SpectralProcessor* spectral;
SpectrumBars* spectrumBars;
SpectrumConfig spectrum;

// In update loop
SpectralProcessorFeed(ctx.spectral, monoBuffer, frameCount);
if (SpectralProcessorUpdate(ctx.spectral)) {
    const float* mag = SpectralProcessorGetMagnitude(ctx.spectral);
    int binCount = SpectralProcessorGetBinCount(ctx.spectral);

    BeatDetectorProcess(&ctx.beat, mag, binCount, deltaTime, sensitivity);
    SpectrumBarsProcess(ctx.spectrumBars, mag, binCount, ctx.spectrum.smoothing);
}

// In render loop (after waveforms)
if (ctx.spectrum.enabled) {
    if (ctx.mode == MODE_CIRCULAR) {
        SpectrumBarsDrawCircular(ctx.spectrumBars, &renderCtx, &ctx.spectrum, ctx.globalTick);
    } else {
        SpectrumBarsDrawLinear(ctx.spectrumBars, &renderCtx, &ctx.spectrum, ctx.globalTick);
    }
}
```

### 3.5 Preset Update

Add SpectrumConfig to Preset struct:

```c
// preset.h
typedef struct {
    char name[PRESET_NAME_MAX];
    EffectsConfig effects;
    AudioConfig audio;
    WaveformConfig waveforms[MAX_WAVEFORMS];
    int waveformCount;
    SpectrumConfig spectrum;  // NEW
} Preset;
```

JSON schema changes:
- `waveforms[].color` becomes nested `ColorConfig` object (was flat fields)
- New `spectrum` object added

**Breaking change**: Existing preset JSON files require manual updates after Phase 2 (ColorConfig) and Phase 3 (SpectrumConfig). No migration logic needed.

### 3.6 Deliverables
- [ ] Accordion-style collapsible sections using GuiToggle
- [ ] `UIDrawSpectrumControls()` function
- [ ] `UIDrawColorControls()` extracted and reused
- [ ] SpectrumBars integrated into main render loop
- [ ] Mode toggle (SPACE) affects spectrum bars
- [ ] SpectrumConfig added to Preset
- [ ] JSON serialization for SpectrumConfig and ColorConfig

---

## Phase Summary

| Phase | Focus | Key Changes | Risk |
|-------|-------|-------------|------|
| 1 | FFT extraction | New SpectralProcessor, refactor BeatDetector | Low - isolated change |
| 2 | ColorConfig + Spectrum | Extract colors, add SpectrumBars renderer | Medium - touches waveform.cpp |
| 3 | UI + Integration | Accordion UI, main loop, presets | Medium - UI changes |

## Dependencies

```
Phase 1 (FFT) ────> Phase 2 (ColorConfig + Spectrum) ────> Phase 3 (UI + Integration)
```

Linear dependency chain. Each phase produces testable output before proceeding.

## Future Work (Out of Scope)

- **Spectral rings**: Concentric frequency rings visualization
- **Visual layer abstraction**: Unified layer system for mixing waveforms and spectrum
