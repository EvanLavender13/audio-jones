---
plan: docs/plans/angular-field-standardization.md
branch: angular-field-standardization
current_phase: 7
total_phases: 7
started: 2026-01-05
last_updated: 2026-01-05
---

# Implementation Progress: Angular Field Standardization

## Phase 1: Config Struct Renames
- Status: completed
- Started: 2026-01-05
- Completed: 2026-01-05
- Files modified:
  - src/config/drawable_config.h - `rotationOffset` → `rotationAngle`
  - src/config/effect_config.h - `rotBase` → `rotationSpeed`, `rotRadial` → `rotationSpeedRadial`
  - src/config/kaleidoscope_config.h - `twistAmount` → `twistAngle`
  - src/config/tunnel_config.h - `twist` → `twistAngle`
  - src/config/mobius_config.h - `rotationSpeed` → `animRotation`
  - src/config/turbulence_config.h - `rotationPerOctave` → `octaveTwist`
  - src/config/infinite_zoom_config.h - `spiralTurns` → `spiralAngle`
  - src/simulation/attractor_flow.h - `rotationX/Y/Z` → `rotationAngleX/Y/Z`
- Notes: Config structs renamed. Build fails until Phase 2 updates usage sites.

## Phase 2: Usage Site Updates
- Status: completed
- Started: 2026-01-05
- Completed: 2026-01-05
- Files modified:
  - src/render/render_pipeline.cpp - Updated all shader uniform references
  - src/render/waveform.cpp - `rotationOffset` → `rotationAngle`
  - src/render/spectrum_bars.cpp - `rotationOffset` → `rotationAngle`
  - src/render/shape.cpp - `rotationOffset` → `rotationAngle`
  - src/simulation/attractor_flow.cpp - `rotationX/Y/Z` → `rotationAngleX/Y/Z`
  - src/config/preset.cpp - Updated JSON serialization field names
  - src/automation/param_registry.cpp - Updated param IDs and target pointers
  - src/automation/drawable_params.cpp - `rotationOffset` → `rotationAngle`
  - src/ui/imgui_effects.cpp - Updated all field references
  - src/ui/drawable_type_controls.cpp - `rotationOffset` → `rotationAngle`
- Notes: All usage sites updated. Project compiles successfully.

## Phase 3: Registry Updates
- Status: completed
- Started: 2026-01-05
- Completed: 2026-01-05
- Files modified:
  - src/automation/param_registry.cpp - Added `mobius.animRotation` and `kaleidoscope.twistAngle` entries
- Notes: All angular params now registered with correct bounds. Mobius animRotation uses 0.0-2.0 range (scalar multiplier, not radians).

## Phase 4: UI Updates
- Status: completed
- Started: 2026-01-05
- Completed: 2026-01-05
- Files modified:
  - src/ui/imgui_effects.cpp - Updated labels and converted Mobius to ModulatableSlider
  - src/ui/drawable_type_controls.cpp - "Rotation"→"Spin", "Offset"→"Angle"
- Notes: All UI labels standardized per naming convention. Label changes: Mobius "Rotation"→"Anim Rotation" (now ModulatableSlider), Turbulence "Rotation"→"Octave Twist", Tunnel "Rotation"→"Spin", Flow Field "Rot Base"→"Spin"/"Rot Radial"→"Spin Radial", Attractor Flow "Rot X/Y/Z"→"Angle X/Y/Z", Infinite Zoom "Spiral"→"Spiral Angle", Drawable "Rotation"→"Spin"/"Offset"→"Angle"

## Phase 5: Accumulation Pattern Fixes
- Status: completed
- Started: 2026-01-05
- Completed: 2026-01-05
- Files modified:
  - src/render/post_effect.h - Added `tunnelRotation` and `mobiusRotation` accumulators, renamed uniform location vars
  - src/render/post_effect.cpp - Updated GetShaderLocation calls, added initialization for new accumulators
  - src/render/render_pipeline.cpp - Per-frame accumulation of rotation values, pass accumulated angles to shaders
  - shaders/tunnel.fs - Changed `time * rotationSpeed` to use CPU-accumulated `rotation` uniform
  - shaders/mobius.fs - Changed `t * rotationSpeed` to use CPU-accumulated `rotation` uniform
- Notes: Tunnel and Mobius rotation now use CPU-side accumulation. Changing rotationSpeed/animRotation mid-animation no longer causes visible jumps. Other shaders (turbulence, infinite_zoom, radial_streak, voronoi, multi_inversion) verified to not have problematic `time * speed` patterns

## Phase 6: Preset Migration
- Status: completed
- Started: 2026-01-05
- Completed: 2026-01-05
- Files modified:
  - presets/BINGBANG.json
  - presets/GRAYBOB.json
  - presets/ICEY.json
  - presets/SOLO.json
  - presets/STAYINNIT.json
  - presets/WINNY.json
  - presets/WOBBYBOB.json
- Notes: Updated JSON files directly instead of adding migration code. All presets now use new field names: `rotationOffset`→`rotationAngle`, `rotBase`→`rotationSpeed`, `rotRadial`→`rotationSpeedRadial`, `twistAmount`→`twistAngle`, `spiralTurns`→`spiralAngle`, `rotationX/Y/Z`→`rotationAngleX/Y/Z`. Also updated modulation route paramIds in SOLO and STAYINNIT

## Phase 7: Verification
- Status: pending
