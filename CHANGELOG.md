# Changelog

## 2026-03-21 — MARCHBLOB

### New Effects
- Protean Clouds generator — volumetric raymarched cloud flythrough with density/fog controls and FFT-driven brightness
- Subdivide generator — recursive BSP quad subdivision with Catmull-Rom interpolation and per-cell FFT coloring
- Cyber March generator — Mandelbox-family fractal fold explorer with DualLissajous camera and depth-mapped FFT
- Voxel March generator — raymarched voxelized sphere shells with domain folding and depth-dependent FFT
- Polyhedral Mirror generator — reflective platonic solid interiors with per-face FFT frequency mapping and wireframe edges
- Dream Fractal generator — carved-sphere IFS fractal with orbiting camera and per-pixel FFT frequency mapping
- Polymorph generator — morphing wireframe polyhedron cycling through all 5 platonic solids with FFT brightness
- Synapse Tree generator — fractal tree with electric synapse pulses, per-fold-depth FFT mapping
- Stripe Shift transform — RGB color bars displaced by input brightness for per-channel interference patterns

### Enhancements
- Surface Depth replaces Heightfield Relief with parallax displacement (Simple/POM), normal lighting, and fresnel rim glow
- Wave Drift replaces Sine Warp, moves to Motion category, adds selectable wave types (triangle, sine, sawtooth, square)
- Slit Scan renamed from Slit Scan Corridor, adds flat mode and center glow
- Ripple Tank gains spectral synthesis mode and wave shape selection (sine/triangle/sawtooth/square)
- Faraday gains full parametric mode with selectable wave shapes
- Signal Frames adds polygon SDF shape morphing across layers, fixes glow/rotation/sweep
- Mobius adds armCount and spiralOffset params, fixes defaults
- Vortex and Shell switched from trail buffer to standard generator scratch texture
- New ZONKTUNNEL preset
- Playlists directory included in release artifacts
- Lint toolchain: clang-tidy pre-commit hook, lint script with cppcheck and lizard

### Fixes
- Fix hex rush wall brightness using per-wall FFT sampling
- Fix duplicate ImGui ID for Save buttons in preset panel

## 2026-03-14 — SHARDNAUT

### New Effects
- Rainbow Road generator — neon bars receding in perspective, each mapped to an FFT frequency band; bass bold up front, treble shimmers in the distance (based on XorDev)
- Vortex generator — raymarched distorted hollow sphere with volumetric spirals and ping-pong trail buffer
- Shell generator — raymarched hollow sphere with per-step view rotation creating contour lines; turbulence distorts into organic nautilus forms
- Laser Dance generator — raymarched volumetric laser ribbons with Lissajous camera drift and position warp for organic filament distortion
- Digital Shard generator — noise-driven angular shards with per-cell gradient coloring and FFT reactivity
- Shard Crush transform — noise-rotated angular blocks with prismatic chromatic aberration
- Spectral Rings generator — FFT-reactive concentric rings with elliptical stretch and noise-texture coloring (based on XorDev)
- Triskelion generator — fractal tiled grid with iterated space-folding and concentric circle interference
- Light Medley generator — fly-through raymarcher with warped octahedral lattice and depth-mapped FFT glow
- Flip Book transform — stop-motion effect with reduced frame rate and hash-based UV jitter
- Twist Cage generator — nested wireframe platonic solids spiraling inward with Lissajous camera and FFT-reactive edges
- Neon Lattice generator — infinite torus lattice with neon glow and per-cell FFT reactivity (based on kishimisu)

### Enhancements
- Dots and bars waveform draw styles with averaged-sample rendering for discrete dot and bar modes
- Constellation: square point shape (Chebyshev glow) and per-point FFT reactivity
- Muons: additive volume color mode, axis feedback, color stretch, cosine turbulence, expanded march steps (Protostar 2-inspired)
- Multi-sample FFT (BAND_SAMPLES) across Nebula, Bit Crush, Data Traffic, Glyph Field, Scan Bars, and Spin Cage
- Scan Bars UI reorganized into Shape, Convergence, and Motion sections
- Preset folders with subdirectory browsing, breadcrumb navigation, and inline folder creation
- Preset playlist with drag-reorderable setlist, transport strip, arrow key navigation, and JSON persistence
- Playlist merged into Presets panel as inline section
- Lissajous modulation wired for Mobius and Wave Ripple
- Infinite zoom parallax reworked to be depth-proportional with new parallaxStrength parameter

### Fixes
- Gradient editor endpoint handles not responding to clicks
- Mobius/Wave Ripple lissajous amplitude and motionSpeed now modulatable (were passing NULL)
- Muons/Vortex colorPhase CPU-accumulated to prevent discontinuous jumps when adjusting slider

## 2026-03-07 — RIPPLORD

### New Effects
- Chladni generator — FFT-driven resonant plate eigenmode visualization with circular plate mode
- Faraday Waves generator — standing wave lattice with FFT-driven spatial scale layers
- Galaxy generator — spiral galaxy with elliptical rings, dust, stars, and Gaussian bulge
- Spin Cage generator — spinning platonic solid wireframes with FFT-reactive glow
- Spiral Walk generator — turtle-walk spiral with per-segment FFT glow
- Spark Flash generator — twinkling four-arm crosshair sparks with sine lifecycle
- Byzantine generator — reaction-diffusion texture with fractal zoom and gradient LUT coloring
- Perspective Tilt transform — 3D plane projection with pitch, yaw, roll, FOV
- Wave Warp transform — multi-mode wave distortion (triangle, sine, sawtooth, square) with fBM octaves

### Enhancements
- Ripple Tank (renamed from Cymatics) — revised wave physics with cylindrical spreading, merged Interference as dual wave source mode, contour visualization modes
- Infinite Zoom — per-layer UV warping, movable zoom center with lissajous, parallax offset, layer blend modes
- Halftone — upgraded to four-color CMYK with traditional screen angles and subtractive compositing
- Oil Paint — rewritten with flockaroo multi-scale brush stroke algorithm
- Pencil Sketch — rewritten with flockaroo notebook-drawings hatching algorithm
- Curl Advection — rewritten as fragment-shader generator (no longer requires OpenGL 4.3)
- Fireworks — rewritten with analytical ballistic physics, episode lifecycle, two burst shapes
- Hue Remap — animated rotation speed params for blend/shift spatial fields
- Effect Solo mode — isolate individual effects in the transform pipeline for tuning
- Reorderable accumulation composite — feedback texture is now a positioned, blend-controlled entry in the pipeline
- Loading screen with progress bar during startup
- Spectral feature mod sources now self-calibrate via running-average normalization
- New Cymatics category in effect browser for wave/resonance generators
- Effects sorted alphabetically within each UI category

### Fixes
- Preset saving strips modulation routes for disabled effects
- Fix additive/screen blend modes being dimmer than weighted average
- Fix Wave Warp wave type output range mismatch
- Fix lens space flat-color regions with spherical UV mapping
- Fix halftone shader coordinate centering
- Fix misleading glitch slice slider labels

## 2026-02-28 — FRAZZLE

### New Effects
- Hex Rush generator — Super Hexagon-inspired polar-coordinate walls rushing inward over colored wedges with FFT-driven brightness
- Fracture Grid warp — cellular tessellation that shatters the image into rect/hex/tri tiles with animated per-cell distortion
- Lens Space warp — recursive L(p,q) crystal-ball raymarching with morphing symmetry parameters
- Scrawl generator — IFS fractal fold with marker stroke rendering, 7x7 AA supersampling, and scanline background
- Prism Shatter generator — volumetric ray caster through a 3D sinusoidal color field with crystalline neon edges
- Risograph transform — CMY ink-layer separation with grain erosion, misregistration drift, and subtractive compositing over warm paper
- Woodblock transform — Ukiyo-e print shader with posterized flat colors, Sobel keyblock outlines, and directional wood grain

### Enhancements
- Muons: 7 distance function modes, 7 turbulence modes, phase & drift controls, trail persistence with FFT audio reactivity, winner-takes-all color mapping, trail blur slider
- Motherboard: rewritten with Kali inversion fractal and exp() glow, three discrete circuit modes with supersampled AA
- Scrawl: 7 fractal fold modes, per-cell rotation, continuous warp phase drift
- Prism Shatter: 6 IFS displacement modes (Absolute Fold, Mandelbox, Sierpinski, Menger, Burning Ship)
- Hex Rush: ring buffer for per-ring gapChance/patternSeed, modulatable wall params, configurable frequency bins, improved color cycling and FFT mapping
- Fracture Grid: waveShape (sine to hold-and-release) and spatialBias (random to coherent spatial grouping) params
- Glyph Field: reworked with step-based band compression, removed wave/LCD overlay
- Moiré: reworked interference with wave-displacement approach, 3 pattern modes (stripes/circles/grid), 4 wave profiles
- Voronoi/Phyllotaxis: simplified from 9 separate intensity floats to single mode + intensity selector
- Chromatic Aberration: extracted from output pipeline into standalone Optical transform with spectral multi-sample shader (3-24 samples) and falloff curve
- Categories: Artistic→Painterly, Graphic→Print, new Novelty category (Disco Ball, LEGO Bricks)
- Draw frequency: now modulatable via LFOs (converted from tick-based to float seconds)
- Colocated all effect and simulation UI into each module's own .cpp file with dispatch framework

### Fixes
- Fix Muons trail artifacts: clamp tanh overflow, blur trail buffer, raise minimum octaves
- Fix Hex Rush color cycling and radial FFT mapping
- Fix dot matrix shader scaling from screen center
- Fix category header glow cycle after Novelty insertion

## 2026-02-20 — SKRUNKLE

### New Effects
- Bit Crush generator — iterative lattice walk mosaic with FFT-driven per-cell brightness and gradient LUT coloring
- Iris Rings generator — concentric arc rings with FFT-driven arc gating, scrambled per-ring rotation, auto-scaling ring width, and perspective tilt
- Data Traffic generator — scrolling lane grid with FFT-driven colored cells, scroll angle, sparks, brightness variation epochs, breathing gap oscillation, proximity glow, per-cell twitch, split/merge, phase shift with spring wobble, and fission cell-divide behavior
- Fireworks generator — overlapping particle bursts with gravity, drag, sparkle, per-particle FFT reactivity, gradient LUT coloring, and ping-pong trail persistence
- Plaid generator — woven tartan fabric with golden-ratio quasi-random band widths, independent warp/weft morph phases, twill thread texture, and FFT-driven band glow
- Slit Scan Corridor generator — 2001-style stargate light tunnel via ping-pong accumulation with fog depth falloff, adjustable perspective wall spread, and configurable slit width
- Lattice Crush transform — chaotic staircase pixelation reusing the Bit Crush lattice walk algorithm as an image transform
- New preset: RINGER

### Enhancements
- All layer-based generators migrated from numOctaves to maxFreq frequency-spread pattern — frequency range now controlled directly in Hz instead of derived from octave count
- Arc strobe glow switched from Lorentzian to reciprocal for wider, more vibrant halos, and max layers raised to 256
- Filaments generator stripped of triangle-wave fBM noise and falloffExponent, switched to reciprocal glow matching arc strobe
- Tone warp baseBright param replaced with bassBoost and Contrast label renamed to Curve
- Walk mode parameter added to Bit Crush and Lattice Crush with 6 variants (Fixed Dir, Rotating Dir, Offset Neighbor, Alternating Snap, Cross-Coupled, Asymmetric Hash)
- Hue Remap gains a hue shift mode for direct hue rotation bypassing the gradient LUT
- Chua double-scroll attractor added to Attractor Lines and Attractor Flow
- Dadras attractor added to Attractor Flow simulation
- Attractor system parameters removed from modulation in Attractor Lines — chaotic params are too fragile for modulation
- Scan bars and glyph field Audio section moved to top of UI panel to match layout convention
- Ping-pong generator dispatch generalized via render function pointer for custom render passes

### Fixes
- FFT frequency lookups in layer-based generators now use band averaging instead of single-point sampling, fixing layers that went dark when layer count was low

## 2026-02-15 — SHIMMER

### New Effects
- Phi Blur — golden-ratio Vogel spiral sampling in rect and disc modes
- Hue Remap — color effect for remapping hue ranges
- Flux Warp — evolving cellular distortion using coupled oscillators with animated divisor, producing cells that split, merge, and restructure over time
- Attractor Lines generator — per-semitone particle traces where active notes glow bright while quiet semitones fade, revealing the attractor shape note-by-note as chords play

### Enhancements
- Shaped aperture kernels (disc, box, hex, star) added to Bokeh and Phi Blur with configurable star points, inner radius, and shape rotation
- Spatial masking added to Hue Remap with five independent modes: radial, angular, linear, luminance, and noise — each applied separately to blend intensity and hue shift
- Nebula noise type selector switching between kaliset and domain-warped FBM gas, with FBM dust lanes and audio-reactive diffraction cross spikes on bright stars
- Nebula kaliset iterations and FBM octaves split into independent controls so switching noise type no longer clobbers the other mode's settings
- Random walk motion mode added to ParametricTrail drawable with smoothness control, three boundary modes (clamp, wrap, drift-toward-center), and hash-driven wandering cursor
- Signal Frames reworked: full-spectrum log-space frequency spread across baseFreq-maxFreq, windowed Lorentzian glow to prevent white-hot blow-out, solid outlines, rotation/orbit bias sliders, and layers parameter (4-36)
- Configurable particle count (1-16) added to attractor lines for quality vs performance tradeoff
- Visual enabled/disabled state on effect section headers — dimmed accent bar and muted text when disabled
- Drawable radius modulation cap increased from 0.45 to 1.0
- Locked ImGui layout file so the default window arrangement is version-controlled and included in release zip

### Fixes
- Trail decay in attractor lines fixed using max() compositing instead of additive accumulation, scaled glow by 1/numParticles to prevent oversaturation
- Multi-scale grid scaling fixed from center instead of bottom-left
- Wrap boundary cross-screen lerp artifact fixed in ParametricTrail

## 2026-02-12 — BIGBANG

- 77 visual effects across 10 categories (artistic, cellular, color, generators, graphic, motion, optical, retro, symmetry, warp)
- 7 GPU simulations (physarum, boids, curl flow, curl advection, attractors, cymatics, particle life)
- Modulation engine with LFO routing to any parameter
- Real-time FFT audio analysis with beat detection and semitone tracking
- 21 presets
