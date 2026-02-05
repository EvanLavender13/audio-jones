# Param Registry Migration

Move param registration from PARAM_TABLE into per-module RegisterParams functions.

**Research**: [docs/research/param-registry-refactor.md](../research/param-registry-refactor.md)

## Batches

- [x] **Batch 0**: Infrastructure — ModEngineGetParamBounds, ParamRegistryGetDynamic update
- [x] **Batch 1**: Warp + Symmetry (20 effects, ~93 params)
- [x] **Batch 2**: Cellular + Motion + Optical (12 effects, ~61 params)
- [x] **Batch 3**: Artistic + Graphic + Retro (16 effects, 66 params)
- [ ] **Batch 4**: Color + Generators + Cleanup (6 effects, ~36 params)

## Batch Details

### Batch 0: Infrastructure

Add `ModEngineGetParamBounds()` to modulation engine. Update `ParamRegistryGetDynamic` to query ModEngine before PARAM_TABLE. Add registration call site in `main.cpp` after `PostEffectInit`.

No effect modules touched. Existing behavior unchanged — PARAM_TABLE still feeds ModEngine.

### Batch 1: Warp + Symmetry (20 effects)

**Warp** (12): sine_warp(2), texture_warp(4), wave_ripple(8), mobius(7), gradient_flow(2), chladni_warp(4), domain_warp(4), surface_warp(5), interference_warp(5), corridor_warp(8), fft_radial_warp(9), circuit_board(5)

**Symmetry** (8): kaleidoscope(3), kifs(3), poincare_disk(6), mandelbox(4), triangle_fold(2), moire_interference(3), radial_ifs(3), radial_pulse(6)

### Batch 2: Cellular + Motion + Optical (12 effects)

**Cellular** (3): voronoi(13), lattice_fold(3), phyllotaxis(16)

**Motion** (5): infinite_zoom(2), droste_zoom(4), density_wave_spiral(4), shake(3), relativistic_doppler(6)

**Optical** (4): bloom(2), anamorphic_streak(4), bokeh(2), heightfield_relief(2)

*Note: radial_streak has 0 modulatable params — empty RegisterParams.*

### Batch 3: Artistic + Graphic + Retro (15 effects)

**Artistic** (6): oil_paint(3), watercolor(3), impressionist(4), ink_wash(5), pencil_sketch(4), cross_hatching(4)

**Graphic** (5): neon_glow(5), kuwahara(1), halftone(3), disco_ball(8), lego_bricks(3)

**Retro** (4): pixelation(2), glitch(16), ascii_art(1), matrix_rain(4)

*Note: toon has 0 modulatable params — empty RegisterParams.*

### Batch 4: Color + Generators + Cleanup (6 effects)

**Color** (3): color_grade(8), false_color(1), palette_quantization(2)

**Generators** (3): plasma(7), interference(9), constellation(9)

**Also**: synthwave(6) — already a module in src/effects/ but params still in PARAM_TABLE.

**Cleanup**: Remove all migrated effect entries from PARAM_TABLE. Delete `ParamRegistryGet` if no callers remain. Verify PARAM_TABLE retains only simulation/feedback/flow/top-level entries (159 params).

## Per-Batch Checklist

- [ ] Add `RegisterParams` declaration to each effect header
- [ ] Add `RegisterParams` implementation to each effect .cpp (copy param IDs, bounds, and pointer offsets from PARAM_TABLE entries)
- [ ] Add `RegisterParams` calls in main.cpp registration site
- [ ] Remove migrated entries from PARAM_TABLE
- [ ] Build and verify param count matches (registered params before == registered params after)

## Pitfalls

- **Rotation bounds**: Use `ROTATION_SPEED_MAX` and `ROTATION_OFFSET_MAX` from `ui/ui_units.h`. Never inline `3.14159265f` literals. `#include "ui/ui_units.h"` in each `.cpp` that needs them.

## Notes

- Effects with 0 modulatable params (toon, radial_streak) get empty `RegisterParams` for API consistency
- DRAWABLE_FIELD_TABLE stays in param_registry.cpp (drawables are dynamic, not modules)
- Simulation params (physarum, boids, curl, attractors, cymatics, particle_life) stay in PARAM_TABLE until those systems become modules
- Feedback/flow params (flowField, feedbackFlow, proceduralWarp) stay in PARAM_TABLE
- Top-level params (effects.blurScale, effects.chromaticOffset, effects.motionScale) stay in PARAM_TABLE
