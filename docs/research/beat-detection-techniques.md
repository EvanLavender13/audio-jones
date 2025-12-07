# Beat Detection Techniques

Research on beat detection algorithms for real-time audio visualization.

## Current Implementation

`src/beat.cpp` implements a low-pass energy threshold algorithm:
1. IIR low-pass filter (~200Hz cutoff) extracts bass frequencies
2. Computes energy as mean squared filtered sample value per frame
3. Maintains 43-sample rolling average (~860ms at 20Hz update rate)
4. Triggers beat when current energy exceeds `average × sensitivity`
5. Enforces 150ms debounce between beats

Strengths: Simple, low CPU, works for bass-heavy music.
Weaknesses: Single frequency band, fixed threshold multiplier, no tempo awareness.

## Algorithm Categories

### 1. Energy-Based Detection

#### Simple Energy Threshold
Compares instantaneous energy against rolling average. Beat triggers when:
```
current_energy > threshold × average_energy
```

Our implementation uses this approach. Limitation: sensitivity parameter requires manual tuning per track.

#### Adaptive Variance Threshold
Adjusts threshold based on energy variance in the history buffer:
```
variance = Σ(history[i] - average)² / history_size
threshold = -15 × variance + 1.55
```

Lower variance (consistent volume) raises threshold; higher variance (dynamic music) lowers it. Source: [Parallelcube Beat Detection](https://www.parallelcube.com/2018/03/30/beat-detection-algorithm/).

#### Standard Deviation Threshold
Triggers when energy exceeds N standard deviations above the mean:
```
beat = current_energy > (mean + N × stddev)
```

Typical N values: 1.0-2.0. Used by [Beat-and-Tempo-Tracking](https://github.com/michaelkrzyzaniak/Beat-and-Tempo-Tracking) library.

### 2. Frequency Band Separation

Splits spectrum into bands, detects beats independently per band.

| Band | Frequency Range | Detects |
|------|----------------|---------|
| Sub-bass | 20-60 Hz | Kick drum fundamental |
| Bass | 60-130 Hz | Kick drum body |
| Low-mid | 130-500 Hz | Bass guitar, toms |
| Mid | 500-2000 Hz | Snare body, vocals |
| High-mid | 2000-4000 Hz | Snare crack, guitars |
| High | 4000-10000 Hz | Hi-hats, cymbals |

Implementation requires FFT. With 44.1kHz sample rate and 1024-sample window:
- Frequency resolution: 44100/1024 ≈ 43 Hz/bin
- Bin N covers frequencies: N×43 Hz to (N+1)×43 Hz

Source: [Minim BeatDetect](https://code.compartmental.net/minim/beatdetect_class_beatdetect.html).

### 3. Spectral Flux (Onset Detection)

Measures rate of spectral change between consecutive frames. More robust than pure energy for detecting note onsets.

#### Basic Spectral Flux
```
SF(n) = Σ HW(|X(k,n)| - |X(k,n-1)|)
```
Where:
- `X(k,n)` = magnitude of bin k at frame n
- `HW(x) = max(0, x)` = half-wave rectifier (ignores energy decreases)

#### Enhanced Spectral Flux
1. Apply logarithmic compression: `Y = log(1 + γ|X|)` where γ ≥ 1
2. Compute flux on compressed spectrum
3. Subtract local average (adaptive baseline)
4. Half-wave rectify result

Logarithmic compression enhances weak spectral components, improving detection of quiet onsets. Source: [OFAI Onset Detection](https://ofai.at/papers/oefai-tr-2006-12.pdf).

#### SuperFlux Algorithm
Adds maximum filter across frequency bins to suppress false positives from vibrato. State-of-the-art for onset detection. Source: [Essentia SuperFlux](https://essentia.upf.edu/reference/streaming_SuperFluxExtractor.html).

### 4. Tempo Estimation

#### Autocorrelation Method
Autocorrelate the onset strength signal to find dominant periodicities:
1. Compute onset strength signal (OSS) via spectral flux
2. Autocorrelate OSS over lag range 300ms-1200ms (50-200 BPM)
3. Find peaks in autocorrelation
4. Peaks correspond to tempo candidates

Optimization: Downsample heavily before autocorrelation. Target signal only needs 1-3 Hz resolution (60-180 BPM range).

Source: [IEEE TASLP Streamlined Tempo Estimation](https://dl.acm.org/doi/10.1109/TASLP.2014.2348916).

#### Comb Filter Bank
Banks of comb filters at different periods resonate when input matches their period:
1. Generate onset detection function
2. Pass through comb filters at candidate tempos (e.g., 60-200 BPM)
3. Filter with highest output indicates tempo
4. Phase of peak indicates beat alignment

Comb filters also respond to integer multiples/fractions of the fundamental period, requiring disambiguation logic. Source: [Scheirer 1998](https://cagnazzo.wp.imt.fr/files/2013/05/Scheirer98.pdf).

### 5. Beat Tracking (Phase + Tempo)

#### Dynamic Programming (Ellis Algorithm)
Offline method finding optimal beat sequence that:
1. Maximizes alignment with onset function peaks
2. Minimizes deviation from constant tempo

Uses dynamic programming similar to Viterbi algorithm. Limitation: Requires estimated global tempo; struggles with tempo changes.

Source: [Ellis Beat Tracking](https://www.ee.columbia.edu/~dpwe/pubs/Ellis07-beattrack.pdf).

#### Particle Filtering (BeatNet)
State-of-the-art real-time approach:
1. CRNN extracts beat activation function from audio
2. Particle filter tracks multiple tempo/phase hypotheses
3. Particles compete; survivors converge on correct beat

Requires trained neural network. Source: [BeatNet GitHub](https://github.com/mjhydri/BeatNet).

## Implementation Recommendations

### Tier 1: Immediate Improvements (No FFT)

**Adaptive threshold**: Replace fixed sensitivity multiplier with variance-based threshold. Reduces manual tuning.

**Multiple low-pass filters**: Add parallel IIR filters at different cutoffs (80Hz, 200Hz, 500Hz) for crude band separation without FFT.

**Decay smoothing**: Exponential decay on beat intensity creates smoother visual transitions.

### Tier 2: FFT-Based (Moderate Complexity)

**Frequency band energy**:
- Add FFT (raylib provides `fft()` or use kiss_fft)
- Compute energy in 3-6 frequency bands
- Independent beat detection per band
- Enables separate kick/snare/hi-hat reactive effects

**Spectral flux onset detection**:
- Compare consecutive FFT frames
- Half-wave rectify positive changes
- More robust than pure energy

Typical FFT parameters for 48kHz audio:
- Window size: 2048 samples (43ms, 23Hz resolution)
- Hop size: 512 samples (10.7ms, ~93 FPS)
- Window function: Hamming or Hann

### Tier 3: Tempo-Aware (Higher Complexity)

**Autocorrelation tempo**:
- Autocorrelate onset signal over 300ms-1200ms lag
- Find dominant tempo
- Predict next beat time

**Phase-locked effects**:
- Use tempo estimate to anticipate beats
- Trigger effects slightly early for perceived sync
- Handle tempo drift with continuous tracking

### Tier 4: ML-Based (Maximum Complexity)

**Pre-trained models**: BeatNet, madmom, or similar provide joint beat/downbeat/tempo tracking. Requires Python or ONNX runtime integration.

Not recommended for this project due to dependency complexity.

## Latency Considerations

| Method | Typical Latency | Notes |
|--------|----------------|-------|
| Energy threshold | 10-20ms | Limited by audio buffer size |
| Spectral flux | 20-50ms | FFT window size adds delay |
| Tempo estimation | 500-2000ms | Needs history for autocorrelation |
| Beat prediction | 0ms (negative) | Predicts ahead once tempo locked |

For reactive visuals, 20-50ms latency is imperceptible. Tempo-aware systems can achieve zero effective latency by predicting beats.

## Sources

- [BeatNet (State-of-the-art)](https://github.com/mjhydri/BeatNet)
- [Beat-and-Tempo-Tracking (C library)](https://github.com/michaelkrzyzaniak/Beat-and-Tempo-Tracking)
- [Parallelcube Beat Detection Algorithm](https://www.parallelcube.com/2018/03/30/beat-detection-algorithm/)
- [Minim BeatDetect](https://code.compartmental.net/minim/beatdetect_class_beatdetect.html)
- [Ellis: Beat Tracking by Dynamic Programming](https://www.ee.columbia.edu/~dpwe/pubs/Ellis07-beattrack.pdf)
- [Scheirer: Tempo and Beat Analysis of Acoustic Musical Signals](https://cagnazzo.wp.imt.fr/files/2013/05/Scheirer98.pdf)
- [OFAI Onset Detection Revisited](https://ofai.at/papers/oefai-tr-2006-12.pdf)
- [Essentia Audio Analysis Library](https://essentia.upf.edu/)
- [ciphrd: Audio Analysis for Music Visualization](https://ciphrd.com/2019/09/01/audio-analysis-for-advanced-music-visualization-pt-1/)
- [Airtight Interactive: Making Audio Reactive Visuals](https://www.airtightinteractive.com/2013/10/making-audio-reactive-visuals/)
