# Constellation Audio Reactivity

Add per-semitone FFT reactivity to the constellation generator. Each star gets a random semitone via cell hash, colors from the gradient LUT by pitch class, and brightness that scales with FFT magnitude. The existing radial wave acts as a traveling brightness multiplier — stars at the wave crest pulse brighter, stars in the trough dampen.

## Classification

- **Category**: GENERATORS (existing effect enhancement)
- **Pipeline Position**: Same slot — generator pass, no new shader file
- **Chosen Approach**: Balanced — adds FFT semitone color/brightness to stars while preserving existing wave, fill, and line mechanics

## References

- `shaders/nebula.fs:55-81` — per-semitone star layer with hash-based assignment, FFT lookup, and LUT color by pitch class
- `shaders/spectral_arcs.fs:50-78` — semitone frequency calculation, `baseFreq * pow(2.0, i / 12.0)`, gain/curve shaping
- `shaders/pitch_spiral.fs:74-88` — frequency-to-bin conversion, `freq / (sampleRate * 0.5)`, magnitude clamping
- `shaders/constellation.fs:44-51` — existing wave mechanic (`radial = sin(dist * waveFreq - wavePhase) * waveAmp`)

## Algorithm

All techniques already exist in the codebase. This combines three established patterns:

### Semitone Assignment (from nebula starLayer)

Each star derives a semitone index from its cell hash:

```
semi = floor(hash(cellID) * totalSemitones)
```

Where `totalSemitones = numOctaves * 12`. The hash value already exists (`N21(cellID)` in the current shader).

### FFT Lookup (from spectral arcs / pitch spiral)

Convert semitone to frequency, then to normalized FFT bin:

```
freq = baseFreq * pow(2.0, semi / 12.0)
bin = freq / (sampleRate * 0.5)
mag = texture(fftTexture, vec2(bin, 0.5)).r   // if bin <= 1.0
mag = pow(clamp(mag * gain, 0.0, 1.0), curve)
```

### Color by Pitch Class (from nebula / spectral arcs)

Replace the current hash-based LUT index with pitch class:

```
pitchClass = fract(semi / 12.0)
color = texture(pointLUT, vec2(pitchClass, 0.5)).rgb
```

Same octave wraps to the same color. Current `lineLUT` sampling (by length) stays unchanged. When `interpolateLineColor` is on, lines automatically inherit semitone colors from their endpoints.

### Wave as Brightness Multiplier (adaptation of existing wave)

The existing `radial` value in `GetPos()` drives position. Compute the same wave factor for brightness:

```
waveFactor = sin(distance_to_waveCenter * waveFreq - wavePhase) * 0.5 + 0.5
brightness = (baseBright + mag * gain) * mix(1.0, waveFactor, waveInfluence)
```

- `waveInfluence = 0.0` → uniform audio brightness, wave only affects position (backwards compatible)
- `waveInfluence = 1.0` → full traveling-wave brightness modulation

The `waveFactor` needs to be passed out of `GetPos()` or computed in `Point()` using the same `cellID + cellOffset - waveCenter` distance.

### Triangle Fill Integration

When `fillEnabled`, the triangle fill uses `BarycentricColor()` which samples `pointLUT` per vertex. Since the LUT index changes from hash to pitch class, fills automatically pick up semitone colors. Fill brightness can optionally scale with the average magnitude of its three vertex semitones.

## Parameters

### New Parameters

| Parameter | Type | Range | Default | Visual Effect |
|-----------|------|-------|---------|---------------|
| baseFreq | float | 20.0–200.0 | 55.0 | Lowest mapped frequency (A1 default) |
| numOctaves | int | 1–8 | 5 | Octave range mapped across stars |
| gain | float | 0.1–10.0 | 2.0 | FFT magnitude amplification |
| curve | float | 0.1–3.0 | 0.7 | Contrast shaping exponent |
| baseBright | float | 0.0–1.0 | 0.15 | Minimum glow when note is silent |
| waveInfluence | float | 0.0–1.0 | 0.5 | How much the wave modulates brightness (0 = uniform, 1 = full wave gating) |

### Existing Parameters (unchanged)

All current constellation parameters remain. The wave mechanic (`waveFreq`, `waveAmp`, `waveSpeed`, `waveCenter`) still drives position AND now also drives brightness modulation.

## Modulation Candidates

- **gain**: louder overall reactivity when boosted, quieter when reduced
- **baseBright**: controls how visible silent stars are — low values make the field dark until notes hit
- **waveInfluence**: sweep between uniform glow and pulsing wave rings
- **waveSpeed**: faster wave = faster brightness ripples
- **numOctaves**: more octaves = wider frequency range mapped, more stars light up

### Interaction Patterns

- **Cascading Threshold**: `baseBright` gates `gain` — when baseBright is near zero, only stars with actual FFT magnitude are visible. The field appears empty during silence and erupts when music plays. With baseBright high, gain just adds subtle variation on top of an already-visible field.
- **Competing Forces**: `waveInfluence` opposes `baseBright` — high wave influence dampens stars in the trough even if baseBright is high, creating dark traveling bands. Low wave influence lets baseBright dominate uniformly.

## Notes

- The `pointLUT` gradient now maps to pitch class instead of hash. Users may want different gradient presets — chromatic (rainbow across 12 notes) vs. harmonic (related notes share hues).
- `lineLUT` (length-based) is unaffected. Only `interpolateLineColor` mode picks up semitone colors.
- Performance impact is minimal: one FFT texture lookup per star point (9 per cell), same as nebula's star layer.
- `sampleRate` and `fftTexture` uniforms follow the same binding pattern as pitch spiral, nebula, and spectral arcs in `shader_setup_generators.cpp`.
- The existing `pointBrightness` parameter multiplies the final result — it remains as a global brightness control on top of the per-semitone reactivity.
