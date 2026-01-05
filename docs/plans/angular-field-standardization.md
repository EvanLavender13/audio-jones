# Angular Field Standardization

Standardize all rotation/angular field names, units, registry entries, and accumulation patterns across the codebase. Enforces the naming convention now documented in CLAUDE.md.

## Current State

Inconsistent naming across 11+ config structs:
- `src/config/drawable_config.h:14-15` - `rotationSpeed`, `rotationOffset`
- `src/config/effect_config.h:61-62` - `rotBase`, `rotRadial` (truncated)
- `src/config/kaleidoscope_config.h:13-14` - `rotationSpeed`, `twistAmount`
- `src/config/tunnel_config.h:9-10` - `rotationSpeed`, `twist`
- `src/config/mobius_config.h:11` - `rotationSpeed` (actually a scalar multiplier)
- `src/config/turbulence_config.h:11` - `rotationPerOctave`
- `src/config/infinite_zoom_config.h:12-13` - `spiralTurns`, `spiralTwist`
- `src/simulation/attractor_flow.h:45-50` - `rotationX/Y/Z`, `rotationSpeedX/Y/Z`

## Naming Convention

| Suffix | Meaning | Units | Registry Constant | UI Format |
|--------|---------|-------|-------------------|-----------|
| `*Speed` | Rate (radians/frame) | rad/f | `ROTATION_SPEED_MAX` | "%.2f °/f" |
| `*Angle` | Static angle | rad | `ROTATION_OFFSET_MAX` | "%.1f °" |
| `*Twist` | Per-unit rotation | rad | `ROTATION_OFFSET_MAX` | "%.1f °" |

## Complete Rename Map

| File | Old Name | New Name | Type |
|------|----------|----------|------|
| `drawable_config.h` | `rotationOffset` | `rotationAngle` | Angle |
| `effect_config.h` | `rotBase` | `rotationSpeed` | Speed |
| `effect_config.h` | `rotRadial` | `rotationSpeedRadial` | Speed |
| `kaleidoscope_config.h` | `twistAmount` | `twistAngle` | Angle |
| `tunnel_config.h` | `twist` | `twistAngle` | Twist |
| `mobius_config.h` | `rotationSpeed` | `animRotation` | Scalar (special) |
| `turbulence_config.h` | `rotationPerOctave` | `octaveTwist` | Twist |
| `infinite_zoom_config.h` | `spiralTurns` | `spiralAngle` | Angle |
| `attractor_flow.h` | `rotationX` | `rotationAngleX` | Angle |
| `attractor_flow.h` | `rotationY` | `rotationAngleY` | Angle |
| `attractor_flow.h` | `rotationZ` | `rotationAngleZ` | Angle |

## UI Label Standardization

| Effect | Old Label | New Label | Field |
|--------|-----------|-----------|-------|
| Drawable | "Rotation" | "Spin" | `rotationSpeed` |
| Drawable | "Offset" | "Angle" | `rotationAngle` |
| Kaleidoscope | "Spin" | "Spin" | `rotationSpeed` (no change) |
| Kaleidoscope | "Twist" | "Twist" | `twistAngle` (no change) |
| Tunnel | "Rotation" | "Spin" | `rotationSpeed` |
| Tunnel | "Twist" | "Twist" | `twistAngle` (no change) |
| Attractor Flow | "Rot X/Y/Z" | "Angle X/Y/Z" | `rotationAngleX/Y/Z` |
| Attractor Flow | "Spin X/Y/Z" | "Spin X/Y/Z" | `rotationSpeedX/Y/Z` (no change) |
| Flow Field | "Rot Base" | "Spin" | `rotationSpeed` |
| Flow Field | "Rot Radial" | "Spin Radial" | `rotationSpeedRadial` |
| Turbulence | "Rotation" | "Octave Twist" | `octaveTwist` |
| Infinite Zoom | "Spiral" | "Spiral Angle" | `spiralAngle` |
| Mobius | "Rotation" | "Anim Rotation" | `animRotation` |

---

## Phase 1: Config Struct Renames

**Goal**: Rename all angular fields in config structs to follow naming convention.

**Build**:
- `src/config/drawable_config.h` - rename `rotationOffset` → `rotationAngle`
- `src/config/effect_config.h` - rename `rotBase` → `rotationSpeed`, `rotRadial` → `rotationSpeedRadial`
- `src/config/kaleidoscope_config.h` - rename `twistAmount` → `twistAngle`
- `src/config/tunnel_config.h` - rename `twist` → `twistAngle`
- `src/config/mobius_config.h` - rename `rotationSpeed` → `animRotation`
- `src/config/turbulence_config.h` - rename `rotationPerOctave` → `octaveTwist`
- `src/config/infinite_zoom_config.h` - rename `spiralTurns` → `spiralAngle`
- `src/simulation/attractor_flow.h` - rename `rotationX/Y/Z` → `rotationAngleX/Y/Z`

**Done when**: All config structs compile with new field names.

---

## Phase 2: Usage Site Updates

**Goal**: Update all code that references renamed fields.

**Build**:
- `src/render/render_pipeline.cpp` - update field references
- `src/render/drawable.cpp` - update field references
- `src/simulation/attractor_flow.cpp` - update field references
- `src/preset/preset.cpp` - update JSON serialization keys
- Shader uniform bindings in render pipeline

**Done when**: Project compiles with no errors.

---

## Phase 3: Registry Updates

**Goal**: Update param registry with new field names and add missing entries.

**Build**:
- `src/automation/param_registry.cpp`:
  - Rename param IDs to match new field names
  - Add Mobius `animRotation` entry (currently missing from registry)
  - Verify all angular fields use `ROTATION_SPEED_MAX` or `ROTATION_OFFSET_MAX`
- `src/automation/drawable_params.cpp` - update drawable field names

**Done when**: All angular params registered with correct bounds.

---

## Phase 4: UI Updates

**Goal**: Standardize all UI labels and unit formats.

**Build**:
- `src/ui/imgui_effects.cpp`:
  - Update slider labels per table above
  - Ensure all Speed fields show "%.2f °/f"
  - Ensure all Angle/Twist fields show "%.1f °"
  - Convert Mobius to use `ModulatableSlider` (currently raw `SliderFloat`)
- `src/ui/drawable_type_controls.cpp`:
  - "Rotation" → "Spin", "Offset" → "Angle"

**Done when**: UI displays consistent labels and units for all angular controls.

---

## Phase 5: Accumulation Pattern Fixes

**Goal**: Ensure all Speed fields use CPU accumulation, never `time * speed` in shaders.

**Build**:
- Audit shaders listed in `docs/plans/rotation-accumulation-fix.md`:
  - `shaders/tunnel.fs` - replace `time * rotationSpeed` with accumulated uniform
  - `shaders/mobius.fs` - verify accumulation pattern
  - `shaders/turbulence.fs` - verify time usage
  - `shaders/infinite_zoom.fs` - verify spiral rotation
  - `shaders/radial_streak.fs` - verify spiral twist
  - `shaders/voronoi.fs` - verify cell animation
  - `shaders/multi_inversion.fs` - verify time usage
- Add accumulator state variables to `src/render/post_effect.h` as needed
- Update `src/render/render_pipeline.cpp` to accumulate and pass values

**Done when**: Changing any Speed slider mid-animation produces smooth transitions, not jumps.

---

## Phase 6: Preset Migration

**Goal**: Handle existing presets with old field names.

**Build**:
- `src/preset/preset.cpp`:
  - Add migration logic to read old field names and map to new
  - Write only new field names
  - Log warning when migrating old preset

**Done when**: Old presets load correctly and save with new field names.

---

## Phase 7: Verification

**Goal**: Confirm all changes work correctly.

**Build**:
- Build project, fix any remaining compile errors
- Load each preset, verify no regressions
- Test modulation on each angular param
- Test Speed fields: change value mid-animation, confirm no jumps
- Run through each effect, verify UI labels and units correct

**Done when**: All effects function correctly with standardized naming.
