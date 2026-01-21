# Pencil Sketch

Transform effect converting visuals into graphite pencil drawings via directional gradient-aligned stroke accumulation, with paper texture, vignette, and subtle wobble animation.

## Current State

- `docs/research/pencil-sketch.md` - Complete algorithm from Flockaroo ShaderToy
- `.claude/skills/add-effect/SKILL.md` - 8-phase effect checklist
- `src/ui/imgui_effects_style.cpp` - Style category UI (cross-hatching, watercolor, etc.)
- `src/config/cross_hatching_config.h` - Similar stylization config pattern

## Technical Implementation

### Source
- [Shadertoy: Hand Drawing](https://www.shadertoy.com/view/MsSGD1) - Flockaroo (2016)
- See `docs/research/pencil-sketch.md` for full analysis

### Core Algorithm

For each pixel:
1. Apply wobble offset (CPU-accumulated time, not shader time)
2. Compute central-difference gradient
3. For each of `angleCount` angles, sample `sampleCount` points along that direction (both positive/negative)
4. Accumulate stroke darkness where gradient aligns with sample direction
5. Apply paper texture and vignette

### Gradient Computation

Central-difference gradient at position `pos` with epsilon `eps`:

```glsl
vec2 getGrad(vec2 pos, float eps)
{
    vec2 d = vec2(eps, 0);
    return vec2(
        getVal(pos + d.xy) - getVal(pos - d.xy),
        getVal(pos + d.yx) - getVal(pos - d.yx)
    ) / eps / 2.0;
}
```

Where `getVal()` returns luminance: `dot(color.rgb, vec3(0.333))`.

### Stroke Accumulation Loop

```glsl
for (int i = 0; i < angleCount; i++)
{
    float ang = PI2 / float(angleCount) * (float(i) + 0.8);
    vec2 v = vec2(cos(ang), sin(ang));

    for (int j = 0; j < sampleCount; j++)
    {
        // Perpendicular offset (stroke width)
        vec2 dpos = v.yx * vec2(1, -1) * float(j) * scale;
        // Parallel offset (curved stroke path)
        vec2 dpos2 = v.xy * float(j * j) / float(sampleCount) * 0.5 * scale;

        for (float s = -1.0; s <= 1.0; s += 2.0)  // Both directions
        {
            vec2 pos2 = pos + s * dpos + dpos2;
            vec2 g = getGrad(pos2, gradientEps);

            // Gradient alignment with sample direction
            float fact = dot(g, v) - 0.5 * abs(dot(g, v.yx * vec2(1, -1)));
            fact = clamp(fact, 0.0, 0.05);
            fact *= 1.0 - float(j) / float(sampleCount) * strokeFalloff;  // Distance fade

            col += fact;
        }
    }
}
```

### Paper Texture

Subtle grid pattern via sin waves:

```glsl
vec2 s = sin(pos.xy * 0.1 / sqrt(resolution.y / 400.0));
vec3 karo = vec3(1.0);
karo -= paperStrength * 0.5 * vec3(0.25, 0.1, 0.1) * dot(exp(-s * s * 80.0), vec2(1));
```

### Vignette

Radial cubic falloff:

```glsl
float r = length(pos - resolution.xy * 0.5) / resolution.x;
float vign = 1.0 - r * r * r * vignetteStrength;
```

### Wobble Animation

Position offset oscillates with CPU-accumulated time (NOT shader time):

```glsl
vec2 pos = fragCoord + wobbleAmount * sin(wobbleTime * vec2(1, 1.7)) * resolution.y / 400.0;
```

CPU accumulates: `pencilSketchWobbleTime += wobbleSpeed * deltaTime`

### Parameters

| Parameter | Type | Range | Default | Effect |
|-----------|------|-------|---------|--------|
| angleCount | int | 2-6 | 3 | Number of hatching directions |
| sampleCount | int | 8-24 | 16 | Samples per direction (stroke length) |
| strokeFalloff | float | 0.0-1.0 | 1.0 | Distance fade rate |
| gradientEps | float | 0.2-1.0 | 0.4 | Edge sensitivity |
| paperStrength | float | 0.0-1.0 | 0.5 | Paper texture visibility |
| vignetteStrength | float | 0.0-1.0 | 1.0 | Edge darkening |
| wobbleSpeed | float | 0.0-2.0 | 1.0 | Animation rate (0 = static) |
| wobbleAmount | float | 0.0-8.0 | 4.0 | Pixel displacement |

### Modulation Candidates

Register in `param_registry.cpp`:
- `pencilSketch.strokeFalloff` - {0.0f, 1.0f}
- `pencilSketch.wobbleAmount` - {0.0f, 8.0f}
- `pencilSketch.vignetteStrength` - {0.0f, 1.0f}
- `pencilSketch.paperStrength` - {0.0f, 1.0f}

---

## Phase 1: Config and Enum

**Goal**: Define effect configuration and register in effect system.

**Build**:
- Create `src/config/pencil_sketch_config.h` with `PencilSketchConfig` struct containing all 8 parameters plus `enabled` bool
- Modify `src/config/effect_config.h`:
  - Add `#include "pencil_sketch_config.h"`
  - Add `TRANSFORM_PENCIL_SKETCH` to `TransformEffectType` enum (before `TRANSFORM_EFFECT_COUNT`)
  - Add name case in `TransformEffectName()`: "Pencil Sketch"
  - Add to `TransformOrderConfig::order` array (at end before closing brace)
  - Add `PencilSketchConfig pencilSketch;` member to `EffectConfig`
  - Add enabled case in `IsTransformEnabled()`

**Done when**: Code compiles with new enum value and config struct.

---

## Phase 2: Shader

**Goal**: Implement core pencil sketch algorithm.

**Build**:
- Create `shaders/pencil_sketch.fs` implementing:
  - Uniforms: `resolution`, `angleCount`, `sampleCount`, `strokeFalloff`, `gradientEps`, `paperStrength`, `vignetteStrength`, `wobbleTime`, `wobbleAmount`
  - `getVal()` luminance helper
  - `getGrad()` central-difference gradient
  - Main loop with configurable angle/sample counts (use `#define` or uniform integers)
  - Paper texture and vignette post-processing
  - Wobble position offset using `wobbleTime` uniform (not `iTime`)

**Done when**: Shader compiles and produces pencil-style output when tested in isolation.

---

## Phase 3: PostEffect Integration

**Goal**: Load shader and bind uniforms.

**Build**:
- Modify `src/render/post_effect.h`:
  - Add `Shader pencilSketchShader;`
  - Add uniform location ints: `pencilSketchResolutionLoc`, `pencilSketchAngleCountLoc`, `pencilSketchSampleCountLoc`, `pencilSketchStrokeFalloffLoc`, `pencilSketchGradientEpsLoc`, `pencilSketchPaperStrengthLoc`, `pencilSketchVignetteStrengthLoc`, `pencilSketchWobbleTimeLoc`, `pencilSketchWobbleAmountLoc`
  - Add `float pencilSketchWobbleTime;` for CPU time accumulation

- Modify `src/render/post_effect.cpp`:
  - Load shader in `LoadPostEffectShaders()`
  - Add to success check
  - Get uniform locations in `GetShaderUniformLocations()`
  - Add to resolution uniforms in `SetResolutionUniforms()`
  - Unload in `PostEffectUninit()`

**Done when**: Shader loads without errors; uniform locations resolve.

---

## Phase 4: Shader Setup and Dispatch

**Goal**: Connect effect to render pipeline.

**Build**:
- Modify `src/render/shader_setup.h`:
  - Declare `void SetupPencilSketch(PostEffect* pe);`

- Modify `src/render/shader_setup.cpp`:
  - Add dispatch case in `GetTransformEffect()`:
    ```cpp
    case TRANSFORM_PENCIL_SKETCH:
        return { &pe->pencilSketchShader, SetupPencilSketch, &pe->effects.pencilSketch.enabled };
    ```
  - Implement `SetupPencilSketch()`:
    - Accumulate wobble time: `pe->pencilSketchWobbleTime += pe->currentDeltaTime * cfg->wobbleSpeed;`
    - Bind all uniforms from config

**Done when**: Effect renders when enabled via code.

---

## Phase 5: UI Panel

**Goal**: Add user controls in Style category.

**Build**:
- Modify `src/ui/imgui_effects.cpp`:
  - Add `case TRANSFORM_PENCIL_SKETCH:` to `GetTransformCategory()` returning `{"STY", 4}`

- Modify `src/ui/imgui_effects_style.cpp`:
  - Add `static bool sectionPencilSketch = false;`
  - Create `DrawStylePencilSketch()` helper with:
    - Enabled checkbox with `MoveTransformToEnd()` call
    - `angleCount` slider (int, 2-6)
    - `sampleCount` slider (int, 8-24)
    - Modulation sliders for strokeFalloff, paperStrength, vignetteStrength
    - Tree node for "Animation" with wobbleSpeed, wobbleAmount sliders
  - Call helper from `DrawStyleCategory()` with spacing

**Done when**: Effect controls appear in Style panel and modify effect in real-time.

---

## Phase 6: Preset Serialization

**Goal**: Enable save/load of effect settings.

**Build**:
- Modify `src/config/preset.cpp`:
  - Add JSON macro: `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(PencilSketchConfig, enabled, angleCount, sampleCount, strokeFalloff, gradientEps, paperStrength, vignetteStrength, wobbleSpeed, wobbleAmount)`
  - Add to_json entry: `if (e.pencilSketch.enabled) { j["pencilSketch"] = e.pencilSketch; }`
  - Add from_json entry: `e.pencilSketch = j.value("pencilSketch", e.pencilSketch);`

**Done when**: Preset save/load preserves pencil sketch settings.

---

## Phase 7: Parameter Registration

**Goal**: Enable modulation for audio-reactive parameters.

**Build**:
- Modify `src/automation/param_registry.cpp`:
  - Add to `PARAM_TABLE` (maintain alphabetical order within section):
    ```cpp
    {"pencilSketch.strokeFalloff",      {0.0f, 1.0f}},
    {"pencilSketch.wobbleAmount",       {0.0f, 8.0f}},
    {"pencilSketch.vignetteStrength",   {0.0f, 1.0f}},
    {"pencilSketch.paperStrength",      {0.0f, 1.0f}},
    ```
  - Add to `targets` array at matching indices:
    ```cpp
    &effects->pencilSketch.strokeFalloff,
    &effects->pencilSketch.wobbleAmount,
    &effects->pencilSketch.vignetteStrength,
    &effects->pencilSketch.paperStrength,
    ```

**Done when**: Parameters respond to LFO and audio modulation routes.

---

## Phase 8: Verification

**Goal**: Confirm complete implementation.

**Verify**:
- [ ] Effect appears in transform order pipeline
- [ ] Effect shows "STY" category badge (not "???")
- [ ] Effect can be reordered via drag-drop
- [ ] Enabling effect adds it to pipeline list
- [ ] UI controls modify effect in real-time
- [ ] Preset save/load preserves settings
- [ ] Modulation routes to registered parameters
- [ ] Build succeeds with no warnings
- [ ] Wobble animation runs smoothly without jumps on parameter changes

**Done when**: All verification items pass.
