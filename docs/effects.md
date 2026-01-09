# Effects Inventory

17 effects across 7 categories. 6 reorderable transforms; rest execute at fixed pipeline stages.

## Particle Simulations

| Effect | Description |
|--------|-------------|
| Physarum | Slime mold agent swarm, emergent patterns via diffusion/decay |
| Curl Flow | Perlin curl noise particle flow with self-feedback |
| Attractor Flow | 3D strange attractors (Lorenz, Rössler, Aizawa, Thomas) |

## Symmetry

| Effect | Description |
|--------|-------------|
| Kaleidoscope | Polar mirror, KIFS fractal, iterative rotation, hex lattice |

## Spatial Distortion

| Effect | Description |
|--------|-------------|
| Sine Warp | Cascading sine shifts with per-octave rotation |
| Texture Warp | Self-referential cascading distortion |
| Voronoi | Cell-based UV distort + 9 overlay effects |

## Zoom / Motion

| Effect | Description |
|--------|-------------|
| Infinite Zoom | Recursive fractal zoom with Droste spiral |
| Radial Blur | Radial motion blur toward center |
| Feedback | Flow field (zoom/rotation/translation) |

## Color

| Effect | Description |
|--------|-------------|
| Chromatic Aberration | RGB channel separation |
| Gamma | Display gamma correction |

## Sharpening / Smoothing

| Effect | Description |
|--------|-------------|
| Clarity | Unsharp mask local contrast |
| FXAA | Edge anti-aliasing |
| Blur H/V | Gaussian blur with decay |

## Compositing

| Effect | Description |
|--------|-------------|
| Trail Boost ×3 | Blend particle trails (Physarum, Curl, Attractor) |

---

## Pipeline Order

1. **Feedback stage** — Particle compute → Feedback shader → Blur
2. **Drawable stage** — Waveforms to accumulation texture
3. **Output stage** — Trail boost → Chromatic → Transforms (user order) → Clarity → FXAA → Gamma

Reorderable transforms (grouped by visual outcome in UI):
- **Symmetry**: Kaleidoscope
- **Warp**: Sine Warp, Texture Warp, Voronoi
- **Motion**: Infinite Zoom, Radial Blur
- **Experimental**: (empty, for future effects)
