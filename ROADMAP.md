# AudioJones Roadmap

Single source of truth for planned work.

## Current Focus

**Spectrum Bars** - Multi-phase feature adding radial spectrum visualization.

- [x] Phase 1: Extract SpectralProcessor from BeatDetector
- [ ] Phase 2: ColorConfig extraction + SpectrumBars renderer
- [ ] Phase 3: Accordion UI + preset integration

Details: `docs/research/visual-types-refactoring-plan.md`

## Next Up

### Beat Detection Improvements

Prioritized fixes:

| Item | Complexity | Impact |
|------|------------|--------|
| Multi-band spectral flux | Medium | High |
| Logarithmic magnitude compression | Low | Medium |
| SuperFlux maximum filter | Low | Medium |

Details: `docs/research/fft-beat-detection-improvements.md`

### Post-Processing Effects

Low-effort visual enhancements using existing pipeline:

- [ ] Vignette pulse (beat-reactive edge darkening)
- [ ] Scanlines/CRT mode
- [ ] Color grading LUTs

## Backlog

Unordered ideas. Pull into "Next Up" when prioritized.

- Replace std:: file I/O with raylib in preset.cpp
- Lissajous overlay (stereo XY plot)
- Kaleidoscope mode (polar mirroring)
- Fractal feedback (recursive zoom)

## Far Future

Major features requiring significant architecture work.

- **Trailing particles** - Emit from waveform peaks, needs particle system
- **Physarum agents** - Agent-based simulation, needs compute shaders
- **Metaballs** - SDF rendering from waveform peaks
- **Spectral rings** - Concentric frequency-reactive rings

Details: `docs/research/audio-reactive-patterns.md`

## Completed

- FFT-based beat detection
- Chromatic aberration (beat-reactive)
- Bloom pulse effect
- Preset save/load (JSON)
- SpectralProcessor extraction (Phase 1)
