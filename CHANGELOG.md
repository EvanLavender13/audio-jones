# Changelog

## 2026-02-20 — SKRUNKLE

- 7 new effects: Plaid, Slit Scan Corridor, Lattice Crush, Fireworks, Data Traffic, Iris Rings, and Bit Crush
- 2 new strange attractors (Chua double-scroll, Dadras) added to attractor systems
- Walk mode system for Bit Crush and Lattice Crush with 6 directional variants
- Data Traffic cell behaviors: row flash, breathing, fission, brightness variation, proximity glow
- 9 generators migrated from octave count to explicit frequency range (baseFreq/maxFreq)
- Hue shift mode for Hue Remap, new RINGER preset

## 2026-02-15 — SHIMMER

- 3 new effects: Flux Warp (jagged shimmering fringe distortion), Hue Remap (custom color wheel), Phi Blur (golden-ratio soft blur)
- Hue Remap spatial masking with 5 modes: radial, angular, linear, luminance, and noise
- Nebula overhaul: noise selector, dust lanes, diffraction spikes, and separate kaliset/FBM controls
- Signal Frames rework with full-spectrum frequency spread, solid outlines, and bias controls
- Shaped aperture kernels (rect, disc) for Bokeh and Phi Blur
- Locked ImGui layout file for consistent UI positioning out of the box
- Random walk motion mode for trail drawables, visual enabled/disabled indicators on effect section headers

## 2026-02-12 — BIGBANG

- 77 visual effects across 10 categories (artistic, cellular, color, generators, graphic, motion, optical, retro, symmetry, warp)
- 7 GPU simulations (physarum, boids, curl flow, curl advection, attractors, cymatics, particle life)
- Modulation engine with LFO routing to any parameter
- Real-time FFT audio analysis with beat detection and semitone tracking
- 21 presets
