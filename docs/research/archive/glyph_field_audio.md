# Glyph Field Audio Reactivity

Add per-semitone FFT reactivity to the glyph field generator. Each cell gets a random semitone via hash, colors from the gradient LUT by pitch class, and brightness that scales with FFT magnitude. Layers stack by octave — front layer covers the highest octave, back layer covers the lowest — so bass notes light up deep layers while treble lights up the front.

## Classification

- **Category**: GENERATORS (existing effect enhancement)
- **Pipeline Position**: Same slot — generator pass, no new shader file
- **Chosen Approach**: Balanced — adds FFT semitone color/brightness per cell with octave-stacked layers, preserving all existing glyph field mechanics (scroll, flutter, wave, drift, inversion, LCD)

## References

- `shaders/nebula.fs:55-81` — per-semitone hash assignment, FFT lookup, LUT color by pitch class, brightness = baseBright + mag * gain
- `shaders/spectral_arcs.fs:50-78` — frequency-to-bin conversion, gain/curve shaping
- `shaders/glyph_field.fs:138-159` — existing per-cell hash values (`h` and `h2` from `pcg3df`), color from `h.z` into gradient LUT

## Algorithm

All techniques already exist in the codebase. This combines the established FFT semitone pattern with glyph field's existing cell hash system.

### Semitone Assignment (from nebula starLayer)

Each cell derives a semitone index from its existing hash. The shader already computes `h = pcg3df(vec3(cellCoord, layerF))` per cell. Use one hash channel for semitone:

```
semi = floor(h.z * float(semitonesThisLayer))
```

### Octave Stacking Across Layers

Each layer covers one octave. With `numOctaves` octaves and `layerCount` layers:

- Layer 0 (front, smallest scale) → highest octave
- Layer N (back, largest scale) → lowest octave

The semitone range per layer is 12 notes. The octave offset per layer:

```
octaveIndex = numOctaves - 1 - layer  (clamped to valid range)
globalSemi = octaveIndex * 12 + floor(h.z * 12.0)
```

When `layerCount > numOctaves`, extra layers share octaves. When `layerCount < numOctaves`, only the outermost octaves are represented.

### FFT Lookup (from spectral arcs / pitch spiral)

Convert semitone to frequency, then to normalized FFT bin:

```
freq = baseFreq * pow(2.0, globalSemi / 12.0)
bin = freq / (sampleRate * 0.5)
mag = texture(fftTexture, vec2(bin, 0.5)).r   // if bin <= 1.0
mag = pow(clamp(mag * gain, 0.0, 1.0), curve)
```

### Color by Pitch Class

Replace the current `lutPos = h.z` with pitch class:

```
pitchClass = fract(float(globalSemi) / 12.0)
glyphColor = textureLod(gradientLUT, vec2(pitchClass, 0.5), 0.0).rgb
```

Same note = same color regardless of octave or layer. The gradient LUT defines the 12-note color palette.

### Brightness Modulation

The existing accumulation line:

```
total += glyphColor * glyphAlpha * opacity;
```

Becomes:

```
float react = baseBright + mag * mag * gain;
total += glyphColor * glyphAlpha * opacity * react;
```

Silent cells dim to `baseBright` (can go to zero for full darkness in silence). Active cells flare proportional to FFT magnitude. The existing `layerOpacity` falloff still applies on top.

### Existing Mechanics Preserved

All current glyph field features remain unchanged:
- Scroll direction and per-row/column speed variation
- Character flutter (cycling through atlas glyphs)
- Wave distortion (sine displacement)
- Drift (per-cell position wander)
- Band distortion (row compression)
- Inversion flicker
- LCD sub-pixel overlay

These operate on UV coordinates and glyph selection — orthogonal to the color/brightness changes.

## Parameters

### New Parameters

| Parameter | Type | Range | Default | Visual Effect |
|-----------|------|-------|---------|---------------|
| baseFreq | float | 20.0–200.0 | 55.0 | Lowest mapped frequency (A1 default) |
| numOctaves | int | 1–8 | 5 | Octave range across layers |
| gain | float | 0.1–10.0 | 2.0 | FFT magnitude amplification |
| curve | float | 0.1–3.0 | 0.7 | Contrast shaping exponent |
| baseBright | float | 0.0–1.0 | 0.15 | Minimum glyph brightness when note is silent |

### Existing Parameters (unchanged)

All current glyph field parameters remain and function identically.

## Modulation Candidates

- **gain**: controls how dramatically active notes flare above the baseline
- **baseBright**: controls visibility of silent cells — zero makes the field dark until notes play
- **curve**: shapes the response curve — low values make more cells visible, high values concentrate brightness on the loudest notes
- **gridSize**: denser grid = more cells per semitone = finer-grained reactivity pattern
- **layerOpacity**: combined with octave stacking, controls how much background (bass) octaves contribute vs foreground (treble)

### Interaction Patterns

- **Cascading Threshold**: `baseBright` gates `gain` — at zero baseBright, the field is black during silence and erupts with glyphs when music plays. At high baseBright, gain adds subtle variation on an already-visible field. The difference between verse and chorus becomes visible: quiet passages dim the field, loud passages fill it.
- **Competing Forces**: `layerOpacity` and octave stacking create frequency-depth tension — low layerOpacity suppresses back layers (bass), making the field treble-dominant. High layerOpacity lets bass notes contribute equally, filling the field with deeper color activity.

## Notes

- The `pcg3df` hash already provides three independent channels (`h.x`, `h.y`, `h.z`). Using `h.z` for semitone assignment means the character selection hash (`h.x`, `h.y`) stays independent — which glyph is displayed doesn't correlate with which note it responds to.
- Performance: one FFT texture lookup per cell per layer. With `gridSize=24` and `layerCount=2`, this is comparable to nebula's star layer cost.
- `sampleRate` and `fftTexture` uniforms follow the same binding pattern as pitch spiral, nebula, and spectral arcs in `shader_setup_generators.cpp`.
- The LCD sub-pixel overlay applies after accumulation — it modulates the final color including audio reactivity, which is correct (LCD should affect everything uniformly).
- When `numOctaves < layerCount`, multiple layers share the same octave. When `numOctaves > layerCount`, only a subset of octaves are visible. The mapping clamps gracefully in both directions.
