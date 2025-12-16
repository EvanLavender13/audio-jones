# Code Review Notes - 2025-12-16

**Reviewer**: gemini-cli
**Date**: 2025-12-16
**Project**: AudioJones

## Phase 1: Context Discovery

### 1.1 Project Conventions (from CLAUDE.md)
- **Language**: C++17 (MSVC/Visual Studio 2022).
- **Libraries**: Raylib (rendering, windowing, audio via miniaudio), ImGui (UI).
- **Build**: CMake (3.21+).
- **Style**:
    - `Snake_Case` for types (classes, structs, enums).
    - `camelCase` for functions and variables.
    - `kPascalCase` for constants.
    - `m_camelCase` for private members.
    - `s_camelCase` for static members.
    - Header guards: `#pragma once`.
    - Includes: Project headers with `""`, system with `<>`.
    - Group includes: Standard Lib, Third Party, Project.
- **Architecture**:
    - Modular design (Analysis, Render, UI, Config).
    - Data-oriented where possible.
    - Separation of concerns (UI shouldn't contain core logic).
    - Config structs for parameters.
- **Error Handling**: Exceptions for fatal errors, return codes/bools for recoverable.
- **Memory**: RAII (smart pointers), avoid raw `new`/`delete`.
- **Performance**:
    - Pre-allocate vectors.
    - Minimize allocations in render loop.
    - Pass large objects by const ref.

### 1.2 Architecture (from docs/architecture.md)
- **Core Loop**: `main.cpp` orchestrates Init -> Update -> Draw.
- **Subsystems**:
    - **Audio/Analysis**: Captures audio, performs FFT (`AudioCapture`, `AnalysisPipeline`).
    - **Render**: Visualizes data (`RenderContext`, `Waveform`, `SpectrumBars`, `Physarum`).
    - **UI**: ImGui based interface (`Layout`, `Panels`).
    - **Config**: Settings and presets (`PresetManager`).
- **Data Flow**: Audio -> FFT -> Analysis Data -> Renderers/UI.
- **State**: Centralized in subsystems, typically configured via "Config" structs.

## Phase 2: Systematic Exploration

### 2.1 Dependency Analysis & Traversal Order
1.  `config` (leaves)
2.  `audio` (leaf)
3.  `automation` (leaf)
4.  `analysis` (depends on audio)
5.  `render` (depends on analysis, config)
6.  `ui` (depends on config, analysis, render)
7.  `main` (root)

### 2.2 Module Investigation

#### Audio (`src/audio/`)
- **Purpose**: Low-level audio capture using `miniaudio`.
- **Interface**: C-style opaque pointer `AudioCapture*`. Init/Start/Stop/Read/Uninit.
- **Resources**: `miniaudio` device and ring buffer. Managed manually via `calloc`/`free`.
- **Concurrency**: Ring buffer handles thread safety between audio callback and main thread.
- **Observations**:
    - Uses `calloc`/`free` instead of `new`/`delete` or smart pointers. Matches `miniaudio` C-style but deviates from `CLAUDE.md` C++ preference. *Acceptable deviation for C-interop layer.*
    - `ma_backend_wasapi` hardcoded. Good for Windows, might limit portability later.
    - Error handling is robust (checks NULLs, returns success/fail).

#### Automation (`src/automation/`)
- **Purpose**: LFO generation for parameter modulation.
- **Interface**: `LFOProcess` taking `LFOState` and `LFOConfig`.
- **Observations**:
    - Uses `rand()` for S&H. Acknowledged with lint suppression.
    - Stateless logic separated from state struct. Functional style.

#### Config (`src/config/`)
- **Purpose**: POD structs for configuration.
- **Observations**:
    - Clean, documented defaults.
    - Uses in-class initialization.
    - `AppConfigs` acts as a context object holding pointers to various config subsystems.

#### Analysis (`src/analysis/`)
- **Components**:
    - **FFT (`fft.h/cpp`)**: Wraps `kiss_fftr`. Handles overlapping windows (75% overlap).
        - *Observation*: `InitHannWindow` uses a static flag without mutex. Potential race condition if multi-threaded init (currently single-threaded).
    - **Beat (`beat.h/cpp`)**: Spectral flux based beat detection (kick band 47-140Hz).
        - *Observation*: Uses adaptive thresholding (rolling avg + 2*stddev).
        - *Observation*: Hardcoded frequency bins for kick drum based on 48kHz/2048 FFT. Dependency on `audio.h` constants is implicit but consistent.
    - **Bands (`bands.h/cpp`)**: Extracts bass/mid/treb energy.
        - *Observation*: Uses Envelope Follower (Attack/Release) for smoothing, preventing jitter.
    - **Pipeline (`analysis_pipeline.h/cpp`)**: Orchestrator.
        - *Observation*: `NormalizeAudioBuffer` implements AGC to handle varying volume levels.
        - *Observation*: Correctly processes multiple FFT windows if audio buffer contains enough data.
        - *Observation*: Uses `audioHopTime` derived from FFT size for consistent timing logic, decoupling physics from frame rate.

#### Render (`src/render/`)
- **Components**:
    - **Context (`render_context.h`)**: POD for screen dims.
    - **Physarum (`physarum.h/cpp`)**: Compute shader slime mold simulation.
        - *Observation*: Sophisticated use of Raylib's `rlgl` layer for Compute Shaders and memory barriers.
        - *Observation*: Correctly manages OpenGL resources (SSBOs, Textures, Shaders) with RAII-like Init/Uninit functions.
        - *Observation*: Handles window resize and config changes (buffer reallocation) robustly.
        - *Observation*: Uses `glMemoryBarrier` to sync Compute and Graphics pipelines.
    - **Waveform (`waveform.h/cpp`)**: Line rendering.
        - *Observation*: `ProcessWaveformSmooth` creates palindromes for seamless circular connection. Smart technique.
        - *Observation*: Uses cubic interpolation (`CubicInterp`) for smooth circular drawing.
    - **Spectrum Bars (`spectrum_bars.h/cpp`)**: Frequency bars.
        - *Observation*: Uses Logarithmic frequency mapping, which is visually correct for audio.
    - **Post Effect (`post_effect.h/cpp`)**: Full-screen effects manager.
        - *Observation*: Tries HDR textures (`RGBA32F`), falls back to standard. Good robustness.
        - *Observation*: Implements complex multi-pass pipeline (Physarum -> Voronoi -> Feedback -> Blur -> Kaleido -> TrailBoost -> Chromatic).
        - *Observation*: Cleanly separates "Accumulation" (drawing scene) from "ToScreen" (final composite).

#### UI (`src/ui/`)
- **Components**:
    - **Common (`ui_common.h`)**: Shared state for dropdown management.
        - *Observation*: Addresses IMGUI overlap issue by tracking `AnyDropdownOpen`.
    - **Panel Audio (`ui_panel_audio.cpp`)**: Audio settings panel.
        - *Observation*: Uses `raygui` and custom layout helpers.

#### Main (`src/main.cpp`)
*Starting review...*
