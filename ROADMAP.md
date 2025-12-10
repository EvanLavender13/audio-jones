# AudioJones Roadmap

Single source of truth for planned work.

## Current Focus

**Visual Parameter Mappings** - Map band energies to visual parameters for audio-reactive effects.

- [ ] Phase 1: Bass → waveform scale
- [ ] Phase 2: Mid → color saturation
- [ ] Phase 3: Treble → bloom intensity

## Next Up

| Item | Complexity | Impact |
|------|------------|--------|
| Log compression for beat detection | Low | Medium |
| Spectral centroid extraction | Low | Medium |
| GPU audio texture pipeline | Medium | High |

## Backlog

- Replace std:: file I/O with raylib in preset.cpp
- Lissajous overlay (stereo XY plot)
- Kaleidoscope mode (polar mirroring)
- Fractal feedback (recursive zoom)

## Far Future

- **Multi-pass bloom** - 6-level downsample chain replaces 5-tap blur, requires 6 render textures and 12 shader passes
- **Trailing particles** - Emit from waveform peaks, requires particle system
- **Physarum agents** - Agent-based simulation, requires compute shaders
- **Metaballs** - SDF rendering from waveform peaks

## Completed

- Directory reorganization (domain-based subdirectories with consistent naming)
- UI modularization (src/ui/ directory with per-panel modules)
- FFT-based beat detection
- Chromatic aberration (beat-reactive)
- Bloom pulse effect
- Preset save/load (JSON)
- Spectrum bars (SpectralProcessor extraction, ColorConfig, accordion UI)
- Multi-band feature extraction (bass/mid/treble RMS, attack/release smoothing, UI panel)
