---
plan: docs/plans/drawable-ordering.md
branch: drawable-ordering
mode: parallel
current_phase: 1
current_wave: 1
total_phases: 1
total_waves: 1
waves:
  1: [1]
started: 2026-01-25
last_updated: 2026-01-25
---

# Implementation Progress: Drawable Ordering

## Phase 1: Implement two-pass rendering
- Status: completed
- Wave: 1
- Started: 2026-01-25
- Completed: 2026-01-25
- Files modified:
  - src/render/drawable.cpp
- Notes: Extracted render condition checks into `DrawableShouldRender()` helper. Replaced single loop with two passes: shapes first, then waveforms/spectrum. Both passes maintain proper waveformIndex/spectrumIndex tracking.
