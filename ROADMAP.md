# AudioJones Roadmap

Single source of truth for planned work.

## Current Focus

**Multi-Band Feature Extraction** - Extract bass, mid, treble energy bands with attack/release smoothing.

- [x] Phase 1.1: Add band energy extraction (bass/mid/treble RMS)
- [x] Phase 1.2: Implement dual attack/release smoothing
- [x] Phase 1.3: Integrate bands into AppContext and main loop
- [ ] Phase 1.4: Expose band energies to UI panel

Details: `docs/plans/audio-reactive-enhancements.md` and `docs/plans/multi-band-energy-extraction.md`

## Next Up

| Item | Complexity | Impact |
|------|------------|--------|
| Log compression for beat detection | Low | Medium |
| Spectral centroid extraction | Low | Medium |
| Visual parameter mappings (bass→scale, mid→saturation, treble→bloom) | Medium | High |
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
