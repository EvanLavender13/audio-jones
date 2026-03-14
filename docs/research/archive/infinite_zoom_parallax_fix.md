# Infinite Zoom Parallax Fix

Fixes the parallax offset in infinite zoom to be depth-proportional instead of index-based. Currently each layer shifts by `offset * layerIndex`, which has no relationship to the layer's current zoom depth. The fix ties the offset to `1/scale`, so near layers (small scale) shift more and far layers (large scale) shift less — matching how real parallax works.

## Classification

- **Category**: TRANSFORMS > Motion (existing effect modification)
- **Pipeline Position**: Transform stage (same slot, no change)
- **Scope**: `infinite_zoom.fs`, `infinite_zoom.h`, `infinite_zoom.cpp`

## References

- Existing codebase: `shaders/infinite_zoom.fs` line 59 (current broken formula)
- Standard parallax: offset ∝ 1/distance. In this shader, `scale = exp2(phase * zoomDepth)` is the distance proxy.

## Reference Code

### Current (broken)

```glsl
// Line 59 — offset scales with layer INDEX, not depth
vec2 layerCenter = center + offset * float(i);

// Lines 65-68 — phase/scale computed AFTER layerCenter
float phase = fract((float(i) - time) / L);
float scale = exp2(phase * zoomDepth);
float alpha = (1.0 - cos(phase * TWO_PI)) * 0.5;
```

### Fixed

```glsl
// Compute phase/scale/alpha FIRST
float phase = fract((float(i) - time) / L);
float scale = exp2(phase * zoomDepth);
float alpha = (1.0 - cos(phase * TWO_PI)) * 0.5;

// Early-out on negligible contribution (unchanged, same position)
if (alpha < 0.001) continue;

// Depth-proportional parallax: near layers (small scale) shift more
vec2 layerCenter = center + offset * (parallaxStrength / scale);
```

## Algorithm

### Shader changes (infinite_zoom.fs)

1. Add `uniform float parallaxStrength;`
2. Reorder the loop body: move `phase`, `scale`, `alpha` computation and the `alpha < 0.001` early-out to **before** the `layerCenter` calculation
3. Replace `offset * float(i)` with `offset * (parallaxStrength / scale)`

The `1/scale` relationship means:
- `scale = 1` (nearest layer, phase ≈ 0): full offset magnitude
- `scale = exp2(zoomDepth)` (farthest layer): offset shrinks exponentially
- With `zoomDepth = 3.0` (default), far layers get 1/8th the offset of near layers

`parallaxStrength` multiplies the depth-proportional term. At 0 → no parallax. At 1 → the offset value directly controls the near-layer shift. Higher values exaggerate the depth separation.

### CPU changes (infinite_zoom.h, infinite_zoom.cpp)

**Config**: Add `float parallaxStrength = 1.0f; // Depth-proportional parallax multiplier (0.0-5.0)`

**Effect struct**: Add `int parallaxStrengthLoc;`

**Init**: Cache `GetShaderLocation(e->shader, "parallaxStrength")`

**Setup**: Set the uniform.

**RegisterParams**: Register `"infiniteZoom.parallaxStrength"` with range 0.0-5.0.

**CONFIG_FIELDS macro**: Add `parallaxStrength` to the field list.

**UI**: Add `ModulatableSlider("Parallax Strength##infzoom", ...)` in the Parallax section, before the offset sliders.

## Parameters

### New

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| `parallaxStrength` | float | 0.0-5.0 | 1.0 | Multiplier for depth-proportional parallax. 0 = no parallax, 1 = natural, >1 = exaggerated |

### Modified behavior (existing params)

| Parameter | Before | After |
|-----------|--------|-------|
| `offsetX` | Shifts per layer index | Shifts per depth, scaled by 1/scale |
| `offsetY` | Shifts per layer index | Shifts per depth, scaled by 1/scale |

## Modulation Candidates

- **parallaxStrength**: Controls depth exaggeration. Low = flat, high = dramatic separation. Responds well to bass for beat-synced depth pulses.

### Interaction Patterns

- **Competing forces: parallaxStrength vs zoomDepth** — Higher zoomDepth increases the scale range between layers (1 to exp2(zoomDepth)), which makes the parallax ratio between near/far more extreme. parallaxStrength controls the absolute magnitude. Modulating both creates a push-pull between how deep the tunnel feels and how much the layers separate.

## Notes

- The reordering of phase/scale/alpha before layerCenter is the only structural change to the loop. All subsequent code (aspect correction, rotation, warp, sampling) remains identical.
- The offset range (-0.1 to 0.1) may need widening since the effective offset is now divided by scale. With zoomDepth=3, the far layer only sees offset/8. Current range should still be fine for subtle parallax; if users want dramatic separation, the parallaxStrength multiplier handles it.
- The lissajous modulation on offset still works identically — it just feeds into the depth-proportional formula instead of the index-based one.
