---
plan: docs/plans/modulation-easing-functions.md
branch: modulation-easing-functions
current_phase: 3
total_phases: 5
started: 2026-01-02
last_updated: 2026-01-02
---

# Implementation Progress: Modulation Easing Functions

## Phase 1: Easing Math Header
- Status: completed
- Started: 2026-01-02
- Completed: 2026-01-02
- Files modified:
  - src/automation/easing.h (created)
- Notes: Created header-only easing module with 6 functions (EaseInCubic, EaseOutCubic, EaseInOutCubic, EaseSpring, EaseElastic, EaseBounce). All return 0 at t=0 and 1 at t=1. Spring/Elastic show overshoot at midpoint.

## Phase 2: Engine Integration
- Status: completed
- Started: 2026-01-02
- Completed: 2026-01-02
- Files modified:
  - src/automation/modulation_engine.h (replaced ModCurve enum)
  - src/automation/modulation_engine.cpp (added easing.h include, BipolarEase helper, new ApplyCurve switch)
- Notes: Replaced EXP/SQUARED with 7 new curve types. BipolarEase preserves sign for LFO sources.

## Phase 3: Curve Preview Widget
- Status: pending

## Phase 4: Dropdown UI
- Status: pending

## Phase 5: Verification
- Status: pending
