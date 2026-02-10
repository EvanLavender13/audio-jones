# Glyph Field Stutter Mask

Enhancement to the existing Glyph Field generator. Adds per-lane freeze/resume toggling that randomly gates scroll motion, plus a discrete quantization slider that blends smooth scroll into whole-cell snapping. Creates a nervous data-stream stutter inspired by Ryoji Ikeda-style cascading text displays.

## Classification

- **Category**: GENERATORS > Texture (existing effect enhancement)
- **Pipeline Position**: Same as current Glyph Field — generator pass
- **Scope**: 3 new config parameters, shader logic additions to existing scroll section

## References

- Leon Denise, "Decodering" (Shadertoy, 2023-10-29) — source of the column stutter mask technique. Key pattern: per-column binary gate via `step(0.5, sin(floor(time * hash(col))))` combined with `floor(time * speed)` discrete cell-snapping.

## Algorithm

### Stutter Mask

For each scroll lane (row in horizontal, column in vertical, ring in radial):

1. Hash the lane index to get a per-lane toggle rate
2. Discretize time by that rate: `floor(stutterTime * laneHash)`
3. Convert to binary: `step(0.5, sin(discretizedTime))`
4. Blend with stutterAmount: `mask = mix(1.0, binaryGate, stutterAmount)`

When mask = 0, the lane's scroll offset freezes. When mask = 1, normal scroll applies.

### Discrete Quantization

After computing the scroll offset for a lane:

1. Compute the quantized (cell-snapped) offset: `floor(offset * gs) / gs`
2. Blend between smooth and quantized: `mix(smoothOffset, quantizedOffset, stutterDiscrete)`

This replaces the current raw scroll offset in each of the three direction branches.

### Integration Points

The stutter mask and discretization insert into the existing scroll section of `glyph_field.fs` (lines 109-135). Each scroll direction branch already computes a scroll offset per lane — the mask multiplies that offset, and the discrete blend wraps it before it modifies `cellCoord`/`localUV`.

## Parameters

| Parameter | Type | Range | Default | Visual Effect |
|-----------|------|-------|---------|---------------|
| stutterAmount | float | 0.0-1.0 | 0.0 | Fraction of lanes frozen at any moment (0 = no stutter) |
| stutterSpeed | float | 0.1-5.0 | 1.0 | Rate of freeze/unfreeze toggling per lane |
| stutterDiscrete | float | 0.0-1.0 | 0.0 | Blend from smooth scroll to whole-cell snapping |

`stutterSpeed` drives a CPU-accumulated `stutterTime` like the existing scroll/flutter/wave accumulators.

## Modulation Candidates

- **stutterAmount**: Controls how many lanes freeze — low values create occasional pauses, high values make most lanes static with sparse scrollers
- **stutterSpeed**: Controls nervous energy — fast toggling feels glitchy, slow toggling feels deliberate
- **stutterDiscrete**: Controls snap aggression — modulating creates moments of smooth flow breaking into staccato

### Interaction Patterns

- **Competing forces: stutterAmount vs scrollSpeed** — high stutter freezes most lanes while high scroll speed makes the few active lanes race. The visual balance shifts between stillness and frantic motion.
- **Cascading threshold: stutterAmount gates flutter visibility** — when a lane is positionally frozen (stutter=high), character flutter becomes the only visible animation in that lane. Stutter must be active before flutter's spatial contrast (frozen-but-cycling vs scrolling-and-cycling) emerges.

## Notes

- `stutterAmount = 0.0` produces identical output to current glyph field — fully backward compatible
- The mask uses `sin(floor(...))` rather than a smooth transition, so lane state changes are instantaneous binary flips
- Discrete quantization at 1.0 with fast scroll can produce visible popping — this is intentional (matches the Decodering aesthetic)
- The stutter time accumulator needs the same CPU-side `+= speed * dt` pattern as existing accumulators to avoid jumps when speed changes mid-frame
