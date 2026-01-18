# Audio Feature Extraction for Visualization

Techniques for extracting meaningful features from FFT magnitude spectra. All algorithms assume we have `magnitude[k]` for bins `k = 0..N-1` where `N = FFT_SIZE/2 + 1`.

## Currently Implemented

### Band Energies (Bass/Mid/Treb)

RMS energy in predefined frequency ranges.

```
bandEnergy = sqrt( sum(magnitude[k]^2) / count )  for k in [binLo, binHi]
```

| Band | Frequency Range | Bins (at 48kHz, 2048 FFT) |
|------|-----------------|---------------------------|
| Bass | 20-250 Hz       | 1-10                      |
| Mid  | 250-4000 Hz     | 11-170                    |
| Treb | 4000-20000 Hz   | 171-853                   |

Bin-to-frequency: `f = k * sampleRate / fftSize`

### Spectral Centroid

Weighted average frequency - perceptual "brightness".

```
centroid = sum(k * magnitude[k]) / sum(magnitude[k])
```

Normalize to 0-1 by dividing by `N-1`. Higher = brighter/sharper sound.

### Beat Detection (Spectral Flux)

Rate of change in spectrum, restricted to kick drum frequencies.

```
flux = sum( max(0, magnitude[k] - prevMagnitude[k]) )  for k in kickBand
```

Only positive differences count (onset detection, not offset). Beat triggers when `flux > mean + 2*stdDev` over history window.

---

## Easy Additions

### Spectral Spread (Bandwidth)

Standard deviation around the centroid - how "spread out" the frequency content is.

```
centroid = sum(k * magnitude[k]) / sum(magnitude[k])
spread = sqrt( sum(magnitude[k] * (k - centroid)^2) / sum(magnitude[k]) )
```

- **Low spread**: Tonal, pure (vocals, synth leads, bass notes)
- **High spread**: Noisy, broadband (drums, cymbals, distortion)

Normalize by dividing by `N/2` for 0-1 range.

### Spectral Rolloff

Frequency below which X% of spectral energy is concentrated (typically 85% or 95%).

```
totalEnergy = sum(magnitude[k]^2)
threshold = 0.85 * totalEnergy

cumulative = 0
for k = 0 to N-1:
    cumulative += magnitude[k]^2
    if cumulative >= threshold:
        rolloff = k
        break
```

Tracks where the "weight" of the spectrum sits. Drops during bass-heavy sections, rises during hi-hats/cymbals.

### Spectral Flatness

Ratio of geometric mean to arithmetic mean. Measures tonality vs noise.

```
geometricMean = exp( sum(log(magnitude[k] + epsilon)) / N )
arithmeticMean = sum(magnitude[k]) / N
flatness = geometricMean / arithmeticMean
```

- **0**: Pure tone (single frequency)
- **1**: White noise (all frequencies equal)
- **0.3-0.7**: Typical music

Add `epsilon = 1e-10` to avoid `log(0)`. Great for detecting percussive hits vs sustained notes.

### Spectral Flux (Full-Band)

Expose the existing beat detection flux as a general feature, computed over full spectrum instead of just kick band.

```
flux = sum( max(0, magnitude[k] - prevMagnitude[k]) )  for all k
```

Useful as a general "activity" or "change" metric. Spikes on any transient, not just bass.

### Crest Factor

Peak-to-RMS ratio of the time-domain signal. Measures "punchiness".

```
peak = max(|sample[i]|)
rms = sqrt( sum(sample[i]^2) / sampleCount )
crest = peak / rms
```

- **High crest (>4)**: Punchy, transient-heavy (acoustic drums, plucks)
- **Low crest (<2)**: Compressed, sustained (heavily limited EDM, pads)

Typically expressed in dB: `crestDb = 20 * log10(crest)`

### Zero Crossing Rate

How often the waveform crosses zero per unit time.

```
crossings = 0
for i = 1 to sampleCount-1:
    if sign(sample[i]) != sign(sample[i-1]):
        crossings++
zcr = crossings / sampleCount
```

- **High ZCR**: Noisy, sibilant, percussive ("s" sounds, hi-hats)
- **Low ZCR**: Tonal, bass-heavy

Simple time-domain feature, no FFT needed.

---

## Derived / Ratio Features

These combine existing features for more semantic meaning.

### Bass/Treble Ratio ("Warmth")

```
warmth = bass / (treb + epsilon)
```

High during bass drops, low during bright sections. Log scale often more useful: `log2(warmth)`.

### Mid Prominence

How much the midrange stands out vs extremes.

```
midProminence = mid / (bass + treb + epsilon)
```

Spikes during vocal sections, guitar solos, synth leads.

### Band Differences

Directional features for modulation.

```
bassDominance = bass - treb      // positive = bass-heavy
trebDominance = treb - bass      // positive = bright
midContrast = mid - (bass + treb) / 2
```

### Spectral Tilt

Linear regression slope across log-magnitude spectrum. Indicates overall spectral balance.

```
// Fit line: logMag[k] = slope * k + intercept
// Using least squares:
meanK = (N-1) / 2
meanLogMag = sum(log(magnitude[k] + epsilon)) / N
slope = sum((k - meanK) * (log(magnitude[k]) - meanLogMag)) / sum((k - meanK)^2)
```

- **Negative slope**: Energy concentrated in low frequencies (typical music)
- **Near zero**: Flat spectrum (noise-like)
- **Positive slope**: Bright, treble-heavy (rare)

---

## Advanced Features

### Tempo Estimation (BPM)

Autocorrelation of onset/flux signal over several seconds.

```
// 1. Compute onset signal (spectral flux) over ~4-8 seconds
// 2. Autocorrelate
for lag = minLag to maxLag:  // lag in samples, corresponds to BPM range
    autocorr[lag] = sum(onset[i] * onset[i + lag])

// 3. Find peaks in autocorrelation
// Peak at lag L corresponds to: BPM = 60 * sampleRate / (L * hopSize)
```

Typical search range: 60-180 BPM. Often needs harmonic analysis (peaks at 2x, 0.5x tempo).

### Onset Detection (Precise)

More sophisticated than simple flux thresholding.

```
// Complex-domain onset detection (phase + magnitude)
deviation[k] = magnitude[k] - |predicted[k]|
// where predicted uses phase extrapolation from previous frames

onset = sum(max(0, deviation[k]))
```

Or use adaptive thresholding:
```
threshold[t] = alpha * max(flux[t-W:t]) + (1-alpha) * median(flux[t-W:t])
onset = flux[t] > threshold[t]
```

### Chroma Features (Pitch Class)

Which musical notes (C, C#, D, ..., B) are present, regardless of octave.

```
// Map each FFT bin to its pitch class (0-11)
for k = 1 to N-1:
    freq = k * sampleRate / fftSize
    midi = 69 + 12 * log2(freq / 440)  // A4 = 69
    pitchClass = round(midi) % 12
    chroma[pitchClass] += magnitude[k]^2

// Normalize
chroma /= sum(chroma)
```

Useful for harmonic analysis, key detection, chord recognition.

### Harmonic-to-Noise Ratio (HNR)

Separates harmonic (pitched) content from noise.

```
// 1. Find fundamental frequency (autocorrelation of time signal)
// 2. Sum energy at f0, 2*f0, 3*f0... = harmonic energy
// 3. Remaining energy = noise
// 4. HNR = 10 * log10(harmonicEnergy / noiseEnergy)
```

High HNR = clean vocals/instruments. Low HNR = breathy, noisy, distorted.

### MFCCs (Mel-Frequency Cepstral Coefficients)

Standard in speech/music ML. Captures spectral shape in a compact form.

```
// 1. Apply mel filterbank (20-40 triangular filters, log-spaced)
// 2. Take log of filterbank energies
// 3. Apply DCT (discrete cosine transform)
// 4. Keep first 12-20 coefficients
```

Mostly useful for ML classification, less intuitive for direct visualization.

---

## Implementation Priority

For visualization bang-for-buck:

1. **Spectral flatness** - Distinguishes percussive vs tonal sections
2. **Spectral spread** - Texture descriptor, complements centroid
3. **Full-band flux** - General activity/change metric
4. **Crest factor** - Punchiness, dynamics indicator
5. **Rolloff** - Alternative brightness measure

Lower priority (more complex, niche use):
- Zero crossing rate (simple but less useful than spectral features)
- Tempo estimation (requires longer analysis window, complex)
- Chroma (music theory applications)
- HNR (requires pitch detection first)
