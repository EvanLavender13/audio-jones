# AudioJones Roadmap

Single source of truth for planned work.

## Current Focus

**Directory Reorganization** - Restructure `src/` from flat layout into domain-based subdirectories with consistent naming.

- [x] Phase 0: Extract RenderContext to shared header
- [x] Phase 1: Rename files (spectral→fft, spectrum→spectrum_bars, visualizer→post_effect)
- [ ] Phase 2: Create directories, move files (audio/, analysis/, render/, config/)
- [ ] Phase 3: Update include paths and symbols
- [ ] Phase 4: Update CMakeLists.txt
- [ ] Phase 5: Verify build
- [ ] Phase 6: Run /sync-architecture

Details: `docs/plans/directory-reorganization.md`

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

- UI Modularization (src/ui/ directory with per-panel modules, ui_main orchestrator)
- FFT-based beat detection
- Chromatic aberration (beat-reactive)
- Bloom pulse effect
- Preset save/load (JSON)
- Spectrum Bars (SpectralProcessor extraction, ColorConfig, accordion UI, preset integration)
