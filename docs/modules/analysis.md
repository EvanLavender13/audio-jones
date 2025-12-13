# Analysis Module

> Part of [AudioJones](../architecture.md)

## Purpose

Extracts spectral features from audio: 2048-point FFT magnitude spectrum, spectral flux beat detection, and 3-band energy levels. The `AnalysisPipeline` struct coordinates these components and runs at frame rate for accurate beat detection.

## Files

- `src/analysis/analysis_pipeline.h` - Pipeline API, aggregates FFT/beat/bands
- `src/analysis/analysis_pipeline.cpp` - Coordinate audio draining, normalization, and analysis
- `src/analysis/fft.h` - FFT processor API
- `src/analysis/fft.cpp` - kiss_fftr implementation with Hann window
- `src/analysis/beat.h` - Beat detector API and struct
- `src/analysis/beat.cpp` - Spectral flux algorithm
- `src/analysis/bands.h` - Band energy extractor API and struct
- `src/analysis/bands.cpp` - RMS energy with attack/release smoothing

## Function Reference

### Analysis Pipeline

| Function | Purpose |
|----------|---------|
| `AnalysisPipelineInit` | Initializes FFT, beat detector, and band energies |
| `AnalysisPipelineUninit` | Frees FFT config |
| `AnalysisPipelineProcess` | Drains audio, normalizes, feeds FFT, updates beat/bands |

### FFT Processor

| Function | Purpose |
|----------|---------|
| `FFTProcessorInit` | Allocates FFT config, initializes Hann window |
| `FFTProcessorUninit` | Frees FFT config |
| `FFTProcessorFeed` | Accumulates stereo samples as mono, returns frames consumed |
| `FFTProcessorUpdate` | Runs FFT when 2048 samples ready, returns true if updated |

### Beat Detector

| Function | Purpose |
|----------|---------|
| `BeatDetectorInit` | Clears history buffers and state |
| `BeatDetectorProcess` | Computes spectral flux in kick bins (47-140Hz), detects beat when flux > mean + 2*stddev |
| `BeatDetectorGetBeat` | Returns true if beat detected this frame |
| `BeatDetectorGetIntensity` | Returns 0.0-1.0 intensity with exponential decay |

### Band Energies

| Function | Purpose |
|----------|---------|
| `BandEnergiesInit` | Clears all energy values to zero |
| `BandEnergiesProcess` | Computes RMS energy per band, applies attack/release smoothing |

## Types

### AnalysisPipeline

| Field | Type | Description |
|-------|------|-------------|
| `fft` | `FFTProcessor` | Embedded FFT processor |
| `beat` | `BeatDetector` | Embedded beat detector |
| `bands` | `BandEnergies` | Embedded band energy extractor |
| `audioBuffer` | `float[6144]` | Raw samples (3072 frames * 2 ch) |
| `peakLevel` | `float` | Tracked peak for volume-independent normalization |
| `lastFramesRead` | `uint32_t` | Frames read in last process call |

### BeatDetector

| Field | Type | Description |
|-------|------|-------------|
| `magnitude`, `prevMagnitude` | `float[1025]` | Current/previous spectra for flux |
| `fluxHistory` | `float[80]` | Rolling ~850ms flux window at 94Hz |
| `historyIndex` | `int` | Current position in circular flux buffer |
| `fluxAverage`, `fluxStdDev` | `float` | Statistics for threshold |
| `beatDetected` | `bool` | Beat this frame |
| `beatIntensity` | `float` | 0.0-1.0, decays after beat |
| `timeSinceLastBeat` | `float` | Seconds since last beat |
| `graphHistory` | `float[64]` | UI visualization buffer |
| `graphIndex` | `int` | Current position in graph buffer |

### BandEnergies

| Field | Type | Description |
|-------|------|-------------|
| `bass`, `mid`, `treb` | `float` | Raw RMS energy per band |
| `bassSmooth`, `midSmooth`, `trebSmooth` | `float` | Attack/release smoothed values |
| `bassAvg`, `midAvg`, `trebAvg` | `float` | Running averages for normalization |

## Constants

| Constant | Value | Purpose |
|----------|-------|---------|
| `FFT_SIZE` | 2048 | FFT window size (43ms at 48kHz) |
| `FFT_BIN_COUNT` | 1025 | Magnitude bins (FFT_SIZE/2 + 1) |
| `BEAT_HISTORY_SIZE` | 80 | ~850ms rolling average at 94Hz FFT rate |
| `BEAT_DEBOUNCE_SEC` | 0.15 | Minimum seconds between beats |
| `BAND_BASS_START/END` | 1, 10 | Bass bin range (20-250 Hz) |
| `BAND_MID_START/END` | 11, 170 | Mid bin range (250-4000 Hz) |
| `BAND_TREB_START/END` | 171, 853 | Treble bin range (4000-20000 Hz) |
| `BAND_ATTACK_TIME` | 0.010 | 10ms attack for transients |
| `BAND_RELEASE_TIME` | 0.150 | 150ms release to prevent jitter |

## Data Flow

1. **Entry:** `FFTProcessorFeed` receives stereo samples from audio module
2. **Transform:** 2048-point FFT with Hann window, 75% overlap
3. **Exit:** Magnitude spectrum to beat detector, spectrum bars, and band energies
