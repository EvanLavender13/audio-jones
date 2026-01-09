# Wave Ripple

Pseudo-3D radial wave displacement effect. Summed sine waves create a height field; gradient displaces UVs for parallax. Optional height-based shading sells the 3D illusion.

## Current State

Transform effects follow a consistent pattern across these files:
- `src/config/sine_warp_config.h:1` - reference config struct pattern
- `src/config/effect_config.h:14` - TransformEffectType enum, EffectConfig struct
- `src/render/post_effect.h:28` - shader and uniform location storage
- `src/render/post_effect.cpp:38` - shader loading in LoadPostEffectShaders
- `src/render/shader_setup.cpp:10` - GetTransformEffect dispatch table
- `src/render/render_pipeline.cpp:143` - time accumulation in ApplyFeedback
- `src/automation/param_registry.cpp:53` - modulatable parameter registration
- `src/ui/imgui_effects.cpp:373` - Warp category UI section
- `src/config/preset.cpp:112` - NLOHMANN serialization macro

## Technical Implementation

**Source**: [docs/research/wave-ripple.md](../research/wave-ripple.md)

### Height Field (Radial Sine Sum)

For each fragment at UV position `p`, compute distance from wave origin and sum octaves:

```glsl
float getHeight(vec2 uv, vec2 origin, float time, int octaves, float baseFreq) {
    float dist = length(uv - origin);
    float height = 0.0;
    float freq = baseFreq;
    float amp = 1.0;

    for (int i = 0; i < octaves; i++) {
        height += sin(dist * freq - time) * amp;
        freq *= 2.0;
        amp *= 0.5;
    }
    return height;
}
```

### Steepness (Gerstner Asymmetry)

Shape the sine wave for sharper crests, flatter troughs:

```glsl
// steepness in [0, 1], 0 = symmetric sine, 1 = sharp crests
float shaped = height - steepness * height * height;
```

### UV Displacement

Displace UV along radial direction, scaled by height:

```glsl
vec2 dir = normalize(uv - origin);
vec2 displacedUV = uv + dir * shaped * strength;
```

Handle the singularity at origin (where `dir` is undefined) by falling back to no displacement when `dist < epsilon`.

### Height-Based Shading

Modulate brightness based on height:

```glsl
float shade = 1.0 + shaped * shadeIntensity;
color.rgb *= shade;
```

### Uniforms

| Uniform | Type | Source |
|---------|------|--------|
| time | float | CPU-accumulated (animSpeed per frame) |
| octaves | int | Config (1-4) |
| strength | float | Config (0.0-0.5) |
| frequency | float | Config (1.0-20.0) |
| steepness | float | Config (0.0-1.0) |
| origin | vec2 | Config (originX, originY, default 0.5) |
| shadeEnabled | bool | Config toggle |
| shadeIntensity | float | Config (0.0-0.5) |

---

## Phase 1: Config and Enum

**Goal**: Define WaveRippleConfig struct and add to effect system.

**Build**:
- Create `src/config/wave_ripple_config.h` with WaveRippleConfig struct (enabled, octaves, strength, animSpeed, frequency, steepness, originX, originY, shadeEnabled, shadeIntensity)
- Edit `src/config/effect_config.h`:
  - Include wave_ripple_config.h
  - Add TRANSFORM_WAVE_RIPPLE to enum (before TRANSFORM_EFFECT_COUNT)
  - Add case to TransformEffectName()
  - Add waveRipple field to EffectConfig
  - Update TransformOrderConfig default order array

**Done when**: Code compiles with new config struct accessible via EffectConfig.

---

## Phase 2: Shader and Rendering

**Goal**: Create shader and wire it into the render pipeline.

**Build**:
- Create `shaders/wave_ripple.fs` implementing the algorithm above
- Edit `src/render/post_effect.h`:
  - Add waveRippleShader field
  - Add uniform location fields (waveRippleTimeLoc, etc.)
  - Add waveRippleTime accumulator field
- Edit `src/render/post_effect.cpp`:
  - Load shader in LoadPostEffectShaders
  - Get uniform locations in GetShaderUniformLocations
  - Unload shader in PostEffectUninit
  - Initialize waveRippleTime in PostEffectInit
- Edit `src/render/shader_setup.h`:
  - Declare SetupWaveRipple function
- Edit `src/render/shader_setup.cpp`:
  - Add TRANSFORM_WAVE_RIPPLE case to GetTransformEffect
  - Implement SetupWaveRipple to set all uniforms
- Edit `src/render/render_pipeline.cpp`:
  - Add time accumulation: `pe->waveRippleTime += deltaTime * pe->effects.waveRipple.animSpeed`

**Done when**: Effect renders (enable in code temporarily) with visible wave pattern.

---

## Phase 3: UI and Modulation

**Goal**: Add UI controls and audio-reactive modulation support.

**Build**:
- Edit `src/ui/imgui_effects.cpp`:
  - Add static bool sectionWaveRipple state
  - Add Wave Ripple section under Warp category (after Texture Warp)
  - Add switch case for TRANSFORM_WAVE_RIPPLE in effect order list
  - Controls: Enabled checkbox, Octaves slider (1-4), Strength modulatable, Anim Speed slider, Frequency modulatable, Steepness modulatable, Origin X/Y modulatable, Shade Enabled checkbox, Shade Intensity modulatable (shown when shadeEnabled)
- Edit `src/automation/param_registry.cpp`:
  - Add entries to PARAM_TABLE for waveRipple.strength, waveRipple.frequency, waveRipple.steepness, waveRipple.originX, waveRipple.originY, waveRipple.shadeIntensity
  - Add pointers in ParamRegistryInit targets array

**Done when**: UI shows Wave Ripple section with functional controls; parameters can be modulated.

---

## Phase 4: Serialization and Polish

**Goal**: Save/load presets with Wave Ripple settings.

**Build**:
- Edit `src/config/preset.cpp`:
  - Add NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT macro for WaveRippleConfig
  - Add serialization in to_json for EffectConfig (if enabled)
  - Add deserialization in from_json for EffectConfig

**Done when**: Create preset with Wave Ripple enabled, save, reload - settings persist correctly.
