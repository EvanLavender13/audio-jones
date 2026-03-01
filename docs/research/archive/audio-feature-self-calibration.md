# Audio Feature Self-Calibration

The five spectral audio features (flatness, spread, rolloff, flux, crest) expose mod source values that cluster in narrow sub-ranges and feel static compared to the self-calibrating band energies (bass/mid/treb). Applying the same running-average normalization to the four non-flux features gives them full dynamic range and consistent behavior with the rest of the audio mod sources.

## Classification

- **Category**: General (audio analysis pipeline)
- **Scope**: `mod_sources.cpp` normalization path

## References

- Existing implementation: `src/automation/mod_sources.cpp` lines 18-25 (band energy normalization pattern)
- Running averages already computed: `src/analysis/audio_features.cpp` lines 44, 69, 90, 136

## Algorithm

The running averages (`flatnessAvg`, `spreadAvg`, `rolloffAvg`, `crestAvg`) are already computed per frame in `AudioFeaturesProcess()` via `UpdateRunningAvg()` but never consumed. The fix applies the same normalization pattern used by band energies.

**Current code** (passes smoothed values through raw):
```c
sources->values[MOD_SOURCE_FLATNESS] = features->flatnessSmooth;
sources->values[MOD_SOURCE_SPREAD]   = features->spreadSmooth;
sources->values[MOD_SOURCE_ROLLOFF]  = features->rolloffSmooth;
sources->values[MOD_SOURCE_CREST]    = features->crestSmooth;
```

**New code** (normalize by running average, matching band energy pattern):
```c
norm = features->flatnessSmooth / fmaxf(features->flatnessAvg, MIN_AVG);
sources->values[MOD_SOURCE_FLATNESS] = fminf(norm / 2.0f, 1.0f);

norm = features->spreadSmooth / fmaxf(features->spreadAvg, MIN_AVG);
sources->values[MOD_SOURCE_SPREAD] = fminf(norm / 2.0f, 1.0f);

norm = features->rolloffSmooth / fmaxf(features->rolloffAvg, MIN_AVG);
sources->values[MOD_SOURCE_ROLLOFF] = fminf(norm / 2.0f, 1.0f);

norm = features->crestSmooth / fmaxf(features->crestAvg, MIN_AVG);
sources->values[MOD_SOURCE_CREST] = fminf(norm / 2.0f, 1.0f);
```

**Flux is unchanged** — it already self-calibrates internally (divided by 3x running average in `AudioFeaturesProcess`). Applying running-average normalization on top would double-calibrate it.

## Parameters

No new parameters. This changes the normalization of 4 existing mod source outputs.

| Mod Source | Before | After |
|------------|--------|-------|
| Flatness | Raw 0–1 (often narrow range) | Centered ~0.5, peaks at 2x avg |
| Spread | Raw 0–~0.3 (conservative normalization) | Centered ~0.5, peaks at 2x avg |
| Rolloff | Raw 0–1 (clusters by genre) | Centered ~0.5, peaks at 2x avg |
| Crest | Raw 0–1 (fixed /6.0 normalization) | Centered ~0.5, peaks at 2x avg |
| Flux | Self-calibrated (unchanged) | Unchanged |

## Notes

- **Preset compatibility**: Existing presets that route these mod sources will see wider dynamic range. Modulation that previously produced subtle movement will become more pronounced. This is the desired fix but users may want to reduce modulation depth on affected routes.
- **Single-file change**: Only `src/automation/mod_sources.cpp` needs modification.
- **Running average warm-up**: The first ~10 seconds after launch will have a near-zero running average, causing the features to saturate at 1.0 until the average stabilizes. This matches existing band energy behavior and is not noticeable in practice.
