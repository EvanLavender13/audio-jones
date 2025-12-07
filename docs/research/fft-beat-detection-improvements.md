# FFT Beat Detection Improvements

Research on improving beat detection for bass-heavy genres (EDM, dubstep, DnB).

## Problem Statement

Current implementation (`src/beat.cpp`) uses single-band spectral flux in bins 1-8 (23-188 Hz). This fails on bass-heavy tracks because:

1. **Sub-bass contamination**: Sustained sub-bass (20-60 Hz) adds constant flux, raising the average and masking transient kicks
2. **Single-band conflation**: Sub-bass, kick body, and toms sum together—a sustained 808 raises the baseline, obscuring kick transients
3. **Slow processing rate**: 21ms hop (1024 samples at 48kHz) smears rapid transients with ~5ms rise times
4. **No spectral compression**: Large magnitude differences between sustained bass and transient kicks make thresholding unreliable

## Current Implementation Parameters

| Parameter | Value | Limitation |
|-----------|-------|------------|
| FFT Size | 2048 samples (43ms) | Adequate |
| Hop Size | 1024 samples (21ms) | Too slow for transient detection |
| Frequency Range | Bins 1-8 (23-188 Hz) | Conflates sub-bass with kick |
| Threshold | `mean + N × stddev` | Static, requires manual tuning |
| Debounce | 150ms | Reasonable for kicks |

See `src/beat.cpp:97-111` for flux calculation, `src/beat.cpp:140` for threshold logic.

## Proposed Improvements

### 1. Multi-Band Spectral Flux

Split spectrum into independent bands with separate detection per band. Prevents sustained sub-bass from masking kick transients.

**Band definitions** (48kHz sample rate, 2048 FFT, 23.4 Hz/bin):

| Band | Bins | Frequency | Detects | Rationale |
|------|------|-----------|---------|-----------|
| Sub-bass | 1-2 | 23-70 Hz | Sustained 808s | Often sustained, not transient |
| Kick | 3-5 | 70-140 Hz | Kick drum body | Primary beat indicator |
| Low-mid | 6-8 | 140-210 Hz | Toms, bass harmonics | Transition zone |
| Attack | 85-170 | 2000-4000 Hz | Kick transient "click" | Arrives before body |

Each band maintains independent history, average, and standard deviation. Beat detection triggers on kick band flux exceeding its own threshold—unaffected by sub-bass energy.

**Source**: [Parallelcube Beat Detection](https://www.parallelcube.com/2018/03/30/beat-detection-algorithm/) documents 60-130 Hz for kick, 301-750 Hz for snare.

### 2. Logarithmic Compression

Apply µ-law style compression before flux calculation:

```cpp
float raw_mag = sqrtf(re * re + im * im);
float compressed_mag = logf(1.0f + 1000.0f * raw_mag);
```

Compresses dynamic range so quiet transients remain detectable relative to loud sustained bass. Compression factor γ = 1000 balances sensitivity vs. noise amplification.

**Source**: [OFAI Onset Detection Revisited](https://ofai.at/papers/oefai-tr-2006-12.pdf) recommends γ ≥ 1000 for onset detection.

### 3. SuperFlux Maximum Filter

Compare current magnitude against maximum of neighboring bins from previous frame:

```cpp
float prev_max = prevMagnitude[k];
if (k > 0) prev_max = fmaxf(prev_max, prevMagnitude[k-1]);
if (k < BEAT_SPECTRUM_SIZE-1) prev_max = fmaxf(prev_max, prevMagnitude[k+1]);
float diff = magnitude[k] - prev_max;
```

Suppresses false positives from vibrato and frequency modulation common in EDM synths. Reduces false detections by up to 60% on modulated sources.

**Source**: [CPJKU SuperFlux](https://github.com/CPJKU/SuperFlux) - Sebastian Böck, DAFx-13.

### 4. Increased Processing Rate

Reduce hop from 1024 to 512 samples:

| Hop Size | Frame Rate | Latency |
|----------|------------|---------|
| 1024 (current) | ~47 Hz | 21ms |
| 512 (proposed) | ~94 Hz | 10.7ms |
| 256 (aggressive) | ~187 Hz | 5.3ms |

EDM kick transients have 5-10ms rise times. Current 21ms hop averages across the attack. 512 samples (10.7ms) captures transient shape; 256 samples approaches transient resolution limit.

**Trade-off**: Higher processing rate increases CPU load proportionally.

### 5. Attack Transient Band

Kick drums have two components:

1. **Click/beater**: High frequency (2-4 kHz), very fast transient (~1ms), arrives first
2. **Body**: Low frequency (60-130 Hz), ~5-10ms after click

Adding a 2-4 kHz band detects the click before the body arrives, improving timing precision by 5-10ms. The attack band confirms kick presence when both attack and body bands trigger within 15ms.

**Source**: [DSP Stack Exchange - Onset Detection](https://dsp.stackexchange.com/questions/35608/musical-onset-detection-approach) documents multi-band transient detection.

## Implementation Complexity

| Phase | Complexity | Impact | Dependencies |
|-------|------------|--------|--------------|
| Multi-band | Medium | High | None |
| Log compression | Low | Medium | None |
| SuperFlux filter | Low | Medium | None |
| Processing rate | Low | Medium | None |
| Attack band | Medium | Low | Multi-band |

Recommended order: Multi-band first (highest impact), then log compression and SuperFlux (low complexity, cumulative benefit), then processing rate, finally attack band.

## References

- [Parallelcube Beat Detection](https://www.parallelcube.com/2018/03/30/beat-detection-algorithm/) - Sub-band energy, variance threshold
- [CPJKU SuperFlux](https://github.com/CPJKU/SuperFlux) - Maximum filter vibrato suppression
- [Librosa SuperFlux](https://librosa.org/doc/main/auto_examples/plot_superflux.html) - Python implementation reference
- [DSP Stack Exchange - Onset Detection](https://dsp.stackexchange.com/questions/35608/musical-onset-detection-approach) - Multi-band flux approach
- [OFAI Onset Detection Revisited](https://ofai.at/papers/oefai-tr-2006-12.pdf) - Logarithmic compression
- [ISMIR Real-Time Beat Tracking](https://transactions.ismir.net/articles/10.5334/tismir.189) - Recent advances
