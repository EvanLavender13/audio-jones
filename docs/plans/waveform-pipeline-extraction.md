# WaveformPipeline Extraction

Extract waveform processing state and logic from `AppContext` into a dedicated module, mirroring `AnalysisPipeline` and `SpectrumBars` patterns.

## Problem

`AppContext` in `src/main.cpp:22-40` mixes application orchestration with waveform-specific state. Eight fields and two functions (`UpdateVisuals`, `RenderWaveforms`) form a cohesive cluster that belongs in its own module.

## Targets

| Location | Issue | Resolution |
|----------|-------|------------|
| `src/main.cpp:32-39` | Waveform buffers in AppContext | Move to WaveformPipeline struct |
| `src/main.cpp:101-126` | `UpdateVisuals` processes waveforms | Extract to `WaveformPipelineProcess` |
| `src/main.cpp:128-144` | `RenderWaveforms` draws waveforms | Extract to `WaveformPipelineDraw` |

## Current State

Waveform state scattered in AppContext:

```c
// src/main.cpp:32-39
float waveform[WAVEFORM_SAMPLES];                          // Base buffer
float waveformExtended[MAX_WAVEFORMS][WAVEFORM_EXTENDED];  // Per-config smoothed
float waveformAccumulator;                                  // 20Hz timing
uint64_t globalTick;                                        // Animation counter
```

Processing logic in `UpdateVisuals` (lines 101-126):
1. Computes waveform offset for most recent audio
2. Calls `ProcessWaveformBase` for channel mixing and normalization
3. Calls `ProcessWaveformSmooth` per config
4. Increments `globalTick`

Rendering logic in `RenderWaveforms` (lines 128-144):
1. Branches on linear vs circular mode
2. Draws waveforms with `DrawWaveformLinear` or `DrawWaveformCircular`
3. Draws spectrum bars if enabled

## Design

### Interface

```c
// render/waveform_pipeline.h
typedef struct WaveformPipeline WaveformPipeline;

WaveformPipeline* WaveformPipelineInit(void);
void WaveformPipelineUninit(WaveformPipeline* wp);

// Process audio into smoothed waveforms (call at visual update rate)
void WaveformPipelineProcess(WaveformPipeline* wp,
                             const float* audioBuffer,
                             uint32_t framesRead,
                             const WaveformConfig* configs,
                             int configCount,
                             ChannelMode channelMode);

// Draw all waveforms (linear or circular based on mode)
void WaveformPipelineDraw(const WaveformPipeline* wp,
                          RenderContext* ctx,
                          const WaveformConfig* configs,
                          int configCount,
                          bool circular);

// Get current tick for external consumers (spectrum bars sync)
uint64_t WaveformPipelineGetTick(const WaveformPipeline* wp);
```

### Internal State

```c
// render/waveform_pipeline.cpp
struct WaveformPipeline {
    float waveform[WAVEFORM_SAMPLES];
    float waveformExtended[MAX_WAVEFORMS][WAVEFORM_EXTENDED];
    uint64_t globalTick;
};
```

Note: `waveformAccumulator` stays in main.cpp - it controls *when* to call Process, not waveform state itself.

### Why Configs Stay External

`WaveformConfig[]`, `waveformCount`, `selectedWaveform` remain in AppContext because:
- UI panels edit them directly via `AppConfigs` pointers
- Preset system reads/writes them
- Pipeline only needs read access during Process/Draw

## File Changes

| File | Change |
|------|--------|
| `src/render/waveform_pipeline.h` | Create - opaque type and interface |
| `src/render/waveform_pipeline.cpp` | Create - implementation |
| `src/main.cpp` | Replace 8 fields with `WaveformPipeline*`, simplify UpdateVisuals/RenderWaveforms |

## Phases

### Phase 1: Create Module

1. Create `src/render/waveform_pipeline.h` with interface
2. Create `src/render/waveform_pipeline.cpp` with:
   - Struct definition (waveform buffers, tick)
   - Init/Uninit (calloc/free)
   - Process (move logic from UpdateVisuals lines 106-125)
   - Draw (move logic from RenderWaveforms lines 130-143)
   - GetTick accessor

### Phase 2: Integrate

1. Add `WaveformPipeline* waveformPipeline` to AppContext
2. Remove replaced fields from AppContext
3. Update AppContextInit to call `WaveformPipelineInit`
4. Update AppContextUninit to call `WaveformPipelineUninit`
5. Simplify main loop to call pipeline functions
6. Pass `WaveformPipelineGetTick()` to spectrum bars draw functions

### Phase 3: Update Build

1. Add `waveform_pipeline.cpp` to CMakeLists.txt

## Validation

- [ ] Build succeeds with no warnings
- [ ] Waveform displays correctly in linear mode
- [ ] Waveform displays correctly in circular mode
- [ ] Multiple waveforms render with independent smoothing
- [ ] Spectrum bars animate in sync with waveforms (shared tick)
- [ ] Preset save/load still works (configs unchanged)
