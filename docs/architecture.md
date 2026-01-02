# AudioJones Architecture

> Last sync: 2026-01-01 | Commit: a9c5a4f

## Overview

Real-time audio visualizer that captures system audio via WASAPI loopback and renders circular or linear waveforms with GPU compute shader physarum simulation and fractal feedback (recursive zoom/rotation). Features 2048-point FFT spectral flux beat detection driving bloom pulse and chromatic aberration. Supports kaleidoscope mirroring and animated voronoi cell overlay. Allows up to 8 concurrent waveforms with per-waveform configuration, stereo channel mixing modes, 32-band spectrum bars, 3-band energy meters, and JSON preset save/load.

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
        ModSources[mod_sources]
        ModEngine[modulation_engine]
    end

    subgraph Render[render/]
        AP -->|f32 stereo| WP[waveform_pipeline]
        WP -->|smoothed samples| WF[waveform]
        Config -->|gradient stops| Grad[gradient]
        Grad -->|interpolated color| WF
        Beat -->|intensity 0-1| PE[post_effect]
        LFO -->|output -1 to 1| ModSources
        Bands -->|bass/mid/treb| ModSources
        Beat -->|intensity 0-1| ModSources
        ModSources -->|8 normalized sources| ModEngine
        ModEngine -->|parameter offsets| Config
        FFT -->|f32 x 1025| SB[spectrum_bars]
        WF -->|line segments| PE
        SB -->|bar geometry| PE
        PE -->|final frame| Screen[Display]
    end

    subgraph Simulation[simulation/]
        PE -->|HDR texture| Physarum[physarum]
        PE -->|HDR texture| CurlFlow[curl_flow]
        PE -->|HDR texture| AttractorFlow[attractor_flow]
        Physarum -->|trail texture| BlendComp[BlendCompositor]
        CurlFlow -->|trail texture| BlendComp
        AttractorFlow -->|trail texture| BlendComp
        BlendComp -->|composited trails| PE
        Config -->|parameters| Physarum
        Config -->|parameters| CurlFlow
        Config -->|parameters| AttractorFlow
    end

    subgraph UI[ui/]
        Panels[panels] -->|config values| Config
        Bands -->|bass/mid/treb| Panels
    end

    subgraph Config[config/]
        Config -->|parameters| WF
        Config -->|parameters| PE
        Config -->|parameters| SB
        Config -->|parameters| LFO
        Config -->|parameters| Physarum
    end
```

**Legend:** Arrows show data flow with payload type. `[(name)]` = buffer. `[name]` = module.

## Module Index

| Module | Purpose | Documentation |
|--------|---------|---------------|
| audio | Captures system audio via WASAPI loopback into a ring buffer for downstream analysis | [audio.md](modules/audio.md) |
| analysis | Transforms raw audio samples into frequency spectrum, beat detection events, and band energy levels for visualization | [analysis.md](modules/analysis.md) |
| automation | Routes audio-reactive and LFO signals to visual parameters via configurable modulation routes | [automation.md](modules/automation.md) |
| render | Draws audio-reactive visuals (waveforms, spectrum bars, shapes) and applies multi-pass post-processing effects to an accumulation buffer | [render.md](modules/render.md) |
| config | Defines configuration structures for all visual and audio parameters, with JSON serialization for preset save/load | [config.md](modules/config.md) |
| ui | Renders ImGui panels for configuring visualization parameters, audio settings, and presets with custom widgets for gradient editing and modulation routing | [ui.md](modules/ui.md) |
| simulation | GPU-accelerated agent simulations (Physarum, Curl Flow, Attractor Flow) depositing colored trails influenced by audio | [simulation.md](modules/simulation.md) |
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
