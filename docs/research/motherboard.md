# Motherboard

Recursive abs-fold kaleidoscope that generates glowing circuit traces radiating from center. Each fold iteration creates a distinct geometric layer at decreasing scale — bass notes flare the broad outer structure while treble crackles through fine inner detail. Horizontal glow scanlines add a pulsing sheen across the traces. Replaces the existing Circuit Board warp effect.

## Classification

- **Category**: GENERATORS (blend)
- **Pipeline Position**: Output stage, generator slot (same as Plasma, Interference, Constellation)
- **Replaces**: Circuit Board (TRANSFORMS > Warp) — removes the warp effect entirely
- **Chosen Approach**: Balanced — full geometric control with depth-weighted semitone mapping. The depth-weighting adds minimal shader complexity (one weighted FFT sample per iteration) and delivers the core audio-reactive design.

## References

- User-provided Shadertoy shader "Funky Motherboard Carpet" — source algorithm for the abs-fold kaleidoscope with distance-based glow and cosine palette
- Dave Hoskins hash functions (hash13, hash31) — used in the original for per-cell randomization; not needed here since we skip the cell grid

## Algorithm

### Core Fold Loop

The effect centers on a single iterative loop that repeatedly folds, rotates, and offsets UV coordinates. Each iteration produces a distance field for one geometric layer.

**Coordinate setup**: Map fragment position to centered, aspect-corrected coordinates `p` in range [-2, 2].

**Per-iteration transform** (for `i = 0` to `iterations - 1`):
1. **Abs-fold**: `p = abs(p) - range * a` — mirrors coordinates across both axes, then offsets. `range` is a vec2 controlling X/Y fold spacing.
2. **Rotate**: `p *= rot(rotAngle)` — rotates by a configurable angle (default pi/4 = 45 degrees). This creates the orthogonal branching trace aesthetic.
3. **Y-fold**: `p.y = abs(p.y) - rangeY` — additional vertical mirror for interlocking patterns.
4. **Distance field**: `dist = max(abs(p.x) + a * sin(TAU * i / iterations), p.y - size)` — combines a sinusoidally-modulated horizontal bar with a vertical threshold. `size` controls trace thickness.
5. **Glow accumulation**: `color += smoothstep(thin, 0.0, dist) * glowIntensity / dist * brightness_i` — distance-inverse glow, gated by `brightness_i` (the per-iteration semitone energy).
6. **Scale decay**: `a /= fallOff` — each iteration operates at finer scale.

### Depth-Weighted Semitone Mapping

Each fold iteration maps to a semitone index (`i % 12`) and samples the FFT texture with an octave weight derived from iteration depth:

- **Semitone index**: `note = i % 12` (C, C#, D, ... B)
- **Octave center**: Linearly interpolate from lowest octave (iteration 0) to highest (iteration N-1). Early iterations favor bass octaves, later iterations favor treble.
- **Energy sample**: Sample the FFT texture at the semitone's frequency bin. Weight contributions from multiple octaves with a falloff centered on the iteration's preferred octave.
- **Brightness**: `brightness_i = baseBright + energy * gain` — baseline glow plus audio-reactive flare.

With 12 iterations, each maps cleanly to one semitone. With fewer, some semitones are skipped. With more than 12, semitones wrap (iteration 12 = C again, but at a finer geometric scale pulling from higher octaves).

### Color

Standard `ColorConfig` + `ColorLUT` integration. The LUT is sampled at a position derived from the accumulated fold distance or iteration index, yielding per-layer color variation.

### Glow Scanlines

Secondary horizontal glow layer: `color += scanlineIntensity / abs(sin(p.y * scanlineFreq + time * scanlineSpeed))`. Toggleable via `scanlineIntensity > 0`.

## Parameters

| Parameter | Type | Range | Default | Visual Effect |
|-----------|------|-------|---------|---------------|
| iterations | int | 4-16 | 12 | Number of fold layers. 12 = one per semitone. |
| rangeX | float | 0.1-1.0 | 0.5 | Horizontal fold spacing — wider = more spread |
| rangeY | float | 0.1-0.5 | 0.2 | Vertical fold offset — controls interlocking density |
| size | float | 0.01-0.3 | 0.1 | Trace thickness per iteration |
| fallOff | float | 1.01-2.0 | 1.15 | Scale decay per iteration. Low = layers overlap, high = rapid shrink |
| rotAngle | float | 0-pi | pi/4 | Rotation per fold. pi/4 = classic circuit traces |
| glowIntensity | float | 0.001-0.1 | 0.01 | Glow numerator — higher = brighter traces |
| thin | float | 0.01-0.3 | 0.1 | Smoothstep edge width — higher = softer glow falloff |
| gain | float | 0.1-10.0 | 2.0 | Multiplier on semitone energy before applying to brightness |
| curve | float | 0.5-4.0 | 1.5 | Exponent on energy — higher = sharper silence-to-loud response |
| baseBright | float | 0.0-1.0 | 0.15 | Baseline brightness when a semitone is silent |
| scanlineIntensity | float | 0.0-0.2 | 0.04 | Horizontal glow scanline strength. 0 = off. |
| scanlineFreq | float | 4.0-24.0 | 12.0 | Scanline spatial frequency |
| scanlineSpeed | float | 0.0-4.0 | 1.0 | Scanline scroll speed |
| color | ColorConfig | — | gradient | Standard generator color config (solid/rainbow/gradient/palette) |

## Modulation Candidates

- **rangeX / rangeY**: Shifts fold geometry — pattern breathes and morphs
- **fallOff**: Compresses or spreads the layer hierarchy
- **size**: Thickens/thins all traces simultaneously
- **rotAngle**: Rotates the fold axis — pattern twists and restructures
- **glowIntensity**: Overall brightness pulse
- **scanlineIntensity**: Scanline pulse on/off
- **gain**: Audio sensitivity — quiet passages dim, loud passages flare

### Interaction Patterns

- **Falloff x Semitone Gating (Competing Forces)**: Tight falloff packs all fold layers into similar scales, making semitone differentiation subtle — the layers blur together. Loose falloff separates layers into distinct geometric rings where each semitone's activation is visually isolated. Modulating falloff shifts between "dense unified carpet" and "layered frequency display."

- **Size x BaseBright (Cascading Threshold)**: When baseBright is low, thin traces (small size) vanish entirely for quiet semitones — only loud notes produce visible geometry. Increasing size lowers the visibility threshold, letting quieter notes show faint traces. Together they gate how much of the frequency spectrum produces visible output at any moment.

## Notes

- Replaces Circuit Board warp — the old effect's files (circuit_board.cpp/h, circuit_board.fs, shader setup, UI panel entries, config struct, param registration) must be removed or repurposed
- The cell grid from the original Shadertoy shader is omitted — all parameters apply uniformly across the full screen
- Hash functions (hash13, hash31) from the original are not needed since there's no per-cell randomization
- Performance: 12 iterations with distance-field glow is lightweight. The FFT texture sample per iteration adds negligible cost.
- The depth-weighted octave mapping requires passing `numOctaves` or octave range bounds as uniforms so the shader knows the frequency span to interpolate across
