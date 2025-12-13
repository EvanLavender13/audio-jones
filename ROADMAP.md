# AudioJones Roadmap

Single source of truth for planned work.

## Current Focus

**Robust Beat Detection** - Reduce false positives from vocals, improve transient detection.

- [x] Narrow frequency bins to 47-70 Hz (kick drum range)
- [ ] Add log compression for volume-independent thresholds
- [x] Remove sensitivity parameter, use fixed 2.0 stddev

Research: `docs/research/robust-beat-detection.md`

## Next Up

| Item | Complexity | Impact |
|------|------------|--------|
| WaveformPipeline extraction | Low | Medium |
| Visual parameter mappings | Medium | High |
| Spectral centroid extraction | Low | Medium |
| GPU audio texture pipeline | Medium | High |

Plan: `docs/plans/waveform-pipeline-extraction.md`

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
- Audio/visual update separation (94Hz FFT analysis, 20Hz waveform visuals)
- Input normalization (volume-independent analysis)
