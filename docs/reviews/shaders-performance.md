# Shader Performance Optimization Review

Ordered from largest expected gains to smallest.

1) Precompute flow fields for agent sims to reduce per-agent work (`shaders/curl_flow_agents.glsl`).
   - Replace analytic simplex noise + gradient with a precomputed curl-noise texture (2-channel), update at a lower rate.
   - Precompute density gradients (or a downsampled density texture) to avoid 5 samples per agent step.
   - Branch on `accumSenseBlend` to skip trail/accum sampling when it is 0 or 1.

2) Cache/encode hue or affinity to avoid per-sample RGB->HSV (`shaders/physarum_agents.glsl`).
   - Store hue or luminance in a single-channel texture and sample that instead of full RGB + HSV conversion.
   - Branch on `accumSenseBlend` to skip trail/accum sampling when it is 0 or 1.
   - Consider lower sensor count or distances for low-quality presets.

3) Reduce RK4 cost for attractor updates (`shaders/attractor_agents.glsl`).
   - Use RK2/Euler for lower quality modes or update a fraction of agents per frame.
   - Precompute the rotation matrix on CPU and pass a `mat3` uniform to avoid per-agent trig.

4) Cut clarity sampling cost (`shaders/clarity.fs`).
   - Replace multi-radius 8-direction sampling with 1-2 mipmap samples (`textureLod`) or a low-res blur pass.
   - Make radii/sample count quality tiers.

5) Reduce kaleidoscope warp + supersampling cost (`shaders/kaleidoscope.fs`).
   - Swap fBM for a precomputed noise texture or reduce octaves.
   - Make 4x supersampling optional; reduce `kifsIterations` in low-quality modes.

6) Lower FXAA search work (`shaders/fxaa.fs`).
   - Reduce `FXAA_ITERATIONS` or add quality tiers.
   - Tighten the edge threshold to early-out more often.

7) Simplify Voronoi evaluation (`shaders/voronoi.fs`).
   - Compute the Voronoi field at lower resolution or precompute points in a texture.
   - Reduce the neighborhood search (2x2) or skip animation when `intensity` is low.

8) Reduce infinite zoom layer cost (`shaders/infinite_zoom.fs`).
   - Cap `layers` based on quality.
   - Precompute per-layer phase/scale/rotation on CPU and pass arrays.

9) Reduce blur and diffusion bandwidth (`shaders/blur_h.fs`, `shaders/blur_v.fs`, `shaders/trail_diffusion.glsl`).
   - Use bilinear sampling tricks to collapse 5 taps into 2-3 samples.
   - Run blur/diffusion on half-resolution textures where possible.
   - Move the `exp` in `shaders/blur_v.fs` to a precomputed uniform (like `decayFactor`).

10) Replace expensive per-pixel gamma (`shaders/gamma.fs`).
    - Use an sRGB framebuffer or approximate gamma for common values (e.g., sqrt for gamma=2.0).

11) Avoid repeated trig when parameters are static (`shaders/shape_texture.fs`, `shaders/feedback.fs`, `shaders/chromatic.fs`).
    - Pass precomputed sin/cos or a rotation matrix as uniforms.
    - Add a fast path for near-zero chromatic offset.

12) Lower precision where quality allows (multiple shaders/buffers).
    - Use `rgba16f` or `r11f_g11f_b10f` for intermediates to cut bandwidth and cache pressure.
