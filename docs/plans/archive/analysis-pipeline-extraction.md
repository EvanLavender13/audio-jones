# Analysis Pipeline Extraction

Extract analysis modules from AppContext into a dedicated AnalysisPipeline struct.

## Problem

AppContext mixes concerns at different abstraction levels:

```
AppContext (current):
├── PostEffect*           (high-level: rendering)
├── AudioCapture*         (low-level: audio capture)
├── UIState*              (high-level: UI)
├── FFTProcessor*         (low-level: analysis)
├── BeatDetector          (low-level: analysis)
├── BandEnergies          (low-level: analysis)
├── SpectrumBars*         (mid-level: visual processing)
├── audioBuffer[]         (low-level: raw data)
├── peakLevel             (low-level: normalization state)
├── waveform stuff...     (mid-level: visual processing)
```

Analysis modules (FFT, beat, bands) plus their shared state (audioBuffer, peakLevel) form a cohesive unit that should be grouped.

## Target Architecture

```
AppContext (after):
├── AnalysisPipeline*     (owns all analysis)
├── VisualPipeline*       (owns waveform + spectrum visuals)  [future]
├── PostEffect*           (rendering)
├── AudioCapture*         (audio source)
├── UIState*              (UI)
├── PresetPanelState*     (UI)
```

## Phase 1: AnalysisPipeline

### Struct Definition

Create `src/analysis/analysis_pipeline.h`:

```c
typedef struct AnalysisPipeline {
    // Sub-modules
    FFTProcessor fft;
    BeatDetector beat;
    BandEnergies bands;

    // Shared buffers
    float audioBuffer[AUDIO_MAX_FRAMES_PER_UPDATE * AUDIO_CHANNELS];

    // Normalization state
    float peakLevel;

    // Frame state
    uint32_t lastFramesRead;
} AnalysisPipeline;
```

Key decisions:
- FFTProcessor embedded by value (no pointer) since it's always present
- audioBuffer lives here since analysis owns it
- peakLevel tracks normalization across frames

### Interface

```c
void AnalysisPipelineInit(AnalysisPipeline* pipeline);
void AnalysisPipelineUninit(AnalysisPipeline* pipeline);

// Process audio from capture source
// beatSensitivity passed in since it's a UI-controlled parameter
void AnalysisPipelineProcess(AnalysisPipeline* pipeline,
                             AudioCapture* capture,
                             float beatSensitivity,
                             float deltaTime);
```

Three parameters for Process:
1. `capture` - audio source (owned by AppContext, not pipeline)
2. `beatSensitivity` - UI parameter (lives in EffectConfig)
3. `deltaTime` - frame timing

### Migration from main.cpp

| Current Location | New Location |
|------------------|--------------|
| `ctx->fft` (pointer) | `ctx->analysis.fft` (embedded) |
| `ctx->beat` | `ctx->analysis.beat` |
| `ctx->bands` | `ctx->analysis.bands` |
| `ctx->audioBuffer[]` | `ctx->analysis.audioBuffer[]` |
| `ctx->peakLevel` | `ctx->analysis.peakLevel` |
| `ctx->lastFramesRead` | `ctx->analysis.lastFramesRead` |
| `NormalizeAudioBuffer()` | moves to analysis_pipeline.cpp |
| `UpdateAudioAnalysis()` | becomes `AnalysisPipelineProcess()` |

### AppContext Changes

```c
typedef struct AppContext {
    AnalysisPipeline analysis;  // embedded, not pointer
    PostEffect* postEffect;
    AudioCapture* capture;
    UIState* ui;
    PresetPanelState* presetPanel;
    SpectrumBars* spectrumBars;
    AudioConfig audio;
    SpectrumConfig spectrum;
    BandConfig bandConfig;
    float waveform[WAVEFORM_SAMPLES];
    float waveformExtended[MAX_WAVEFORMS][WAVEFORM_EXTENDED];
    WaveformConfig waveforms[MAX_WAVEFORMS];
    int waveformCount;
    int selectedWaveform;
    WaveformMode mode;
    float waveformAccumulator;
    uint64_t globalTick;
} AppContext;
```

### Main Loop Changes

Before:
```c
UpdateAudioAnalysis(ctx, deltaTime);
```

After:
```c
AnalysisPipelineProcess(&ctx->analysis, ctx->capture,
                        ctx->postEffect->effects.beatSensitivity, deltaTime);
```

### UpdateVisuals Changes

Before:
```c
SpectrumBarsProcess(ctx->spectrumBars, ctx->fft->magnitude, FFT_BIN_COUNT, &ctx->spectrum);
ProcessWaveformBase(ctx->audioBuffer + ..., ...);
```

After:
```c
SpectrumBarsProcess(ctx->spectrumBars, ctx->analysis.fft.magnitude, FFT_BIN_COUNT, &ctx->spectrum);
ProcessWaveformBase(ctx->analysis.audioBuffer + ..., ...);
```

### UI/AppConfigs Changes

`AppConfigs` currently references:
- `beat` - pointer to BeatDetector
- `bandEnergies` - pointer to BandEnergies

These become:
- `beat` - `&ctx->analysis.beat`
- `bandEnergies` - `&ctx->analysis.bands`

## File Changes

| File | Change |
|------|--------|
| `src/analysis/analysis_pipeline.h` | Create - struct and function declarations |
| `src/analysis/analysis_pipeline.cpp` | Create - implementation with NormalizeAudioBuffer |
| `src/analysis/fft.h` | Change FFTProcessor to embeddable (remove malloc) |
| `src/analysis/fft.cpp` | Add FFTProcessorInit that takes pointer, keep Uninit |
| `src/main.cpp` | Replace scattered analysis fields with AnalysisPipeline |
| `CMakeLists.txt` | Add analysis_pipeline.cpp to sources |

## Phase 2: FFTProcessor Embedding (prerequisite)

FFTProcessor currently uses malloc internally:
```c
FFTProcessor* FFTProcessorInit(void) {
    FFTProcessor* fft = (FFTProcessor*)malloc(sizeof(FFTProcessor));
    fft->fftConfig = kiss_fftr_alloc(...);
    ...
}
```

Change to init-in-place pattern:
```c
bool FFTProcessorInit(FFTProcessor* fft) {
    fft->fftConfig = kiss_fftr_alloc(...);
    ...
    return true;
}
```

This allows embedding FFTProcessor in AnalysisPipeline without extra allocation.

## Validation

- [ ] Build succeeds with no warnings
- [ ] Beat detection still works (visual pulse on kicks)
- [ ] Band energies display correctly in UI
- [ ] Spectrum bars still animate
- [ ] Waveform still renders
- [ ] main.cpp AppContext is smaller and cleaner
