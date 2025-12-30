# Render Module
> Part of [AudioJones](../architecture.md)

## Purpose
Draws audio-reactive visuals (waveforms, spectrum bars, shapes) and applies multi-pass post-processing effects (feedback, blur, kaleidoscope, chromatic aberration) to an accumulation buffer.

## Files
- **color_config.h**: Defines ColorConfig struct with solid/rainbow/gradient modes and GradientStop type
- **draw_utils.h/.cpp**: Converts ColorConfig to raylib Color at position t with opacity
- **drawable.h/.cpp**: Orchestrates waveform/spectrum/shape rendering with feedbackPhase-based opacity splitting
- **gradient.h/.cpp**: Evaluates gradient stops to interpolated Color at position t
- **physarum.h/.cpp**: Simulates slime-mold agents via compute shaders; deposits colored trails to trailMap
- **post_effect.h/.cpp**: Loads shaders, allocates HDR render textures, exposes draw stage begin/end
- **render_context.h**: Defines RenderContext struct (screen geometry, accumTexture reference, PostEffect pointer)
- **render_pipeline.h/.cpp**: Chains feedback/blur/voronoi/kaleidoscope/chromatic/FXAA/gamma passes via ping-pong buffers
- **render_utils.h/.cpp**: Creates HDR framebuffers, draws fullscreen quads with Y-flip
- **shape.h/.cpp**: Draws solid or textured polygons with rotation animation
- **spectrum_bars.h/.cpp**: Maps FFT bins to 32 log-spaced bands; draws circular or linear bar visualizations
- **waveform.h/.cpp**: Normalizes audio samples, applies smoothing, draws circular or linear waveforms

## Data Flow
```mermaid
graph TD
    Audio[Audio Buffer] -->|float samples| WP[ProcessWaveformBase]
    WP -->|normalized 1024| PS[ProcessWaveformSmooth]
    PS -->|extended 2048| Draw[DrawWaveformCircular/Linear]

    FFT[FFT Magnitude] -->|float bins| SBP[SpectrumBarsProcess]
    SBP -->|32 bands| SBD[SpectrumBarsDrawCircular/Linear]

    Config[Drawable Config] --> Draw
    Config --> SBD
    Config --> Shape[ShapeDrawSolid/Textured]

    Draw -->|primitives| Accum[(accumTexture)]
    SBD -->|primitives| Accum
    Shape -->|primitives| Accum

    Accum -->|HDR texture| Feedback[RenderPipelineApplyFeedback]
    Feedback -->|voronoi/feedback/blur| Accum

    Accum -->|HDR texture| Output[RenderPipelineApplyOutput]
    Physarum[Physarum trailMap] -->|boost blend| Output
    Output -->|kaleido/chromatic/FXAA/gamma| Screen[Screen]
```

## Internal Architecture

### Drawable System
DrawableState holds per-waveform extended buffers and a SpectrumBars instance. DrawableRenderAll iterates all Drawables, computes opacity from feedbackPhase, and dispatches to waveform/spectrum/shape renderers. DrawableRenderFull bypasses phase-based opacity for physarum trail input.

Validation enforces limits: max 1 spectrum, 8 waveforms, 4 shapes per preset.

### Waveform Rendering
ProcessWaveformBase mixes stereo to mono via ChannelMode, normalizes to peak amplitude, fills WAVEFORM_SAMPLES (1024). ProcessWaveformSmooth creates a palindrome (2048 samples) and applies 3-pass sliding-window blur with peak restoration.

DrawWaveformCircular uses cubic interpolation between samples and applies per-segment gradient coloring from ColorConfig. DrawWaveformLinear scrolls color offset via rotation speed.

### Spectrum Visualization
SpectrumBarsInit precomputes 32 logarithmically-spaced bin ranges (20 Hz to 20 kHz). SpectrumBarsProcess finds peak magnitude per band, converts to dB, normalizes with minDb/maxDb, and applies exponential smoothing.

DrawCircular renders trapezoid quads centered on innerRadius. DrawLinear renders vertical rectangles with color offset animation.

### Shape Rendering
ShapeDrawSolid triangulates an N-sided polygon with per-triangle gradient coloring. ShapeDrawTextured samples accumTexture via shapeTextureShader with zoom/angle/brightness uniforms. Both apply rotation from globalTick.

### Post-Effect Pipeline
PostEffectInit allocates accumTexture plus two ping-pong buffers as HDR (RGBA32F) render textures. Loads 10 fragment shaders (feedback, blur_h, blur_v, chromatic, kaleidoscope, voronoi, physarum_boost, fxaa, gamma, shape_texture).

RenderPipelineApplyFeedback chains: voronoi (optional) -> feedback -> blur_h -> blur_v (with decay). RenderPipelineApplyOutput chains: trail boost (optional) -> kaleidoscope (optional) -> blit to outputTexture -> chromatic -> FXAA -> gamma -> screen.

Ping-pong pattern alternates source/destination each pass to avoid read-after-write hazards.

### Physarum Simulation
PhysarumInit creates an SSBO with agentCount agents (100k default), each with position, heading, hue, and spectrumPos. Two compute shaders run per frame:
1. physarum_agents.glsl: senses trailMap or accumTexture (per accumSenseBlend), turns toward concentration gradient, steps forward, deposits color
2. physarum_trail.glsl: applies separable box-filter diffusion and exponential decay

PhysarumApplyConfig reallocates the SSBO when agentCount changes or reinitializes hues when ColorConfig changes. PhysarumResize recreates trailMap textures.

### Thread Safety
All rendering executes on the main thread. No cross-thread access to render state. Audio data arrives via copied buffers (waveform, fftMagnitude) passed by value each frame.
