# Effects Inventory

22 effects organized by UI panel groups. 11 reorderable transforms; rest execute at fixed pipeline stages.

## FEEDBACK

| Effect | Description |
|--------|-------------|
| Blur | Gaussian blur with decay (half-life control) |
| Flow Field | Zoom/rotation/translation feedback with radial falloff |

## OUTPUT

| Effect | Description |
|--------|-------------|
| Chromatic Aberration | RGB channel separation |
| Gamma | Display gamma correction |
| Clarity | Unsharp mask local contrast |
| FXAA | Edge anti-aliasing |

## SIMULATIONS

| Effect | Description |
|--------|-------------|
| Physarum | Slime mold agent swarm, emergent patterns via diffusion/decay |
| Curl Flow | Perlin curl noise particle flow with self-feedback |
| Attractor Flow | 3D strange attractors (Lorenz, Rössler, Aizawa, Thomas) |

## TRANSFORMS

Reorderable effects with sub-categories:

### Symmetry

| Effect | Description |
|--------|-------------|
| Kaleidoscope | Polar mirror, KIFS fractal, iterative rotation, hex lattice |
| Poincare Disk | Hyperbolic tiling via Möbius translation and fundamental domain folding |

### Warp

| Effect | Description |
|--------|-------------|
| Sine Warp | Cascading sine shifts with per-octave rotation |
| Texture Warp | Self-referential cascading distortion |
| Wave Ripple | Radial sine wave UV displacement with height shading |
| Voronoi | Cell-based UV distort + 9 overlay effects |
| Mobius | Two-point conformal warp with log-polar spiral animation |

### Motion

| Effect | Description |
|--------|-------------|
| Infinite Zoom | Recursive fractal zoom with Droste spiral |
| Radial Blur | Radial motion blur toward center |

### Style

| Effect | Description |
|--------|-------------|
| Pixelation | UV quantization mosaic with Bayer dithering and color posterization |
| Glitch | CRT barrel, analog noise, digital block displacement, VHS tracking bars |

---

## Pipeline Order

1. **Feedback stage** — Particle compute → Flow Field shader → Blur
2. **Drawable stage** — Waveforms to accumulation texture
3. **Output stage** — Trail boost → Chromatic → Transforms (user order) → Clarity → FXAA → Gamma
