# Anamorphic Streak

Horizontal light streaks extending from bright areas, simulating the optical artifact from anamorphic cinema lenses. Oval aperture elements stretch point light sources into smooth, continuous horizontal lines — from soft diffused glow to sharp defined streaks. Optional blue tint mimics the lens coating reflections characteristic of real anamorphic glass.

## Classification

- **Category**: TRANSFORMS > Optical
- **Pipeline Position**: Output stage, alongside Bloom (after transforms, before Clarity/FXAA)
- **Chosen Approach**: Mip-chain horizontal blur matching the existing Bloom architecture, based on Keijiro Takahashi's Kino/Streak technique

## Problem with Current Implementation

The current 3-tap iterative Kawase blur at full resolution produces visible "string of pearls" artifacts — discrete sample points along the streak instead of a continuous line. Each pass only samples center + 2 side points, leaving gaps the iteration count cannot fill.

## References

- [Kino/Streak Shader](https://github.com/keijiro/Kino/blob/master/Packages/jp.keijiro.kino.post-processing/Resources/Streak.shader) - Reference implementation: mip-chain horizontal downsample/upsample with tint
- [Kino/Streak Runtime](https://github.com/keijiro/Kino/blob/master/Packages/jp.keijiro.kino.post-processing/Runtime/Streak.cs) - Render pass structure: up to 16 mip levels, pyramid down then up
- [Anamorphic Lens Flares - Bart Wronski](https://bartwronski.com/2015/03/09/anamorphic-lens-flares-and-visual-effects/) - Core insight: squeeze blur buffers horizontally by 2:1, apply standard blur, stretch back
- [Custom Lens Flare - Froyok](https://www.froyok.fr/blog/2021-09-ue4-custom-lens-flare/) - Dual Kawase blur pyramid with threshold stabilization

## Algorithm

### 1. Prefilter (reuse existing)

Soft-knee threshold extraction, identical to current implementation and shared with Bloom.

### 2. Horizontal Downsample (mip chain)

Progressive horizontal-only downsample. Each mip level halves the width (height stays at half-screen from prefilter). Uses 6 horizontal taps with triangle-weighted kernel for smooth coverage:

```
Tap offsets (texels): -5, -3, -1, +1, +3, +5
Weights:               1,  2,  3,  3,  2,  1  (sum = 12)
```

This wide 6-tap kernel fills gaps between sample positions, eliminating the banding artifact. Each successive mip level doubles the effective reach because the texel size doubles.

### 3. Horizontal Upsample (pyramid reconstruction)

Walk back up the mip chain, blending each level with the one above. Uses 3 horizontal taps:

```
Tap offsets (texels): -1.5, 0, +1.5
Weights:               1/4, 1/2, 1/4
```

At each level, lerp between the high-res mip and the upsampled streak result using a `stretch` parameter — higher stretch favors the wider blur levels, extending the streak.

### 4. Composite

Additive blend with intensity and color tint:

```
streak_contribution = streak_color * tint * intensity
result = original + streak_contribution
```

## Structural Changes

The current `AnamorphicStreakEffect` struct lacks a mip chain. The rework mirrors `BloomEffect`:

- Replace single `blurShader` with `downsampleShader` + `upsampleShader`
- Add `RenderTexture2D mips[MIP_COUNT]` array (match bloom's 5 levels, or configurable)
- Add `AnamorphicStreakEffectResize()` for window resize handling
- Prefilter and composite shaders stay as-is

Render pass loop follows the same pattern as `BloomEffectSetup`:
1. Prefilter → mips[0]
2. For each level: downsample mips[i-1] → mips[i]
3. For each level (reverse): upsample mips[i] blended with mips[i-1]
4. Composite mips[0] onto scene

## Parameters

| Parameter | Type | Range | Default | Visual Effect |
|-----------|------|-------|---------|---------------|
| enabled | bool | - | false | Enable/disable effect |
| threshold | float | 0.0-2.0 | 0.8 | Brightness cutoff for streak activation |
| knee | float | 0.0-1.0 | 0.5 | Soft threshold falloff |
| intensity | float | 0.0-2.0 | 0.5 | Streak brightness in final composite |
| stretch | float | 0.0-1.0 | 0.8 | Upsample blend: favors wider blur levels at higher values |
| tint | vec3 | 0.0-1.0 per channel | (0.55, 0.65, 1.0) | Streak color tint — default blue mimics anamorphic coating |
| iterations | int | 3-7 | 5 | Mip chain depth, affects maximum streak width |

### Removed Parameters

- **sharpness**: No longer needed. The 6-tap downsample kernel produces smooth streaks inherently. Streak character now controlled by `stretch` (wide vs narrow) and `iterations` (max reach).

## Modulation Candidates

- **intensity**: Streak prominence pulses
- **stretch**: Streak width breathes in/out
- **threshold**: Sensitivity to bright areas shifts dynamically
- **tint**: Streak color drifts (e.g., blue to warm)

## Notes

- Shares prefilter shader with Bloom (identical threshold extraction logic)
- Mip chain mirrors Bloom's `RenderTexture2D mips[]` pattern — allocation, resize, and teardown follow the same structure
- Blue tint default `(0.55, 0.65, 1.0)` approximates Panavision C-series anamorphic coating reflections
- Performance: similar to Bloom (~5 down + 5 up passes at progressively smaller resolutions)
- The `stretch` parameter in Kino/Streak controls the lerp at each upsample level — at 0.0 the streak collapses to just the prefilter, at 1.0 the widest mip dominates
