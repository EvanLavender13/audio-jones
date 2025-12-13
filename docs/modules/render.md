# Render Module

> Part of [AudioJones](../architecture.md)

## Purpose

Renders waveforms and spectrum bars with GPU post-processing (blur trails, bloom, chromatic aberration).

## Files

- `src/render/render_context.h` - Shared screen geometry struct
- `src/render/color_config.h` - ColorMode enum and ColorConfig struct
- `src/render/waveform_pipeline.h` - Waveform pipeline API and struct
- `src/render/waveform_pipeline.cpp` - Coordinates waveform processing and drawing
- `src/render/waveform.h` - Waveform processing and drawing API
- `src/render/waveform.cpp` - Linear/circular waveform rendering
- `src/render/spectrum_bars.h` - Spectrum visualization API
- `src/render/spectrum_bars.cpp` - 32-band bar rendering
- `src/render/post_effect.h` - Post-processing API and struct
- `src/render/post_effect.cpp` - Shader-based blur and chromatic aberration

## Function Reference

### Waveform Pipeline

| Function | Purpose |
|----------|---------|
| `WaveformPipelineInit` | Initializes pipeline state (zeroes buffers) |
| `WaveformPipelineUninit` | Cleans up pipeline (no-op, no allocations) |
| `WaveformPipelineProcess` | Processes audio into per-layer smoothed waveforms |
| `WaveformPipelineDraw` | Draws all waveforms (dispatches to linear or circular) |

### Waveform

| Function | Purpose |
|----------|---------|
| `ProcessWaveformBase` | Mixes stereo to mono per ChannelMode, normalizes to peak=1.0 |
| `ProcessWaveformSmooth` | Creates palindrome, applies sliding window smoothing |
| `DrawWaveformLinear` | Renders horizontal oscilloscope |
| `DrawWaveformCircular` | Renders circular waveform with cubic interpolation |

### Spectrum Bars

| Function | Purpose |
|----------|---------|
| `SpectrumBarsInit` | Allocates processor |
| `SpectrumBarsUninit` | Frees processor |
| `SpectrumBarsProcess` | Maps 1025 FFT bins to 32 display bands |
| `SpectrumBarsDrawCircular` | Renders circular spectrum bars |
| `SpectrumBarsDrawLinear` | Renders linear spectrum bars |

### Post Effect

| Function | Purpose |
|----------|---------|
| `PostEffectInit` | Loads shaders, creates ping-pong textures |
| `PostEffectUninit` | Frees shaders and textures |
| `PostEffectResize` | Recreates textures at new dimensions |
| `PostEffectBeginAccum` | Runs feedback → H blur → V blur + decay, begins drawing |
| `PostEffectEndAccum` | Ends texture mode |
| `PostEffectToScreen` | Applies chromatic aberration, blits to screen |

## Types

### WaveformPipeline

| Field | Type | Description |
|-------|------|-------------|
| `waveform` | `float[1024]` | Base normalized waveform |
| `waveformExtended` | `float[8][2048]` | Per-layer smoothed palindromes |
| `globalTick` | `uint64_t` | Shared rotation counter for synchronized animation |

### RenderContext

| Field | Type | Description |
|-------|------|-------------|
| `screenW`, `screenH` | `int` | Screen dimensions |
| `centerX`, `centerY` | `int` | Screen center |
| `minDim` | `float` | min(screenW, screenH) for scaling |

### ColorConfig

| Field | Default | Description |
|-------|---------|-------------|
| `mode` | `COLOR_MODE_SOLID` | Solid or rainbow |
| `solid` | `WHITE` | Solid color |
| `rainbowHue` | 0.0 | Starting hue (0-360) |
| `rainbowRange` | 360.0 | Hue span (0-360) |
| `rainbowSat`, `rainbowVal` | 1.0 | Saturation and brightness |

### PostEffect

| Field | Description |
|-------|-------------|
| `accumTexture`, `tempTexture` | Ping-pong render textures |
| `feedbackShader` | UV zoom/rotation transform for recursive effect |
| `blurHShader`, `blurVShader` | 5-tap Gaussian blur shaders |
| `chromaticShader` | Radial RGB split shader |
| `effects` | EffectConfig parameters |

## Constants

| Constant | Value | Purpose |
|----------|-------|---------|
| `WAVEFORM_SAMPLES` | 1024 | Base waveform resolution |
| `WAVEFORM_EXTENDED` | 2048 | Palindrome for circular wrap |
| `MAX_WAVEFORMS` | 8 | Concurrent waveform layers |
| `SPECTRUM_BAND_COUNT` | 32 | Frequency bands displayed |

## Shaders

| Shader | Purpose |
|--------|---------|
| `shaders/feedback.fs` | UV zoom (0.98) + rotation (0.005 rad) for recursive tunnel effect |
| `shaders/blur_h.fs` | Horizontal 5-tap Gaussian |
| `shaders/blur_v.fs` | Vertical 5-tap Gaussian + exponential decay |
| `shaders/chromatic.fs` | Radial chromatic aberration |

## Data Flow

1. **Entry:** Waveform samples from audio, magnitude from FFT, beat intensity from beat detector
2. **Transform:** Normalize → smooth → palindrome → cubic interpolation → line segments
3. **Exit:** Accumulated texture → feedback (zoom/rotate) → blur passes → chromatic aberration → screen
