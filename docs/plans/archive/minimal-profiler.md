# Minimal Profiler

Add CPU timing instrumentation to the render pipeline with flame graph bars and per-zone sparklines in the Analysis panel. Validate whether bottlenecks are CPU-side before investing in GPU timer queries.

## Current State

- `src/render/render_pipeline.cpp:387-409` - 6-stage pipeline, no timing instrumentation
- `src/ui/imgui_analysis.cpp:253-280` - Analysis panel shows FPS/frame time only
- `src/analysis/beat.h:10-29` - Ring buffer pattern to reuse (`graphHistory[64]`, circular index)
- `src/ui/theme.h` - Widget colors for consistent styling

## Phase 1: Data Structures

**Goal**: Define profiler state and zone identifiers in render_pipeline.h.

**Build**:
- Add `PROFILER_HISTORY_SIZE` constant (64 frames)
- Add `ProfileZoneId` enum with 8 zones: `ZONE_PRE_FEEDBACK`, `ZONE_FEEDBACK`, `ZONE_PHYSARUM_TRAILS`, `ZONE_CURL_TRAILS`, `ZONE_ATTRACTOR_TRAILS`, `ZONE_POST_FEEDBACK`, `ZONE_OUTPUT`, `ZONE_COUNT`
- Add `ProfileZone` struct: `name`, `startTime`, `lastMs`, `history[64]`, `historyIndex`
- Add `Profiler` struct: `zones[ZONE_COUNT]`, `frameStartTime`, `enabled`
- Declare functions: `ProfilerInit`, `ProfilerFrameBegin`, `ProfilerFrameEnd`, `ProfilerBeginZone`, `ProfilerEndZone`

**Done when**: Header compiles with no errors.

---

## Phase 2: Timing Implementation

**Goal**: Implement profiler functions and instrument the render pipeline.

**Build**:
- Add static inline profiler functions in `render_pipeline.cpp`:
  - `ProfilerInit` - set zone names, zero history, enable by default
  - `ProfilerFrameBegin` - store `GetTime()` as frame start
  - `ProfilerFrameEnd` - advance all history indices
  - `ProfilerBeginZone` - store zone start timestamp
  - `ProfilerEndZone` - compute delta, update `lastMs` and `history[]`
- Add `Profiler profiler` field to `AppContext` in `main.cpp`
- Call `ProfilerInit` in `AppContextInit`
- Wrap `RenderPipelineExecute` with `ProfilerFrameBegin`/`ProfilerFrameEnd`
- Pass `Profiler*` to `RenderPipelineExecute`, instrument each stage with begin/end calls

**Done when**: Profiler collects timing data (verify with breakpoint or temporary printf).

---

## Phase 3: Flame Graph UI

**Goal**: Display stacked horizontal bars showing per-zone time breakdown.

**Build**:
- Add `DrawProfilerFlame` static function in `imgui_analysis.cpp`
- Draw background box with `DrawGradientBox`
- For each zone with `lastMs > 0`: draw proportional-width bar, color-coded by zone index
- Show total frame time in header
- Update `ImGuiDrawAnalysisPanel` signature to accept `const Profiler*`
- Add "Profiler" section header with `Theme::ACCENT_ORANGE`
- Call `DrawProfilerFlame` after Band Energy section

**Done when**: Colored bars appear showing relative time per pipeline stage.

---

## Phase 4: Sparkline Graphs

**Goal**: Display per-zone timing history as small line graphs.

**Build**:
- Add `DrawProfilerSparklines` static function in `imgui_analysis.cpp`
- For each zone: draw 64-sample history as vertical bars (reuse `DrawBeatGraph` pattern)
- Show zone name, current ms, and sparkline on same row
- Use `Theme::BAND_*` colors cycling through zones
- Add collapsible section toggle for sparklines (default expanded)

**Done when**: Sparklines update each frame, spikes visible when effects enabled.
