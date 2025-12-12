# Robust Beat Detection

Eliminate false positives from voices and noise, detect kicks reliably without sensitivity tuning.

Research: `docs/research/robust-beat-detection.md`

## Goal

Current beat detection triggers on vocals, sustained notes, and noise. Users must tune sensitivity manually. This plan implements SuperFlux-inspired improvements to achieve reliable kick detection with fixed parameters.

## Current State

`src/analysis/beat.cpp` uses spectral flux with these problems:

| Location | Issue |
|----------|-------|
| `beat.cpp:9-10` | Bins 1-8 (23-188 Hz) include voice fundamentals |
| `beat.cpp:20-21` | Linear diff without compression |
| `beat.cpp:85` | Mean-based average is outlier-sensitive |
| `beat.cpp:100` | User-tunable sensitivity parameter |
| `beat.cpp:20` | No vibrato suppression |

## Algorithm

### Phase 1: Frequency Narrowing and Log Compression

**Narrow kick bins** from 8 to 2 bins:

```cpp
static const int KICK_BIN_START = 2;  // ~47 Hz
static const int KICK_BIN_END = 3;    // ~70 Hz
```

Rationale: Kick fundamentals sit at 40-80 Hz. Male voice starts ~85 Hz. Bins 2-3 at 48kHz/2048 FFT capture 47-70 Hz.

**Add log compression** before flux calculation:

```cpp
const float compressed = logf(1.0f + 100.0f * magnitude[k]);
```

| Parameter | Value | Rationale |
|-----------|-------|-----------|
| γ (gamma) | 100 | SuperFlux/librosa default; strong compression normalizes dynamic range |

### Phase 2: Maximum Filter (SuperFlux)

Store max-filtered previous frame to suppress vibrato:

```cpp
// New buffer in BeatDetector struct
float maxFilteredPrev[BEAT_SPECTRUM_SIZE];

// In flux calculation
float maxPrev = prevMag[k];
if (k > 0) maxPrev = fmaxf(maxPrev, prevMag[k - 1]);
if (k < binCount - 1) maxPrev = fmaxf(maxPrev, prevMag[k + 1]);

const float diff = compressed - logf(1.0f + 100.0f * maxPrev);
```

Effect: Gradual pitch changes from vibrato appear in adjacent bins; taking the max suppresses the resulting false flux.

### Phase 3: Robust Threshold

**Replace mean with median** for threshold baseline:

```cpp
static float ComputeMedian(float* values, int count)
{
    // Copy to temp buffer, sort, return middle
    float temp[BEAT_HISTORY_SIZE];
    memcpy(temp, values, count * sizeof(float));
    // Simple insertion sort (n=43, fast enough)
    for (int i = 1; i < count; i++) {
        float key = temp[i];
        int j = i - 1;
        while (j >= 0 && temp[j] > key) {
            temp[j + 1] = temp[j];
            j--;
        }
        temp[j + 1] = key;
    }
    return temp[count / 2];
}
```

**Fixed 2σ threshold** replaces sensitivity parameter:

```cpp
const float threshold = fluxMedian + 2.0f * fluxStdDev;
```

**Local maximum check** ensures current flux exceeds recent window:

```cpp
bool isLocalMax = true;
for (int i = 0; i < BEAT_HISTORY_SIZE; i++) {
    if (bd->fluxHistory[i] > flux) {
        isLocalMax = false;
        break;
    }
}
```

## Integration

### BeatDetector Struct Changes

Add to `src/analysis/beat.h`:

```cpp
typedef struct BeatDetector {
    // Existing fields...

    // New: max-filtered previous magnitude for SuperFlux
    float maxFilteredPrev[BEAT_SPECTRUM_SIZE];

    // Renamed: fluxAverage -> fluxMedian (or keep both during transition)
    float fluxMedian;
} BeatDetector;
```

### API Changes

Remove sensitivity parameter from `BeatDetectorProcess()`:

```cpp
// Before
void BeatDetectorProcess(BeatDetector* bd, const float* magnitude, int binCount,
                         float deltaTime, float sensitivity);

// After
void BeatDetectorProcess(BeatDetector* bd, const float* magnitude, int binCount,
                         float deltaTime);
```

### Call Sites

| File | Change |
|------|--------|
| `src/main.cpp` | Remove sensitivity arg from `BeatDetectorProcess()` call |
| `src/ui/ui_panel_effects.cpp` | Remove sensitivity slider from UI |
| `src/config/app_configs.h` | Remove sensitivity from `EffectsConfig` |

## File Changes

| File | Change |
|------|--------|
| `src/analysis/beat.h` | Add maxFilteredPrev buffer, change fluxAverage to fluxMedian, remove sensitivity param |
| `src/analysis/beat.cpp` | Implement log compression, max filter, median threshold, local max check |
| `src/main.cpp` | Update BeatDetectorProcess() call |
| `src/ui/ui_panel_effects.cpp` | Remove sensitivity slider |
| `src/config/app_configs.h` | Remove beatSensitivity field |
| `src/config/preset.cpp` | Remove beatSensitivity from JSON serialization |

## Validation

- [ ] Kick drums trigger beat detection reliably
- [ ] Vocals do not trigger false positives
- [ ] Sustained notes with vibrato do not trigger
- [ ] Quiet sections do not become oversensitive
- [ ] No user-adjustable sensitivity parameter in UI
- [ ] Existing presets load without error (handle missing beatSensitivity gracefully)

## Implementation Order

1. **Phase 1** - Narrow bins + log compression (lowest risk, immediate improvement)
2. **Phase 2** - Max filter (moderate complexity, biggest false-positive reduction)
3. **Phase 3** - Median threshold + local max (API change, requires call site updates)

Each phase can be validated independently before proceeding.

## References

- [SuperFlux paper](https://github.com/CPJKU/SuperFlux) - Maximum filter algorithm
- [librosa SuperFlux](https://librosa.org/doc/0.11.0/auto_examples/plot_superflux.html) - γ=100 parameter
- [AudioLabs log compression](https://www.audiolabs-erlangen.de/resources/MIR/FMP/C3/C3S1_LogCompression.html) - Theory
