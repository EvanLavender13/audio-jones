# Phyllotaxis Warp

UV displacement along spiral arms derived from sunflower phyllotaxis geometry. Creates swirling galaxy-arm or water-spiral distortion using analytical arm distance calculation.

## Research Source

[docs/research/phyllotaxis-warp.md](../research/phyllotaxis-warp.md) — Contains complete GLSL implementations, parameter ranges, and edge case handling.

## Current State

- `src/config/phyllotaxis_config.h` — Existing cellular phyllotaxis (loop-based nearest-seed). Naming: `PhyllotaxisConfig`, `TRANSFORM_PHYLLOTAXIS`
- `shaders/wave_ripple.fs:47-52` — Reference for radial UV displacement pattern
- `shaders/sine_warp.fs:31-32` — Reference for animated coordinate warping
- `src/ui/imgui_effects_transforms.cpp:450-467` — `DrawWarpCategory()` orchestrator where new effect goes

## Technical Implementation

### Shader Algorithm

**Source**: [docs/research/phyllotaxis-warp.md](../research/phyllotaxis-warp.md)

#### Core: Analytical Arm Distance

```glsl
#define GOLDEN_ANGLE 2.39996322972865

float getArmDistance(vec2 uv, float scale, float divergenceAngle, float spinOffset) {
    vec2 centered = uv - 0.5;
    float r = length(centered);
    float theta = atan(centered.y, centered.x) - spinOffset;

    if (r < 0.001) return 0.0;

    float estimatedN = (r / scale) * (r / scale);
    float expectedTheta = estimatedN * divergenceAngle;
    float armPhase = (theta - expectedTheta) / divergenceAngle;
    float distToArm = (armPhase - round(armPhase)) * divergenceAngle;

    return distToArm;
}
```

#### Combined Warp Function

```glsl
vec2 phyllotaxisWarp(vec2 uv, float scale, float divergenceAngle, float spinOffset,
                     float tangentStrength, float radialStrength, float falloff) {
    vec2 centered = uv - 0.5;
    float r = length(centered);
    if (r < 0.001) return uv;

    float distToArm = getArmDistance(uv, scale, divergenceAngle, spinOffset);
    float displacement = distToArm * exp(-abs(distToArm) * falloff);

    vec2 tangent = vec2(-centered.y, centered.x) / r;
    vec2 radial = centered / r;

    vec2 offset = tangent * displacement * tangentStrength * r
                + radial * displacement * radialStrength;

    return uv + offset;
}
```

### Parameters

| Parameter | Type | Range | Uniform Name | Notes |
|-----------|------|-------|--------------|-------|
| `scale` | float | [0.02, 0.15] | `scale` | Arm spacing. Smaller = tighter spirals |
| `divergenceAngle` | float | [2.0, 2.8] | `divergenceAngle` | 2.4 = golden angle. Off-values create spoke patterns |
| `warpStrength` | float | [0.0, 1.0] | `warpStrength` | Global displacement multiplier |
| `warpFalloff` | float | [1.0, 20.0] | `warpFalloff` | Sharpness of arm edges. Higher = narrower displacement band |
| `tangentIntensity` | float | [0.0, 1.0] | `tangentIntensity` | Swirl/twist amount (perpendicular to radius) |
| `radialIntensity` | float | [0.0, 1.0] | `radialIntensity` | Breathing/pulsing amount (along radius) |
| `spinSpeed` | float | [-2.0, 2.0] | (CPU accumulated) | Radians/second, passed as `spinOffset` |

### Modulation Targets

Register in `param_registry.cpp`:
- `phyllotaxisWarp.warpStrength` — [0.0, 1.0]
- `phyllotaxisWarp.warpFalloff` — [1.0, 20.0]
- `phyllotaxisWarp.tangentIntensity` — [0.0, 1.0]
- `phyllotaxisWarp.radialIntensity` — [0.0, 1.0]

---

## Phase 1: Config and Registration

**Goal**: Create config struct and register effect in transform system.

**Build**:
- Create `src/config/phyllotaxis_warp_config.h` with `PhyllotaxisWarpConfig` struct
- Modify `src/config/effect_config.h`:
  - Add include
  - Add `TRANSFORM_PHYLLOTAXIS_WARP` enum (before `TRANSFORM_EFFECT_COUNT`)
  - Add `TransformEffectName` case returning "Phyllotaxis Warp"
  - Add to `TransformOrderConfig::order` array at end
  - Add `PhyllotaxisWarpConfig phyllotaxisWarp` member to `EffectConfig`
  - Add `IsTransformEnabled` case

**Done when**: Code compiles. Effect enum exists.

---

## Phase 2: Shader

**Goal**: Implement fragment shader with combined tangent/radial warp.

**Build**:
- Create `shaders/phyllotaxis_warp.fs` implementing:
  - `getArmDistance()` helper
  - `phyllotaxisWarp()` combined function
  - Main function sampling displaced UV
- Uniforms: `scale`, `divergenceAngle`, `warpStrength`, `warpFalloff`, `tangentIntensity`, `radialIntensity`, `spinOffset`

**Done when**: Shader compiles (check via raylib shader load).

---

## Phase 3: PostEffect Integration

**Goal**: Wire shader loading, uniform locations, and time accumulation.

**Build**:
- Modify `src/render/post_effect.h`:
  - Add `Shader phyllotaxisWarpShader`
  - Add uniform location ints: `phyllotaxisWarpScaleLoc`, `phyllotaxisWarpDivergenceAngleLoc`, `phyllotaxisWarpWarpStrengthLoc`, `phyllotaxisWarpWarpFalloffLoc`, `phyllotaxisWarpTangentIntensityLoc`, `phyllotaxisWarpRadialIntensityLoc`, `phyllotaxisWarpSpinOffsetLoc`
  - Add `float phyllotaxisWarpSpinOffset` time accumulator
- Modify `src/render/post_effect.cpp`:
  - LoadShader in `LoadPostEffectShaders()`
  - Add to success check
  - GetShaderLocation calls in `GetShaderUniformLocations()`
  - Initialize `phyllotaxisWarpSpinOffset = 0.0f`
  - UnloadShader in `PostEffectUninit()`
- Modify `src/render/render_pipeline.cpp`:
  - Add time accumulation: `pe->phyllotaxisWarpSpinOffset += deltaTime * pe->effects.phyllotaxisWarp.spinSpeed`

**Done when**: Shader loads without errors. Uniform locations resolve.

---

## Phase 4: Shader Setup

**Goal**: Connect config values to shader uniforms.

**Build**:
- Modify `src/render/shader_setup.h`: declare `SetupPhyllotaxisWarp(PostEffect* pe)`
- Modify `src/render/shader_setup.cpp`:
  - Add `TRANSFORM_PHYLLOTAXIS_WARP` case in `GetTransformEffect()`
  - Implement `SetupPhyllotaxisWarp()` setting all uniforms from config + accumulated spin offset

**Done when**: Effect renders (enable in code, verify visual output).

---

## Phase 5: UI Panel

**Goal**: Add controls in Warp category.

**Build**:
- Modify `src/ui/imgui_effects.cpp`: add `TRANSFORM_PHYLLOTAXIS_WARP` case in `GetTransformCategory()` returning `CATEGORY_WARP`
- Modify `src/ui/imgui_effects_transforms.cpp`:
  - Add `static bool sectionPhyllotaxisWarp = false`
  - Add `DrawWarpPhyllotaxisWarp()` function with:
    - Enabled checkbox with `MoveTransformToEnd` call
    - `ImGui::SliderFloat` for scale, divergenceAngle (non-modulatable)
    - `ModulatableSlider` for warpStrength, warpFalloff, tangentIntensity, radialIntensity
    - `ImGui::SliderFloat` for spinSpeed (rad/s display)
  - Call from `DrawWarpCategory()` after Domain Warp

**Done when**: UI controls visible and responsive.

---

## Phase 6: Preset Serialization

**Goal**: Save/load effect settings in presets.

**Build**:
- Modify `src/config/preset.cpp`:
  - Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(PhyllotaxisWarpConfig, ...)` macro
  - Add `if (e.phyllotaxisWarp.enabled) { j["phyllotaxisWarp"] = e.phyllotaxisWarp; }` in `to_json`
  - Add `e.phyllotaxisWarp = j.value("phyllotaxisWarp", e.phyllotaxisWarp);` in `from_json`

**Done when**: Preset save/load preserves effect state.

---

## Phase 7: Parameter Registration

**Goal**: Enable modulation routing for audio-reactive parameters.

**Build**:
- Modify `src/automation/param_registry.cpp`:
  - Add to `PARAM_TABLE` (maintain index alignment):
    - `{"phyllotaxisWarp.warpStrength", {0.0f, 1.0f}}`
    - `{"phyllotaxisWarp.warpFalloff", {1.0f, 20.0f}}`
    - `{"phyllotaxisWarp.tangentIntensity", {0.0f, 1.0f}}`
    - `{"phyllotaxisWarp.radialIntensity", {0.0f, 1.0f}}`
  - Add to `targets[]` array at matching indices:
    - `&effects->phyllotaxisWarp.warpStrength`
    - `&effects->phyllotaxisWarp.warpFalloff`
    - `&effects->phyllotaxisWarp.tangentIntensity`
    - `&effects->phyllotaxisWarp.radialIntensity`

**Done when**: Modulation routes work in UI. LFO/audio affects parameters.

---

## Phase 8: Verification

**Goal**: Confirm full integration.

**Verify**:
- [ ] Effect appears in transform pipeline list
- [ ] Shows "Warp" category badge (not "???")
- [ ] Can be reordered via drag-drop
- [ ] Enabling adds to pipeline
- [ ] UI controls modify effect in real-time
- [ ] spinSpeed creates animated rotation
- [ ] tangentIntensity=1, radialIntensity=0 produces pure swirl
- [ ] tangentIntensity=0, radialIntensity=1 produces pure breathing
- [ ] divergenceAngle near 2.4 produces smooth spirals; off-values show spoke patterns
- [ ] Preset save/load preserves all settings
- [ ] Modulation routes respond to LFO/audio
- [ ] Build succeeds with no warnings

**Done when**: All verification items pass.
