# Audio Feature Self-Calibration

Apply running-average normalization to the four non-flux spectral features (flatness, spread, rolloff, crest) so they self-calibrate like band energies, giving full dynamic range instead of clustering in narrow sub-ranges.

**Research**: `docs/research/audio-feature-self-calibration.md`

## Design

### Algorithm

Replace the raw pass-through of the four feature mod sources with the same `smooth / max(avg, MIN_AVG)` normalization already used by bass/mid/treb on lines 18-25 of `mod_sources.cpp`. The `MIN_AVG` constant (`1e-6f`) and `norm` variable already exist.

Flux is unchanged — it self-calibrates internally via `flux / (fluxAvg * 3.0)` in `audio_features.cpp`.

### Parameters

No new parameters. Four existing mod source outputs change normalization:

| Mod Source | Before | After |
|------------|--------|-------|
| Flatness | Raw smoothed (narrow range) | `smooth / avg`, clamped to 0–1 |
| Spread | Raw smoothed (narrow range) | `smooth / avg`, clamped to 0–1 |
| Rolloff | Raw smoothed (clusters by genre) | `smooth / avg`, clamped to 0–1 |
| Crest | Raw smoothed (fixed /6.0 scaling) | `smooth / avg`, clamped to 0–1 |
| Flux | Self-calibrated (unchanged) | Unchanged |

---

## Tasks

### Wave 1: Normalize feature mod sources

#### Task 1.1: Apply running-average normalization to four features

**Files**: `src/automation/mod_sources.cpp` (modify)

**Do**: Replace lines 33-38 (the comment and four feature assignments). Keep the existing comment style but update the comment text. Apply the same normalization pattern used by band energies on lines 18-25:

```c
// Audio features (normalize by running average, matching band energies)
norm = features->flatnessSmooth / fmaxf(features->flatnessAvg, MIN_AVG);
sources->values[MOD_SOURCE_FLATNESS] = fminf(norm / 2.0f, 1.0f);

norm = features->spreadSmooth / fmaxf(features->spreadAvg, MIN_AVG);
sources->values[MOD_SOURCE_SPREAD] = fminf(norm / 2.0f, 1.0f);

norm = features->rolloffSmooth / fmaxf(features->rolloffAvg, MIN_AVG);
sources->values[MOD_SOURCE_ROLLOFF] = fminf(norm / 2.0f, 1.0f);

// Flux: already self-calibrates internally (divided by 3x running average)
sources->values[MOD_SOURCE_FLUX] = features->fluxSmooth;

norm = features->crestSmooth / fmaxf(features->crestAvg, MIN_AVG);
sources->values[MOD_SOURCE_CREST] = fminf(norm / 2.0f, 1.0f);
```

**Verify**: `cmake.exe --build build` compiles with no warnings.

---

## Final Verification

- [ ] Build succeeds with no warnings
- [ ] Flatness, Spread, Rolloff, Crest mod sources show wider dynamic range during playback
- [ ] Flux mod source behavior unchanged
- [ ] Band energy mod sources (Bass, Mid, Treb) unchanged
