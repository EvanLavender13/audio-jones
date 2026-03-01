# Reorderable Accum Blend

The accumulated feedback+drawable texture becomes a positioned, blend-controlled entry in the reorderable transform pipeline list instead of being the implicit starting point. The output chain starts from a black canvas, and the accum texture blends in at the user's chosen position with a selectable blend mode and intensity.

## Classification

- **Category**: General / Architecture
- **Pipeline Position**: Inside the output transform loop (replaces hardcoded `src = &pe->accumTexture` start)
- **Scope**: Render pipeline dispatch, config/serialization, pipeline list UI

## References

- Internal: existing `BlendCompositor` pattern used by generators and sim boosts
- Internal: `TransformOrderConfig` reordering and serialization system

## Algorithm

### Current pipeline

```
src = pe->accumTexture
for each transform in order:
    apply transform(src) → pingPong → swap src
```

The accumTexture is always the implicit base image. Transforms process everything — drawables, feedback trails, and generators — in a fixed order.

### New pipeline

```
clear pingPong[0] to black
src = &pe->pingPong[0]
writeIdx = 1

for each entry in order:
    if entry == TRANSFORM_ACCUM_COMPOSITE:
        BlendCompositorApply(bc, pe->accumTexture.texture, accumIntensity, accumBlendMode)
        RenderPass(src → pingPong[writeIdx]) using blend compositor shader
        swap src
    else:
        (existing transform/generator/sim-boost dispatch logic)
```

### Key changes

1. **Output chain starts black**: `RenderPipelineApplyOutput()` clears `pingPong[0]` and starts `src` from it instead of from `accumTexture`.

2. **New enum entry**: Add `TRANSFORM_ACCUM_COMPOSITE` to `TransformEffectType`. It uses the existing `BlendCompositor` shader — no new shader needed. Follows the sim-boost single-pass pattern (no scratch pass).

3. **Always enabled**: `IsTransformEnabled()` always returns `true` for this entry. The pipeline list always shows it. No enable checkbox.

4. **Default position: first**: The `TransformOrderConfig` constructor places `TRANSFORM_ACCUM_COMPOSITE` at index 0 (before all other effects). Combined with the default blend (Mix at 1.0), this produces identical visuals to the current pipeline.

5. **Backward compatibility**: `TransformOrderFromJson` must prepend the accum entry (not append) when it's missing from a saved preset. This ensures old presets that don't include it get the accum entry first, preserving current visual behavior. All other unseen entries still append as before.

6. **Config fields**: Add `accumBlendMode` and `accumBlendIntensity` to `EffectConfig` (top-level, not in a sub-config struct). Serialize alongside the existing top-level fields (`halfLife`, `gamma`, etc.).

7. **Setup function**: A new `SetupAccumComposite(PostEffect* pe)` calls `BlendCompositorApply(pe->blendCompositor, pe->accumTexture.texture, pe->effects.accumBlendIntensity, pe->effects.accumBlendMode)`. Located in `shader_setup.cpp` alongside other setup functions.

8. **Descriptor entry**: Register via `EffectDescriptorRegister()` (manual registration, like bloom) since it needs a custom `getShader` returning the blend compositor shader and has no init/uninit.

9. **Pipeline list UI**: The accum entry appears in the drag-and-drop list with a distinct badge (e.g., `"TRL"` for Trails) and a unique section color. It participates in drag-and-drop reordering. No enable checkbox. Its blend controls (mode combo + intensity slider) could render inline below the pipeline list or in a small dedicated section.

10. **Feedback loop unchanged**: Drawables still render into `accumTexture`. The feedback stage still reads/writes `accumTexture`. Only the output chain dispatch changes.

## Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| accumBlendMode | EffectBlendMode | enum | BLEND_MODE_MIX | How the accum texture composites onto the scene |
| accumBlendIntensity | float | 0.0-1.0 | 1.0 | Opacity of the accum texture blend |

## Modulation Candidates

- **accumBlendIntensity**: Fading the feedback/drawable layer in and out — lets generators or sim-boost content peek through during quiet sections, with drawables dominating during loud passages.

## Notes

- **Blend mode awareness**: Mix at 1.0 replaces everything before it. If a user positions generators before the accum entry with Mix mode, those generators become invisible. This is expected — the user would switch to Screen, Overlay, or another compositing mode to see both layers.
- **No new shader**: Reuses the existing `effect_blend.fs` via `BlendCompositor`. No fragment shader work needed.
- **No `EFFECT_FLAG_BLEND`**: The accum entry should NOT use the generator two-pass path (scratch + blend). It's a single blend pass — the accumTexture is already rendered. Use a direct single `RenderPass` with the blend compositor, similar to sim boosts.
- **TransformOrderConfig size**: `TRANSFORM_EFFECT_COUNT` increases by 1. All arrays sized by it (`order[]`, `g_effectSectionOpen[]`) adjust automatically.
- **Category section**: The accum entry does not belong to any effect category section (Symmetry, Warp, etc.). It should have no collapsible category section in the effects panel. Its UI controls live elsewhere (inline in the pipeline list area or in the feedback panel).
