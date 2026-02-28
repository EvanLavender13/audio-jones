# Changelog

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
