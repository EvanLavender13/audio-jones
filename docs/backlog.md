# Backlog

Ideas and potential features. Not committed, just captured.

## Smoothing Modes

Linear mode shows edge artifacts from smoothing:
- Left edge: truncated window (no data before sample 0)
- Right edge: mirror point plateau (sample 1023 appears twice)

Could add configurable modes:
- Wrap (circular buffer) - ends average toward each other, "plucked string" effect
- Truncate (current) - edges fade out
- None - raw data at boundaries

Low priority since circular mode handles this well via palindrome.

## Circular Palindrome Symmetry

The palindrome creates visible mirror symmetry in circular mode - an axis where both sides reflect each other. This is inherent to mirroring 1024 samples to get 2048.

Options:
- Use 2048 actual samples (no mirror) - but then start/end won't match seamlessly
- Crossfade the ends somehow
- Accept it as a visual characteristic

May not be worth "fixing" - the symmetry has its own aesthetic.

## Code Style Violations

### preset.cpp: Replace std:: file I/O with raylib

`preset.cpp` uses `std::fstream` and `std::filesystem` for file operations. Could use raylib's file API instead:

Current:
- `std::ofstream` / `std::ifstream` for reading/writing JSON
- `std::filesystem` for `exists()`, `create_directories()`, `directory_iterator()`

raylib alternatives:
- `LoadFileText()` / `SaveFileText()` - text file I/O
- `DirectoryExists()` / `MakeDirectory()` - directory checks
- `LoadDirectoryFiles()` / `UnloadDirectoryFiles()` - directory listing

nlohmann/json still needed (parses from `char*` just fine). Would eliminate `<fstream>` and `<filesystem>` includes.

Low priority - current code works, isolated to one file per style guidelines.

## Beat Detection Improvements

Current FFT-based spectral flux (`src/beat.cpp:97-111`) fails on bass-heavy EDM/dubstep/DnB. Sustained sub-bass (20-60 Hz) raises the flux average, masking transient kick detection. See `docs/research/fft-beat-detection-improvements.md` for full analysis.

### Multi-Band Spectral Flux

Split spectrum into independent bands with separate history, average, and threshold per band.

**Band definitions** (48kHz, 2048 FFT, 23.4 Hz/bin):

| Band | Bins | Frequency | Detects |
|------|------|-----------|---------|
| Sub-bass | 1-2 | 23-70 Hz | Sustained 808s |
| Kick | 3-5 | 70-140 Hz | Kick drum body |
| Low-mid | 6-8 | 140-210 Hz | Toms, bass harmonics |

**Implementation**:
- Add `FrequencyBand` struct with `startBin`, `endBin`, `flux`, `fluxHistory[43]`, `average`, `stdDev`
- Create array of 3 bands in `BeatDetector`
- Compute flux per band independently in `BeatDetectorProcess()`
- Trigger beat on kick band threshold—unaffected by sub-bass energy

**Complexity**: Medium. **Impact**: High. **Dependencies**: None.

### Logarithmic Magnitude Compression

Apply µ-law compression before flux calculation to reduce dynamic range between sustained bass and transient kicks.

**Implementation**:
```cpp
// In BeatDetectorProcess(), after FFT, before flux calculation
float raw_mag = sqrtf(re * re + im * im);
bd->magnitude[k] = logf(1.0f + 1000.0f * raw_mag);  // γ = 1000
```

Quiet transients remain detectable relative to loud sustained bass. Compression factor γ = 1000 balances sensitivity vs. noise amplification.

**Complexity**: Low. **Impact**: Medium. **Dependencies**: None.

**Source**: [OFAI Onset Detection](https://ofai.at/papers/oefai-tr-2006-12.pdf)

### SuperFlux Maximum Filter

Compare current magnitude against maximum of neighboring bins from previous frame. Suppresses false positives from vibrato and frequency modulation in EDM synths.

**Implementation**:
```cpp
// Replace simple diff in flux calculation
for (int k = kickBinStart; k <= kickBinEnd; k++) {
    float prev_max = bd->prevMagnitude[k];
    if (k > 0) prev_max = fmaxf(prev_max, bd->prevMagnitude[k-1]);
    if (k < BEAT_SPECTRUM_SIZE-1) prev_max = fmaxf(prev_max, bd->prevMagnitude[k+1]);
    float diff = bd->magnitude[k] - prev_max;
    if (diff > 0.0f) flux += diff;
}
```

Reduces false detections by up to 60% on modulated sources.

**Complexity**: Low. **Impact**: Medium. **Dependencies**: None.

**Source**: [CPJKU SuperFlux](https://github.com/CPJKU/SuperFlux)

### Increased Processing Rate

Reduce hop from 1024 to 512 samples for faster transient response.

| Hop Size | Frame Rate | Latency |
|----------|------------|---------|
| 1024 (current) | ~47 Hz | 21ms |
| 512 (proposed) | ~94 Hz | 10.7ms |

EDM kick transients have 5-10ms rise times. Current 21ms hop smears transient shape.

**Implementation**:
```cpp
// In BeatDetectorProcess(), change overlap from 50% to 75%
int overlap = BEAT_FFT_SIZE * 3 / 4;  // Keep 75%, hop 25% (512 samples)
```

**Trade-off**: ~2x CPU increase for beat detection.

**Complexity**: Low. **Impact**: Medium. **Dependencies**: None.

### Attack Transient Band

Add high-frequency band (2-4 kHz, bins 85-170) for kick transient "click" detection. The click arrives 5-10ms before the body, improving timing precision.

**Implementation**:
- Add fourth band: bins 85-170 (2000-4000 Hz)
- Detect attack when both attack and kick bands trigger within 15ms window
- Use attack timing for visual effects, kick magnitude for intensity

**Complexity**: Medium. **Impact**: Low. **Dependencies**: Multi-Band Spectral Flux.

**Source**: [DSP Stack Exchange](https://dsp.stackexchange.com/questions/35608/musical-onset-detection-approach)
