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
| Particle Life | Colored species orbiting, chasing, and clustering like microscopic organisms competing for territory |

## TRANSFORMS

Reorderable effects with sub-categories:

### Symmetry

| Effect | Description |
|--------|-------------|
| Kaleidoscope | Symmetrical pattern radiating from center like looking through a toy kaleidoscope |
| KIFS | Recursive fractal mirroring that creates infinite zooming symmetry |
| Poincare Disk | Escher-like infinite tiling that curves toward the edges of a disk |
| Mandelbox | Box-and-sphere folding fractal with crystalline-yet-organic recursive patterns |
| Triangle Fold | Interlocking diamond weave like a kaleidoscopic textile |
| Moire Interference | Overlays rotated/scaled copies of the image to produce large-scale wave fringes like fabric grid patterns |
| Radial IFS | Branching snowflake/flower fractals radiating from center with twisting recursive arms |

### Warp

| Effect | Description |
|--------|-------------|
| Sine Warp | Layered wavy distortion that builds up like ripples |
| Texture Warp | Image warps itself in layers, creating fractal-like distortion |
| Wave Ripple | Circular ripples spreading outward like water drops |
| Mobius | Swirling spiral warp controlled by two draggable points |
| Gradient Flow | Smears pixels along or across edges for a streaky painted look |
| Chladni Warp | Warps image along vibrating-plate resonance patterns (sand on speaker) |
| Circuit Board | Folded circuit-trace maze patterns with optional chromatic RGB separation like a glitched PCB schematic |
| Domain Warp | Flowing organic distortion like melting terrain or swirling marble veins |
| Surface Warp | Rolling 3D hills where peaks stretch toward you and shaded valleys sink away like a billowing fabric surface |
| Interference Warp | Lattice-like distortion where overlapping wave patterns create shimmering quasicrystal geometries that slowly drift in and out of phase |
| Corridor Warp | Infinite hallway stretching to a vanishing point, floor and ceiling scrolling toward you like walking through a neon-lit tunnel |
| Shake | Vibrating handheld-camera blur where edges shimmer and jitter at controllable frequency |
| Radial Pulse | Pulsing rings and flower-petal waves radiating from center |

### Cellular

| Effect | Description |
|--------|-------------|
| Voronoi | Organic cell/bubble patterns with various shading modes |
| Lattice Fold | Tiles the image into repeating square or hexagonal grids |
| Phyllotaxis | Sunflower seed spiral cells with golden-angle arrangement and animated drift |
| Disco Ball | Spinning mirror ball throwing dancing light spots across the walls like a 70s dance floor |

### Motion

| Effect | Description |
|--------|-------------|
| Infinite Zoom | Endless zooming tunnel effect that loops seamlessly |
| Radial Blur | Radial motion blur toward center |
| Droste Zoom | Infinite recursive spiral zoom (like the Droste cocoa box) |
| Density Wave Spiral | Galaxy-style spiral arms that wind inward with differential rotation (inner spins faster) |

### Artistic

| Effect | Description |
|--------|-------------|
| Oil Paint | Thick impasto brush strokes layered along image contours, with glossy paint surface catching the light like a gallery oil painting |
| Watercolor | Soft ink washes bleeding along edges like wet-on-wet painting, where pigment pools in the valleys and paper grain shows through the thin layers |
| Impressionist | Thick circular brush dabs on a dark canvas that rebuild the image like a Monet painting, with visible stroke texture and darkened edges |
| Ink Wash | Sumi-e brush painting where ink pools dark along contours and bleeds softly into the surrounding paper grain |
| Pencil Sketch | Hand-drawn graphite shading on rough paper, where strokes follow the contours of the image like a life-drawing study |
| Cross-Hatching | Flickering ink hatching like a sketchbook come to life, with hand-drawn wobble |

### Graphic

| Effect | Description |
|--------|-------------|
| Toon | Cartoon cel-shading with bold black outlines and flat color bands |
| Neon Glow | Glowing colored outlines for a Tron-style wireframe look |
| Kuwahara | Flat posterized color regions with crisp edges, like a cell-shaded animation or stained glass window without the outlines |
| Halftone | Comic book/newspaper dot pattern like old print media |

### Retro

| Effect | Description |
|--------|-------------|
| Pixelation | Chunky retro pixels with optional color reduction and dithering |
| Glitch | Damaged VHS tape playing on a dying CRT: warped edges, horizontal tearing, color bleed, and blocky compression artifacts like a corrupted broadcast signal |
| ASCII Art | Renders image as text characters (# @ * .) based on brightness |
| Matrix Rain | Cascading green symbol streams like the Matrix digital rain, overlaid on the scene |
| LEGO Bricks | 3D-styled brick pixelation with raised studs and edge shadows, where adjacent similar colors merge into larger rectangular bricks like a toy construction set |

### Optical

| Effect | Description |
|--------|-------------|
| Bloom | Soft glow that bleeds outward from bright areas, like HDR photography or squinting at lights |
| Bokeh | Dreamy out-of-focus blur where bright spots become soft glowing circles like city lights at night |
| Heightfield Relief | Embossed 3D-looking surface lit from the side with shiny highlights |

### Color

| Effect | Description |
|--------|-------------|
| Color Grade | Full-spectrum color manipulation: hue, saturation, exposure, contrast, temperature, lift/gamma/gain |
| False Color | Heat-map coloring where dark-to-bright becomes a rainbow or custom gradient |
| Palette Quantization | Reduces colors to limited palette with Bayer dithering for retro/pixel-art look |

---

## Pipeline Order

1. **Feedback stage** — Particle compute → Flow Field shader → Blur
2. **Drawable stage** — Waveforms to accumulation texture
3. **Output stage** — Trail boost → Chromatic → Transforms (user order) → Clarity → FXAA → Gamma
