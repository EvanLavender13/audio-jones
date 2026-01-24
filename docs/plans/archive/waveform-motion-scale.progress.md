---
plan: docs/plans/waveform-motion-scale.md
branch: waveform-motion-scale
mode: sequential
current_phase: done
total_phases: 2
started: 2026-01-24
last_updated: 2026-01-24
completed: 2026-01-24
---

# Implementation Progress: Waveform Motion Scale

## Phase 1: Config + Buffer + Processing
- Status: completed
- Started: 2026-01-24
- Completed: 2026-01-24
- Files modified:
  - src/config/drawable_config.h
  - src/render/drawable.h
  - src/render/drawable.cpp
- Notes: Added waveformMotionScale field (default 1.0), per-drawable smoothedWaveform buffer in DrawableState, and EMA interpolation pass before spatial smoothing.

## Phase 2: UI + Serialization + Modulation
- Status: completed
- Started: 2026-01-24
- Completed: 2026-01-24
- Files modified:
  - src/ui/drawable_type_controls.cpp
  - src/config/preset.cpp
  - src/automation/drawable_params.cpp
- Notes: Added logarithmic Motion slider in Geometry section, added waveformMotionScale to JSON serialization macro, registered modulation param with [0.0, 1.0] range gated on DRAWABLE_WAVEFORM.
