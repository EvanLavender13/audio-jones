# Detailed Shader Optimization Plan

This document outlines specific, actionable steps to implement the performance optimizations identified for the AudioJones project.

## 1. Critical Performance Bottlenecks (Compute Shaders)

These optimizations target the high-count agent simulations, offering the largest performance gains.

### 1.1. Attractor Agents (`shaders/attractor_agents.glsl`)
**Issue:** The rotation matrix is recalculated for every agent (millions of threads), involving 6 trigonometric operations and matrix multiplications per thread.
**Action:**
- **CPU Side:** Compute the `mat3` rotation matrix in C++ whenever `rotation` angles change.
- **Shader:** Replace `uniform vec3 rotation` with `uniform mat3 rotationMatrix`.
- **Gain:** Removes ~6 `sin`/`cos` and matrix construction per agent.

### 1.2. Curl Flow Agents (`shaders/curl_flow_agents.glsl`)
**Issue:** `snoise3_grad` (Simplex noise with analytic gradient) is computationally expensive and called per agent. Additionally, density gradient calculation samples the density map 4 times per agent.
**Action:**
- **Phase 1 (Texture lookup):** Instead of computing noise on the fly, generate a seamless 3D noise texture (or 2D with time scrolling) and sample it.
- **Phase 2 (Gradient Map):** Precompute the density gradient into a lower-resolution texture (RG channels for dX, dY) in a separate pass. Agents sample this once instead of sampling the trail map 4-5 times.
- **Gain:** Massive ALU reduction; significant bandwidth reduction.

### 1.3. Physarum Agents (`shaders/physarum_agents.glsl`)
**Issue:** `rgb2hsv` conversion is performed 3 times per agent (for 3 sensors) to calculate hue affinity.
**Action:**
- **Texture Format:** If memory permits, use a texture format that stores Hue directly, or PACK hue into the Alpha channel of the trail map if unused.
- **Alternative:** Pre-filter the affinity map. Run a compute pass that generates a single-channel "affinity" texture based on the *current* agent species hue (if all agents share a target hue), or use a simpler RGB distance metric instead of HSV distance.
- **Gain:** Removes complex branching and math in the inner loop.

## 2. Heavy Fragment Shaders (Post-Processing)

### 2.1. Clarity Effect (`shaders/clarity.fs`)
**Issue:** Performs 24 texture samples per pixel (3 radii * 8 directions).
**Action:**
- **Downsampling:** Use `textureLod` to sample lower mipmaps instead of wide-radius scatter sampling.
- **Separable Blur:** If quality is critical, approximate the wide radius with a separable blur (2 passes) or a Box Blur.
- **Reduction:** Reduce to 1 radius with 4 taps for "Low Quality" mode.

### 2.2. Kaleidoscope (`shaders/kaleidoscope.fs`)
**Issue:** 4x supersampling is always on. Domain warping (`fBM`) adds multiple noise lookups per pixel.
**Action:**
- **Uniform Control:** Add a `quality` uniform.
    - High: 4x supersampling + fBM.
    - Low: 1 sample, disable fBM or reduce octaves.
- **Branching:** Ensure the compiler can optimize out the loop (using constant expressions or specialized shaders) or simply wrap the 4x loop in an `if (quality > 0.5)`.

### 2.3. Blur & Diffusion (`shaders/blur_h.fs`, `shaders/blur_v.fs`, `shaders/trail_diffusion.glsl`)
**Issue:** 5-tap Gaussian blur.
**Action:**
- **Bilinear Filtering:** Utilize GPU bilinear filtering to fetch 2 weights at once. A 5-tap kernel can often be approximated by 3 linear samples (center + 1 optimized offset per side).
- **Resolution:** Run diffusion/blur at half resolution if the trail map is high-res.

## 3. Micro-Optimizations & Uniforms

### 3.1. Shape & Feedback (`shaders/shape_texture.fs`, `shaders/feedback.fs`)
**Issue:** Trigonometry (`sin`/`cos`) on uniform values performed per-pixel.
- `shape_texture.fs`: `cos(texAngle)`, `sin(texAngle)`.
- `feedback.fs`: `cos(rot)`, `sin(rot)` (though `rot` varies radially here, so it might be necessary, but base rotation can be precomputed).
**Action:** Pass precomputed values as uniforms (`uniform vec2 trigValues`) or rotation matrices.

### 3.2. Gamma (`shaders/gamma.fs`)
**Issue:** `pow(x, 1.0/2.2)` is expensive.
**Action:**
- **Hardware:** Enable `GL_FRAMEBUFFER_SRGB` to let the GPU hardware handle gamma correction on the final write.
- **Approximation:** Use `sqrt(x)` (gamma 2.0) which is often a single instruction.

### 3.3. Precision
**Action:** Switch `rgba32f` (128-bit) textures to `rgba16f` (64-bit) or `r11f_g11f_b10f` (32-bit) where high dynamic range isn't strictly necessary. This halves bandwidth usage.
