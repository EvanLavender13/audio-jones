---
plan: docs/plans/minimal-profiler.md
branch: minimal-profiler
current_phase: 4
total_phases: 4
started: 2026-01-03
last_updated: 2026-01-03
---

# Implementation Progress: Minimal Profiler

## Phase 1: Data Structures
- Status: completed
- Started: 2026-01-03
- Completed: 2026-01-03
- Files modified:
  - src/render/render_pipeline.h
- Notes: Added PROFILER_HISTORY_SIZE constant, ProfileZoneId enum (7 zones + ZONE_COUNT), ProfileZone struct with name/startTime/lastMs/history/historyIndex, Profiler struct with zones array/frameStartTime/enabled, and 5 function declarations.

## Phase 2: Timing Implementation
- Status: completed
- Started: 2026-01-03
- Completed: 2026-01-03
- Files modified:
  - src/render/render_pipeline.cpp
  - src/render/render_pipeline.h
  - src/main.cpp
- Notes: Implemented ProfilerInit/ProfilerFrameBegin/ProfilerFrameEnd/ProfilerBeginZone/ProfilerEndZone functions. Added Profiler field to AppContext, initialized in AppContextInit. Updated RenderPipelineExecute to accept Profiler* and instrumented all 7 pipeline stages.

## Phase 3: Flame Graph UI
- Status: completed
- Started: 2026-01-03
- Completed: 2026-01-03
- Files modified:
  - src/ui/imgui_panels.h
  - src/ui/imgui_analysis.cpp
  - src/main.cpp
- Notes: Added DrawProfilerFlame function with stacked horizontal bars color-coded by zone. Updated ImGuiDrawAnalysisPanel signature to accept const Profiler*. Added "Profiler" section with ACCENT_ORANGE header. Bars show proportional width based on zone timing with zone names displayed when bar is wide enough. Total frame time shown in header.

## Phase 4: Sparkline Graphs
- Status: completed
- Started: 2026-01-03
- Completed: 2026-01-03
- Files modified:
  - src/ui/imgui_analysis.cpp
- Notes: Added DrawProfilerSparklines function with collapsible "Zone Timing" section. Each zone displays name, current ms value, and 64-sample history graph as vertical bars. Bars auto-scale to max value in history. Uses ZONE_COLORS for consistent color-coding across flame graph and sparklines.
