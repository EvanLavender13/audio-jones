---
archctl_doc_id: audiothing-existing-impl
title: AudioThing Existing Implementation Analysis
tags: [research, prior-art, audio-visualization]
date: 2025-12-02
---

# AudioThing Existing Implementation Analysis

## Overview

AudioThing is a Windows audio visualizer that captures system audio via WASAPI loopback and renders circular waveforms with persistent trail effects. The application operates in the **time domain**—it displays raw audio samples as polar coordinates, not frequency spectra. Despite FFTW3 appearing in dependencies, no FFT processing occurs.

The codebase totals ~2,078 LOC across 36 files. Architecture separates concerns cleanly: audio capture thread, visualization logic, GPU rendering pipeline, and ImGui-based UI.

## Key Findings

### Audio Capture

Windows WASAPI loopback captures system audio output (speakers), not microphone input.

**Implementation details:**
- Uses `eRender` endpoint with `AUDCLNT_STREAMFLAGS_LOOPBACK` flag
- Captures float32 samples at system sample rate (typically 48kHz)
- Background thread polls every 10ms, writes to shared buffer
- Main thread reads via mutex + condition_variable handoff
- Buffer size: 1024 samples

**Threading model:**
```
Capture Thread                    Main Thread
     │                                 │
     ├─ GetBuffer()                    │
     ├─ Copy to shared buffer          │
     ├─ notify_one() ─────────────────►│
     │                                 ├─ wait() returns
     │                                 ├─ Swap buffers
     │                                 └─ Process audio
```

### Audio Processing

No frequency analysis. Processing consists of:

1. **Normalization**: Scale samples to [-1, 1] based on max absolute value
2. **Smoothing**: Sliding window average with O(N) complexity
   - Window size = 2 * smoothness + 1
   - Configurable per waveform (1-50)

```cpp
// Sliding window: remove left sample, add right sample each iteration
windowSum -= audioData[i - smoothness - 1]
windowSum += audioData[i + smoothness]
smoothedData[i] = windowSum / count
```

### Visualization Geometry

Circular waveform display using polar coordinates.

**Data pipeline:**
1. Raw audio (1024 samples)
2. Mirror buffer to 2048 samples (seamless circle wraparound)
3. Apply smoothing filter
4. Generate 10,240 points via cubic interpolation (10x oversampling)

**Point generation:**
```cpp
for each point i in 0..10240:
    angle = (i / 10240) * 2π + rotationAngle
    sampleValue = cubicInterpolate(buffer samples)
    radius = baseRadius + sampleValue * displayHeight
    x = centerX + radius * cos(angle)
    y = centerY + radius * sin(angle)
    color = hsvToRgb(hue + hueOffset + i/10240, 1.0, 0.7)
```

**Dual waveform rendering:**
- Thin line: original samples, configurable alpha
- Thick line: smoothed samples, drawn with multiple parallel strokes

### Shader Effects

Single GLSL fragment shader (`fade_blur.frag`) creates trail persistence.

**Effect chain:**
1. **Gaussian blur**: 5×5 kernel on previous frame
2. **Pixelation**: Anti-aliased block sampling (8×8 jittered samples)
3. **Blend**: Mix blur and pixelation by configurable factor
4. **Fade**: Exponential decay `color *= pow(fadeFactor, 1.5)` where fadeFactor ≈ 0.99
5. **Saturation boost**: Preserve color vividness during fade (HSV manipulation)
6. **Dithering**: Random noise to prevent color banding
7. **Threshold**: Black out pixels below luminance threshold

**Trail mechanism:**
- RenderTexture accumulates frames
- Each frame: apply fade shader to texture, draw new waveforms on top
- Creates persistent "echo" effect

### Rendering Pipeline

Dual framerate architecture:
- **60 Hz**: Rendering and display
- **30 Hz**: Audio buffer updates and waveform geometry

```cpp
// Main loop
while (running) {
    processEvents()

    if (waveformUpdateAccumulator >= 1/30) {
        // 30 Hz: Update audio and geometry
        processAudioData()
        visualizer.update(audioBuffer, deltaTime)
        waveformUpdateAccumulator = 0
    }

    // 60 Hz: Always render
    visualizer.render(window)
    uiManager.render()
    window.display()
}
```

### UI System

ImGui with collapsible sections:

| Section | Controls |
|---------|----------|
| Performance | FPS, frame time |
| Shader Effects | fadeFactor, pixelSize, blendFactor, fadeThreshold, saturationBoost, ditherStrength |
| Waveforms List | Add/remove waveforms, selection |
| Waveform Settings | enabled, height, smoothness, rotationSpeed, radius, thickness, hueOffset, alpha, thickAlpha |
| Preset Manager | Save/load JSON presets |

### Configuration

Three config objects:

**VisualizerConfig** (global):
- smoothness, rotationSpeed, waveformHeight
- radiusFactor, thickness, hueOffset
- hue (animated), hueRotationSpeed

**WaveformConfig** (per-waveform):
- displayHeight, smoothness, rotationSpeed
- radiusFactor, thickness, hueOffset
- alpha, thickAlpha, enabled

**ShaderConfig** (GPU effects):
- fadeFactor, pixelSize, blendFactor
- fadeThreshold, saturationBoost, ditherStrength

**Preset format (JSON):**
```json
{
  "name": "PresetName",
  "visualizerConfig": { ... },
  "shaderConfig": { ... },
  "waveforms": [
    { "displayHeight": 10, "smoothness": 50, ... }
  ]
}
```

## Technical Details

### Dependencies

| Library | Purpose |
|---------|---------|
| SFML | Window management, 2D rendering, OpenGL context |
| ImGui-SFML | Immediate mode GUI |
| Windows WASAPI | System audio capture (Windows-only) |
| FFTW3 | Listed but unused |

### Class Structure

```
AudioThing (main)
├── AudioCaptureRAII → AudioCapture (WASAPI thread)
├── ImGuiRAII → ImGui context
├── AudioVisualizer
│   ├── sf::RenderTexture (accumulation buffer)
│   ├── sf::Shader (fade_blur.frag)
│   └── vector<Waveform*>
│       └── Waveform
│           ├── WaveformConfig
│           └── sf::VertexArray (geometry)
├── UIManager
│   └── ConfigSerializer
└── Config objects (VisualizerConfig, ShaderConfig)
```

### Performance Optimizations

1. **Pre-computed trig**: Sin/cos values cached for circle points
2. **Static allocations**: WaveformDrawer reuses vectors
3. **Sliding window**: O(N) smoothing, not O(N*W)
4. **10x oversampling with interpolation**: Smooth curves without excessive buffer size
5. **Dual framerate**: Reduces GPU pressure by updating geometry at half render rate

### File Manifest

| Component | Files | LOC |
|-----------|-------|-----|
| Audio Capture | AudioCapture.h/cpp, AudioCaptureRAII.h | 280 |
| Audio Processing | AudioUtils.h/cpp, AudioFilter.h, Pipeline.h/cpp | 157 |
| Visualization | AudioVisualizer.h/cpp, Waveform.h/cpp, WaveformDrawer.h | 471 |
| Shader | fade_blur.frag | 118 |
| UI | UIManager.h/cpp, ImGuiRAII.h | 355 |
| Configuration | *Config.h/cpp, ConfigSerializer.h/cpp | 364 |
| Main | AudioThing.cpp | 177 |

## Decisions

### Time-domain vs Frequency-domain

The implementation visualizes raw waveform samples, not frequency spectra. This creates an oscilloscope-like display rather than spectrum analyzer bars. The circular polar projection and trail effects compensate for the simpler data representation.

### Windows-only Audio

WASAPI loopback is Windows-specific. Cross-platform would require:
- PulseAudio monitor on Linux
- CoreAudio tap on macOS
- Or: process audio file instead of system capture

### SFML vs Lower-level

SFML provides convenience (windowing, basic 2D, OpenGL context) but limits shader complexity. The single fragment shader handles all effects in one pass.

## Open Questions

1. **Should FFT be added?** Original dependency suggests intent, but time-domain works for this aesthetic
2. **Cross-platform audio?** Would require platform abstraction layer
3. **Multiple visualization modes?** Current code supports only circular waveform
4. **Beat detection?** Not implemented, could drive visual intensity

## References

- Source repository: https://github.com/EvanLavender13/AudioThing
- WASAPI documentation: https://learn.microsoft.com/en-us/windows/win32/coreaudio/wasapi
- SFML: https://www.sfml-dev.org/
- ImGui: https://github.com/ocornut/imgui
