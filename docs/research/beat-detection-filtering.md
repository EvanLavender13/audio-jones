# Beat Detection Filtering

The beat detector false-fires on voices, bass slides, and synth swells because it only measures magnitude change in the kick band, not whether the change is percussive. Two approaches to fix this, documented independently for sequential evaluation.

## Classification

- **Category**: General (Analysis Pipeline Enhancement)
- **Pipeline Position**: Inside `BeatDetectorProcess()`, after flux computation, before threshold trigger

## References

- [Spectral Flatness (Wiener Entropy)](https://www.sciencedirect.com/topics/computer-science/spectral-flatness) - Formula and percussive vs tonal classification
- [Musical Note Onset Detection Based on Spectral Sparsity](https://pmc.ncbi.nlm.nih.gov/articles/PMC8550344/) - Low-energy bin analysis; "a transient requires a much larger number of sinusoids than a steady-state component"
- [Parallelcube Beat Detection](https://www.parallelcube.com/2018/03/30/beat-detection-algorithm/) - Multi-band energy comparison, variance-adaptive thresholding
- [Aubio Onset Detection Methods](https://github.com/aubio/aubio/issues/106) - HFC for percussive onsets, spectral-difference for tonal; practical method comparison
- [Real-time Temporal Segmentation of Note Objects](https://aubio.org/articles/brossier04realtimesegmentation.pdf) - Combining detection functions: "different detection functions complement each other"

---

## Approach A: Kick-Band Tonality Rejection

### Problem It Solves

Male vocals, bass guitar, and synth basses have fundamentals in 47–140 Hz — the same range as kick drums. When these instruments onset (start a new note or syllable), they produce positive spectral flux that passes the current 2σ threshold. The detector cannot tell the difference because it only looks at *how much* the magnitude changed, not *what shape* the change has.

### Core Insight

Drum hits are broadband noise-bursts: energy distributes roughly evenly across the kick band bins. Voices and bass notes are tonal: energy concentrates in one or two harmonic peaks. Spectral flatness (geometric mean / arithmetic mean) measures exactly this distinction.

### Algorithm

Compute spectral flatness over the kick band (bins 2–6) each frame, using the same formula already in `AudioFeaturesProcess()`:

```
geometricMean = exp( sum(log(magnitude[k] + epsilon)) / N )  for k in [KICK_BIN_START, KICK_BIN_END]
arithmeticMean = sum(magnitude[k]) / N
kickFlatness = geometricMean / arithmeticMean
```

- **kickFlatness near 1.0**: Energy spread evenly → noise-like → percussive
- **kickFlatness near 0.0**: Energy in one or two bins → tonal → voice/bass

Gate the beat trigger: only fire if `kickFlatness > kickFlatnessAvg * flatnessGateRatio`.

The threshold self-calibrates via running average (`UpdateRunningAvg` from `smoothing.h`). A fixed threshold would break across genres and recording levels. The ratio comparison asks "is this frame more noise-like than usual?" rather than "is this frame noise-like in absolute terms?"

### Why 5 Bins Is Enough

Full-spectrum flatness (853 bins) averages out local shape. With only 5 bins, flatness is highly sensitive to whether energy is peaked (one dominant harmonic) or spread (broadband impulse). This is exactly the distinction needed. The tradeoff is noise — flatness fluctuates frame-to-frame — but the running average comparison absorbs that.

### Changes

- `beat.h`: Add `kickFlatness`, `kickFlatnessAvg` fields to `BeatDetector`
- `beat.cpp`: Add `ComputeKickBandFlatness()` static helper; add flatness gate to trigger condition; init new fields to zero
- No UI changes, no new parameters exposed to user

### Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| flatnessGateRatio | float | hardcoded | 1.0 | Frames must exceed average flatness by this ratio to trigger |

Hardcoded constant, not user-facing. Tuned by testing.

### Notes

- **No additional FFT cost.** Uses the same `magnitude[]` array. Adds ~5 log/exp operations per frame.
- **Genre-robust.** Even compressed kicks maintain broadband spectral shape in the low end. Tonal instruments remain tonal regardless of production style.
- **Weakness:** If a kick drum is extremely sub-heavy (all energy in one bin, e.g. an 808), its flatness may be low enough to get rejected. This is the failure mode to watch for during testing.

---

## Approach B: Broadband Transient Confirmation

### Problem It Solves

Same false positives as Approach A, but attacks the problem from the opposite direction: instead of asking "is the kick band noise-like?", it asks "did something also happen in a distant frequency band at the same time?"

### Core Insight

Real kick drums don't only produce low-frequency energy. The beater impact creates a broadband transient "click" that splashes across mid and treble frequencies simultaneously. Voices and bass instruments produce energy in their fundamental + harmonics but do NOT produce simultaneous broadband flux in distant frequency bands. Requiring correlated flux in a high band rejects onsets that only have low-frequency content.

### Algorithm

Compute spectral flux in a confirmation band (2–8 kHz, bins 85–341) alongside the existing kick-band flux:

```
confirmFlux = sum( max(0, magnitude[k] - prevMagnitude[k]) )  for k in [CONFIRM_BIN_START, CONFIRM_BIN_END]
```

Track rolling average. Gate the beat trigger: only fire if `confirmFlux > confirmFluxAvg * confirmGateRatio`.

Same self-calibrating pattern as the existing kick flux statistics. The ratio asks "did the high band spike relative to its own baseline?" rather than comparing absolute values between bands.

### Why 2–8 kHz

- Below 2 kHz overlaps with vocal formants and would reintroduce false positives.
- Above 8 kHz catches cymbal-only transients that aren't beats.
- 2–8 kHz captures the "click" component of kick drums and the snap of snare drums without picking up sustained tonal content.

### Changes

- `beat.h`: Add `confirmFlux`, `confirmFluxAvg`, `confirmPrevMagnitude[]` fields to `BeatDetector`
- `beat.cpp`: Add `ComputeConfirmBandFlux()` static helper; add confirmation gate to trigger condition; init new fields to zero
- No UI changes, no new parameters exposed to user

### Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| confirmGateRatio | float | hardcoded | 1.5 | Confirmation band flux must exceed its average by this ratio |

Hardcoded constant, not user-facing. Higher than the flatness ratio because the confirmation signal is noisier.

### Notes

- **Adds ~256 subtractions per frame** for the confirmation band flux. Negligible cost.
- **Weakness: compressed/limited masters.** In EDM, hyperpop, and other heavily compressed genres, kick transients are clipped and the broadband click component may be squashed. This filter could over-reject legitimate beats in that material.
- **Existing overlap:** `AudioFeatures.flux` already computes full-band flux, but it covers all bins including the voice formant range. The confirmation band is intentionally narrower.
