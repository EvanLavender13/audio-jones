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
        RD[AudioCaptureRead] --> NRM[Normalize + Mirror]
        NRM --> WF[waveform array]
    end

    subgraph Waveform["Waveform Drawing (waveform.c)"]
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
Renders waveform samples as visual output.

| Function | Line | Description |
|----------|------|-------------|
| `CubicInterp` | `waveform.c:5` | Cubic interpolation between four points for smooth curves |
| `HsvToRgb` | `waveform.c:14` | Converts HSV (0-1 range) to raylib Color |
| `DrawWaveformLinear` | `waveform.c:42` | Oscilloscope-style horizontal waveform |
| `DrawWaveformCircularRainbow` | `waveform.c:54` | Circular waveform with 10x interpolation and rainbow hue sweep |

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
| Initialization | `12-41` | Creates window, visualizer, audio capture |
| Main loop | `51-129` | Updates waveform at 30fps, renders at 60fps |
| Cleanup | `131-135` | Stops audio, frees resources |

## Data Flow

1. **Audio Callback** (`audio.c:14`): miniaudio WASAPI loopback triggers callback with system audio samples
2. **Ring Buffer Write** (`audio.c:26`): Callback writes to `ma_pcm_rb` (lock-free, thread-safe)
3. **Ring Buffer Read** (`main.c:64`): Main loop reads samples every 33ms (30fps)
4. **Normalize** (`main.c:78-88`): Scale samples so peak amplitude = 1.0
5. **Mirror** (`main.c:90-99`): Create palindrome buffer for seamless circular loop
6. **Interpolate** (`waveform.c:67-81`): Cubic interpolation generates 10x smooth points
7. **Draw** (`waveform.c:90-100`): Render line segments with rainbow HSV colors
8. **Blur Pass 1** (`visualizer.c:73-80`): Horizontal 5-tap Gaussian blur via shader
9. **Blur Pass 2 + Decay** (`visualizer.c:82-90`): Vertical blur + exponential decay
10. **Composite** (`visualizer.c:92`): New waveform drawn on blurred background
11. **Display** (`main.c:126`): Accumulated texture blitted to screen

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
| Window size | 1920x1080 | `main.c:14` |
| Render FPS | 60 | `main.c:15` |
| Waveform update rate | 30fps | `main.c:48` |
| Base radius | 250px | `main.c:118` |
| Amplitude | 400px | `main.c:119` |
| Trail half-life | 0.5s | `visualizer.c:25` |
| Blur kernel | 5-tap Gaussian | `blur_h.fs`, `blur_v.fs` |
| Rotation speed | 0.01 rad/update | `main.c:103` |
| Hue speed | 0.0025/update | `main.c:104` |

---

*Run `/sync-architecture` to regenerate this document from current code.*
