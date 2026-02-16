# Data Traffic

Horizontal lanes of rectangular data cells scrolling at different speeds — a flat 2D data highway where each cell's color maps to a frequency band and pulses with FFT energy. Quiet passages show dim grayscale filler drifting by; loud moments light up scattered constellations of color across the grid. Lane density ranges from a few sparse wires to a packed wall of activity.

## Classification

- **Category**: GENERATORS > Texture
- **Pipeline Position**: Generator stage (after trail boost, before transforms)
- **Blend**: Standard blend compositor (EFFECT_FLAG_BLEND)

## References

- User-provided Shadertoy shader "Data Traffic" — full source pasted in brainstorm session. Core techniques: epoch-based stateless animation, analytical AA via box coverage, hash-randomized cell layout.
- Existing codebase `shaders/scan_bars.fs` — LUT-to-frequency-bin pattern (lines 120-124): `t` drives both color and FFT lookup.

## Reference Code

### Box coverage (analytical AA from reference)

```glsl
// Returns fraction of pixel-sized box [pc-pw, pc+pw] overlapping interval [lo, hi]
float boxCov(float lo, float hi, float pc, float pw) {
    return clamp((min(hi, pc + pw) - max(lo, pc - pw)) / (2.0 * pw), 0.0, 1.0);
}
```

### Scan Bars frequency-from-LUT pattern (existing codebase)

```glsl
float binIdx = floor(t * float(freqBins));
float quantT = (binIdx + 0.5) / float(freqBins);
float freq = baseFreq * pow(maxFreq / baseFreq, quantT);
float bin = freq / (sampleRate * 0.5);
float mag = (bin <= 1.0) ? texture(fftTexture, vec2(bin, 0.5)).r : 0.0;
mag = pow(clamp(mag * gain, 0.0, 1.0), curve);
```

### Hash functions (from reference)

```glsl
float h11(float p) { p = fract(p * 443.8975); p *= p + 33.33; return fract(p * p); }
float h21(vec2 p) { float n = dot(p, vec2(127.1, 311.7)); return fract(sin(n) * 43758.5453); }
```

### Epoch pattern (from reference, simplified)

```glsl
// Discrete random value that changes every (1/rate) seconds, with progress within epoch
float epoch = floor(time * rate + seed);
float progress = fract(time * rate + seed);
float randomVal = h21(vec2(cellId + epoch * K, rowId));
```

## Algorithm

### Adaptation from reference

**Keep verbatim:**
- `boxCov()` function for analytical antialiasing of cell edges
- Hash functions `h11`, `h21` for deterministic randomness
- Epoch pattern (`floor`/`fract` of `time * rate + seed`) for stateless animation
- Evenly-spaced cell slots with hash-randomized widths

**Replace:**
- 3D perspective camera → flat 2D fullscreen (`fragTexCoord` directly)
- Hardcoded 6-color palette → `texture(gradientLUT, vec2(t, 0.5)).rgb`
- Time-based brightness epochs → FFT-driven brightness using scan bars pattern (`t → freq bin → magnitude`)
- 12+ animation behaviors (cradle, domino, fission, etc.) → simple scroll + epoch-based width variation + optional gentle jitter

**Strip entirely:**
- GlowCell struct and gaussian proximity glow pass
- Spark detection between adjacent cells
- Newton's cradle, domino cascade, fission, magnetic snap, ricochet, heartbeat, breathing, phase shift
- Row-level flash

### Core loop (per pixel)

1. Map `fragTexCoord.y` to lane index using `lanes` parameter. Compute lane gap for row separators.
2. Look up per-lane state: scroll speed (hash-based, some fast/slow/still, direction varies), cell base width, spacing.
3. Apply scroll offset: `x = fragTexCoord.x + time * laneSpeed`.
4. Compute cell index from `floor(x / spacing)`. For the current cell and neighbors (~3 cells), compute:
   - Cell center: `cellIdx * spacing`
   - Cell half-width: hash-randomized, epoch-based so widths shift over time with easing
   - LUT coordinate `t`: hash per cell → drives color AND frequency
5. Use `boxCov()` to compute pixel coverage against each cell rect (x-axis) and lane bounds (y-axis).
6. For colored cells: sample `gradientLUT` at `t`, look up FFT magnitude at frequency derived from `t`, compute `brightness = baseBright + mag`.
7. For grayscale filler cells (gated by `colorMix` parameter): dim fixed brightness, no FFT reactivity.
8. Composite: `cellColor * coverage * brightness`.

### Per-lane scroll animation

Each lane gets a hash-derived base speed and direction. Speed uses the epoch pattern with easing — lanes periodically accelerate, coast, and brake rather than scrolling at constant speed. Some lanes (hash gated, ~20%) pause entirely during an epoch, creating idle periods.

## Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| lanes | int | 4-60 | 20 | Number of horizontal lanes |
| cellWidth | float | 0.01-0.3 | 0.08 | Base cell width (before random variation) |
| spacing | float | 1.5-6.0 | 3.0 | Cell spacing multiplier (spacing = cellWidth * this) |
| scrollSpeed | float | 0.0-3.0 | 0.8 | Global scroll speed multiplier |
| widthVariation | float | 0.0-1.0 | 0.6 | How much cell widths vary from base (0 = uniform, 1 = full range) |
| colorMix | float | 0.0-1.0 | 0.5 | Fraction of cells that are colored/reactive vs grayscale filler |
| gapSize | float | 0.02-0.3 | 0.08 | Dark gap between lanes |
| jitter | float | 0.0-1.0 | 0.3 | Gentle positional jitter amplitude |
| changeRate | float | 0.05-0.5 | 0.15 | How often cell widths and lane speeds re-randomize |
| baseFreq | float | 27.5-440.0 | 55.0 | FFT low frequency bound (Hz) |
| maxFreq | float | 1000-16000 | 14000.0 | FFT high frequency bound (Hz) |
| gain | float | 0.1-10.0 | 2.0 | FFT magnitude amplification |
| curve | float | 0.1-3.0 | 1.0 | FFT magnitude contrast curve |
| baseBright | float | 0.0-1.0 | 0.05 | Minimum brightness for reactive cells (visible even when silent) |
| glowIntensity | float | 0.0-2.0 | 0.0 | Soft glow bleed from bright cells (0 = off, future enhancement) |

## Modulation Candidates

- **lanes**: density shifts create zooming-in/out feeling
- **scrollSpeed**: tempo-synced speed changes make traffic pulse with rhythm
- **colorMix**: modulating between sparse highlights and full color field
- **cellWidth**: thin slivers vs fat blocks shift the visual texture
- **gapSize**: tight-packed vs airy layout
- **jitter**: nervous energy vs calm flow
- **changeRate**: frenetic re-randomization vs slow evolution

### Interaction Patterns

**Cascading threshold (colorMix + FFT gain):** At low colorMix, only a few cells are colored — gain must be high for any visible reaction. At high colorMix, even quiet frequencies produce visible color. Modulating colorMix with a slow LFO creates verses (sparse, subtle) vs choruses (full reactive field).

**Competing forces (scrollSpeed + changeRate):** Fast scroll with slow changeRate = long stable streams. Slow scroll with fast changeRate = cells morphing in place like a flickering display. The tension between "moving through" and "changing in place" gives two distinct moods from the same effect.

## Notes

- No 3D camera, no perspective — pure 2D keeps it simple and performant
- Analytical AA via `boxCov` means crisp edges at any resolution without multisampling
- All animation is stateless (hash + time) — no ping-pong buffers needed
- `glowIntensity` parameter is reserved at 0.0 default for future enhancement (gaussian bleed from bright cells)
- The effect is entirely horizontal lanes — vertical or diagonal variants could be a future parameter but are out of scope
