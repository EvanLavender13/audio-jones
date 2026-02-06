# Moire Generator

Procedural moire pattern generator that overlays 2-4 high-frequency gratings with independent frequency, angle, speed, warp, scale, and phase per layer. The multiplicative superposition of nearly-matched gratings produces low-frequency interference fringes — visible "beat patterns" in 2D space. A global shape selector switches between stripe, circle, and grid base patterns. Final intensity remaps through a gradient LUT for color.

## Classification

- **Category**: GENERATORS (alongside Plasma, Interference, Constellation)
- **Pipeline Position**: Generator stage — after trail boost, before transforms
- **Chosen Approach**: Balanced — 3 shape modes, full per-layer params, LUT color, sharp/smooth toggle. Rich enough for expressive results without the complexity of AA gratings or displaced centers.

## References

- [Algorithmic Pattern Salon - Film Moire](https://alpaca.pubpub.org/pub/ni3sodf3/release/1) — GLSL implementations of line, circle, and polar moire generators with step-based gratings
- [Comfortably Numbered - Anti-Anti-Aliasing / Moire](https://hardmath123.github.io/moire.html) — Fourier/frequency-space explanation of why multiplicative superposition creates low-frequency phantoms
- [Observable - Graphene Moire Fragment Shader](https://observablehq.com/@pamacha/graphene-moire-fragment-shader) — Multi-layer hex lattice with cumulative rotation producing twisted-bilayer moire
- [Inigo Quilez - Filterable Procedurals](https://iquilezles.org/articles/filterableprocedurals/) — Analytically antialiased grid and checkerboard patterns (future enhancement reference)
- [EasyCalculation - Moire Fringe Spacing](https://www.easycalculation.com/analytical/parallel-pattern-moire-fringes.php) — Parallel grating fringe spacing formula
- [Britannica - Moire Pattern](https://www.britannica.com/science/moire-pattern) — Angled grating fringe spacing derivation
- [GPU Gems Ch.8 - Simulating Diffraction](https://developer.nvidia.com/gpugems/gpugems/part-i-natural-effects/chapter-8-simulating-diffraction) — Grating interference constructive condition formula
- User-provided Shadertoy references (5 shaders) — Layered stripe interference, hyperbolic grid, multi-frequency layers with warp, circle-dot noise moire, trig grid rotation

## Algorithm

### Core Principle

Two periodic patterns with spatial frequencies k1 and k2, when multiplied together, produce a beat pattern at the difference frequency:

```
cos(k1*x) * cos(k2*x) = 0.5 * cos((k1-k2)*x) + 0.5 * cos((k1+k2)*x)
```

The `(k1-k2)` term creates visible low-frequency fringes. The `(k1+k2)` term is too fine to resolve. This extends to 2D with angle offsets: two identical gratings rotated by angle `alpha` produce fringes spaced at `p / (2*sin(alpha/2))`.

### Base Pattern Functions

**Stripes** — 1D sinusoidal grating along one axis:
```
grating(coord, freq) = 0.5 + 0.5 * cos(2*PI * coord * freq)
```
Sharp variant replaces cosine with `step(0.5, fract(coord * freq))`.

**Circles** — radial distance grating from center:
```
r = length(uv - center)
grating(r, freq) = 0.5 + 0.5 * cos(2*PI * r * freq)
```
Produces concentric rings. Two layers with different frequencies create radial beat fringes.

**Grid** — 2D lattice (product of two orthogonal stripe gratings):
```
gridGrating(uv, freq) = grating(uv.x, freq) * grating(uv.y, freq)
```
Two overlaid grids at different angles produce a textile-like 2D moire.

### Per-Layer Transform Pipeline

Each layer applies transforms in this order:

1. **Scale**: `uv = (uv - 0.5) * scale + 0.5`
2. **Rotation**: `uv = rotate2d(angle + speed * time) * (uv - 0.5) + 0.5`
3. **Warp**: `uv.x += warp * sin(uv.y * 5.0 + time * 2.0 + phase)`
4. **Grating**: compute pattern value from transformed UV
5. **Phase offset**: add `phase` to the grating's inner argument

### Layer Combination

All layers multiply together, then normalize to prevent excessive darkening:

```
result = layer0 * layer1 * ... * layerN
result = pow(result, 1.0 / numLayers)
```

### Sharp vs Smooth Toggle

- **Smooth**: `0.5 + 0.5 * cos(2*PI * coord * freq)` — soft sinusoidal, classic analog moire
- **Sharp**: `step(threshold, fract(coord * freq))` — hard square-wave, high-contrast fringes with richer harmonics that produce more complex interference

### Gradient LUT Color Remap

The monochrome interference intensity (0-1) indexes into a 1D gradient texture. The gradient editor (already in the codebase for False Color) provides the color mapping. A `colorIntensity` parameter blends between grayscale and LUT-colored output.

## Parameters

### Global

| Parameter | Type | Range | Default | Visual Effect |
|-----------|------|-------|---------|---------------|
| patternMode | enum | stripes/circles/grid | stripes | Base pattern shape for all layers |
| layerCount | int | 2-4 | 3 | Number of overlaid gratings |
| sharpMode | bool | on/off | off | Square-wave vs sinusoidal gratings |
| colorIntensity | float | 0-1 | 0.0 | Blend between grayscale and LUT color |
| globalBrightness | float | 0-2 | 1.0 | Overall output brightness |
| blendWithScene | float | 0-1 | 0.5 | Mix ratio between generator output and scene content |

### Per-Layer (x4 max)

| Parameter | Type | Range | Default | Visual Effect |
|-----------|------|-------|---------|---------------|
| frequency | float | 1-100 | 20/22/24/26 | Grating line density; small differences between layers create wide fringes |
| angle | float | 0-360 deg | 0/5/10/15 | Static rotation offset; even 1-2 deg difference produces dramatic fringes |
| rotationSpeed | float | -180-180 deg/s | 0 | Continuous rotation rate; layers drifting at different speeds create flowing fringes |
| warpAmount | float | 0-0.5 | 0.0 | Sinusoidal UV distortion amplitude; adds organic waviness to straight gratings |
| scale | float | 0.5-4.0 | 1.0 | Zoom level; scale differences create expanding/contracting fringes from center |
| phase | float | 0-360 deg | 0 | Wave phase offset; shifts grating position without changing frequency |

Default per-layer values stagger slightly (freq 20/22/24/26, angle 0/5/10/15) so the effect produces visible moire immediately.

## Modulation Candidates

- **frequency**: Small frequency changes create dramatic fringe width shifts. Modulating layer 1 frequency while others stay fixed produces pulsing beat patterns.
- **angle**: Modulating rotation offset causes fringes to sweep across the screen. Very sensitive — 1 degree of change produces large fringe movement.
- **rotationSpeed**: Modulating creates acceleration/deceleration of fringe drift.
- **warpAmount**: Modulating adds/removes organic waviness to the interference pattern.
- **scale**: Modulating creates breathing/pulsing fringes expanding from center.
- **phase**: Modulating slides the pattern smoothly without changing its structure.
- **colorIntensity**: Modulating fades between mono and colored output.

## Notes

### Visual Characteristics

- Stripe mode: classic fabric/screen-door moire with parallel fringe bands
- Circle mode: radial beat rings emanating from center, hyperbolic curves when layers offset
- Grid mode: 2D textile-like moire with diamond and checkerboard interference patterns
- Sharp mode amplifies harmonics, producing richer/noisier interference than smooth mode
- The effect is most striking with high frequencies (30+) and small parameter differences between layers

### Performance

- Each layer requires one grating evaluation per pixel (trig ops: sin/cos or step/fract)
- 4 layers at 1080p: ~4 trig calls per pixel, negligible GPU cost
- No texture reads required — purely procedural

### Relationship to Existing Effects

- **Different from Moire Interference (transform)**: That effect overlays rotated copies of the *input texture*. This generator creates patterns from scratch.
- **Different from Interference (generator)**: Existing Interference uses concentric ripples from drifting point sources. This uses flat/radial gratings with controllable frequency and multiplicative combination.
- **Complementary with feedback**: Running the moire generator output through Blur feedback creates evolving organic patterns as fringes accumulate and smear.
