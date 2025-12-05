# AudioJones Architecture

> Auto-generated via `/sync-architecture`. Last sync: 2025-12-04

## Overview

Real-time circular waveform audio visualizer. Captures system audio via WASAPI loopback, renders reactive circular waveforms with rainbow colors and physarum-inspired trail effects using separable Gaussian blur.

## System Diagram

```mermaid
flowchart TD
    subgraph Audio["Audio Capture (audio.c)"]
        CB[audio_data_callback] --> RB[ma_pcm_rb<br/>ring buffer]
    end

    subgraph Main["Main Loop (main.c)"]
        RD[AudioCaptureRead] --> PW[ProcessWaveform]
        PW --> WF[waveform arrays]
    end

    subgraph Waveform["Waveform Processing (waveform.c)"]
        PW --> NRM[Normalize + Mirror]
        WF --> CI[CubicInterp]
        CI --> DWC[DrawWaveformCircularRainbow]
        CI --> DWL[DrawWaveformLinear]
    end

    subgraph Visualizer["Render Pipeline (visualizer.c)"]
        AT[accumTexture] --> BH[blur_h.fs]
        BH --> TT[tempTexture]
        TT --> BV[blur_v.fs + decay]
        BV --> AT
        DWC --> AT
        DWL --> AT
        AT --> SC[Screen]
    end

    RB --> RD
```

## Modules

### audio.c / audio.h
Captures system audio via miniaudio WASAPI loopback.

| Function | Line | Description |
|----------|------|-------------|
| `audio_data_callback` | `audio.c:14` | Writes incoming samples to ring buffer |
| `AudioCaptureInit` | `audio.c:32` | Creates loopback device and ring buffer |
| `AudioCaptureRead` | `audio.c:125` | Reads samples from ring buffer (main thread) |

**Key constants:**
- `AUDIO_SAMPLE_RATE`: 48000 Hz
- `AUDIO_CHANNELS`: 2 (stereo)
- `AUDIO_RING_BUFFER_FRAMES`: 4096 frames

### waveform.c / waveform.h
Processes raw audio into display-ready waveforms and renders them.

| Function | Line | Description |
|----------|------|-------------|
| `ProcessWaveform` | `waveform.c:4` | Normalizes samples to peak=1.0, creates palindrome mirror for seamless circular display |
| `CubicInterp` | `waveform.c:41` | Cubic interpolation between four points for smooth curves |
| `HsvToRgb` | `waveform.c:50` | Converts HSV (0-1 range) to raylib Color |
| `DrawWaveformLinear` | `waveform.c:78` | Oscilloscope-style horizontal waveform |
| `DrawWaveformCircularRainbow` | `waveform.c:90` | Circular waveform with 10x interpolation and rainbow hue sweep |

**Key constants:**
- `WAVEFORM_SAMPLES`: 1024
- `WAVEFORM_EXTENDED`: 2048 (palindrome for seamless loop)
- `INTERPOLATION_MULT`: 10 (smoothness factor)

### visualizer.c / visualizer.h
Manages accumulation buffer and separable blur shaders for physarum-style trail diffusion.

| Function | Line | Description |
|----------|------|-------------|
| `VisualizerInit` | `visualizer.c:18` | Loads blur shaders, creates ping-pong RenderTextures |
| `VisualizerBeginAccum` | `visualizer.c:69` | Two-pass blur: horizontal then vertical + decay |
| `VisualizerEndAccum` | `visualizer.c:95` | Ends texture mode |
| `VisualizerToScreen` | `visualizer.c:100` | Blits accumulation texture to screen |

**Trail settings:** halfLife = 0.5s (exponential decay)

### main.c
Application entry point and main loop.

| Section | Lines | Description |
|---------|-------|-------------|
| Initialization | `15-44` | Creates window, visualizer, audio capture |
| Main loop | `55-100` | Updates waveform at 20fps, renders at 60fps |
| Cleanup | `102-106` | Stops audio, frees resources |

## Data Flow

1. **Audio Callback** (`audio.c:14`): miniaudio WASAPI loopback triggers callback with system audio samples
2. **Ring Buffer Write** (`audio.c:26`): Callback writes to `ma_pcm_rb` (lock-free, thread-safe)
3. **Ring Buffer Read** (`main.c:67`): Main loop reads samples every 50ms (20fps)
4. **Process Waveform** (`waveform.c:4`): Normalize samples (peak=1.0), create palindrome mirror, smooth joins
5. **Interpolate** (`waveform.c:102-117`): Cubic interpolation generates 10x smooth points
6. **Draw** (`waveform.c:127-137`): Render line segments with rainbow HSV colors
7. **Blur Pass 1** (`visualizer.c:73-80`): Horizontal 5-tap Gaussian blur via shader
8. **Blur Pass 2 + Decay** (`visualizer.c:82-90`): Vertical blur + exponential decay
9. **Composite** (`visualizer.c:92`): New waveform drawn on blurred background
10. **Display** (`main.c:93`): Accumulated texture blitted to screen

## Shaders

Separable Gaussian blur with physarum-style diffusion:

| Shader | Purpose |
|--------|---------|
| `shaders/blur_h.fs` | Horizontal 5-tap Gaussian `[1,4,6,4,1]/16` |
| `shaders/blur_v.fs` | Vertical 5-tap Gaussian + exponential decay |

**Ping-pong pipeline:**
```
accumTexture --[blur_h]--> tempTexture --[blur_v + decay]--> accumTexture
```

## Thread Model

```
┌─────────────────────────────────┐
│ Audio Thread (miniaudio)        │
│ - audio_data_callback           │
│ - Writes to ring buffer         │
└──────────────┬──────────────────┘
               │ ma_pcm_rb (lock-free)
               ▼
┌─────────────────────────────────┐
│ Main Thread (raylib)            │
│ - Reads from ring buffer        │
│ - Updates waveform @ 30fps      │
│ - Renders @ 60fps               │
└─────────────────────────────────┘
```

## Configuration

| Parameter | Value | Location |
|-----------|-------|----------|
| Window size | 1920x1080 | `main.c:17` |
| Render FPS | 60 | `main.c:18` |
| Waveform update rate | 20fps | `main.c:52` |
| Base radius | 250px | `main.c:85` |
| Amplitude | 400px (default) | `main.c:49` |
| Trail half-life | 0.5s | `visualizer.c:25` |
| Blur kernel | 5-tap Gaussian | `blur_h.fs`, `blur_v.fs` |
| Rotation speed | 0.01 rad/update | `main.c:71` |
| Hue speed | 0.0025/update | `main.c:72` |

---

*Run `/sync-architecture` to regenerate this document from current code.*
