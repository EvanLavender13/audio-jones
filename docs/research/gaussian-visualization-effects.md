# Gaussian Visualization Effects

Background research for Gaussian-based visual effects in audio visualizers.

## Core Concept

Gaussian functions produce smooth, bell-shaped falloff curves. The 2D Gaussian:

```
G(x,y) = exp(-(x² + y²) / (2σ²))
```

Creates circular falloff from a center point. Parameter σ (sigma) controls spread width. Values decay smoothly to near-zero at ~3σ distance.

Gaussians appear throughout graphics: blur kernels, particle rendering, noise generation, metaball surfaces, and density estimation.

## Effect Categories

### 1. Gaussian Blur Variants

AudioJones implements separable Gaussian blur (`shaders/blur_h.fs`, `shaders/blur_v.fs`). Extensions:

#### Radial Blur

Blur emanates from center point. Each pixel samples along the line toward/from center. Creates "zoom" motion effect.

```glsl
vec2 dir = normalize(uv - center);
vec3 color = vec3(0.0);
for (int i = 0; i < SAMPLES; i++) {
    float t = float(i) / float(SAMPLES - 1);
    float offset = t * blurStrength;
    color += texture(tex, uv - dir * offset).rgb;
}
color /= float(SAMPLES);
```

**Audio mapping:** Beat intensity controls `blurStrength` (0.0-0.1). Creates punch-out effect on transients.

#### Directional/Motion Blur

Blur along arbitrary angle. Simulates motion in specific direction.

```glsl
vec2 dir = vec2(cos(angle), sin(angle));
// Sample along dir instead of radial
```

**Audio mapping:** LFO controls angle. FFT bass energy controls blur amount.

#### Variable-Radius Blur (Depth of Field)

Blur radius varies per-pixel based on auxiliary buffer. Requires two passes: compute blur radius, then apply spatially-varying blur.

**Complexity:** High. Requires separate radius texture and iterative sampling.

### 2. Gaussian Metaballs

Render multiple points as summed Gaussian density fields. Threshold creates smooth, blobby isosurfaces that merge when overlapping.

```glsl
float density = 0.0;
for (int i = 0; i < NUM_BLOBS; i++) {
    vec2 pos = blobs[i].xy;
    float radius = blobs[i].z;
    float d = length(uv - pos);
    density += exp(-d * d / (radius * radius));
}
float surface = smoothstep(threshold - 0.1, threshold + 0.1, density);
```

**Characteristics:**
- Blobs merge smoothly when overlapping (no sharp boundaries)
- Threshold controls surface tension (higher = smaller blobs)
- Exponential falloff creates organic appearance
- O(n) per pixel where n = blob count

**Audio mapping:**
- Bass → blob radius expansion
- Beat → spawn additional blobs at random positions
- FFT bands → per-blob radius (frequency-to-blob mapping)

**Performance:** 8-16 blobs viable at 60fps. Beyond 32, consider texture-based lookup or compute shader.

### 3. Gaussian Particle Rendering (Soft Particles)

Render particles as soft Gaussian splats instead of hard points or textured quads.

```glsl
// Per-particle fragment
float d = length(fragCoord - particleCenter);
float alpha = exp(-d * d / (particleRadius * particleRadius));
color = particleColor * alpha;
```

**Advantages over textured particles:**
- Resolution-independent softness
- Natural additive blending (summed Gaussians)
- No texture sampling overhead

**Disadvantages:**
- More ALU per particle
- Uniform circular shape only (no oriented splats without modification)

#### Anisotropic Gaussian Splats

Oriented elliptical Gaussians with rotation and separate x/y scales. Inspired by 3D Gaussian Splatting techniques.

```glsl
// Transform to local splat space
vec2 local = rotate2D(fragCoord - center, -rotation);
local.x /= scaleX;
local.y /= scaleY;
float alpha = exp(-dot(local, local));
```

**Audio mapping:**
- Rotation follows waveform phase
- Scale stretches with amplitude
- Opacity pulses with beat

### 4. Gaussian Noise / Film Grain

Generate Gaussian-distributed noise for organic texture. Box-Muller transform converts uniform random to Gaussian.

```glsl
// Box-Muller transform
float u1 = hash(uv + time);
float u2 = hash(uv + time + 0.1);
float gaussian = sqrt(-2.0 * log(u1)) * cos(6.28318 * u2);
// gaussian ∈ approximately (-3, 3), centered at 0

color += gaussian * noiseIntensity;
```

**Characteristics:**
- Most values cluster near zero (subtle grain)
- Occasional outliers create sparkle
- Temporal variation (time offset) prevents static pattern

**Audio mapping:**
- Beat → grain intensity spike (0.02 → 0.08)
- High frequencies → grain density

**Alternative:** Pre-computed Gaussian noise texture. Sample with time-based UV offset. Lower ALU cost, slight memory overhead.

### 5. Gaussian Vignette

Edge darkening with Gaussian falloff from center. More natural than linear or power-function vignettes.

```glsl
float dist = length((uv - 0.5) * 2.0);  // 0 at center, ~1.4 at corners
float vignette = exp(-dist * dist * vignetteStrength);
color *= vignette;
```

**Parameters:**
- `vignetteStrength`: 0.5 (subtle) to 3.0 (dramatic)
- Elliptical variant: scale x/y before distance calculation

**Audio mapping:**
- Beat pushes vignette outward (decrease strength momentarily)
- Creates breathing/pulsing frame effect

### 6. Difference of Gaussians (DoG)

Subtract wide Gaussian blur from narrow blur. Approximates Laplacian of Gaussian (edge detection). Creates ethereal glowing edges.

```glsl
vec3 narrow = gaussianBlur(tex, uv, sigma1);  // e.g., 2px
vec3 wide = gaussianBlur(tex, uv, sigma2);    // e.g., 8px
vec3 edges = narrow - wide;
```

**Sigma ratio:** Typically 1:4 or 1:5 for best edge response.

**Application in AudioJones:** Apply to feedback buffer. Edges of waveforms gain halo effect. Combine with original via screen blend.

**Audio mapping:**
- Beat → sigma values increase (thicker edges)
- Treble → edge intensity

### 7. Gaussian-Smoothed FFT

Apply Gaussian kernel to FFT magnitude bins before visualization. Reduces noise, creates smoother spectrum response.

```glsl
// CPU-side or compute shader
for (int i = 0; i < numBins; i++) {
    float smoothed = 0.0;
    float weightSum = 0.0;
    for (int j = -kernelRadius; j <= kernelRadius; j++) {
        int idx = clamp(i + j, 0, numBins - 1);
        float weight = exp(-float(j * j) / (2.0 * sigma * sigma));
        smoothed += fftMagnitude[idx] * weight;
        weightSum += weight;
    }
    smoothedFFT[i] = smoothed / weightSum;
}
```

**Trade-off:** Reduces frequency resolution. Peaks spread across adjacent bins. Use small sigma (1-3 bins) to preserve detail while reducing noise.

### 8. Gaussian Density Fields

Deposit Gaussian blobs onto accumulation texture. Similar to physarum trail deposit but explicit Gaussian shape.

```glsl
// Fragment shader for deposit pass
float contribution = depositAmount * exp(-d * d / (radius * radius));
trailTexture += contribution;
```

**Difference from physarum:** No agent behavior. Positions controlled directly by audio data or particle simulation.

**Audio mapping:**
- Waveform sample positions → deposit locations
- Beat → deposit radius pulse
- Creates ghostly trails following waveform motion

## Implementation Complexity

| Effect | Passes | ALU Cost | Audio Integration |
|--------|--------|----------|-------------------|
| Radial blur | 1 | Medium (16-32 samples) | Beat → intensity |
| Metaballs | 1 | Medium-High (n blobs) | FFT → blob params |
| Gaussian splats | 1 per particle | Low per-splat | Amplitude → scale |
| Film grain | 1 | Low | Beat → intensity |
| Gaussian vignette | 0 (combine with existing) | Minimal | Beat → strength |
| DoG edges | 2 (two blur passes) | High | Sigma modulation |
| Smoothed FFT | 0 (CPU preprocessing) | Minimal GPU | Always-on |
| Density fields | 1 deposit + existing trail | Medium | Waveform positions |

## Integration with Existing Pipeline

Current AudioJones render passes (from `docs/architecture.md`):
1. Waveform/spectrum rendering
2. Physarum compute
3. Feedback zoom/rotation
4. Blur (bloom)
5. Chromatic aberration + voronoi + kaleidoscope

**Low-effort additions (modify existing passes):**
- Gaussian vignette: Add to chromatic aberration shader
- Film grain: Add to final composite
- Radial blur: Replace or supplement existing blur

**Medium-effort additions (new pass):**
- Metaballs: Dedicated pass before or after waveforms
- DoG edges: Two additional blur passes + blend

**High-effort additions:**
- Gaussian splat particles: New particle system + render pass
- Density fields: New deposit system parallel to physarum

## Performance Considerations

### Separable Blur Advantage

2D Gaussian decomposes into 1D horizontal + 1D vertical passes. NxN kernel becomes 2N samples instead of N². Current blur implementation already uses this.

### Kernel Size vs Quality

| Kernel Radius | Samples (separable) | Coverage (3σ) |
|---------------|---------------------|---------------|
| 5px | 10 | σ ≈ 1.7px |
| 9px | 18 | σ ≈ 3px |
| 15px | 30 | σ ≈ 5px |

Larger kernels require more samples or become visibly truncated (hard edge at kernel boundary).

### Metaball Optimization

- Fixed blob count enables loop unrolling
- Screen-space bounding boxes skip distant blobs
- Hierarchical evaluation: coarse grid → refine near surface

### Noise Texture Alternative

Pre-computed 256x256 Gaussian noise texture sampled with animated UV offset reduces per-pixel ALU. Trades compute for texture bandwidth.

## Recommended First Implementation

**Radial blur on beat** offers:
- Simple modification of existing blur pass
- High visual impact
- Direct audio-reactive mapping
- Low implementation risk

**Implementation sketch:**
1. Add `beatIntensity` uniform to blur shader
2. Calculate `blurDir = normalize(uv - vec2(0.5))`
3. Offset sample positions along `blurDir * beatIntensity * 0.05`
4. Pass beat value from `analysis/beat.h` through render pipeline

**Alternative first pick:** Gaussian vignette for minimal effort (5-10 lines in existing shader).

## Sources

- [3D Gaussian Splatting - INRIA](https://repo-sam.inria.fr/fungraph/3d-gaussian-splatting/)
- [NVIDIA Gaussian Splatting Sample](https://developer.nvidia.com/blog/real-time-gpu-accelerated-gaussian-splatting-with-nvidia-designworks-sample-vk_gaussian_splatting/)
- [Dynamic Gaussian Visualizer - Eskandar & Díaz](https://blancocd.com/notes/dynamic_viewer.pdf)
- [The Book of Shaders - Generative Designs](https://thebookofshaders.com/)
- [Shadertoy - Various Gaussian/Metaball Examples](https://www.shadertoy.com/)
- [GPU Gems - Gaussian Blur Optimizations](https://developer.nvidia.com/gpugems/gpugems3/part-vi-gpu-computing/chapter-40-incremental-computation-gaussian)

## Rejected Approaches

### Full 3D Gaussian Splatting

Renders photorealistic scenes from trained Gaussian primitives. Requires:
- Pre-trained scene data
- Sophisticated sorting/splatting pipeline
- View-dependent color computation

Overkill for 2D audio visualization. The technique's strength (photorealism from captures) doesn't apply here.

### Gaussian Mixture Models for Audio

Statistical technique for clustering audio features. Useful for music information retrieval but not visual rendering.

### Gaussian Process Regression

Machine learning technique for interpolation/prediction. No direct application to real-time visualization rendering.
