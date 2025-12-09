# Analysis Module

> Part of [AudioJones](../architecture.md)

## Purpose

Extracts spectral features from audio: 2048-point FFT magnitude spectrum and spectral flux beat detection.

## Files

- `src/analysis/fft.h` - FFT processor API
- `src/analysis/fft.cpp` - kiss_fftr implementation with Hann window
- `src/analysis/beat.h` - Beat detector API and struct
- `src/analysis/beat.cpp` - Spectral flux algorithm

## Function Reference

### FFT Processor

| Function | Purpose |
|----------|---------|
| `FFTProcessorInit` | Allocates FFT config, initializes Hann window |
| `FFTProcessorUninit` | Frees FFT config and processor |
| `FFTProcessorFeed` | Accumulates stereo samples as mono |
| `FFTProcessorUpdate` | Runs FFT when 2048 samples ready, returns true if updated |
| `FFTProcessorGetMagnitude` | Returns pointer to 1025-bin magnitude array |
| `FFTProcessorGetBinCount` | Returns 1025 |
| `FFTProcessorGetBinFrequency` | Converts bin index to Hz |

### Beat Detector

| Function | Purpose |
|----------|---------|
| `BeatDetectorInit` | Clears history buffers and state |
| `BeatDetectorProcess` | Computes spectral flux in bass bins, detects beat when flux > mean + sensitivity*stddev |
| `BeatDetectorGetBeat` | Returns true if beat detected this frame |
| `BeatDetectorGetIntensity` | Returns 0.0-1.0 intensity with exponential decay |

## Types

### BeatDetector

| Field | Type | Description |
|-------|------|-------------|
| `magnitude`, `prevMagnitude` | `float[1025]` | Current/previous spectra for flux |
| `fluxHistory` | `float[43]` | Rolling ~860ms flux window |
| `fluxAverage`, `fluxStdDev` | `float` | Statistics for threshold |
| `beatDetected` | `bool` | Beat this frame |
| `beatIntensity` | `float` | 0.0-1.0, decays after beat |
| `graphHistory` | `float[64]` | UI visualization buffer |

## Constants

| Constant | Value | Purpose |
|----------|-------|---------|
| `FFT_SIZE` | 2048 | FFT window size (43ms at 48kHz) |
| `FFT_BIN_COUNT` | 1025 | Magnitude bins (FFT_SIZE/2 + 1) |
| `BEAT_HISTORY_SIZE` | 43 | ~860ms rolling average at 20Hz |
| `BEAT_DEBOUNCE_SEC` | 0.15 | Minimum seconds between beats |

## Data Flow

1. **Entry:** `FFTProcessorFeed` receives stereo samples from audio module
2. **Transform:** 2048-point FFT with Hann window, 75% overlap
3. **Exit:** Magnitude spectrum to beat detector and spectrum bars
