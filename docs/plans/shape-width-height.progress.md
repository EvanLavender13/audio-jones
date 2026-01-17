---
plan: docs/plans/shape-width-height.md
branch: shape-width-height
current_phase: 5
total_phases: 5
started: 2026-01-16
last_updated: 2026-01-16
---

# Implementation Progress: Shape Width/Height

## Phase 1: Config Update
- Status: completed
- Started: 2026-01-16
- Completed: 2026-01-16
- Files modified:
  - src/config/drawable_config.h
- Notes: Replaced `float size = 0.2f` with `float width = 0.4f`, `float height = 0.4f`, and `bool aspectLocked = true` in ShapeData struct. Build fails as expected - downstream files still reference the removed `size` field.

## Phase 2: Rendering Update
- Status: completed
- Started: 2026-01-16
- Completed: 2026-01-16
- Files modified:
  - src/render/shape.cpp
- Notes: Replaced `float radius` with `float radiusX` and `float radiusY` in ShapeGeometry. Updated ShapeCalcGeometry to compute `radiusX = width * screenW * 0.5f` and `radiusY = height * screenH * 0.5f`. Updated vertex calculations in ShapeDrawSolid and ShapeDrawTextured to use separate X/Y radii.

## Phase 3: UI Controls
- Status: completed
- Started: 2026-01-16
- Completed: 2026-01-16
- Files modified:
  - src/ui/drawable_type_controls.cpp
- Notes: Replaced Size slider with Width/Height sliders (0.01-2.0 range). Added Lock checkbox on same line as Width slider. Implemented aspect-lock logic that proportionally updates the other dimension when one changes.

## Phase 4: Preset Serialization
- Status: completed
- Started: 2026-01-16
- Completed: 2026-01-16
- Files modified:
  - src/config/preset.cpp
  - presets/BOIDO.json
  - presets/GRAYBOB.json
  - presets/ICEY.json
  - presets/YUPP.json
- Notes: Updated NLOHMANN_DEFINE_TYPE macro to serialize width, height, aspectLocked instead of size. Updated all preset files with shapes: converted size * 2.0 to width/height values.

## Phase 5: Modulation Support
- Status: pending
