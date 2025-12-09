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
| `UpdateWaveformAudio` | Drains audio, feeds FFT, processes beat, updates waveforms |
| `RenderWaveforms` | Dispatches to linear or circular drawing per mode |
| `main` | Creates window, runs 60fps loop with 20Hz audio updates |

## Types

### WaveformMode

| Value | Description |
|-------|-------------|
| `WAVEFORM_LINEAR` | Horizontal oscilloscope |
| `WAVEFORM_CIRCULAR` | Circular waveform rings |

### AppContext

| Field | Type | Description |
|-------|------|-------------|
| `postEffect` | `PostEffect*` | GPU effects processor |
| `capture` | `AudioCapture*` | WASAPI loopback |
| `ui` | `UIState*` | UI state |
| `presetPanel` | `PresetPanelState*` | Preset panel state |
| `fft` | `FFTProcessor*` | FFT processor |
| `spectrumBars` | `SpectrumBars*` | Spectrum display |
| `beat` | `BeatDetector` | Beat detection state |
| `audio` | `AudioConfig` | Channel mode |
| `spectrum` | `SpectrumConfig` | Spectrum settings |
| `audioBuffer[6144]` | `float` | Raw samples (3072 frames * 2 ch) |
| `waveform[1024]` | `float` | Base normalized waveform |
| `waveformExtended[8][2048]` | `float` | Per-layer smoothed palindromes |
| `waveforms[8]` | `WaveformConfig` | Per-layer configuration |
| `waveformCount` | `int` | Active layers (1-8) |
| `selectedWaveform` | `int` | UI selection index |
| `mode` | `WaveformMode` | Linear or circular |
| `waveformAccumulator` | `float` | Fixed timestep accumulator |
| `globalTick` | `uint64_t` | Shared rotation counter |

## Main Loop

```
60fps render loop:
├── Handle window resize → PostEffectResize
├── Toggle mode on SPACE
├── Every 50ms (20Hz):
│   ├── Drain audio ring buffer
│   ├── Feed FFT, process beat
│   └── Update all waveform layers
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
5. `FFTProcessorInit` - FFT buffers
6. `SpectrumBarsInit` - Spectrum processor
7. `BeatDetectorInit` - Beat state

## Data Flow

1. **Entry:** Window events, audio from ring buffer
2. **Transform:** Audio → FFT → beat/waveforms → accumulation texture → blur → screen
3. **Exit:** Rendered frame to display
