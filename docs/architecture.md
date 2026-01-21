# AudioJones Architecture

> Last sync: 2026-01-21 | Commit: 106f9f0

## Overview

Modular audio visualizer with a six-stage render pipeline. WASAPI loopback feeds an analysis stage producing FFT spectrum, beat intensity, band energy, spectral features, and waveform history. These drive a modulation engine that routes audio-reactive signals and LFOs to any registered parameter. Drawables (waveforms, spectrum bars, shapes) render to an HDR accumulation buffer. Six GPU agent simulations (Physarum, Curl Flow, Curl Advection, Attractor Flow, Boids, Cymatics) read and write to this buffer. A reorderable chain of 40 shader transforms processes the output before final color correction.

## System Diagram

```mermaid
flowchart LR
    subgraph Audio[audio/]
        WASAPI[WASAPI Loopback] -->|f32 stereo 48kHz| RB[(Ring Buffer)]
    end

    subgraph Analysis[analysis/]
        RB -->|f32 x 6144| AP[analysis_pipeline]
        AP -->|normalize + FFT| FFT[fft]
        FFT -->|f32 x 1025 magnitude| Beat[beat]
        FFT -->|f32 x 1025 magnitude| Bands[bands]
        FFT -->|f32 x 1025 magnitude| Features[audio_features]
        AP -->|waveform history| WaveHist[waveformTexture]
    end

    subgraph Automation[automation/]
        LFO[lfo]
        Bands -->|bass/mid/treb/centroid| ModSources[mod_sources]
        Beat -->|intensity 0-1| ModSources
        Features -->|flatness/spread/rolloff/flux/crest| ModSources
        LFO -->|output -1 to 1| ModSources
        ModSources -->|14 normalized sources| ModEngine[modulation_engine]
    end

    subgraph Render[render/]
        AP -->|f32 stereo samples| Drawables[drawable]
        FFT -->|f32 x 1025| Drawables
        Drawables -->|geometry| RP[render_pipeline]
        RP -->|final frame| Screen[Display]
    end

    subgraph Simulation[simulation/]
        Physarum[physarum]
        CurlFlow[curl_flow]
        CurlAdvection[curl_advection]
        AttractorFlow[attractor_flow]
        Boids[boids]
        Cymatics[cymatics]
    end

    RP -->|accumTexture| Physarum
    RP -->|accumTexture| CurlFlow
    RP -->|accumTexture| CurlAdvection
    RP -->|accumTexture| AttractorFlow
    RP -->|accumTexture| Boids
    WaveHist -->|waveform ring buffer| Cymatics
    Physarum -->|trail texture| RP
    CurlFlow -->|trail texture| RP
    CurlAdvection -->|trail texture| RP
    AttractorFlow -->|trail texture| RP
    Boids -->|trail texture| RP
    Cymatics -->|trail texture| RP

    subgraph UI[ui/]
        Panels[panels]
        Beat -->|intensity 0-1| Panels
        Bands -->|bass/mid/treb| Panels
    end

    ModEngine -->|parameter offsets| Drawables
    ModEngine -->|parameter offsets| RP
    Panels -->|config values| Drawables
    Panels -->|config values| RP
    Panels -->|config values| Physarum
    Panels -->|config values| CurlFlow
    Panels -->|config values| CurlAdvection
    Panels -->|config values| AttractorFlow
    Panels -->|config values| Boids
    Panels -->|config values| Cymatics
```

**Legend:** Arrows show data flow with payload type. `[(name)]` = buffer. `[name]` = module.

## Module Index

| Module | Purpose | Documentation |
|--------|---------|---------------|
| audio | Captures system audio via WASAPI loopback into a ring buffer for downstream analysis | [audio.md](modules/audio.md) |
| analysis | Transforms raw audio samples into frequency spectrum, beat detection events, band energy levels, spectral features, and waveform history for visualization and modulation | [analysis.md](modules/analysis.md) |
| automation | Routes audio-reactive and LFO signals to visual parameters via configurable modulation routes with curve shaping | [automation.md](modules/automation.md) |
| render | Draws audio-reactive visuals (waveforms, spectrum bars, shapes) and applies multi-pass post-processing effects to an accumulation buffer | [render.md](modules/render.md) |
| config | Defines configuration structures for all visual and audio parameters, with JSON serialization for preset save/load | [config.md](modules/config.md) |
| ui | Renders ImGui panels for visualization configuration, audio settings, presets, and performance monitoring with custom gradient, modulation, and analysis widgets | [ui.md](modules/ui.md) |
| simulation | GPU-accelerated agent simulations (Physarum, Curl Flow, Curl Advection, Attractor Flow, Boids, Cymatics) that deposit colored trails influenced by audio analysis | [simulation.md](modules/simulation.md) |
| main | Initializes subsystems, runs 60 FPS main loop, orchestrates audio analysis, modulation updates, and six-stage render pipeline | [main.md](modules/main.md) |

## Thread Model

```
┌─────────────────────────────────┐
│ Audio Thread (miniaudio)        │
│ - audio_data_callback           │
│ - Writes to ma_pcm_rb           │
└──────────────┬──────────────────┘
               │ lock-free ring buffer
               ▼
┌─────────────────────────────────┐
│ Main Thread (raylib)            │
│ - Drains audio @ 60fps          │
│ - FFT + beat detection @ 60fps  │
│ - Visual updates @ 20Hz         │
│ - Renders @ 60fps               │
│ - Handles UI input              │
└─────────────────────────────────┘
```

## Directory Structure

```
src/
├── main.cpp              Entry point, AppContext
├── audio/                WASAPI capture
├── analysis/             FFT, beat detection
├── automation/           LFO oscillators
├── render/               Waveform, spectrum bars, post-effects
├── simulation/           GPU agent simulations (Physarum, Curl Flow, Curl Advection, Attractor Flow, Boids, Cymatics)
├── config/               Serializable parameters
└── ui/                   Dear ImGui panels
```

---

*Run `/sync-architecture` to regenerate this document from current code.*
