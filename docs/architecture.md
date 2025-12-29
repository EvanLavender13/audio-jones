# AudioJones Architecture

> Last sync: 2025-12-28 | Commit: 3b0c93c

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
        PE -->|HDR texture| Physarum[physarum]
        Physarum -->|agent deposits| PE
        PE -->|final frame| Screen[Display]
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
| audio | Captures system audio via WASAPI loopback into lock-free ring buffer for real-time processing | [audio.md](modules/audio.md) |
| analysis | Converts stereo audio samples into frequency-domain data, extracts beat events, and computes smoothed bass/mid/treble energy levels | [analysis.md](modules/analysis.md) |
| automation | Routes modulation sources (audio bands, beat detection, LFOs) to effect parameters through configurable curves | [automation.md](modules/automation.md) |
| render | Converts audio waveforms and spectrum data into GPU-rendered visuals through shader-based feedback accumulation | [render.md](modules/render.md) |
| config | Centralizes runtime parameters as POD structs with JSON serialization for preset save/load | [config.md](modules/config.md) |
| ui | Renders ImGui panels with modulation-aware controls and custom themed widgets | [ui.md](modules/ui.md) |
| main | Orchestrates application lifecycle, routing audio through analysis/modulation/rendering pipelines at 60fps with 20Hz visual throttling | [main.md](modules/main.md) |

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
├── config/               Serializable parameters
└── ui/                   Dear ImGui panels
```

---

*Run `/sync-architecture` to regenerate this document from current code.*
