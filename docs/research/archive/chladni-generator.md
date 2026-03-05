# Chladni Generator

FFT-driven resonant plate mode visualization. Each (n,m) eigenmode has a resonant frequency proportional to n²+m². The shader generates all valid mode pairs mathematically, maps each to its corresponding FFT bin, and weights by energy. The result is a superposition of standing wave patterns that directly reflects the audio spectrum — every frequency present in the audio excites its corresponding plate mode.

## Classification

- **Category**: GENERATORS > Field (Cymatics subcategory)
- **Pipeline Position**: Generator stage
- **Compute Model**: Fragment shader with ping-pong trail persistence

## References

- [Paul Bourke - Chladni Plate Interference Surfaces](https://paulbourke.net/geometry/chladni/) - Mathematical foundation, rectangular and circular plate equations, mode shape gallery
- Existing `shaders/chladni_warp.fs` - Rectangular plate equation and gradient already implemented in the codebase (used for UV displacement)
- Existing `shaders/interference.fs` - Color modes (intensity, per-source, chromatic) to borrow

## Reference Code

### Rectangular Plate Equation (from chladni_warp.fs)

```glsl
const float PI = 3.14159265;

float chladni(vec2 uv, float n_val, float m_val, float L) {
    float nx = n_val * PI / L;
    float mx = m_val * PI / L;
    return cos(nx * uv.x) * cos(mx * uv.y) - cos(mx * uv.x) * cos(nx * uv.y);
}
```

Note: returns 0 when n == m (trivial mode). All mode pairs must have n != m.

### Chromatic Color Mode (from interference.fs)

```glsl
// Run wave computation 3 times with slightly offset parameters per RGB channel
vec3 chromaScale = vec3(1.0 - chromaSpread, 1.0, 1.0 + chromaSpread);
vec3 chromaWave;
for (int c = 0; c < 3; c++) {
    chromaWave[c] = computePattern(uv, plateSize * chromaScale[c]);
}
```

## Algorithm

### Mode Generation (mathematical, not hardcoded)

Generate all (n,m) integer pairs where n < m (skip n == m, skip n > m to avoid duplicates). Each pair's resonant frequency is proportional to n² + m². Map the lowest mode (1,2) to `baseFreq`:

```
freqScale = baseFreq / 5.0    // 5.0 = 1² + 2²
modeFreq(n,m) = freqScale * (n² + m²)
```

Iterate n,m from 1 up to `sqrt(maxFreq / freqScale)`, capped at 20 for GPU loop bounds. This covers all modes whose resonant frequency falls within [baseFreq, maxFreq]. With default settings (baseFreq=55, maxFreq=14000), this produces dozens of modes spanning the full spectrum.

### FFT-to-Mode Mapping (direct bin lookup, not band averaging)

Each mode maps directly to its resonant frequency's FFT bin:

```glsl
float bin = modeFreq / (sampleRate * 0.5);
float mag = texture(fftTexture, vec2(bin, 0.5)).r;
mag = pow(clamp(mag * gain, 0.0, 1.0), curve);
mag = pow(mag, mix(1.0, 4.0, coherence)); // coherence sharpening
```

No baseBright — silent frequencies produce zero weight. No energy = no pattern.

### Coherence

`pow(mag, mix(1.0, 4.0, coherence))` — at coherence=0, raw FFT magnitudes used directly. At coherence=1, weak modes are raised to the 4th power, crushing them toward zero so only the loudest modes survive.

### Nodal Emphasis

Blends both the pattern value and the brightness model:
- Pattern: `mix(totalPattern, abs(totalPattern), nodalEmphasis)`
- Brightness: `mix(compressed * 0.5 + 0.5, abs(compressed), nodalEmphasis)`

At 0: asymmetric — positive regions bright, negative dark. At 1: symmetric — both peaks bright, nodal lines dark. At 0.5: blend between the two (visually the richest).

### Trail Persistence

Screen-blend accumulation (self-capping, no overflow):
```glsl
existing *= decayFactor;
finalColor = existing + newColor * (1.0 - existing);
```

### Normalization

`totalPattern /= totalWeight` — keeps output range consistent regardless of how many modes are active.

## Parameters

### Wave

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| plateSize | float | 0.5-3.0 | 1.0 | Virtual plate scale. Smaller = denser nodal patterns |
| coherence | float | 0.0-1.0 | 0.5 | Sharpens FFT response — high values suppress weak modes |
| visualGain | float | 0.5-5.0 | 1.5 | Output intensity multiplier |
| nodalEmphasis | float | 0.0-1.0 | 0.0 | 0=asymmetric signed, 0.5=blend, 1=symmetric abs |

### Audio

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| baseFreq | float | 27.5-440 | 55.0 | Lowest resonant frequency — sets the (1,2) mode |
| maxFreq | float | 1000-16000 | 14000.0 | Highest resonant frequency — caps mode generation |
| gain | float | 0.1-10.0 | 2.0 | FFT energy sensitivity |
| curve | float | 0.1-3.0 | 1.5 | Contrast exponent on FFT energy |

### Trail

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| decayHalfLife | float | 0.05-5.0 | 0.3 | Trail persistence seconds |
| diffusionScale | int | 0-8 | 2 | Spatial blur tap spacing |

Plus standard ColorConfig gradient and blend compositor (blendMode, blendIntensity).

## Modulation Candidates

- **coherence**: Modulate between full superposition and dominant-mode-only
- **plateSize**: Shifts all nodal line density
- **visualGain**: Brightness response to audio dynamics
- **nodalEmphasis**: Shift between asymmetric/symmetric visualization
- **baseFreq**: Shifts which frequencies excite which modes

### Interaction Patterns

- **coherence vs spectrum width (cascading threshold)**: At high coherence, only the loudest frequency's mode shows. At low coherence, every active frequency contributes. Modulating coherence with overall energy creates verse (clean) vs chorus (complex) dynamics.
- **plateSize vs mode complexity (resonance)**: Smaller plate + many high-frequency modes = very dense nodal grids. Larger plate + bass-only = simple bold shapes.

## Implementation Notes

These notes reflect lessons from the initial implementation and subsequent fixes.

- **No hardcoded mode table**: Modes are generated mathematically by iterating (n,m) pairs. The number of active modes is determined entirely by baseFreq, maxFreq, and the audio content. Do not limit to a fixed count.
- **No baseBright**: Silent frequencies produce zero weight. No energy = no pattern. This is correct — showing patterns where there is no audio is meaningless.
- **Direct FFT bin lookup, not band averaging**: Each mode maps to exactly one FFT bin at its resonant frequency. Band averaging was designed for effects with a fixed layer count; Chladni modes have natural frequencies that map directly.
- **GPU loop bound**: `maxN = min(int(sqrt(maxFreq / freqScale)) + 1, 20)`. The nested n,m loop is at most 20x20=400 iterations, but early `continue` on out-of-range modes and silent bins keeps actual work much lower.
- **No plate boundary mask**: The pattern fills the full screen. The Chladni equation is periodic so it tiles, but with many modes superimposed the tiling is not visually obvious. A boundary mask cuts off content and makes the visual a small circle — users do not want this.
- **Aspect correction**: UV is aspect-corrected (`uv.x *= aspect`) so patterns are not stretched on non-square screens.
- **Screen-blend trail accumulation**: `existing + newColor * (1 - existing)`. Self-capping at 1.0, no overflow. Do not use `max(existing, newColor)` — it causes burn-in.
- **Circular plate modes (Bessel) not yet implemented**: The current implementation is rectangular only. Circular modes require Bessel function approximations in GLSL. This is a future enhancement, not a blocker.
- **Color modes from Interference not yet implemented**: Chromatic and per-source color modes were discussed but not added. These would be valuable additions.
