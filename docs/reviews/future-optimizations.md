# Future Shader Optimizations

Candidates for future planning sessions, selected from the performance review.

## High Priority

### 1. Clarity Mipmap Sampling
**Current**: 24 texture samples per pixel (3 radii × 8 directions)
**Fix**: Replace multi-radius sampling with 1-2 `textureLod()` mipmap samples
**Impact**: ~90% reduction in clarity shader texture bandwidth
**Complexity**: Low
**File**: `shaders/clarity.fs`

### 2. sRGB Framebuffer for Gamma
**Current**: `pow(x, 1.0/2.2)` per pixel in fragment shader
**Fix**: Enable `GL_FRAMEBUFFER_SRGB` for hardware gamma correction
**Impact**: Removes expensive pow() from every pixel
**Complexity**: Low (one GL call)
**File**: `shaders/gamma.fs`, render pipeline init

### 3. Physarum Hue Caching
**Current**: `rgb2hsv()` called 3× per agent (once per sensor)
**Fix**: Store hue in a single-channel texture or use RGB distance metric instead of HSV
**Impact**: Removes complex branching and trig from inner loop
**Complexity**: Medium
**File**: `shaders/physarum_agents.glsl`

## Medium Priority

### 4. Blur Bilinear Optimization
**Current**: 5-tap Gaussian blur with discrete samples
**Fix**: Use GPU bilinear filtering to collapse 5 taps into 3 samples (center + 1 optimized offset per side)
**Impact**: 40% reduction in blur texture samples
**Complexity**: Low-Medium
**Files**: `shaders/blur_h.fs`, `shaders/blur_v.fs`

### 5. Precompute Static Trig Uniforms
**Current**: `sin()`/`cos()` on uniform values computed per-pixel
**Fix**: Pass precomputed sin/cos or rotation matrices as uniforms
**Impact**: Removes redundant trig across multiple shaders
**Complexity**: Low
**Files**: `shaders/shape_texture.fs`, `shaders/feedback.fs`, `shaders/chromatic.fs`

### 6. Texture Precision Reduction
**Current**: `rgba32f` (128-bit) for intermediate textures
**Fix**: Use `rgba16f` (64-bit) or `r11f_g11f_b10f` (32-bit) where HDR range permits
**Impact**: Halves texture bandwidth and cache pressure
**Complexity**: Medium (requires testing for banding artifacts)
**Files**: `src/simulation/trail_map.cpp`, `src/render/render_utils.cpp`

## Lower Priority (Quality Tiers)

These add UI complexity but allow users to trade quality for performance:

- **Kaleidoscope**: Make 4× supersampling optional, reduce fBM octaves
- **FXAA**: Reduce `FXAA_ITERATIONS`, tighten edge threshold
- **Infinite Zoom**: Cap layer count based on quality setting
- **Voronoi**: Compute at lower resolution, reduce neighborhood search
