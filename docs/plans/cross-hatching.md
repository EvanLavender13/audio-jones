# Cross-Hatching Effect

NPR (non-photorealistic rendering) effect simulating hand-drawn shading via procedural diagonal strokes. Maps input luminance to 4 density layers with Sobel edge outlines. Supports color blending (original color → pure ink) and optional sketchy jitter for organic hand-drawn feel.

## Current State

- Research doc: `docs/research/cross-hatching.md` - complete algorithm specification
- UI category: Style (alongside Toon/Pixelation/ASCII) - fully reorderable in transform chain
- Similar effect: `shaders/toon.fs` - same Sobel edge detection pattern

Hook points:
- `src/config/effect_config.h:39` - TransformEffectType enum
- `src/render/post_effect.cpp:63` - shader loading section
- `src/ui/imgui_effects_transforms.cpp:510` - Toon UI (Style category)

## Technical Implementation

**Source**: `docs/research/cross-hatching.md` (User-provided Shadertoy/Matchbox port)

### Core Algorithm

**Luminance extraction** (Rec. 709):
```glsl
float luma = dot(color.rgb, vec3(0.2126, 0.7152, 0.0722));
```

**4-layer hatching** with fixed thresholds (0.8, 0.6, 0.3, 0.15):
```glsl
// Layer 1: Light shading (diagonal /)
float hatch1 = (luma < 0.8 * threshold) ? hatchLine(uv, density, width, 1.0, 0.0) : 0.0;

// Layer 2: Medium shading (diagonal \)
float hatch2 = (luma < 0.6 * threshold) ? hatchLine(uv, density, width, -1.0, 0.0) : 0.0;

// Layer 3: Dark shading (offset /)
float hatch3 = (luma < 0.3 * threshold) ? hatchLine(uv, density, width, 1.0, density * 0.5) : 0.0;

// Layer 4: Deep shadow (offset \)
float hatch4 = (luma < 0.15 * threshold) ? hatchLine(uv, density, width, -1.0, density * 0.5) : 0.0;
```

**Line generation** with smoothstep anti-aliasing:
```glsl
float hatchLine(vec2 uv, float density, float width, float dir, float offset) {
    float coord = uv.x * resolution.x + dir * uv.y * resolution.y + offset;
    float d = mod(coord, density);
    return smoothstep(width, width * 0.5, d) + smoothstep(density - width, density - width * 0.5, d);
}
```

**Sketchy jitter** (optional UV perturbation):
```glsl
vec2 jitteredUV = uv + vec2(
    sin(uv.y * 50.0) * jitter * 0.002,
    cos(uv.x * 50.0) * jitter * 0.002
);
```

**Sobel edge detection** (reuse Toon pattern):
```glsl
// 3x3 neighborhood sampling on luminance
vec4 sobelH = n[2] + 2.0*n[5] + n[8] - (n[0] + 2.0*n[3] + n[6]);
vec4 sobelV = n[0] + 2.0*n[1] + n[2] - (n[6] + 2.0*n[7] + n[8]);
float edge = length(sqrt(sobelH * sobelH + sobelV * sobelV).rgb);
float outlineMask = smoothstep(0.1, 0.3, edge) * outline;
```

**Final compositing**:
```glsl
float ink = max(max(hatch1, hatch2), max(hatch3, hatch4));
ink = max(ink, outlineMask);
vec3 inkColor = mix(color.rgb, vec3(0.0), ink);  // color → black ink
vec3 result = mix(color.rgb, inkColor, blend);    // original → hatched
```

### Parameters

| Uniform | Type | Range | Default | Effect |
|---------|------|-------|---------|--------|
| `density` | float | 4.0-50.0 | 10.0 | Pixel spacing between hatch lines |
| `width` | float | 0.5-4.0 | 1.0 | Line thickness in pixels |
| `threshold` | float | 0.0-2.0 | 1.0 | Global luminance sensitivity multiplier |
| `jitter` | float | 0.0-1.0 | 0.0 | Sketchy hand-tremor displacement |
| `outline` | float | 0.0-1.0 | 0.5 | Sobel edge outline strength |
| `blend` | float | 0.0-1.0 | 1.0 | Mix: original color (0) → ink (1) |

---

## Phase 1: Config and Registration

**Goal**: Create config struct and register effect in the transform system.

**Build**:
- Create `src/config/cross_hatching_config.h` with CrossHatchingConfig struct (enabled + 6 parameters)
- Modify `src/config/effect_config.h`:
  - Add include for cross_hatching_config.h
  - Add `TRANSFORM_CROSS_HATCHING` to TransformEffectType enum
  - Add case to TransformEffectName() returning "Cross-Hatching"
  - Add `TRANSFORM_CROSS_HATCHING` to TransformOrderConfig::order array
  - Add `CrossHatchingConfig crossHatching` member to EffectConfig
  - Add case to IsTransformEnabled()

**Done when**: Project compiles with new enum and config accessible.

---

## Phase 2: Shader Implementation

**Goal**: Create the fragment shader with 4-layer hatching + Sobel edges.

**Build**:
- Create `shaders/cross_hatching.fs` implementing:
  - `hatchLine()` helper with smoothstep anti-aliasing
  - 4-layer luminance thresholding
  - Optional jitter UV perturbation
  - Sobel edge detection (3x3 kernel)
  - Color blend compositing
- Uniforms: resolution, density, width, threshold, jitter, outline, blend

**Done when**: Shader compiles without errors (test via manual LoadShader call).

---

## Phase 3: PostEffect Integration

**Goal**: Load shader, cache uniform locations, wire into transform pipeline.

**Build**:
- Modify `src/render/post_effect.h`:
  - Add `Shader crossHatchingShader`
  - Add uniform locations: crossHatchingResolutionLoc, crossHatchingDensityLoc, crossHatchingWidthLoc, crossHatchingThresholdLoc, crossHatchingJitterLoc, crossHatchingOutlineLoc, crossHatchingBlendLoc
- Modify `src/render/post_effect.cpp`:
  - Load shader in LoadPostEffectShaders()
  - Add to success check
  - Get uniform locations in GetShaderUniformLocations()
  - Set resolution in SetResolutionUniforms()
  - Unload in PostEffectUninit()
- Modify `src/render/shader_setup.h`: declare SetupCrossHatching()
- Modify `src/render/shader_setup.cpp`:
  - Add TRANSFORM_CROSS_HATCHING case in GetTransformEffect()
  - Implement SetupCrossHatching() binding all uniforms

**Done when**: Effect renders when enabled via config (even without UI).

---

## Phase 4: UI Panel

**Goal**: Add controls to the Style category in Effects panel.

**Build**:
- Modify `src/ui/imgui_effects.cpp`: add TRANSFORM_CROSS_HATCHING case to GetTransformCategory() returning "Style"
- Modify `src/ui/imgui_effects_transforms.cpp`:
  - Add static bool sectionCrossHatching
  - Add DrawStyleCrossHatching() helper with:
    - Enabled checkbox with MoveTransformToEnd on enable
    - ModulatableSlider for density (4.0-50.0)
    - ModulatableSlider for width (0.5-4.0)
    - ModulatableSlider for threshold (0.0-2.0)
    - ModulatableSlider for jitter (0.0-1.0)
    - ModulatableSlider for outline (0.0-1.0)
    - ModulatableSlider for blend (0.0-1.0)
  - Call DrawStyleCrossHatching() from DrawStyleCategory()

**Done when**: Effect controllable via UI, shows correct "Style" badge in pipeline.

---

## Phase 5: Serialization and Modulation

**Goal**: Enable preset save/load and audio modulation.

**Build**:
- Modify `src/config/preset.cpp`:
  - Add NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT for CrossHatchingConfig
  - Add to_json entry for crossHatching
  - Add from_json entry for crossHatching
- Modify `src/automation/param_registry.cpp`:
  - Add 6 entries to PARAM_TABLE with ranges matching UI sliders
  - Add 6 corresponding pointers to targets array (same order)

**Done when**: Presets persist cross-hatching settings; parameters respond to LFO/audio modulation.

---

## Verification Checklist

- [ ] Effect appears in transform pipeline list
- [ ] Shows "Style" category badge (not "???")
- [ ] Drag-drop reordering works
- [ ] Enabling moves effect to end of active chain
- [ ] All 6 sliders modify effect in real-time
- [ ] Preset save/load preserves all settings
- [ ] All parameters modulatable via LFOs and audio bands
- [ ] Smoothstep lines render without harsh aliasing
- [ ] Jitter creates organic hand-drawn feel at high values
- [ ] Blend=0 shows original, blend=1 shows pure ink
