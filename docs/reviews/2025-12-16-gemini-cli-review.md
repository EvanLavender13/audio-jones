# Code Review Report

**Scope**: Full codebase (`src/`)
**Conventions Reference**: CLAUDE.md

## Summary

AudioJones demonstrates a solid, data-oriented architecture well-suited for a real-time visualizer. The code strictly adheres to a consistent C-style C++ pattern, effectively wrapping low-level libraries (miniaudio, raylib/rlgl) into modular subsystems. Critical paths (audio capture, compute shaders) are implemented robustly with proper synchronization.

## Findings

### [Minor] Resource Management Convention Deviation
**Location**: Global (e.g., `AudioCaptureInit`, `PostEffectInit`, `SpectrumBarsInit`)
**Category**: Convention

`CLAUDE.md` specifies "RAII (smart pointers), avoid raw new/delete". However, the codebase uniformly uses `calloc`/`free` and explicit `Init`/`Uninit` functions.

**Evidence**:
`src/audio/audio.cpp`: `AudioCapture* capture = (AudioCapture*)calloc(1, sizeof(AudioCapture));`

**Recommendation**:
Update `CLAUDE.md` to reflect the actual architectural decision (C-style resource management matching Raylib), OR refactor to use `std::unique_ptr` with custom deleters. Given the project's strong C-style coherence, updating the documentation is the pragmatic choice.

### [Minor] Potential Race Condition in FFT Initialization
**Location**: `src/analysis/fft.cpp:InitHannWindow`
**Category**: Logic / Thread Safety

The `InitHannWindow` function uses a static `hannInitialized` flag without synchronization. While currently safe because `AnalysisPipelineInit` is called from the main thread, future multi-threaded initialization could cause a race.

**Evidence**:
```cpp
static bool hannInitialized = false;
static void InitHannWindow(void) {
    if (hannInitialized) return;
    // ... writes to shared array ...
    hannInitialized = true;
}
```

**Recommendation**:
Use `std::call_once` or an atomic flag, or move the static buffer initialization to a strictly single-threaded startup phase.

### [Note] Visual Update Throttling
**Location**: `src/main.cpp:UpdateVisuals`
**Category**: Performance / UX

Visual state (waveform smoothing, spectrum bars) updates are throttled to 20Hz. While this reduces CPU load, it may cause the visualization to appear less fluid (stuttery) than the 60fps rendering, especially for fast-moving waveforms.

**Evidence**:
`const float waveformUpdateInterval = 1.0f / 20.0f;`

**Recommendation**:
Consider decoupling the physics/smoothing tick rate from the data extraction rate, or increasing the update rate to 30-60Hz if performance permits.

### [Note] Implicit Dependency on Config Count
**Location**: `src/config/app_configs.h:AppConfigs`
**Category**: Safety

The `AppConfigs` struct uses raw pointers for `waveformCount` and `selectedWaveform`. This creates an implicit dependency on the lifetime of the owner of these integers (AppContext).

**Evidence**:
```cpp
struct AppConfigs {
    int* waveformCount;
    int* selectedWaveform;
    // ...
};
```

**Recommendation**:
Document ownership clearly or use a safer reference mechanism if the architecture moves towards C++ references.

## Observations

- **Robust Compute Shader Integration**: The `Physarum` module correctly handles the complexity of mixing Raylib's rendering with raw OpenGL compute shaders, including necessary `glMemoryBarrier` calls.
- **Data-Oriented Design**: The strict separation of POD config structs (`src/config/`) from logic modules makes the system easy to serialize/deserialize and debug.
- **Audio/Visual Decoupling**: The `AnalysisPipeline` correctly calculates `audioHopTime` based on sample count rather than frame time, ensuring beat detection behaves consistently regardless of framerate.
- **Error Handling**: Initialization routines consistently check for allocation failures and partial initialization states (e.g., `INIT_OR_FAIL` macros in `main.cpp`).
