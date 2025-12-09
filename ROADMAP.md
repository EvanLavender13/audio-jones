# AudioJones Roadmap

Single source of truth for planned work.

## Current Focus

**UI Modularization** - Restructure `src/ui.cpp` into `src/ui/` directory with per-panel modules.

- [x] Phase 1: Create directory, extract ui_common and ui_color
- [x] Phase 2: Extract individual panels (effects, audio, spectrum, waveform)
- [x] Phase 3: Create ui_main orchestrator, delete original files
- [ ] Phase 4: Run /sync-architecture, verify presets and dropdowns

Details: `docs/research/ui-modularization-plan.md`

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
- Spectrum Bars (SpectralProcessor extraction, ColorConfig, accordion UI, preset integration)
