# Main Module

> Part of [AudioJones](../architecture.md)

## Purpose

Application entry point that orchestrates subsystem lifecycle and runs the main loop.

## Files

- `src/main.cpp` - Entry point, AppContext, main loop

## Function Reference

| Function | Purpose |
|----------|---------|
| `AppContextInit` | Allocates context, initializes subsystems in dependency order |
| `AppContextUninit` | Frees resources in reverse order (NULL-safe) |
| `UpdateVisuals` | Processes spectrum bars and waveforms from analysis results |
| `RenderWaveforms` | Dispatches to linear or circular drawing per mode |
| `main` | Creates window, runs 60fps loop with frame-rate analysis and 20Hz visual updates |

## Types

### WaveformMode

| Value | Description |
|-------|-------------|
| `WAVEFORM_LINEAR` | Horizontal oscilloscope |
| `WAVEFORM_CIRCULAR` | Circular waveform rings |

### AppContext

| Field | Type | Description |
|-------|------|-------------|
| `analysis` | `AnalysisPipeline` | Embedded FFT, beat, bands (see [analysis.md](analysis.md)) |
| `postEffect` | `PostEffect*` | GPU effects processor |
| `capture` | `AudioCapture*` | WASAPI loopback |
| `ui` | `UIState*` | UI state |
| `presetPanel` | `PresetPanelState*` | Preset panel state |
| `spectrumBars` | `SpectrumBars*` | Spectrum display |
| `audio` | `AudioConfig` | Channel mode |
| `spectrum` | `SpectrumConfig` | Spectrum settings |
| `bandConfig` | `BandConfig` | Band sensitivity settings |
| `waveform[1024]` | `float` | Base normalized waveform |
| `waveformExtended[8][2048]` | `float` | Per-layer smoothed palindromes |
| `waveforms[8]` | `WaveformConfig` | Per-layer configuration |
| `waveformCount` | `int` | Active layers (1-8) |
| `selectedWaveform` | `int` | UI selection index |
| `mode` | `WaveformMode` | Linear or circular |
| `waveformAccumulator` | `float` | Fixed timestep accumulator for 20Hz visual updates |
| `globalTick` | `uint64_t` | Shared rotation counter |

## Main Loop

```
60fps render loop:
├── Handle window resize → PostEffectResize
├── Toggle mode on SPACE
├── Every frame:
│   └── AnalysisPipelineProcess (drain audio, FFT, beat, bands)
├── Every 50ms (20Hz):
│   └── UpdateVisuals (spectrum bars, waveforms)
├── PostEffectBeginAccum (blur + decay)
├── RenderWaveforms
├── PostEffectEndAccum
├── BeginDrawing
│   ├── PostEffectToScreen
│   ├── Draw FPS overlay
│   └── Draw UI panels
└── EndDrawing
```

## Initialization Order

1. `PostEffectInit` - Shaders and textures
2. `AudioCaptureInit` + `Start` - Audio device
3. `UIStateInit` - UI state
4. `PresetPanelInit` - Preset file list
5. `AnalysisPipelineInit` - FFT, beat, bands
6. `SpectrumBarsInit` - Spectrum processor

## Data Flow

1. **Entry:** Window events, audio from ring buffer
2. **Transform:** Audio → FFT → beat/waveforms → accumulation texture → blur → screen
3. **Exit:** Rendered frame to display
