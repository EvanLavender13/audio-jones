# Shader Optimization Techniques

Cross-cutting optimization patterns for reducing brute-force shader costs.

## Classification

- **Type**: General (architecture)
- **Scope**: Applies to multiple effects across TRANSFORMS, OUTPUT, and FEEDBACK categories

## References

- [Fast Blurs](https://fgiesen.wordpress.com/2012/07/30/fast-blurs-1/) - Separable passes, box blur stacking, integral images
- [GPU-Efficient Recursive Filtering and SAT](https://hhoppe.com/proj/recursive/) - Summed area table algorithms
- [Anisotropic Kuwahara on GPU](https://www.kyprianidis.com/p/gpupro/) - Variance-weighted sector filtering
- [Guided Filtering](https://bartwronski.com/2019/09/22/local-linear-models-guided-filter/) - Edge-aware upsampling alternative to bilateral
- [Compute Blur Analysis](https://lisyarus.github.io/blog/posts/compute-blur.html) - Shared memory vs texture cache tradeoffs
- [AMD FidelityFX SPD](https://gpuopen.com/fidelityfx-spd/) - Single-pass mipmap downsampling
- [TAA Starter Pack](https://alextardif.com/TAA.html) - Temporal reprojection techniques

---

## Shader Audit

| Shader | Pattern | Complexity | Samples/Pixel |
|--------|---------|------------|---------------|
| `kuwahara.fs` | 4-sector variance | O(4×r²) | ~100 at r=5 |
| `impressionist.fs` | Splat grid search | O(passes×cells×splats) | ~200+ |
| `oil_paint_stroke.fs` | Multi-layer strokes | O(layers×9×gradients) | varies |
| `watercolor.fs` | Gradient tracing | O(samples×4) | 4×samples |
| `pencil_sketch.fs` | Angle×sample nested | O(angles×samples×2) | angles×samples×2 |
| `neon_glow.fs` | Sobel + glow spread | O(9 + glowSamples×18) | 9–180+ |
| `bokeh.fs` | Disc sampling | O(iterations) | 64+ typical |
| `voronoi.fs` | 2-pass cell search | O(9 + 25) | 34 |
| `gradient_flow.fs` | Iterated Sobel blur | O(iters×9×9) | 81×iterations |
| `clarity.fs` | Multi-radius sampling | O(radii×directions) | 24 |

---

## Optimization Techniques

### 1. Separable Filters

**Applies to**: Box blur, Gaussian blur, any rank-1 kernel

**Principle**: A 2D convolution with a separable kernel decomposes into two 1D passes (horizontal then vertical). Reduces O(r²) to O(2r).

**Savings**:
| Kernel | Non-separable | Separable | Reduction |
|--------|---------------|-----------|-----------|
| 3×3 | 9 | 6 | 33% |
| 5×5 | 25 | 10 | 60% |
| 7×7 | 49 | 14 | 71% |
| 15×15 | 225 | 30 | 87% |

**Implementation**: Two shader passes, ping-pong between textures.

**Limitation**: Only works for separable kernels. Non-separable filters (Sobel, Kuwahara sectors) cannot use this directly.

**Current usage**: `blur_h.fs` and `blur_v.fs` already use separable Gaussian.

---

### 2. Hardware Bilinear Filtering

**Applies to**: Any weighted sampling where neighbors have adjacent weights

**Principle**: GPU texture units perform free bilinear interpolation. Sampling between two texels returns their weighted blend in a single fetch.

**Savings**: Reduces fetches by ~50% for Gaussian and similar smooth kernels.

**Example**: A 33-tap 1D kernel becomes 17 fetches by sampling at fractional positions calculated from weight ratios.

**Limitation**: Requires adjacent weights with smooth transitions. Sharp cutoffs (box filter edges) don't benefit.

---

### 3. Integral Images (Summed Area Tables)

**Applies to**: Box filters, variance computation, any rectangular region sum

**Principle**: Precompute cumulative sums. Any axis-aligned rectangle sums in 4 lookups regardless of size.

```
sum(rect) = SAT[br] - SAT[tr] - SAT[bl] + SAT[tl]
```

**Savings**: O(r²) → O(1) for box filters after O(n) setup.

**Kuwahara application**: Each sector's mean and variance become constant-time operations. The 4-sector pattern at radius=5 drops from ~100 samples to ~16 lookups (4 corners × 4 sectors) plus setup.

**Limitations**:
- Requires extra render pass to build SAT
- Precision issues at large radii (cumulative sums overflow)
- Memory: need 32-bit float per channel minimum
- Only works for rectangular regions (not circular/anisotropic)

**When to use**: Radius > 7, or when radius varies per-pixel.

---

### 4. Half-Resolution Rendering + Edge-Aware Upscale

**Applies to**: Blur-heavy effects, SSAO-style sampling, any low-frequency effect

**Principle**: Compute expensive effect at half resolution (4× fewer pixels), then upsample with edge awareness to avoid halos.

**Upscaling methods**:

| Method | Quality | Cost | Notes |
|--------|---------|------|-------|
| Bilinear | Poor | Cheapest | Blurs edges |
| Bilateral | Good | 4+ samples + depth compare | Needs depth buffer |
| Guided filter | Better | Linear regression | No tuning parameters |
| Joint bilateral | Good | 9+ samples | Uses full-res guide |

**Bilateral upscale formula**:
```glsl
// Compare half-res depth to full-res depth at 4 bilinear neighbors
// Weight by depth similarity, renormalize
weight *= exp(-depthDiff * depthDiff / sigma);
weights /= dot(weights, vec4(1.0));
```

**Savings**: 4× pixel reduction, partially offset by upscale cost. Net ~2-3× speedup typical.

**Candidates**: `bokeh.fs`, `neon_glow.fs` (glow spread), `clarity.fs`

---

### 5. Mipmap Pyramid / Multi-Scale

**Applies to**: Bloom, DOF, any large-radius blur

**Principle**: Downsample progressively, blur at each level, composite. Large blurs become small blurs at lower resolution.

**AMD SPD approach**: Single compute pass generates up to 12 mip levels without GPU sync between levels.

**Bloom pipeline** (already used in codebase):
1. Prefilter bright pixels
2. Downsample chain (bloom_downsample.fs)
3. Upsample chain with additive blend (bloom_upsample.fs)

**Savings**: Radius-100 blur at 1/16 resolution = radius-6 blur cost.

**Candidates**: Could optimize `bokeh.fs` with pyramid approach instead of flat disc sampling.

---

### 6. Temporal Reprojection

**Applies to**: Any effect where result changes slowly over time

**Principle**: Blend current frame with reprojected previous frame. Accumulate samples across frames instead of per-frame.

**Core algorithm**:
```glsl
vec2 prevUV = currentUV - motionVector;
vec3 history = texture(historyBuffer, prevUV).rgb;
vec3 current = computeExpensiveEffect(); // can use fewer samples
result = mix(history, current, 0.1); // 90% history, 10% new
```

**Anti-ghosting**: Clamp history to neighborhood min/max of current frame (variance clipping).

**Savings**: Can reduce per-frame samples by 10× if temporal stability is acceptable.

**Limitations**:
- Requires motion vectors (camera + object)
- Ghosting on fast motion or disocclusion
- Not suitable for effects that should feel "instant"

**Candidates**: `pencil_sketch.fs` (wobble could mask temporal artifacts), `impressionist.fs`

---

### 7. Anisotropic Kuwahara

**Applies to**: `kuwahara.fs` specifically

**Principle**: Replace fixed rectangular sectors with elliptical sectors aligned to local image gradient. Uses smooth polynomial weighting instead of hard sector boundaries.

**Benefits**:
- Eliminates block artifacts at sector boundaries
- Preserves directional features better
- Can use smaller effective radius for same visual quality

**Implementation**: Requires structure tensor (gradient direction/magnitude) as preprocessing step or computed inline.

**Tradeoff**: More complex math per pixel, but better quality allows smaller radius = fewer samples overall.

---

### 8. Precomputed Gradient Textures

**Applies to**: Shaders that compute Sobel/gradient multiple times

**Principle**: Compute gradient once per frame into a texture, sample it instead of recomputing.

**Current redundancy**:
- `gradient_flow.fs`: Computes Sobel 81× per pixel (9×9 blurred gradient per iteration)
- `watercolor.fs`: 4 gradient computations per sample
- `pencil_sketch.fs`: Gradient per angle×sample

**Savings**: If multiple shaders or iterations need the same gradient, single-pass precompute amortizes cost.

**Implementation**: Add a gradient prepass that writes `vec2(gx, gy)` to RG16F texture.

---

### 9. Importance Sampling / Adaptive Sampling

**Applies to**: Disc/spiral sampling patterns (`bokeh.fs`, `impressionist.fs`)

**Principle**: Sample more densely where contribution is highest, fewer samples in low-impact regions.

**Bokeh example**: Weight samples by brightness (already done), but also reduce sample count in dark regions.

**Impressionist example**: Skip splat evaluation if cell is far from current pixel or splat radius is small.

**Early-out patterns**:
```glsl
if (distanceToSplat > maxRadius) continue; // Skip distant splats
if (brightness < threshold) continue;      // Skip dark regions
```

---

## Applicability Matrix

| Technique | kuwahara | impressionist | oil_paint | watercolor | pencil_sketch | neon_glow | bokeh | voronoi | gradient_flow | clarity |
|-----------|:--------:|:-------------:|:---------:|:----------:|:-------------:|:---------:|:-----:|:-------:|:-------------:|:-------:|
| Separable | - | - | - | - | - | - | - | - | - | - |
| HW bilinear | - | - | - | - | - | - | - | - | - | partial |
| Integral image | **yes** | - | - | - | - | - | - | - | - | - |
| Half-res | partial | - | - | - | - | **yes** | **yes** | - | - | **yes** |
| Mipmap pyramid | - | - | partial | - | - | - | **yes** | - | - | - |
| Temporal | - | **yes** | - | - | **yes** | - | - | - | - | - |
| Anisotropic | **yes** | - | - | - | - | - | - | - | - | - |
| Precomputed gradient | - | - | partial | **yes** | **yes** | partial | - | - | **yes** | - |
| Importance sampling | - | **yes** | - | - | - | - | **yes** | - | - | - |

---

## Priority Recommendations

### High Impact (>2× speedup potential)

1. **Kuwahara → Integral Image SAT**: Constant-time variance regardless of radius
2. **Gradient Flow → Precomputed Gradient**: Eliminate 81× redundant Sobel
3. **Bokeh → Half-res + Pyramid**: 4× pixel reduction for soft blur
4. **Neon Glow → Half-res glow**: Glow spread doesn't need full resolution

### Medium Impact (1.5-2× speedup)

5. **Pencil Sketch → Temporal accumulation**: Wobble already provides motion
6. **Clarity → Half-res blur component**: Only the blur needs radius; detail extraction stays full-res
7. **Watercolor → Precomputed gradient**: 4 traces share same gradient field

### Lower Priority (complexity vs gain)

8. **Impressionist → Early-out distance checks**: Many small gains
9. **Voronoi → Already reasonably efficient**: 34 samples is acceptable
10. **Oil Paint Stroke → Complex architecture**: Would need significant restructuring

---

## Notes

- Texture cache on modern GPUs is highly effective—shared memory (LDS) doesn't always win for 2D stencils
- Fragment shader implementations often outperform compute for image-to-image transforms
- Precision matters for SAT: use R32F minimum, consider R32G32B32A32F for color
- Temporal techniques require motion vectors; this codebase may not have them yet
