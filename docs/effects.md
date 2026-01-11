# Effects Inventory

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
| Kaleidoscope | Polar wedge mirroring with optional smooth edges |
| KIFS | Kaleidoscopic IFS fractal folding via Cartesian abs() reflection |
| Poincare Disk | Hyperbolic tiling via Möbius translation and fundamental domain folding |

### Warp

| Effect | Description |
|--------|-------------|
| Sine Warp | Cascading sine shifts with per-octave rotation |
| Texture Warp | Self-referential cascading distortion |
| Wave Ripple | Radial sine wave UV displacement with height shading |
| Mobius | Two-point conformal warp with log-polar spiral animation |
| Gradient Flow | Sobel-based UV displacement along luminance edges |

### Cellular

| Effect | Description |
|--------|-------------|
| Voronoi | Cell-based UV distort + 9 overlay effects |
| Lattice Fold | Grid-based tiling symmetry (square, hexagon) |

### Motion

| Effect | Description |
|--------|-------------|
| Infinite Zoom | Recursive fractal zoom via layer blending |
| Radial Blur | Radial motion blur toward center |
| Droste Zoom | Conformal log-polar recursive zoom with multi-arm spirals |

### Style

| Effect | Description |
|--------|-------------|
| Pixelation | UV quantization mosaic with Bayer dithering and color posterization |
| Glitch | CRT barrel, analog noise, digital block displacement, VHS tracking bars |
| Toon | Luminance posterization with Sobel edge outlines and noise-varied brush strokes |
| Heightfield Relief | Embossed lighting from luminance gradients with specular highlights |
| Color Grade | Full-spectrum color manipulation: hue, saturation, exposure, contrast, temperature, lift/gamma/gain |
| ASCII Art | Luminance-based character rendering with 5x5 bit-packed glyphs and mono/CRT color modes |

---

## Pipeline Order

1. **Feedback stage** — Particle compute → Flow Field shader → Blur
2. **Drawable stage** — Waveforms to accumulation texture
3. **Output stage** — Trail boost → Chromatic → Transforms (user order) → Clarity → FXAA → Gamma
