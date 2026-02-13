# Scan Bars Audio Reactivity

Add per-bar FFT brightness modulation to the scan bars generator. Each bar stripe gets a semitone from its bar index. Active notes make their bar glow brighter; silent bars dim. The existing chaos color function stays untouched — FFT only drives brightness, preserving the wild palette-jumping character. In ring mode, inner rings respond to bass and outer rings to treble.

## Classification

- **Category**: GENERATORS (existing effect enhancement)
- **Pipeline Position**: Same slot — generator pass, no new shader file
- **Chosen Approach**: Balanced — adds FFT per-bar brightness while preserving all existing scan bar mechanics (chaos coloring, convergence, scroll, all three layout modes)

## References

- `shaders/nebula.fs:55-81` — FFT semitone lookup pattern: frequency-to-bin conversion, gain/curve shaping
- `shaders/spectral_arcs.fs:53-59` — `baseFreq * pow(2.0, float(i) / 12.0)` semitone frequency calculation
- `shaders/scan_bars.fs:93-109` — existing bar identity from `fract(barDensity * coord + scroll)`, chaos color from `safeTan(coord * chaosFreq + colPhase)`

## Algorithm

All techniques already exist in the codebase. This adds the established FFT semitone lookup to scan bars' existing bar identity system.

### Bar Index to Semitone

Each bar has an implicit index from the repeating stripe coordinate. Extract the bar index before `fract()`:

```
barCoord = barDensity * coord + scroll
barIndex = floor(barCoord)
```

Map bar index to semitone with wrapping:

```
totalSemitones = numOctaves * 12
semi = mod(barIndex, totalSemitones)
```

Wrapping ensures bars beyond the semitone count cycle back through the note range. Adjacent bars get adjacent semitones — in ring mode this creates a natural frequency-to-radius mapping.

### FFT Lookup (from spectral arcs / pitch spiral)

Convert semitone to frequency, then to normalized FFT bin:

```
freq = baseFreq * pow(2.0, semi / 12.0)
bin = freq / (sampleRate * 0.5)
mag = texture(fftTexture, vec2(bin, 0.5)).r   // if bin <= 1.0
mag = pow(clamp(mag * gain, 0.0, 1.0), curve)
```

### Brightness Modulation

The existing final output:

```
finalColor = vec4(color.rgb * mask, 1.0);
```

Becomes:

```
float react = baseBright + mag * gain;
finalColor = vec4(color.rgb * mask * react, 1.0);
```

The chaos color (`color.rgb`) is unchanged. The bar mask (`mask`) is unchanged. Only a brightness multiplier is added.

### Mode-Specific Behavior

The semitone mapping works identically across all three modes because it operates on `coord`, which each mode computes differently:

- **Linear** (`coord = rotated.x`): bars map left-to-right by frequency along the angle direction. Low notes on one side, high on the other.
- **Spokes** (`coord = atan(uv.y, uv.x) / TAU + 0.5`): notes distributed around the circle. Adjacent angular bars are adjacent semitones.
- **Rings** (`coord = length(uv)`): inner rings = low frequencies, outer rings = high. Natural spectral-ring layout — bass pulses in the center, treble flares at the edges.

### Scroll Interaction

Since `barIndex = floor(barDensity * coord + scroll)`, the semitone assignment travels with the bar as it scrolls. A bar that starts as C4 remains C4 as it moves across the screen. The frequency mapping slides with the animation rather than being pinned to screen position.

## Parameters

### New Parameters

| Parameter | Type | Range | Default | Visual Effect |
|-----------|------|-------|---------|---------------|
| baseFreq | float | 20.0–200.0 | 55.0 | Lowest mapped frequency (A1 default) |
| numOctaves | int | 1–8 | 5 | Octave range mapped across bars |
| gain | float | 0.1–10.0 | 2.0 | FFT magnitude amplification |
| curve | float | 0.1–3.0 | 0.7 | Contrast shaping exponent |
| baseBright | float | 0.0–1.0 | 0.15 | Minimum bar brightness when note is silent |

### Existing Parameters (unchanged)

All current scan bars parameters remain and function identically. The chaos color system (`chaosFreq`, `chaosIntensity`, `colorSpeed`) is fully preserved.

## Modulation Candidates

- **gain**: controls how dramatically active bars flare above the baseline
- **baseBright**: controls visibility of silent bars — zero makes only active-note bars visible
- **curve**: shapes the response — low values spread brightness across more bars, high values concentrate on the loudest notes
- **barDensity**: more bars = finer semitone resolution, fewer bars = coarser (each bar spans more semitones)
- **convergence**: combined with frequency mapping, convergence bunches frequency bands together at the focal point — bass and treble bars crowd together where convergence is strong

### Interaction Patterns

- **Cascading Threshold**: `baseBright` gates visibility — at zero, only bars with active FFT energy appear. The bar field goes dark during silence and erupts during loud passages. At high baseBright, all bars are visible and gain adds subtle brightness variation.
- **Competing Forces**: `barDensity` and `numOctaves` interact — when barDensity exceeds totalSemitones, bars wrap and multiple bars share the same note (they flare together). When barDensity is less than totalSemitones, some semitones are never mapped and their energy is invisible.

## Notes

- The `barIndex` can be negative (bars to the left of center in linear mode, or during reverse scroll). Use `mod()` (not `%`) for correct wrapping with negative values.
- Performance impact is minimal: one FFT texture lookup per pixel. The bar index computation (`floor()`) is trivial.
- `sampleRate` and `fftTexture` uniforms follow the same binding pattern as other FFT-reactive generators in `shader_setup_generators.cpp`.
- The convergence distortion applies before bar identity — converged bars still map correctly because `coord` is computed after convergence warping.
- Plasma stays unreactive — its continuous bolt structure and gradient-by-distance coloring don't lend themselves to per-semitone mapping without losing the effect's character.
