# Kaleidoscope Effect Split

Split the bundled Kaleidoscope effect into focused, single-purpose effects. Create a new "Cellular" category for grid-based effects.

## Problem

The current Kaleidoscope effect bundles four techniques with intensity sliders:
- Polar (standard kaleidoscope)
- KIFS (kaleidoscopic IFS fractal)
- Iterative Mirror (rotation + Cartesian mirroring)
- Hex Fold (hexagonal lattice symmetry)

These share parameters (segments, rotation, twist) but the parameters mean different things for each technique. "Segments" controls wedge count for Polar but iteration count for Iterative Mirror. KIFS was forced into a radial mold when it's really a general fractal technique. The bundling creates confusion and limits each technique's potential.

## Goal

Split into separate effects where each has focused, meaningful parameters. Reorganize categories to reflect the underlying math.

## Changes

### Effects to Create (from existing code)

| New Effect | Source | Category | Notes |
|------------|--------|----------|-------|
| Kaleidoscope | Polar technique | Symmetry | Add smoothing parameter for soft wedge edges |
| KIFS | KIFS technique | Symmetry | Standalone. Remove forced radial constraint |
| Iterative Mirror | iterMirror technique | Symmetry | Rename "segments" to "iterations" |
| Lattice Fold | Hex Fold technique | Cellular (new) | Generalize to support Triangle/Square/Hex cell types |

### Category Changes

| Category | Before | After |
|----------|--------|-------|
| Symmetry | Kaleidoscope (bundled), Poincare Disk | Kaleidoscope, KIFS, Iterative Mirror, Poincare Disk |
| Warp | Voronoi, others... | Remove Voronoi |
| Cellular (NEW) | — | Voronoi, Lattice Fold |

### Parameter Changes

**Kaleidoscope** (was Polar):
- Keep: segments, rotation, twist
- Add: smoothing (0-0.5, controls soft blend at wedge seams)
- Remove: shared warp/focal parameters (move to effect-specific if needed)

**KIFS**:
- Keep: iterations, scale, offset
- Keep: segments (polar fold per iteration)
- Remove: forced dependency on Kaleidoscope's rotation accumulator

**Iterative Mirror**:
- Rename: segments → iterations
- Keep: rotation, twist

**Lattice Fold** (was Hex Fold):
- Add: cellType (Triangle=3, Square=4, Hexagon=6)
- Rename: hexScale → cellScale
- Keep: rotation

### Files Affected

- `src/config/kaleidoscope_config.h` → Split into per-effect configs
- `shaders/kaleidoscope.fs` → Split into per-effect shaders
- `src/config/effect_config.h` → Add new effect configs, add Cellular category
- `src/ui/imgui_effects_transforms.cpp` → Update UI for split effects
- `src/render/post_effect.cpp` → Register new effects
- `src/automation/param_registry.cpp` → Register parameters for new effects
- `docs/effects.md` → Update inventory with new category and effects

### Preset Migration

Existing presets use `kaleidoscope.*` fields with intensity blending. Migration strategy needed:
- Map `polarIntensity > 0` → enable new Kaleidoscope effect
- Map `kifsIntensity > 0` → enable new KIFS effect
- Map `iterMirrorIntensity > 0` → enable new Iterative Mirror effect
- Map `hexFoldIntensity > 0` → enable new Lattice Fold effect (cellType=6)

Presets that blend multiple techniques will need manual review.

## Research Reference

See `docs/research/kaleidoscope-new-techniques.md` for algorithm details on:
- Smooth polar folding (pabs polynomial blend)
- Lattice fold cell types (triangular, square, hexagonal coordinate transforms)

## Out of Scope

- Adding new symmetry techniques beyond what exists
- Changing the transform pipeline order
- Modifying Poincare Disk or Voronoi internals
