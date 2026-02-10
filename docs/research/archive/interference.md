# Interference

Concentric circular waves radiating from multiple point sources, creating classic constructive/destructive interference patterns. Where wavefronts align, intensity doubles; where they oppose, they cancel to darkness. The pattern shifts as sources drift, producing moiré-like geometry that breathes and morphs.

## Classification

- **Category**: GENERATORS
- **Pipeline Position**: Output stage, after trail boost, before transforms
- **Type**: Stateless fragment shader (no previous frame dependency)
- **Chosen Approach**: Balanced - CPU source animation (reuses DualLissajousConfig) + GPU wave summation with configurable visualization modes

## References

- Shadertoy examples (user-provided) demonstrating:
  - Two-source superposition: `wave = sin(d / wavelength - t * speed)`
  - Multi-source with polar interference summation
  - Chromatic separation via per-channel wavelength coefficients
  - Distance attenuation variants (1/d, 1/d², Gaussian)

## Algorithm

### Core Wave Equation

For each pixel, sum waves from all sources:

```glsl
float totalWave = 0.0;
for (int i = 0; i < sourceCount; i++) {
    float dist = length(uv - sources[i]);
    float wave = sin(dist * waveFreq - time * speed + phases[i]);
    float atten = falloff(dist, falloffType, falloffStrength);
    totalWave += wave * atten;
}
```

### Falloff Functions

```glsl
float falloff(float d, int type, float strength) {
    float safeDist = max(d, 0.001);
    if (type == 0) return 1.0;                              // None
    if (type == 1) return 1.0 / (1.0 + safeDist * strength); // Inverse
    if (type == 2) return 1.0 / (1.0 + safeDist * safeDist * strength); // Inverse square
    return exp(-safeDist * safeDist * strength);            // Gaussian
}
```

### Visualization Modes

**Raw (mode 0):** Direct sine output, produces smooth gradients
```glsl
intensity = totalWave * visualGain;
```

**Absolute (mode 1):** Takes abs() of sine, creates sharper rings with dark nodes
```glsl
intensity = abs(totalWave) * visualGain;
```

**Contour (mode 2):** Quantizes into discrete bands like Cymatics
```glsl
intensity = floor(totalWave * contourCount + 0.5) / contourCount * visualGain;
```

### Color Mapping

Two modes controlled by `colorMode` uniform:

**Mode 0 - Intensity (default):**
Global interference intensity determines LUT position. Peaks glow in gradient endpoint colors, nodes fade dark.

```glsl
// Map [-1,1] to [0,1] for LUT sampling
float t = totalWave * 0.5 + 0.5;
vec3 color = texture(colorLUT, vec2(t, 0.5)).rgb;
float brightness = abs(totalWave) * value;
finalColor = vec4(color * brightness, 1.0);
```

**Mode 1 - Per-source:**
Each source samples a fixed color from its LUT position (uniformly distributed). Waves blend additively where they overlap.

```glsl
vec3 color = vec3(0.0);
for (int i = 0; i < sourceCount; i++) {
    float lutPos = (float(i) + 0.5) / float(sourceCount);
    vec3 srcColor = texture(colorLUT, vec2(lutPos, 0.5)).rgb;
    color += srcColor * abs(waveContribution[i]);
}
float brightness = length(color) * value;
finalColor = vec4(color * brightness, 1.0);
```

With rainbow gradient and 3 sources: red/green/blue rings radiate outward, blending to cyan/magenta/yellow where two overlap, white where all three align.

### Chromatic Mode (Optional Variant)

Compute separate wavelengths for R/G/B channels:
```glsl
vec3 chromaScale = vec3(0.97, 1.0, 1.03); // RGB wavelength multipliers
vec3 wave;
for (int c = 0; c < 3; c++) {
    wave[c] = sin(dist * waveFreq * chromaScale[c] - time * speed);
}
```

### Source Animation (CPU Side)

Reuse Cymatics source pattern:
```cpp
// Circular distribution with Lissajous motion
for (int i = 0; i < count; i++) {
    float angle = TWO_PI * i / count + patternAngle;
    float baseX = baseRadius * cosf(angle);
    float baseY = baseRadius * sinf(angle);
    float offsetX, offsetY;
    DualLissajousCompute(&lissajous, phase, i * TWO_PI / count, &offsetX, &offsetY);
    sources[i] = vec2(baseX + offsetX, baseY + offsetY);
}
```

### Mirror Sources (Boundaries)

Reuse Cymatics boundary reflection pattern. For each real source, create 4 mirror sources reflected across screen edges:

```glsl
if (boundaries) {
    vec2 mirrors[4];
    mirrors[0] = vec2(-2.0 * aspect - sourcePos.x, sourcePos.y);  // Left
    mirrors[1] = vec2( 2.0 * aspect - sourcePos.x, sourcePos.y);  // Right
    mirrors[2] = vec2(sourcePos.x, -2.0 - sourcePos.y);           // Bottom
    mirrors[3] = vec2(sourcePos.x,  2.0 - sourcePos.y);           // Top

    for (int m = 0; m < 4; m++) {
        float mDist = length(uv - mirrors[m]);
        float wave = sin(mDist * waveFreq - time * speed);
        float mAtten = falloff(mDist, falloffType, falloffStrength) * reflectionGain;
        totalWave += wave * mAtten;
    }
}
```

This creates interference patterns that "bounce" off screen edges, filling corners with additional wave structure.

## Parameters

| Parameter | Type | Range | Default | Visual Effect |
|-----------|------|-------|---------|---------------|
| sourceCount | int | 1-8 | 3 | Number of wave emitters |
| baseRadius | float | 0.0-1.0 | 0.4 | Distance of sources from center |
| patternAngle | float | 0-2π | 0.0 | Rotation of source arrangement |
| lissajous | DualLissajousConfig | — | — | Source motion pattern |
| waveFreq | float | 5.0-100.0 | 30.0 | Ripple density (higher = tighter rings) |
| waveSpeed | float | 0.0-10.0 | 2.0 | Animation speed |
| falloffType | int | 0-3 | 3 | None/Inverse/InvSquare/Gaussian |
| falloffStrength | float | 0.0-5.0 | 1.0 | How quickly waves fade with distance |
| boundaries | bool | — | false | Enable edge reflections (mirror sources) |
| reflectionGain | float | 0.0-1.0 | 0.5 | Mirror source attenuation |
| visualMode | int | 0-2 | 0 | Raw/Absolute/Contour |
| contourCount | int | 0-20 | 0 | Band count (0 = smooth) |
| visualGain | float | 0.5-5.0 | 1.5 | Output intensity |
| chromatic | bool | — | false | Toggle RGB wavelength separation |
| chromaSpread | float | 0.0-0.1 | 0.03 | RGB wavelength difference (when chromatic=true) |
| colorMode | int | 0-1 | 0 | 0=Intensity (global), 1=PerSource (each source colored) |
| color | ColorConfig | — | gradient | Solid/Rainbow/Gradient via LUT |

## Modulation Candidates

- **waveFreq**: Ring density pulses with audio energy
- **waveSpeed**: Tempo-synced propagation
- **baseRadius**: Sources breathe inward/outward
- **patternAngle**: Slow rotation of the whole pattern
- **visualGain**: Intensity follows audio amplitude
- **falloffStrength**: Tighter/looser falloff with dynamics
- **reflectionGain**: Edge reflections fade in/out with dynamics
- **chromaSpread**: Rainbow fringing intensity follows audio

## Implementation Notes

**Stateless**: Unlike Cymatics (which writes to a trail map), this generator computes everything per-frame. No compute shader needed—standard fragment shader like Constellation/Plasma.

**Source limit**: 8 sources matches Cymatics. Loop unrolling may help performance for fixed counts.

**Tonemap**: When sources overlap, intensities can exceed [-1,1]. Apply `tanh()` or `1 - exp(-x)` like Plasma does:
```glsl
total = tanh(total * visualGain);
```

**Chromatic toggle**: When enabled, computes separate wavelengths for R/G/B. Single shader with uniform branch—the 3× cost only applies when toggled on. Creates rainbow fringing at interference nodes. Chromatic mode bypasses colorMode—it computes RGB intensities directly rather than sampling the LUT.

**Boundaries**: With 8 sources and boundaries enabled, worst case is 8 + 8×4 = 40 distance calculations per pixel. Still fast for a fragment shader, but worth noting.

**Contour banding**: Reuses Cymatics pattern—`floor(x * n + 0.5) / n` quantizes smoothly.
