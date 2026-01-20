# Cross-Hatching Hand-Drawn Enhancement

Replace mechanical grid-like strokes with organic hand-drawn feel: temporal stutter (12 FPS redraw), varied stroke angles per layer, sin-modulated line widths, and per-pixel noise.

## Current State

- `shaders/cross_hatching.fs` - Current shader with static UV jitter
- `src/config/cross_hatching_config.h` - Config struct (6 params)
- `src/render/post_effect.h:281-287` - Uniform locations
- `src/render/post_effect.cpp:371-377` - GetShaderLocation calls
- `src/render/shader_setup.cpp:726-741` - SetupCrossHatching function
- `src/ui/imgui_effects_transforms.cpp:914-938` - UI sliders
- `src/automation/param_registry.cpp:160-165` - Modulation registration

## Technical Implementation

**Source**: Shadertoy MsSGD1 (user-provided code in conversation)

### Core Algorithm

**1. Temporal Stutter (12 FPS quantization)**
```glsl
uniform float time;
float frame = floor(time * 12.0);  // Quantize to ~83ms steps
```

**2. Per-Pixel Hash Noise**
```glsl
float hash(float x) {
    return fract(sin(x) * 43758.5453);
}
```

**3. Hatching Line Function (replaces current `hatchLine`)**
```glsl
float hatchLine(vec2 coord, float slope, float offset, float freq, float baseWidth, float noise)
{
    // Varied angle via slope coefficient on y
    float lineCoord = coord.x + slope * coord.y + offset;

    // Sin-modulated line width for organic thickness variation
    float widthMod = 0.5 * sin((coord.x + slope * coord.y) * 0.05) + 0.5;
    float width = baseWidth * (0.5 + widthMod);

    // Per-pixel noise displacement
    float noiseOffset = (hash(coord.x - 0.75 * coord.y + frame) - 0.5) * noise * 4.0;
    float d = mod(lineCoord + noiseOffset, freq);

    // Anti-aliased line
    return smoothstep(width, width * 0.5, d) + smoothstep(freq - width, freq - width * 0.5, d);
}
```

**4. Layer Configuration (matching Shadertoy)**

| Layer | Threshold | Slope | Frequency | Approx Angle |
|-------|-----------|-------|-----------|--------------|
| 1 | 0.8 × threshold | +0.15 | 10.0 | 8.5° |
| 2 | 0.6 × threshold | +1.0 | 14.0 | 45° |
| 3 | 0.3 × threshold | -0.75 | 8.0 | -37° |
| 4 | 0.15 × threshold | +0.15 | 7.0 | 8.5° |

**5. Main Shader Logic**
```glsl
void main()
{
    vec2 uv = fragTexCoord;
    vec4 color = texture(texture0, uv);
    float luma = dot(color.rgb, vec3(0.2126, 0.7152, 0.0722));

    // Scale coords to pixels, add temporal offset
    vec2 coord = uv * resolution;
    coord += vec2(hash(frame * 1.23), hash(frame * 4.56)) * 2.0;  // Frame jitter

    // 4-layer hatching with varied angles
    float hatch1 = (luma < 0.8 * threshold) ? hatchLine(coord, 0.15, 0.0, 10.0, width, noise) : 0.0;
    float hatch2 = (luma < 0.6 * threshold) ? hatchLine(coord, 1.0, 0.0, 14.0, width, noise) : 0.0;
    float hatch3 = (luma < 0.3 * threshold) ? hatchLine(coord, -0.75, 0.0, 8.0, width, noise) : 0.0;
    float hatch4 = (luma < 0.15 * threshold) ? hatchLine(coord, 0.15, 4.0, 7.0, width, noise) : 0.0;

    // Sobel edge detection (unchanged from current)
    // ... existing Sobel code ...

    float ink = max(max(hatch1, hatch2), max(hatch3, hatch4));
    ink = max(ink, outlineMask);

    vec3 inkColor = mix(color.rgb, vec3(0.0), ink);
    vec3 result = mix(color.rgb, inkColor, blend);
    finalColor = vec4(result, color.a);
}
```

### Parameters

| Parameter | Type | Range | Default | Change |
|-----------|------|-------|---------|--------|
| density | float | 4.0-50.0 | 10.0 | Remove (hardcoded per-layer) |
| width | float | 0.5-4.0 | 1.5 | Keep (base width) |
| threshold | float | 0.0-2.0 | 1.0 | Keep |
| jitter | float | — | — | Remove (replaced by temporal) |
| noise | float | 0.0-1.0 | 0.5 | New (per-pixel irregularity) |
| outline | float | 0.0-1.0 | 0.5 | Keep |
| blend | float | 0.0-1.0 | 1.0 | Keep |

Net change: 6 params → 5 params (remove `density`, `jitter`; add `noise`)

---

## Phase 1: Config and Uniforms

**Goal**: Update config struct and shader uniform plumbing.

**Build**:
- `cross_hatching_config.h`: Remove `density`, `jitter`; add `noise = 0.5f`; update defaults
- `post_effect.h`: Remove `crossHatchingDensityLoc`, `crossHatchingJitterLoc`; add `crossHatchingTimeLoc`, `crossHatchingNoiseLoc`; add `float crossHatchingTime` accumulator
- `post_effect.cpp`: Update GetShaderLocation calls
- `shader_setup.cpp`: Update SetupCrossHatching to accumulate time, set new uniforms

**Done when**: Project compiles (shader will fail until Phase 2).

---

## Phase 2: Shader Rewrite

**Goal**: Replace shader with hand-drawn algorithm.

**Build**:
- `cross_hatching.fs`: Implement algorithm from Technical Implementation section above
  - Add `uniform float time`, `uniform float noise`
  - Remove `uniform float density`, `uniform float jitter`
  - Replace `hatchLine()` with new version using slope, noise, width modulation
  - Update main() with 4-layer varied-angle hatching
  - Keep existing Sobel edge detection unchanged

**Done when**: Effect renders with organic hand-drawn strokes and 12 FPS stutter.

---

## Phase 3: UI and Registry

**Goal**: Update UI sliders and modulation registry.

**Build**:
- `imgui_effects_transforms.cpp`: Remove Density/Jitter sliders; add Noise slider
- `param_registry.cpp`: Remove `crossHatching.density`, `crossHatching.jitter`; add `crossHatching.noise` with range [0.0, 1.0]

**Done when**: UI reflects new parameters, modulation works on noise.

---

## Phase 4: Polish

**Goal**: Tune defaults and verify visual quality.

**Build**:
- Test with various input content (waveforms, particle trails)
- Adjust default `noise` and `width` values if needed
- Verify temporal stutter feels natural (not too fast/slow)
- Confirm Sobel outlines still work correctly

**Done when**: Effect produces convincing hand-drawn aesthetic matching Shadertoy reference.
