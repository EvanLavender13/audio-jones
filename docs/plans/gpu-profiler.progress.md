---
plan: docs/plans/gpu-profiler.md
branch: gpu-profiler
current_phase: 3
total_phases: 3
started: 2026-01-11
last_updated: 2026-01-11
status: complete
---

# Implementation Progress: GPU Profiler

## Phase 1: Add GPU Query Infrastructure
- Status: completed
- Completed: 2026-01-11
- Files modified:
  - src/render/profiler.h
  - src/render/profiler.cpp
- Notes: Added glad.h include, double-buffered GLuint queries[ZONE_COUNT][2] and writeIdx to Profiler struct. Removed startTime from ProfileZone. ProfilerInit allocates queries with glGenQueries and runs dummy queries to prevent first-frame read errors. ProfilerUninit frees queries. Begin/EndZone stubbed (no timing yet).

## Phase 2: Implement GPU Timing
- Status: completed
- Completed: 2026-01-11
- Files modified:
  - src/render/profiler.cpp
- Notes: ProfilerBeginZone calls glBeginQuery with double-buffered query ID. ProfilerEndZone calls glEndQuery. ProfilerFrameBegin reads previous frame's query results using glGetQueryObjectui64v and updates lastMs/history. ProfilerFrameEnd flips writeIdx buffer.

## Phase 3: Verify and Test
- Status: completed
- Completed: 2026-01-11
- Files modified:
  - src/render/profiler.h
  - src/render/profiler.cpp
  - src/render/render_pipeline.h
  - src/render/render_pipeline.cpp
  - src/ui/imgui_analysis.cpp
- Notes: Added EMA smoothing (smoothedMs with 15% factor) for stable UI display. Fixed zone nesting by extracting RenderPipelineApplySimulation() - GL_TIME_ELAPSED queries don't support nesting. User verified correct zone attribution and smooth updates.
