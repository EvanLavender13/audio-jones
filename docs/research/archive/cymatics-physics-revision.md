# Cymatics Physics Revision

Review and correction of the Cymatics generator's wave propagation model, trail accumulation, and contour visualization for physical accuracy.

## Classification

- **Category**: GENERATORS > Field
- **Pipeline Position**: Generator stage (section 13)
- **Existing files**: `src/effects/cymatics.h`, `src/effects/cymatics.cpp`, `shaders/cymatics.fs`

## References

- [Paul Bourke - Chladni Plate Interference Surfaces](https://paulbourke.net/geometry/chladni/) - Mathematical foundation for 2D wave interference and nodal patterns
- Cylindrical wave spreading law: amplitude ∝ 1/√r for 2D point sources (energy conservation over expanding wavefront circumference)

## Current Issues

### 1. Attenuation Model

Current: `exp(-r² * falloff)` (pure Gaussian). At default falloff=1.0, a source at r=2.0 is attenuated by exp(-4) ≈ 0.018 — effectively dead. Prevents meaningful long-range interference between sources.

Physically correct 2D point-source waves attenuate as 1/√r (cylindrical spreading from energy conservation). The Gaussian was an early design choice that limits the effect to localized ripples around each source.

### 2. Trail Blending

Current: `max(existing * decay, newColor)`. Brightest-ever value persists at each pixel. Creates burn-in where bright peaks never clear faster than the decay rate, accumulating into solid bright blobs rather than showing dynamic interference.

### 3. Contour Visualization

Current: `floor(totalWave * contourCount + 0.5) / contourCount`. Posterizes (quantizes) the wave value into flat bands. Does not produce actual contour lines — just stepped color regions.

### 4. Source Normalization

Current: raw sum across all sources. Changing sourceCount from 1 to 8 multiplies brightness by ~8x, coupling brightness to source count instead of treating them independently.

## Algorithm

### Attenuation: Physical Spreading + Gaussian Envelope

Replace the pure Gaussian with 1/√(r+ε) cylindrical spreading multiplied by a Gaussian envelope for adjustable range control:

```glsl
float epsilon = 0.01;
float spreading = 1.0 / sqrt(dist + epsilon); // physical 2D wave attenuation
float envelope = exp(-dist * dist * falloff);  // adjustable kill radius
totalWave += fetchWaveform(delay) * spreading * envelope;
```

- `epsilon` softens the 1/0 singularity at the source center (1/√0.01 = 10.0 max)
- `spreading` provides physically correct far-field behavior — waves carry across the screen
- `envelope` (existing `falloff` param) controls how far waves reach before the Gaussian kills them
- Same change applies to mirror sources in boundary reflections

### Trail Blending: Additive with Screen-Blend Capping

Replace `max()` with screen-blend accumulation:

```glsl
existing *= decayFactor;
finalColor = existing + newColor * (1.0 - existing);
```

- When buffer is empty (existing ≈ 0): full new energy, equivalent to pure additive
- When buffer is bright (existing ≈ 1): almost no new energy adds, natural saturation
- Combined with decay: quiet audio causes existing to decay, opening room for new energy
- Self-capping at 1.0 — never overflows, never darkens mid-range like Reinhard

### Source Normalization

Divide the accumulated wave sum by source count:

```glsl
totalWave /= float(sourceCount);
```

Keeps output range consistent regardless of source count. `visualGain` becomes the sole brightness control.

### Contour Modes

Add a `contourMode` parameter (0=off, 1=bands, 2=lines):

**Mode 1 — Bands (current posterization):**
```glsl
wave = floor(totalWave * float(contourCount) + 0.5) / float(contourCount);
```

**Mode 2 — Iso-amplitude lines:**
```glsl
float contoured = fract(abs(totalWave) * float(contourCount));
float lineWidth = 0.1;
wave = totalWave * smoothstep(0.0, lineWidth, contoured)
                 * smoothstep(lineWidth * 2.0, lineWidth, contoured);
```

Thin bright lines at each amplitude level, like a topographic map. Reveals interference structure more clearly than flat bands.

## Parameters

Changes to existing parameters:

| Parameter | Change | Notes |
|-----------|--------|-------|
| falloff | Rebalance range | Now controls Gaussian envelope radius, not sole attenuation. May need wider default range since 1/√r handles near-field |
| visualGain | Rebalance range | 1/√r produces more energy at distance; gain may need lower default |

New parameters:

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| contourMode | int | 0-2 | 0 | 0=off, 1=bands (posterize), 2=iso-lines (topographic) |

## Modulation Candidates

Existing params already registered. New `contourMode` is an int toggle — not a modulation target.

### Interaction Patterns

- **waveScale vs falloff (competing forces)**: With 1/√r spreading, waves carry far. `falloff` envelope limits reach. Low falloff + high waveScale = dense full-screen interference. High falloff chokes it to localized ripples. This tension was absent before when Gaussian alone killed everything at short range.
- **diffusion vs decay (resonance)**: High diffusion + low decay = soft expanding energy pools. High diffusion + high decay = brief glowing halos. Controls whether the medium feels like water, sand, or smoke.

## Notes

- The `epsilon` constant (0.01) gives a max multiplier of 10x at the source center. If this proves too hot, increase epsilon or add a per-source energy normalization.
- The screen-blend trail model assumes newColor is in [0,1] range. The tanh compression + abs(intensity) already ensures this, so no additional clamping needed on the new frame.
- Mirror source attenuation uses the same 1/√r + envelope model. The `reflectionGain` multiplier remains unchanged.
- All changes are in the shader (`cymatics.fs`) plus one new config field (`contourMode`) in the header. The C++ setup code needs the new uniform location cached and bound.
