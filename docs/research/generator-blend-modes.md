# Generator Blend Modes

Generators (constellation, plasma, interference) currently hardcode additive blending in their shaders. Simulations already composite via `BlendCompositor` with 16 selectable blend modes and per-effect intensity. This refactor gives generators the same treatment, enabling effects like colored backgrounds, subtractive overlays, and tinted composites.

## Classification

- **Type**: General (architecture refactor)
- **Pipeline Position**: Reorderable in the `TransformEffectType` transform chain (same as simulation trail boosts)
- **Chosen Approach**: Reuse existing BlendCompositor infrastructure + move generators into transform order

## Current State

### Generator compositing (hardcoded additive)

Each generator shader reads `texture0` (previous content) and blends internally:

```glsl
// constellation.fs:152-153
vec3 source = texture(texture0, fragTexCoord).rgb;
finalColor = vec4(source + constellation, 1.0);
```

Plasma and interference follow the same pattern. No blend mode choice, no intensity control.

### Simulation compositing (configurable via BlendCompositor)

Each simulation writes to its own `TrailMap` texture. The output stage composites trails onto the pipeline via `BlendCompositorApply()`, which binds `effect_blend.fs` with the simulation's texture, intensity, and blend mode:

- `effect_blend.fs` — 16 Photoshop-style blend modes (boost, screen, overlay, soft light, etc.)
- `EffectBlendMode` enum in `src/render/blend_mode.h`
- `BlendCompositor` in `src/render/blend_compositor.cpp`
- Each simulation config holds `EffectBlendMode blendMode` + `float boostIntensity`

## Proposed Change

### Generator shaders output only their generated content

Strip the input texture read and additive blend from each generator shader. Output raw generated pixels:

```glsl
// Before: source + constellation
// After:  just constellation
finalColor = vec4(constellation, 1.0);
```

### Generators become reorderable transform entries

Generators currently run at fixed positions before the transform loop in `RenderPipelineApplyOutput`. Simulation trail boosts are already `TransformEffectType` entries (e.g. `TRANSFORM_PHYSARUM_BOOST`) — fully reorderable in the user's transform chain.

Generators follow the same pattern:

1. Add `TRANSFORM_CONSTELLATION`, `TRANSFORM_PLASMA`, `TRANSFORM_INTERFERENCE`, `TRANSFORM_SOLID_COLOR` to the `TransformEffectType` enum
2. Remove hardcoded generator passes from `RenderPipelineApplyOutput`
3. Each generator's `GetTransformEffect` entry uses `BlendCompositor` shader + a setup function (same pattern as `SetupTrailBoost`)
4. Generator renders its content to a scratch texture, then `BlendCompositorApply` composites it onto the pipeline

This lets users place generators anywhere in the transform chain — before or after warps, symmetries, etc.

### Per-generator config additions

Each generator config gains two fields:

| Field | Type | Default | Purpose |
|-------|------|---------|---------|
| `blendMode` | `EffectBlendMode` | `EFFECT_BLEND_SCREEN` | Compositing mode from existing 16-mode enum |
| `blendIntensity` | `float` | `1.0` | Strength of composite (0.0 = no effect) |

Screen default matches the current additive-like behavior.

### Solid Color generator

A minimal new generator that outputs a flat color. Combined with blend modes, this replaces "configurable background color" without touching the feedback/decay system.

| Parameter | Type | Range | Default | Visual Effect |
|-----------|------|-------|---------|---------------|
| `color` | `vec3` | RGB 0-1 | `(1,1,1)` | Output color |

The shader is trivial — `finalColor = vec4(color, 1.0)`. The blend mode determines how it interacts with content: screen for light backgrounds, overlay for tinting, difference for inversion, etc.

### Render target for generator output

Simulation trail boosts don't need a scratch texture — they already have their `TrailMap` texture. Generators need somewhere to render their raw content before compositing. Options:

1. **Reuse existing ping-pong buffer** — the transform loop already ping-pongs; one buffer holds the raw generator output, `BlendCompositor` composites to the other
2. **Dedicated generator scratch texture** — cleaner separation but one more texture allocation

Option 1 fits the existing ping-pong flow with no new allocations. The two-step (render then composite) uses two ping-pong swaps instead of one, same as if two transforms ran back-to-back.

## Modulation Candidates

- **blendIntensity**: fading generators in/out
- **color** (solid color generator): R/G/B channels shift the background dynamically

## Effect-Modules Migration

This change aligns with Batch 10 of the effect-modules refactor (`docs/research/effect-modules.md`). When generators migrate to self-contained modules, each module's `Setup` function configures its own blend mode and intensity via `BlendCompositorApply`, matching the simulation trail boost pattern already in the transform dispatch.

## Notes

- Existing presets that use generators need migration: add default `blendMode = EFFECT_BLEND_SCREEN` and `blendIntensity = 1.0` on load if fields are missing
- Screen blend at intensity 1.0 closely approximates the current hardcoded additive behavior, so visual output stays nearly identical for existing presets
- The solid color generator combined with screen blend on white produces the "colored background" aesthetic from the original Shadertoy references — dark content on a light field

---

*To plan implementation: `/feature-plan docs/research/generator-blend-modes.md`*
