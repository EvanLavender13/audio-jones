# Chladni Warp

UV displacement transform using Chladni plate equations. The gradient field of the Chladni function displaces texture coordinates toward or along nodal lines, creating kaleidoscopic warping patterns. Animated n/m parameters produce morphing symmetry; audio-reactive parameters sync pattern complexity to music.

## Classification

- **Category**: TRANSFORMS → Warp (alongside Sine Warp, Gradient Flow, Wave Ripple)
- **Core Operation**: Compute Chladni field + gradient, displace UVs along gradient direction
- **Pipeline Position**: Runs during transform stage (user-ordered with other transforms)

## References

- [Paul Bourke - Chladni Plate Interference Surfaces](https://paulbourke.net/geometry/chladni/) - Mathematical equations for rectangular/circular plates, nodal patterns
- [Mark Serena - UE Chladni Material](https://www.markserena.com/post/ue_chladni_material/) - Parameter ranges, sine vs cosine variants
- [luciopaiva/chladni GitHub](https://github.com/luciopaiva/chladni) - Gradient field via finite differences for particle flow
- Shadertoy: Fernando Recio "Chladni patterns" - Reciprocal visualization (`1/|f|`)
- Shadertoy: Korhan Kaya "Chladni Plate Simulation" - Modal superposition with resonance
- Shadertoy: Envy24 variant - Animated n/m parameters, gradient-based edge detection

## Algorithm

### Core Chladni Equation (Rectangular Plate)

```glsl
float chladni(vec2 uv, float n, float m, float L) {
    float PI = 3.14159265;
    return cos(n * PI * uv.x / L) * cos(m * PI * uv.y / L)
         - cos(m * PI * uv.x / L) * cos(n * PI * uv.y / L);
}
```

Nodal lines occur where `chladni(uv, n, m, L) = 0`. Different n/m ratios produce distinct symmetric patterns.

### Gradient Computation (Finite Differences)

```glsl
vec2 chladniGradient(vec2 uv, float n, float m, float L) {
    float h = 0.001;  // Step size
    float dx = chladni(uv + vec2(h, 0), n, m, L) - chladni(uv - vec2(h, 0), n, m, L);
    float dy = chladni(uv + vec2(0, h), n, m, L) - chladni(uv - vec2(0, h), n, m, L);
    return vec2(dx, dy) / (2.0 * h);
}
```

Gradient points away from nodal lines (toward maxima/minima of the field).

### Analytical Gradient (More Efficient)

```glsl
vec2 chladniGradientAnalytical(vec2 uv, float n, float m, float L) {
    float PI = 3.14159265;
    float nx = n * PI / L;
    float mx = m * PI / L;

    // Partial derivatives via chain rule
    float dfdx = -nx * sin(nx * uv.x) * cos(mx * uv.y)
                 +mx * sin(mx * uv.x) * cos(nx * uv.y);
    float dfdy = -mx * cos(nx * uv.x) * sin(mx * uv.y)
                 +nx * cos(mx * uv.x) * sin(nx * uv.y);

    return vec2(dfdx, dfdy);
}
```

### UV Displacement Transform

```glsl
// Warp UV coordinates based on Chladni field
vec2 chladniWarp(vec2 uv, float n, float m, float L, float strength, int mode) {
    float f = chladni(uv, n, m, L);
    vec2 grad = chladniGradientAnalytical(uv, n, m, L);
    float gradLen = length(grad);

    if (gradLen < 0.001) return uv;  // Avoid division by zero at critical points

    vec2 dir = grad / gradLen;  // Normalized gradient direction

    if (mode == 0) {
        // Mode 0: Flow toward nodal lines (sand accumulation)
        // Displace in direction of gradient, scaled by field value
        return uv - dir * strength * f;
    } else if (mode == 1) {
        // Mode 1: Flow perpendicular to gradient (stream along nodal lines)
        vec2 perp = vec2(-dir.y, dir.x);
        return uv + perp * strength * f;
    } else {
        // Mode 2: Intensity-based displacement (stronger near antinodes)
        return uv + dir * strength * abs(f);
    }
}
```

### Animation via Time-Varying Parameters

From Envy24's shader - animate n/m for morphing patterns:

```glsl
// Smooth oscillation between integer modes
float t = time * 0.25;  // Slow animation
float n1 = 6.0 + 3.0 * sin(t);
float m1 = 4.0 + 3.0 * cos(t);

// Optional: blend two Chladni fields for richer patterns
float n2 = 5.0 + 2.5 * cos(2.0 * t);
float m2 = 3.0 + 2.5 * sin(2.0 * t);

float f = chladni(uv, n1, m1, L) + chladni(uv, n2, m2, L);
```

### Kaleidoscopic Pre-Fold (Optional)

Apply coordinate folding before Chladni evaluation for additional symmetry:

```glsl
// Cartesian fold (like KIFS)
vec2 foldedUV = abs(uv);

// Or polar wedge fold
float angle = atan(uv.y, uv.x);
float foldedAngle = mod(angle, PI / float(segments)) * sign(mod(angle, PI / float(segments) * 2.0) - PI / float(segments));
vec2 foldedUV = length(uv) * vec2(cos(foldedAngle), sin(foldedAngle));
```

### Edge-Tracing Visualization (Fernando Recio's Technique)

For stylized rendering along nodal lines:

```glsl
float f = chladni(uv, n, m, L);
float edgeIntensity = 1.0 / (abs(f) + 0.02);  // Reciprocal: bright at nodes
edgeIntensity = pow(edgeIntensity, 1.5);       // Sharpen
```

## Parameters

| Parameter | Type | Range | Effect |
|-----------|------|-------|--------|
| n | float | 1.0 - 12.0 | X-axis frequency mode (integer = pure mode, fractional = interpolated) |
| m | float | 1.0 - 12.0 | Y-axis frequency mode |
| plateSize | float | 0.5 - 2.0 | Normalized plate dimension (L). Smaller = denser patterns |
| strength | float | 0.0 - 0.5 | UV displacement magnitude |
| warpMode | int | 0, 1, 2 | 0=toward nodes, 1=along nodes, 2=intensity-based |
| animSpeed | float | 0.0 - 2.0 | Rate of n/m oscillation (radians/second) |
| animRange | float | 0.0 - 5.0 | Amplitude of n/m oscillation |
| blendLayers | int | 1 - 3 | Number of overlapping Chladni fields |
| preFold | bool | false | Enable Cartesian abs() symmetry before evaluation |

### Animation Fields (CPU-Accumulated)

| Field | Type | Range | Effect |
|-------|------|-------|--------|
| animPhase | float | accumulated | Internal phase for n/m animation (accumulated on CPU) |

## Audio Reactivity

Use the existing modulation system (`ModulatableSlider`) - no shader FFT sampling needed.

### Existing Mod Sources

| Source | Description |
|--------|-------------|
| `BASS` | Low frequency energy (20-250 Hz) |
| `MID` | Mid frequency energy (250-4000 Hz) |
| `TREB` | High frequency energy (4000-20000 Hz) |
| `BEAT` | Beat detection trigger |
| `LFO1-4` | Oscillators with configurable waveforms |

### Suggested New Source: Spectral Centroid

Add `MOD_SOURCE_CENTROID` for timbral brightness (weighted average frequency). Implementation in `bands.cpp`:

```cpp
// In BandEnergiesProcess(), after computing band energies:
float weightedSum = 0.0f;
float totalEnergy = 0.0f;
for (int i = 1; i < binCount; i++) {
    weightedSum += (float)i * magnitude[i];
    totalEnergy += magnitude[i];
}
float centroid = (totalEnergy > 0.001f) ? weightedSum / totalEnergy : 0.0f;
bands->centroid = centroid / (float)binCount;  // Normalize to [0,1]
```

### Parameter Mapping

| Parameter | Suggested Source | Effect |
|-----------|------------------|--------|
| `n` | CENTROID | Brighter audio → higher mode complexity |
| `m` | MID | Mid energy → pattern asymmetry |
| `strength` | BASS | Bass hits → stronger warping |
| `animSpeed` | BEAT | Pulse animation phase on beat |

All parameters use standard `ModulatableSlider` with per-parameter depth/source selection.

## Implementation Notes

### Performance

- Analytical gradient saves 4 trig calls vs finite differences (6 vs 10 trig ops per pixel)
- blendLayers > 2 adds significant cost (full Chladni + gradient per layer)
- preFold adds 2-4 ops (abs or atan)

### Coordinate Space

- Center UV at origin: `vec2 centered = uv * 2.0 - 1.0`
- Aspect correction: `centered.x *= aspectRatio`
- Pattern centered at (0,0) with typical L=1.0 spans [-1, 1]

### Edge Handling

At large displacements, UVs may exit [0,1] range. Options:
- Clamp: `clamp(warpedUV, 0.0, 1.0)`
- Repeat: `fract(warpedUV)`
- Mirror: `abs(fract(warpedUV * 0.5) * 2.0 - 1.0)` (seamless tiling)

### Relation to Existing Effects

- **Gradient Flow**: Similar concept (UV displacement along gradient) but uses Sobel on input texture rather than procedural field
- **Wave Ripple**: Radial sine displacement - Chladni Warp generalizes this to rectangular plate modes
- **Kaleidoscope**: Pre-fold symmetry can combine with Chladni for compound effects
- **KIFS**: Cartesian folding shares the `abs(uv)` technique

### Angular Field Naming

If adding rotation animation (e.g., rotating the Chladni pattern):
- `rotationSpeed`: rate in radians/frame, CPU-accumulated
- `rotationAngle`: static offset in radians
