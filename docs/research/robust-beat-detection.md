# Robust Beat Detection Research

Research for improving beat detection: fewer false positives (voices, noise), fewer missed beats, no manual tuning.

## Current Implementation Problems

| Issue | Current | Problem |
|-------|---------|---------|
| Frequency range | 23-188 Hz (bins 1-8) | Voice fundamentals (100+ Hz) trigger false positives |
| Magnitude scaling | Linear | Quiet sections become oversensitive |
| Vibrato handling | None | Sustained notes cause false triggers |
| Threshold | Mean + sensitivity×stddev | Requires manual tuning, outlier-sensitive |
| Sensitivity | User parameter (1.0-3.0) | Shouldn't need tuning |

## Research Findings

### 1. Narrower Kick Frequency Range

Kick drums live in 40-80 Hz, not 23-188 Hz.

- Kick fundamental: 40-100 Hz ([EDMProd](https://www.edmprod.com/eq-kick-drums/))
- Sub bass: 50-60 Hz ([Gearspace](https://gearspace.com/board/electronic-music-instruments-and-electronic-music-production/643375-what-perfect-sub-frequency-support-kick-drum-52-hz-48-hz-50-hz.html))
- Beat detection implementations use 60-130 Hz ([Parallelcube](https://www.parallelcube.com/2018/03/30/beat-detection-algorithm/))

Male voice fundamentals start around 85-180 Hz. By staying below 80 Hz, we exclude most voice content.

**At 48kHz with 2048 FFT (23.4 Hz/bin):**
- Bin 2: ~47 Hz
- Bin 3: ~70 Hz
- Bin 4: ~94 Hz

**Recommendation**: Use bins 2-3 (~47-70 Hz) for pure kick detection.

### 2. Log Compression

Standard in all professional implementations. Formula:

```
Y = log(1 + γ·X)
```

Where γ (gamma) controls compression strength. Typical values:
- γ = 100: Strong compression (librosa, SuperFlux default)
- γ = 1: Mild compression (madmom)

**Why it helps:**
- Compresses dynamic range so loud beats don't dominate
- Enhances transient visibility
- Makes threshold work across volume levels

Source: [AudioLabs Erlangen](https://www.audiolabs-erlangen.de/resources/MIR/FMP/C3/C3S1_LogCompression.html)

### 3. Maximum Filter (SuperFlux)

The key innovation from Böck & Widmer's SuperFlux algorithm.

**How it works:**
1. For each frequency bin k, compute the maximum of bins {k-1, k, k+1} from the previous frame
2. Calculate flux as difference from this maximum (not from exact previous bin)
3. This suppresses gradual changes from vibrato/tremolo

```cpp
// Instead of:
diff = magnitude[k] - prevMagnitude[k];

// Use:
maxPrev = max(prevMagnitude[k-1], prevMagnitude[k], prevMagnitude[k+1]);
diff = magnitude[k] - maxPrev;
```

**Result**: Reduces false positives by up to 60% on vibrato-heavy material.

Source: [CPJKU/SuperFlux](https://github.com/CPJKU/SuperFlux)

### 4. Median-Based Threshold

Mean is sensitive to outliers (a few loud beats skew the average up). Median is robust.

**Example**: History = [0.1, 0.1, 0.1, 0.1, 5.0]
- Mean = 1.08 (skewed by outlier)
- Median = 0.1 (robust)

**Options:**
1. Replace mean with median entirely
2. Use combined threshold: `δ + λ·median + α·mean`
3. Use median for average, keep stddev calculation

**Tradeoff**: Median requires sorting (O(n log n) vs O(n) for mean). With 43 samples this is negligible.

### 5. Fixed Threshold (No Sensitivity Parameter)

With proper preprocessing (log compression, narrow bins, max filter), a fixed threshold works:

- **2σ (2 standard deviations)**: 95th percentile, statistically significant
- **1.5σ**: More sensitive, may need other safeguards

The sensitivity parameter exists because the raw signal is too variable. Fix the signal, remove the parameter.

### 6. Peak Picking Improvements

From [madmom](https://madmom.readthedocs.io/en/v0.16/modules/features/onsets.html):

- **pre_max**: Ensure current frame is local maximum over past N frames
- **post_max**: Look ahead for larger peak (offline only)
- **combine**: Minimum interval between detections (we have debounce)

**For real-time**: Check that current flux is greater than all values in recent history window.

## Implementation Plan

### Phase 1: Quick Wins (Minimal Changes)
1. Narrow bins to 2-3 (~47-70 Hz)
2. Add log compression: `logf(1.0f + 100.0f * magnitude[k])`
3. Remove sensitivity parameter, use fixed 2.0

### Phase 2: SuperFlux Integration
1. Store 3-bin max-filtered previous frame
2. Compute flux against max-filtered values
3. Validate vibrato suppression

### Phase 3: Robust Threshold
1. Replace mean with median in threshold calculation
2. Add local maximum check (flux must be highest in recent window)
3. Tune debounce if needed

## References

- [Parallelcube Beat Detection](https://www.parallelcube.com/2018/03/30/beat-detection-algorithm/) - Energy-based algorithm with variance threshold
- [CPJKU/SuperFlux](https://github.com/CPJKU/SuperFlux) - Reference implementation
- [madmom onset detection](https://madmom.readthedocs.io/en/v0.16/modules/features/onsets.html) - Peak picking parameters
- [librosa SuperFlux](https://librosa.org/doc/0.11.0/auto_examples/plot_superflux.html) - Parameter values
- [AudioLabs Log Compression](https://www.audiolabs-erlangen.de/resources/MIR/FMP/C3/C3S1_LogCompression.html) - Gamma parameter explanation
- Böck & Widmer, "Maximum Filter Vibrato Suppression for Onset Detection", DAFx-13
