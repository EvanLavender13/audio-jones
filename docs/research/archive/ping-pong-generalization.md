# Ping-Pong Render Pipeline Generalization

## Problem

The render pipeline in `src/render/render_pipeline.cpp` (line ~304-339) dispatches transform effects in a loop. Most effects use a single `RenderPass`. Generators with their own ping-pong feedback textures need a special three-step sequence:

1. Call `scratchSetup` to bind uniforms
2. Call the effect's custom `Render` function (ping-pong pass)
3. Call `RenderPass` with the blend compositor shader

Currently this is hardcoded as `else if` branches:

```cpp
} else if (effectType == TRANSFORM_ATTRACTOR_LINES_BLEND) {
    EFFECT_DESCRIPTORS[effectType].scratchSetup(pe);
    AttractorLinesEffectRender(...);
    RenderPass(pe, src, &pe->pingPong[writeIdx], *entry.shader, entry.setup);
} else if (effectType == TRANSFORM_FIREWORKS_BLEND) {
    EFFECT_DESCRIPTORS[effectType].scratchSetup(pe);
    FireworksEffectRender(...);
    RenderPass(pe, src, &pe->pingPong[writeIdx], *entry.shader, entry.setup);
}
```

Each new ping-pong generator adds another branch. This doesn't scale.

## Proposed Solution

Add a `render` function pointer to `EffectDescriptor`:

```
void (*render)(PostEffect *pe);  // NULL for most effects
```

When non-NULL, the pipeline calls it instead of the default `RenderPass` for the scratch pass. The function pointer wraps the effect-specific render call.

The dispatch loop becomes:

```cpp
if (descriptor.render != nullptr) {
    descriptor.scratchSetup(pe);
    descriptor.render(pe);
    RenderPass(pe, src, &pe->pingPong[writeIdx], *entry.shader, entry.setup);
}
```

This eliminates the per-effect `else if` branches entirely. The registration macro (`REGISTER_GENERATOR_FULL`) would gain a `RenderFn` parameter.

## Affected Files

- `src/config/effect_descriptor.h` — add `render` field to `EffectDescriptor`, update `REGISTER_GENERATOR_FULL` macro
- `src/render/render_pipeline.cpp` — replace special-case branches with `render != nullptr` check
- `src/effects/attractor_lines.cpp` — add static render wrapper
- `src/effects/fireworks.cpp` — add static render wrapper

## Scope

Small refactor, ~20 lines changed across 4 files. No functional change. Should be done when a third ping-pong generator is added, or as cleanup.
