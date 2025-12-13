# AudioJones Roadmap

Single source of truth for planned work.

## Current Focus

(none)

## Backlog

- Visual parameter mappings
- Spectral centroid extraction
- GPU audio texture pipeline
- Replace std:: file I/O with raylib in preset.cpp

## Far Future

- **Multi-pass bloom** - 6-level downsample chain replaces 5-tap blur, requires 6 render textures and 12 shader passes
- **Trailing particles** - Emit from waveform peaks, requires particle system
- **Physarum agents** - Agent-based simulation, requires compute shaders
- **Metaballs** - SDF rendering from waveform peaks

## Completed

- Kaleidoscope mode (polar mirroring into radial segments)
- Feedback trail controls (zoom, rotation, desaturation with luminance-matched dark fade)
- Fractal feedback (recursive zoom with 2% inward motion + slow rotation per frame)
- WaveformPipeline extraction (waveform buffers and processing moved from AppContext to dedicated module)
- Robust beat detection (47-70 Hz kick range, fixed 2.0 stddev threshold)
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
