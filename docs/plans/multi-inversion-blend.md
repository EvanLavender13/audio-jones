# Multi-Inversion Blend

Post-processing effect that chains circle inversions with depth-weighted accumulation. Creates bulging/pinching distortions that compound across iterations, with animated centers following a Lissajous path.

## Current State

Relevant files for integration:
- `shaders/mobius.fs:40-76` - Reference accumulation pattern
- `src/config/mobius_config.h` - Reference config structure
- `src/config/effect_config.h:14-31` - Transform effect enum and order
- `src/render/post_effect.h:29-31` - Shader fields pattern
- `src/render/post_effect.cpp:39-112` - Shader load/uniform pattern
- `src/render/render_pipeline.cpp:248-256` - Setup function pattern
- `src/ui/imgui_effects.cpp:111-120` - UI section pattern

## Technical Implementation

### Source

[Iterated Inversion System (IIS)](https://github.com/soma-arc/IteratedInversionSystem) by Kento Nakamura

### Core Algorithm

**Circle inversion formula** (verified from IIS):
```glsl
vec2 circleInvert(vec2 pos, vec2 center, float radius) {
    vec2 d = pos - center;
    float r2 = radius * radius;
    return center + d * (r2 / dot(d, d));
}
```

**Main shader logic**:
```glsl
void main() {
    vec3 colorAccum = vec3(0.0);
    float weightAccum = 0.0;

    // Map UV from [0,1] to [-1,1] centered
    vec2 z = (fragTexCoord - 0.5) * 2.0;

    for (int i = 0; i < iterations; i++) {
        // Phase-offset Lissajous: each iteration sees different point on path
        float t = time * animSpeed + float(i) * phaseOffset;
        vec2 center = focalAmplitude * vec2(sin(t * focalFreqX), cos(t * focalFreqY));

        // Per-iteration radius scaling
        float r = radius * pow(radiusScale, float(i));

        // Apply circle inversion
        z = circleInvert(z, center, r);

        // Smooth remap to UV space (avoids hard boundaries)
        vec2 sampleUV = 0.5 + 0.4 * sin(z * 0.5);

        // Depth-based weight: earlier iterations contribute less
        float weight = 1.0 / float(i + 2);
        colorAccum += texture(texture0, sampleUV).rgb * weight;
        weightAccum += weight;
    }

    finalColor = vec4(colorAccum / weightAccum, 1.0);
}
```

### Parameters

| Uniform | Type | Range | Description |
|---------|------|-------|-------------|
| iterations | int | 1-12 | Inversion depth |
| radius | float | 0.1-1.0 | Base inversion radius |
| radiusScale | float | 0.5-1.5 | Per-iteration radius multiplier |
| focalAmplitude | float | 0.0-0.3 | Lissajous center offset magnitude |
| focalFreqX | float | 0.1-5.0 | Lissajous X frequency |
| focalFreqY | float | 0.1-5.0 | Lissajous Y frequency |
| phaseOffset | float | 0.0-2.0 | Per-iteration phase offset along Lissajous |
| animSpeed | float | 0.0-2.0 | Animation speed multiplier |

---

## Phase 1: Shader and Config

**Goal**: Create the shader and config struct.

**Build**:
- `shaders/multi_inversion.fs` - Fragment shader with circle inversion and accumulation
- `src/config/multi_inversion_config.h` - Config struct with all parameters and defaults

**Done when**: Shader compiles, config struct exists with sensible defaults.

---

## Phase 2: Pipeline Integration

**Goal**: Wire shader into render pipeline as a transform effect.

**Build**:
- `src/config/effect_config.h` - Add `TRANSFORM_MULTI_INVERSION` to enum, include config, add to `EffectConfig` struct and default order
- `src/render/post_effect.h` - Add shader field and uniform location fields
- `src/render/post_effect.cpp` - Load shader, get uniform locations, unload on cleanup
- `src/render/render_pipeline.cpp` - Add `SetupMultiInversion()`, add case to `GetTransformEffect()`, increment time in `RenderPipelineApplyFeedback()`, compute Lissajous focal in `RenderPipelineApplyOutput()`

**Done when**: Effect renders when enabled via code (e.g., setting `enabled = true` in config defaults).

---

## Phase 3: UI and Serialization

**Goal**: Add UI controls and preset save/load.

**Build**:
- `src/ui/imgui_effects.cpp` - Add section with checkbox and sliders for all parameters
- `src/config/preset.cpp` - Add JSON serialization for `MultiInversionConfig`

**Done when**: Effect controllable via UI, persists across preset save/load.
