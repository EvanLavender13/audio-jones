# Effect Modules Migration

Refactor effects from monolithic PostEffect into self-contained modules.

**Research**: [docs/research/effect-modules.md](../research/effect-modules.md)

## Batches

- [x] **Batch 0**: Pilot (sine_warp) — validate pattern before mass migration
- [x] **Batch 1**: Warp (12 effects)
- [ ] **Batch 2**: Symmetry (8 effects)
- [ ] **Batch 3**: Cellular (3 effects)
- [ ] **Batch 4**: Motion (6 effects)
- [ ] **Batch 5**: Artistic (6 effects)
- [ ] **Batch 6**: Graphic (6 effects)
- [ ] **Batch 7**: Retro (5 effects)
- [ ] **Batch 8**: Optical (4 effects)
- [ ] **Batch 9**: Color (3 effects)
- [ ] **Batch 10**: Generators (3 effects)

## Notes

### Agent Pitfalls (Batch 1)

Parallel agents made these errors. Warn against in future batch prompts.

1. **Including deleted config headers**: Effect modules must define config inline. Don't include `config/*_config.h`—they get deleted.

2. **Wrong return type in PostEffectInit**: Returns `PostEffect*`, not `bool`. Use `free(pe); return NULL;` on failure.

3. **Stale includes in integration files**: `imgui_effects_*.cpp` and `preset.cpp` still included deleted headers. Grep for stale includes.

4. **Stale field refs in render_pipeline.cpp**: Time accumulators (`pe->waveRippleTime`, etc.) must be removed—module Setup handles this.

5. **Lissajous logic in render_pipeline.cpp**: Move into effect module's Setup function.

6. **Shader uniform name mismatch**: `chladni_warp.cpp` used `"mode"` but shader defines `warpMode`. Verify against actual .fs files.

### Prevention Checklist

- [ ] Effect headers define config inline (no config includes)
- [ ] `free(pe); return NULL;` not `return false;` in PostEffectInit
- [ ] Grep for stale config includes in touched files
- [ ] Remove time accumulators from render_pipeline.cpp
- [ ] Remove Lissajous computations from render_pipeline.cpp
- [ ] Verify uniform names against shader files
