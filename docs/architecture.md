# AudioJones Architecture

> Last sync: 2026-01-03 | Commit: 0ae693a

## Overview

NEEDS UPDATE!

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
    end

    subgraph Automation[automation/]
        LFO[lfo]
        Bands -->|bass/mid/treb| ModSources[mod_sources]
        Beat -->|intensity 0-1| ModSources
        LFO -->|output -1 to 1| ModSources
        ModSources -->|8 normalized sources| ModEngine[modulation_engine]
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
        AttractorFlow[attractor_flow]
    end

    RP -->|accumTexture| Physarum
    RP -->|accumTexture| CurlFlow
    RP -->|accumTexture| AttractorFlow
    Physarum -->|trail texture| RP
    CurlFlow -->|trail texture| RP
    AttractorFlow -->|trail texture| RP

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
    Panels -->|config values| AttractorFlow
```

**Legend:** Arrows show data flow with payload type. `[(name)]` = buffer. `[name]` = module.

## Module Index

| Module | Purpose | Documentation |
|--------|---------|---------------|
| audio | Captures system audio via WASAPI loopback into a ring buffer for downstream analysis | [audio.md](modules/audio.md) |
| analysis | Transforms raw audio samples into frequency spectrum, beat detection events, and band energy levels for visualization | [analysis.md](modules/analysis.md) |
| automation | Routes audio-reactive and LFO signals to visual parameters via configurable modulation routes with curve shaping | [automation.md](modules/automation.md) |
| render | Draws audio-reactive visuals (waveforms, spectrum bars, shapes) and applies multi-pass post-processing effects to an accumulation buffer | [render.md](modules/render.md) |
| config | Defines configuration structures for all visual and audio parameters, with JSON serialization for preset save/load | [config.md](modules/config.md) |
| ui | Renders ImGui panels for visualization configuration, audio settings, presets, and performance monitoring with custom gradient, modulation, and analysis widgets | [ui.md](modules/ui.md) |
| simulation | GPU-accelerated agent simulations (Physarum, Curl Flow, Attractor Flow) that deposit colored trails influenced by audio analysis | [simulation.md](modules/simulation.md) |
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
├── simulation/           GPU agent simulations (Physarum, Curl Flow, Attractor Flow)
├── config/               Serializable parameters
└── ui/                   Dear ImGui panels
```

---

*Run `/sync-architecture` to regenerate this document from current code.*
