# Effects Inventory

## FEEDBACK

| Effect | Description |
|--------|-------------|
| Blur | Gaussian blur with decay (half-life control) |
| Flow Field | Zooming/spinning tunnel effect with movable center and wavy distortion |
| Feedback Flow | Smears content along brightness edges for a painted/melted look |

## OUTPUT

| Effect | Description |
|--------|-------------|
| Chromatic Aberration | RGB channel separation |
| Gamma | Display gamma correction |
| Clarity | Sharpens and enhances local detail and texture |
| FXAA | Edge anti-aliasing |

## SIMULATIONS

| Effect | Description |
|--------|-------------|
| Physarum | Slime mold simulation where agents leave trails and form organic networks |
| Curl Flow | Particles follow swirling smoke-like paths that evolve over time |
| Curl Advection | Flowing vein-like patterns that organically evolve and branch |
| Attractor Flow | Chaotic butterfly-effect particle paths (Lorenz, Rössler, etc.) |
| Boids | Flocking bird/fish swarm that moves together in coordinated groups |
| Cymatics | Vibrating plate patterns like sand on a speaker responding to audio |

## TRANSFORMS

Reorderable effects with sub-categories:

### Symmetry

| Effect | Description |
|--------|-------------|
| Kaleidoscope | Classic kaleidoscope wedge mirroring with adjustable segment count |
| KIFS | Recursive fractal mirroring that creates infinite zooming symmetry |
| Poincare Disk | Escher-like infinite tiling that curves toward the edges of a disk |
| Radial Pulse | Pulsing rings and flower-petal waves radiating from center |
| Mandelbox | Box-and-sphere folding fractal with crystalline-yet-organic recursive patterns |
| Triangle Fold | Triangular hall-of-mirrors that tessellates into honeycomb-like recursive patterns |
| Moire Interference | Overlays rotated/scaled copies of the image to produce large-scale wave fringes like fabric grid patterns |

### Warp

| Effect | Description |
|--------|-------------|
| Sine Warp | Layered wavy distortion that builds up like ripples |
| Texture Warp | Image warps itself in layers, creating fractal-like distortion |
| Wave Ripple | Circular ripples spreading outward like water drops |
| Mobius | Swirling spiral warp controlled by two draggable points |
| Gradient Flow | Smears pixels along or across edges for a streaky painted look |
| Chladni Warp | Warps image along vibrating-plate resonance patterns (sand on speaker) |
| Domain Warp | Flowing organic distortion like melting terrain or swirling marble veins |
| Phyllotaxis Warp | Swirling spiral arms like galaxy rotation or water draining in a whirlpool |

### Cellular

| Effect | Description |
|--------|-------------|
| Voronoi | Organic cell/bubble patterns with various shading modes |
| Lattice Fold | Tiles the image into repeating square or hexagonal grids |
| Phyllotaxis | Sunflower seed spiral cells with golden-angle arrangement and animated drift |

### Motion

| Effect | Description |
|--------|-------------|
| Infinite Zoom | Endless zooming tunnel effect that loops seamlessly |
| Radial Blur | Radial motion blur toward center |
| Droste Zoom | Infinite recursive spiral zoom (like the Droste cocoa box) |
| Density Wave Spiral | Galaxy-style spiral arms that wind inward with differential rotation (inner spins faster) |

### Style

| Effect | Description |
|--------|-------------|
| Pixelation | Chunky retro pixels with optional color reduction and dithering |
| Glitch | CRT barrel, analog noise, digital block displacement, VHS tracking bars |
| Toon | Cartoon cel-shading with bold black outlines and flat color bands |
| Heightfield Relief | Embossed 3D-looking surface lit from the side with shiny highlights |
| ASCII Art | Renders image as text characters (# @ * .) based on brightness |
| Oil Paint | Smooth painterly look that flattens areas while keeping sharp edges |
| Watercolor | Edge darkening, paper granulation, and color bleeding for watercolor simulation |
| Neon Glow | Glowing colored outlines for a Tron-style wireframe look |
| Cross-Hatching | Flickering ink hatching like a sketchbook come to life, with hand-drawn wobble |
| Bokeh | Dreamy out-of-focus blur where bright spots become soft glowing circles like city lights at night |
| Bloom | Soft glow that bleeds outward from bright areas, like HDR photography or squinting at lights |
| Pencil Sketch | Graphite hatching strokes that follow object edges, with paper texture and subtle wobble animation |
| Matrix Rain | Falling columns of procedural rune glyphs with bright leading tips and fading green trails |

### Color

| Effect | Description |
|--------|-------------|
| Color Grade | Full-spectrum color manipulation: hue, saturation, exposure, contrast, temperature, lift/gamma/gain |
| False Color | Heat-map coloring where dark-to-bright becomes a rainbow or custom gradient |
| Halftone | Comic book/newspaper dot pattern like old print media |
| Palette Quantization | Reduces colors to limited palette with Bayer dithering for retro/pixel-art look |

---

## Pipeline Order

1. **Feedback stage** — Particle compute → Flow Field shader → Blur
2. **Drawable stage** — Waveforms to accumulation texture
3. **Output stage** — Trail boost → Chromatic → Transforms (user order) → Clarity → FXAA → Gamma
