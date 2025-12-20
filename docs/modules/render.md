# Render Module

> Part of [AudioJones](../architecture.md)

## Purpose

Renders waveforms and spectrum bars with GPU post-processing (blur trails, bloom, chromatic aberration, physarum simulation).

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
- `src/render/post_effect.cpp` - Shader-based blur, chromatic aberration, and physarum integration
- `src/render/physarum.h` - Physarum simulation API and config struct
- `src/render/physarum.cpp` - GPU compute shader physarum agent simulation

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
| `PostEffectInit` | Loads shaders, creates HDR ping-pong textures (RGBA32F), initializes physarum |
| `PostEffectUninit` | Frees shaders, textures, and physarum |
| `PostEffectResize` | Recreates textures at new dimensions, resizes physarum |
| `PostEffectBeginAccum` | Runs physarum → voronoi → feedback → blur, begins drawing |
| `PostEffectEndAccum` | Ends texture mode, applies kaleidoscope |
| `PostEffectToScreen` | Applies chromatic aberration, blits to screen |

### Physarum

| Function | Purpose |
|----------|---------|
| `PhysarumSupported` | Returns true if compute shaders supported (OpenGL 4.3+) |
| `PhysarumInit` | Loads compute shader, allocates agent SSBO with random positions |
| `PhysarumUninit` | Frees shader program and SSBO |
| `PhysarumUpdate` | Dispatches compute shader to sense, turn, move, and deposit |
| `PhysarumProcessTrails` | Applies diffusion and exponential decay to trail texture |
| `PhysarumDrawDebug` | Draws grayscale trail map overlay for debugging |
| `PhysarumResize` | Updates dimensions, reinitializes agents |
| `PhysarumReset` | Reinitializes agents to random positions |
| `PhysarumApplyConfig` | Handles agent count changes (buffer realloc) and color changes (hue redistribution) |

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
| `accumTexture`, `tempTexture` | HDR ping-pong render textures (RGBA32F) |
| `feedbackShader` | UV zoom/rotation/desaturate transform for recursive effect |
| `blurHShader`, `blurVShader` | 5-tap Gaussian blur shaders |
| `chromaticShader` | Radial RGB split shader |
| `kaleidoShader` | Mirror segments around center |
| `voronoiShader` | Animated cell overlay |
| `feedbackZoomLoc`, `feedbackRotationLoc`, `feedbackDesaturateLoc` | Feedback shader uniform locations |
| `kaleidoSegmentsLoc`, `kaleidoRotationLoc` | Kaleidoscope shader uniform locations |
| `voronoiResolutionLoc`, `voronoiScaleLoc`, `voronoiIntensityLoc`, `voronoiTimeLoc`, `voronoiSpeedLoc`, `voronoiEdgeWidthLoc` | Voronoi shader uniform locations |
| `warpStrengthLoc`, `warpScaleLoc`, `warpOctavesLoc`, `warpLacunarityLoc`, `warpTimeLoc` | Domain warping shader uniform locations |
| `effects` | EffectConfig parameters |
| `warpTime` | Time accumulator for domain warping animation |
| `rotationLFOState` | LFO state for animated rotation offset |
| `voronoiTime` | Time accumulator for voronoi animation |
| `physarum` | Physarum simulation pointer (NULL if compute shaders unsupported) |

### Physarum

| Field | Type | Description |
|-------|------|-------------|
| `agentBuffer` | `unsigned int` | SSBO containing agent data |
| `computeProgram` | `unsigned int` | OpenGL compute shader program |
| `agentCount` | `int` | Number of simulated agents |
| `width`, `height` | `int` | Simulation dimensions |
| `time` | `float` | Accumulated time for randomness |
| `config` | `PhysarumConfig` | Cached configuration |
| `supported` | `bool` | True if compute shaders available |

### PhysarumAgent

| Field | Type | Description |
|-------|------|-------------|
| `x`, `y` | `float` | Agent position |
| `heading` | `float` | Movement direction (radians) |
| `hue` | `float` | Species identity for coloring (0-1) |

### PhysarumConfig

| Field | Default | Description |
|-------|---------|-------------|
| `enabled` | false | Enable physarum simulation |
| `agentCount` | 100000 | Number of simulated agents |
| `sensorDistance` | 20.0 | Distance to sense ahead |
| `sensorAngle` | 0.5 | Angle between sensors (radians) |
| `turningAngle` | 0.3 | Max turn per step (radians) |
| `stepSize` | 1.5 | Movement distance per frame |
| `depositAmount` | 0.05 | Trail intensity deposited |
| `decayHalfLife` | 0.5 | Trail decay half-life in seconds (0.1-5.0) |
| `diffusionScale` | 1 | Blur kernel scale for trail diffusion (0-4) |
| `boostIntensity` | 0.0 | Trail brightness boost multiplier (0.0-2.0) |
| `accumSenseBlend` | 0.0 | Blend between trail (0) and accum (1) texture sensing (0.0-1.0) |
| `debugOverlay` | false | Enable debug visualization overlay |
| `color` | - | ColorConfig for agent coloring |

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
| `shaders/feedback.fs` | UV zoom + rotation + desaturation for recursive tunnel effect |
| `shaders/blur_h.fs` | Horizontal 5-tap Gaussian |
| `shaders/blur_v.fs` | Vertical 5-tap Gaussian + exponential decay |
| `shaders/chromatic.fs` | Radial chromatic aberration |
| `shaders/kaleidoscope.fs` | Mirror around center with configurable segment count |
| `shaders/voronoi.fs` | Animated voronoi cell overlay with edge detection |
| `shaders/physarum_agents.glsl` | Compute shader: sense, turn, move, and deposit agents |

## Data Flow

1. **Entry:** Waveform samples from audio, magnitude from FFT, beat intensity from beat detector
2. **Transform:** Normalize → smooth → palindrome → cubic interpolation → line segments
3. **Exit:** HDR texture → physarum compute → voronoi → feedback (zoom/rotate) → blur passes → chromatic aberration → screen
