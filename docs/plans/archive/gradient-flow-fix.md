# Gradient Flow Fix

Replace per-pixel Sobel gradients with structure tensor averaging to eliminate pixelated noise in the Gradient Flow effect.

## Current State

- `shaders/gradient_flow.fs:17-33` - `computeSobelGradient()` uses 3x3 neighborhood
- `src/config/gradient_flow_config.h:4-10` - Config struct (no smoothRadius yet)
- `src/render/post_effect.h:217-221` - Uniform locations
- `src/render/shader_setup.cpp:551-562` - `SetupGradientFlow()` sets uniforms
- `src/ui/imgui_effects_warp.cpp:74-91` - UI controls

## Technical Implementation

**Source**: `docs/research/gradient-flow-fix.md`, OpenCV GST tutorial

**Problem**: Sobel operates on 3x3 neighborhood. After displacement, the new position has a different local gradient. Direction changes accumulate into noise.

**Solution**: Structure tensor accumulates gradient products (Ix², Iy², IxIy) over a window, then extracts dominant orientation via eigendecomposition.

### Structure Tensor Algorithm

For each pixel in window [-halfSize, halfSize]:
```
Ix = (luminance(right) - luminance(left)) * 0.5
Iy = (luminance(bottom) - luminance(top)) * 0.5
J11 += Ix * Ix
J22 += Iy * Iy
J12 += Ix * Iy
```

Extract orientation:
```
angle = 0.5 * atan2(2.0 * J12, J22 - J11)
tangent = vec2(cos(angle), sin(angle))
```

Rotate by flowAngle:
```
flow.x = cos(flowAngle) * tangent.x - sin(flowAngle) * tangent.y
flow.y = sin(flowAngle) * tangent.x + cos(flowAngle) * tangent.y
```

### Shader Code

Replace `computeSobelGradient()` with:

```glsl
uniform int smoothRadius;  // Window half-size (2 = 5x5 window)

vec2 computeFlowDirection(vec2 uv, vec2 texel) {
    float J11 = 0.0, J22 = 0.0, J12 = 0.0;

    for (int y = -smoothRadius; y <= smoothRadius; y++) {
        for (int x = -smoothRadius; x <= smoothRadius; x++) {
            vec2 sampleUV = uv + vec2(float(x), float(y)) * texel;

            float l = luminance(texture(texture0, sampleUV + vec2(-texel.x, 0.0)).rgb);
            float r = luminance(texture(texture0, sampleUV + vec2( texel.x, 0.0)).rgb);
            float t = luminance(texture(texture0, sampleUV + vec2(0.0, -texel.y)).rgb);
            float b = luminance(texture(texture0, sampleUV + vec2(0.0,  texel.y)).rgb);

            float Ix = (r - l) * 0.5;
            float Iy = (b - t) * 0.5;

            J11 += Ix * Ix;
            J22 += Iy * Iy;
            J12 += Ix * Iy;
        }
    }

    float angle = 0.5 * atan(2.0 * J12, J22 - J11);
    vec2 tangent = vec2(cos(angle), sin(angle));

    float s = sin(flowAngle);
    float c = cos(flowAngle);
    return vec2(c * tangent.x - s * tangent.y, s * tangent.x + c * tangent.y);
}
```

Update `main()` to use the new function and handle edge weighting:

```glsl
void main() {
    vec2 texel = 1.0 / resolution;
    vec2 warpedUV = fragTexCoord;

    for (int i = 0; i < iterations; i++) {
        vec2 flow = computeFlowDirection(warpedUV, texel);

        // Compute edge magnitude for weighting (reuse structure tensor logic or approximate)
        vec2 gradient = computeSobelGradient(warpedUV, texel);  // Keep for edge magnitude
        float gradMag = length(gradient);

        // Blend between uniform and edge-weighted displacement
        float displacement = strength * mix(1.0, gradMag, edgeWeight);

        warpedUV += flow * displacement;
    }

    finalColor = texture(texture0, clamp(warpedUV, 0.0, 1.0));
}
```

### New Parameter

| Parameter | Type | Range | Default | UI Label |
|-----------|------|-------|---------|----------|
| smoothRadius | int | 1-4 | 2 | Smooth Radius |

### Performance Note

5x5 window with 8 iterations: ~1000 texture samples per pixel. Acceptable for transform effects. Consider reducing default iterations to 4-6 since smoother flow requires fewer iterations.

---

## Phase 1: Config and Shader

**Goal**: Add smoothRadius parameter and implement structure tensor in shader.

**Build**:
- `src/config/gradient_flow_config.h` - Add `int smoothRadius = 2;`
- `shaders/gradient_flow.fs` - Replace `computeSobelGradient()` with structure tensor, add `uniform int smoothRadius`

**Done when**: Shader compiles (build succeeds).

---

## Phase 2: Uniform Plumbing

**Goal**: Wire smoothRadius uniform from C++ to shader.

**Build**:
- `src/render/post_effect.h` - Add `int gradientFlowSmoothRadiusLoc;`
- `src/render/post_effect.cpp` - Add `GetShaderLocation()` call for smoothRadius
- `src/render/shader_setup.cpp` - Add `SetShaderValue()` call in `SetupGradientFlow()`

**Done when**: Build succeeds, effect renders (may need runtime test).

---

## Phase 3: UI and Serialization

**Goal**: Expose smoothRadius in UI and persist in presets.

**Build**:
- `src/ui/imgui_effects_warp.cpp` - Add `ImGui::SliderInt("Smooth Radius##gradflow", ...)` in `DrawWarpGradientFlow()`
- `src/config/preset.cpp` - Add `smoothRadius` to the `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT` macro for `GradientFlowConfig`

**Done when**: Slider appears in UI, changing it affects the effect, preset save/load preserves value.

---

## Implementation Notes

The original plan (structure tensor) was implemented but rejected due to severe performance issues and questionable value.

### Structure Tensor: Rejected

**Problem**: The structure tensor approach required ~100 texture samples per iteration (5×5 window × 4 samples). With 8 iterations, that's ~800 samples per pixel—comparable to the Boids simulation. The visual improvement didn't justify the cost.

**Additional issue**: The `flowAngle` parameter forced users to choose an arbitrary asymmetric direction. The tangent to an edge is a line, not a vector—there's no "correct" direction to flow along it.

### Final Implementation: Blur + Random Direction

Replaced structure tensor with a simpler approach:

1. **3×3 averaged Sobel gradients**: Compute Sobel at 9 offset positions and average. ~72 samples per iteration (9 × 8). Smoother than single Sobel, much cheaper than structure tensor.

2. **Removed `flowAngle`**: Tangent direction is now fixed (90° counterclockwise from gradient).

3. **Added `randomDirection` toggle**: Per-pixel random sign flip creates a "crunchy" stylized look. Off by default for smooth coherent flow.

4. **Reduced max iterations**: 8 instead of 32.

### Hash Function Fix

Initial random direction implementation used screen-space coordinates (`fragTexCoord`), causing the noise pattern to stay fixed while content warped around it—looked like a static background layer.

**Fix**: Compute random sign from `warpedUV` inside the loop, so noise follows the warped content.

**Hash choice**: Switched from `sin()` hash to Dave Hoskins' "hash without sine":
```glsl
float hash(vec2 p) {
    vec3 p3 = fract(vec3(p.xyx) * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}
```
Uses only ALU ops (fract, multiply, add, dot)—faster than `sin()` and better distribution.

### Final Config

```cpp
struct GradientFlowConfig {
    bool enabled = false;
    float strength = 0.01f;
    int iterations = 8;
    float edgeWeight = 1.0f;
    bool randomDirection = false;
};
```

### Lesson Learned

The "mathematically correct" solution (structure tensor) was overkill. A simple blur approximation achieved 80% of the smoothing at 10% of the cost. Always profile before committing to expensive algorithms.
