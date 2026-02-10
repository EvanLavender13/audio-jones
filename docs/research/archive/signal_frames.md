# Signal Frames

Layered rotating geometric outlines — rectangles and triangles alternating in concentric rings, each mapped to an FFT semitone. A brightness sweep rolls through the layer stack like a strobe scanning into the screen. Active notes flare in their gradient LUT color; quiet notes dim to faint embers.

## Classification

- **Category**: GENERATORS
- **Pipeline Position**: Generator slot in output stage
- **Chosen Approach**: Full — variable octaves, dual-shape alternation, orbital controls with full-window spread, Lorentzian outline glow, FFT semitone coloring via LUT, phase-sweep strobe

## References

- [Inigo Quilez — 2D Distance Functions](https://iquilezles.org/articles/distfunctions2d/) — exact `sdBox` and `sdEquilateralTriangle` SDF implementations
- [Glow Shader in Shadertoy](https://inspirnathan.com/posts/65-glow-shader-in-shadertoy/) — inverse-distance glow technique with bounded falloff

## Algorithm

### SDF Functions

Box (rectangle with arbitrary aspect ratio):
```glsl
float sdBox(vec2 p, vec2 b) {
    vec2 d = abs(p) - b;
    return length(max(d, 0.0)) + min(max(d.x, d.y), 0.0);
}
```

Equilateral triangle (radius `r`):
```glsl
float sdEquilateralTriangle(vec2 p, float r) {
    const float k = sqrt(3.0);
    p.x = abs(p.x) - r;
    p.y = p.y + r / k;
    if (p.x + k * p.y > 0.0) p = vec2(p.x - k * p.y, -k * p.x - p.y) / 2.0;
    p.x -= clamp(p.x, -2.0 * r, 0.0);
    return -length(p) * sign(p.y);
}
```

### Layer Loop

Total layers = `numOctaves * 12`. Each layer `i` maps to one semitone. Normalized index `t = float(i) / float(totalLayers)` drives per-layer scaling.

Per layer:
1. **Rotation**: `uv * rotate(t * rotationSpeed * time + sin(time * orbitSpeed))`
2. **Orbital offset**: `uv += vec2(sin, cos)(time * orbitSpeed + t * TAU) * orbitRadius * t`
3. **Size**: `mix(sizeMin, sizeMax, t)` — inner layers smaller, outer layers larger
4. **Shape selection**: even layers use shape A (box), odd layers use shape B (triangle)
5. **SDF evaluation**: box uses `vec2(size * aspectRatio, size)`, triangle uses `size` as radius
6. **Outline extraction**: `abs(sdf) - outlineThickness`
7. **Glow**: Lorentzian profile `W * W / (W * W + d * d)` where `W = glowWidth`

### Sweep Strobe

A brightness pulse rolls through the layer stack over time:
```
sweepPhase = fract(time * sweepSpeed + t)
sweepBoost = sweepIntensity / (sweepPhase + 0.0001)
```
When `sweepPhase` approaches zero for a layer, that layer spikes bright. The offset `t` staggers the spike across layers, creating a rolling wave.

### Compositing

Per layer, final brightness = `glow * (baseBright + mag * gain^curve) * (1.0 + sweepBoost)`. Color sampled from `gradientLUT` at `vec2(pitchClass / 12.0, 0.5)`. All layers accumulate additively.

### FFT Semitone Mapping

Standard generator pattern:
```
freq = baseFreq * pow(2.0, float(semitone) / 12.0)
bin = freq / (sampleRate * 0.5)
mag = texture(fftTexture, vec2(bin, 0.5)).r
```

## Parameters

| Parameter | Type | Range | Default | Visual Effect |
|-----------|------|-------|---------|---------------|
| numOctaves | int | 1-5 | 3 | Layer count (12 per octave) |
| baseFreq | float | 27.5-440.0 | 55.0 | Lowest mapped frequency (Hz) |
| gain | float | 1.0-20.0 | 5.0 | FFT magnitude amplifier |
| curve | float | 0.5-3.0 | 1.5 | FFT contrast exponent |
| baseBright | float | 0.0-0.5 | 0.05 | Minimum ember brightness for quiet notes |
| rotationSpeed | float | -3.0-3.0 | 0.5 | Base rotation rate (rad/s), scaled per layer |
| orbitRadius | float | 0.0-1.5 | 0.3 | Orbital drift distance (0 = centered, 1+ = past screen edge) |
| orbitSpeed | float | 0.0-3.0 | 0.4 | Orbital drift rate |
| sizeMin | float | 0.01-0.5 | 0.05 | Innermost layer size |
| sizeMax | float | 0.1-1.5 | 0.6 | Outermost layer size |
| aspectRatio | float | 0.2-5.0 | 1.5 | Rectangle width/height ratio (box layers only) |
| outlineThickness | float | 0.002-0.05 | 0.01 | Stroke width of shape outlines |
| glowWidth | float | 0.001-0.05 | 0.005 | Lorentzian half-max distance (glow spread) |
| glowIntensity | float | 0.5-10.0 | 2.0 | Glow brightness multiplier |
| sweepSpeed | float | 0.0-3.0 | 0.5 | Phase roll rate through layer stack |
| sweepIntensity | float | 0.0-0.1 | 0.02 | Sweep pulse brightness numerator |

`ColorConfig gradient` — standard gradient LUT, default mode `COLOR_MODE_GRADIENT`.

## Modulation Candidates

- **rotationSpeed**: spin rate shifts with audio energy
- **orbitRadius**: shapes breathe inward/outward
- **sizeMin / sizeMax**: layer size range pulses
- **glowWidth**: glow tightens or blooms
- **glowIntensity**: overall brightness pumps
- **sweepSpeed**: strobe rate follows tempo
- **sweepIntensity**: sweep punch scales with dynamics
- **aspectRatio**: rectangles stretch/compress
- **outlineThickness**: strokes fatten on hits

### Interaction Patterns

- **Cascading threshold — sweep x FFT**: The sweep rolls through layers continuously, but only layers with active semitones flare visibly. During quiet passages the sweep passes through dim embers. During loud chords with many active notes, the sweep ignites a chain of bright flashes. The sweep gates which layers get attention; FFT magnitude controls how much they respond.
- **Competing forces — orbitRadius x sizeMax**: Large orbit pushes shapes toward screen edges while large size fills the window from center. Modulating both creates tension between scattered-distant and dense-central layouts — the visual density shifts with the balance.

## Notes

- Layer count caps at 60 (5 octaves x 12). At 60 layers the loop is the main cost; keep SDF math minimal per iteration.
- The Lorentzian glow is naturally bounded [0,1] — no clamp needed, no screen-wide bleed.
- `orbitRadius > 1.0` intentionally allows shapes to drift past screen edges for partial-visibility looks.
- Aspect ratio only affects box layers. Triangle layers use equilateral geometry (aspect = 1).
