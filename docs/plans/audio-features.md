# Audio Features

Add 5 new audio analysis features (spectral flatness, spread, rolloff, full-band flux, crest factor) as modulation sources with UI visualization. Creates a clean `AudioFeatures` module separate from existing band energies.

## Current State

- `src/analysis/bands.h:17-37` - `BandEnergies` struct with bass/mid/treb + centroid
- `src/analysis/bands.cpp:57-96` - `BandEnergiesProcess()` computes RMS and centroid from magnitude
- `src/analysis/analysis_pipeline.h:11-20` - `AnalysisPipeline` embeds FFT, beat, bands
- `src/automation/mod_sources.h:9-24` - `ModSource` enum with 13 sources (bass/mid/treb/beat/LFOs/centroid)
- `src/ui/imgui_analysis.cpp:511-543` - Analysis panel with beat graph and band meters
- `docs/research/audio-features.md` - Algorithms for all features

## Technical Implementation

### Spectral Flatness (tonality vs noise)

**Source**: `docs/research/audio-features.md:79-94`

```
geometricMean = exp( sum(log(magnitude[k] + epsilon)) / N )
arithmeticMean = sum(magnitude[k]) / N
flatness = geometricMean / arithmeticMean
```

- Range: 0 (pure tone) to 1 (white noise)
- epsilon = 1e-10 to avoid log(0)

### Spectral Spread (bandwidth around centroid)

**Source**: `docs/research/audio-features.md:47-59`

```
centroid = sum(k * magnitude[k]) / sum(magnitude[k])
spread = sqrt( sum(magnitude[k] * (k - centroid)^2) / sum(magnitude[k]) )
```

- Normalize by dividing by N/2 for 0-1 range
- Low = tonal/pure, High = noisy/broadband

### Spectral Rolloff (energy concentration point)

**Source**: `docs/research/audio-features.md:61-77`

```
totalEnergy = sum(magnitude[k]^2)
threshold = 0.85 * totalEnergy

cumulative = 0
for k = 0 to N-1:
    cumulative += magnitude[k]^2
    if cumulative >= threshold:
        rolloff = k / (N-1)  // normalized 0-1
        break
```

- 85% rolloff point, normalized to 0-1
- Low = bass-heavy, High = treble-heavy

### Full-Band Spectral Flux (activity/change)

**Source**: `docs/research/audio-features.md:95-103`

```
flux = sum( max(0, magnitude[k] - prevMagnitude[k]) )  for all k
```

- Positive-only difference (onset detection)
- Normalize by self-calibration (flux / fluxAvg, clamped)

### Crest Factor (punchiness)

**Source**: `docs/research/audio-features.md:105-118`

```
peak = max(|sample[i]|)
rms = sqrt( sum(sample[i]^2) / sampleCount )
crest = peak / (rms + epsilon)
```

- Typical range 1-10, normalize to 0-1 via `min(crest / 6.0, 1.0)`
- High = punchy/transient, Low = compressed/sustained

---

## Phase 1: AudioFeatures Module

**Goal**: Create core analysis module computing all 5 features.

**Build**:
- Create `src/analysis/audio_features.h`:
  - `AudioFeatures` struct with 5 features × 3 fields each (raw, smooth, avg)
  - `prevMagnitude[FFT_BIN_COUNT]` for flux calculation
  - `AudioFeaturesInit()` and `AudioFeaturesProcess()` declarations
- Create `src/analysis/audio_features.cpp`:
  - Implement `AudioFeaturesProcess(features, magnitude, binCount, samples, sampleCount, dt)`
  - Apply `ApplyEnvelope()` for attack/release smoothing (copy pattern from `bands.cpp:45-50`)
  - Update running averages with `AVG_DECAY/AVG_ATTACK` pattern from bands

**Done when**: Module compiles independently with all 5 algorithms implemented.

---

## Phase 2: Pipeline Integration

**Goal**: Wire AudioFeatures into the analysis pipeline.

**Build**:
- Modify `src/analysis/analysis_pipeline.h`:
  - Add `#include "audio_features.h"`
  - Add `AudioFeatures features;` field to `AnalysisPipeline` struct
- Modify `src/analysis/analysis_pipeline.cpp`:
  - Call `AudioFeaturesInit()` in `AnalysisPipelineInit()`
  - Call `AudioFeaturesProcess()` after `BandEnergiesProcess()` in the FFT update loop
  - Pass magnitude array plus audioBuffer + lastFramesRead for crest calculation
- Update `CMakeLists.txt` if needed to include new source file

**Done when**: Features compute each frame; verify with debug prints or breakpoint.

---

## Phase 3: Modulation Sources

**Goal**: Expose features as modulation sources for parameter routing.

**Build**:
- Modify `src/automation/mod_sources.h`:
  - Add enum entries: `MOD_SOURCE_FLATNESS`, `MOD_SOURCE_SPREAD`, `MOD_SOURCE_ROLLOFF`, `MOD_SOURCE_FLUX`, `MOD_SOURCE_CREST`
  - Update `MOD_SOURCE_COUNT` to 18
- Modify `src/automation/mod_sources.cpp`:
  - Include `"analysis/audio_features.h"`
  - Update `ModSourcesUpdate()` signature to accept `const AudioFeatures*`
  - Add normalized 0-1 mappings for each feature (use smooth/avg self-calibration)
  - Add `ModSourceGetName()` entries: "Flat", "Sprd", "Roll", "Flux", "Crst"
  - Add `ModSourceGetColor()` entries using complementary Theme colors
- Update callers of `ModSourcesUpdate()` (likely in `main.cpp`)

**Done when**: Features appear in modulation routing dropdown and respond to audio.

---

## Phase 4: UI Panel

**Goal**: Create distinctive visualization for audio features in Analysis panel.

**Build**:
- Use `/frontend-design` skill to design the UI component
- Implement in `src/ui/imgui_analysis.cpp`:
  - Add new section "Audio Features" with accent color
  - Create meter visualization distinct from existing band meters
  - Show all 5 features with labels and animated bars
  - Include section collapse toggle like "Zone Timing"
- Update `ImGuiDrawAnalysisPanel()` signature to accept `const AudioFeatures*`
- Update caller in UI rendering code

**Done when**: Audio Features section displays in Analysis panel with reactive meters.

---

## Phase 5: Validation

**Goal**: Verify correctness and responsiveness.

**Build**:
- Test with varied audio (drums, vocals, synths, silence)
- Verify:
  - Flatness low on pure tones, high on noise/cymbals
  - Spread low on bass, high on full-spectrum content
  - Rolloff tracks brightness changes
  - Flux spikes on transients
  - Crest high on uncompressed, low on limited audio
- Check modulation routing works for all 5 sources
- Profile to ensure <1ms overhead

**Done when**: All features respond correctly to test audio; no performance regression.
