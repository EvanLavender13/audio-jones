# Turbulence Cascade

Post-processing shader that stacks sine-based coordinate shifts with rotation and scaling. Uses depth-weighted accumulation (like Möbius/KIFS) to heavily transform the input texture into organic swirl patterns.

**Research**: See `docs/research/heavy-texture-transforms.md` (Technique 3: Turbulence Cascade, lines 185-260) for algorithm details, GLSL reference code, and audio-reactive mapping suggestions.

## Current State

Relevant files and integration points:

- `shaders/mobius.fs:48-73` - Reference accumulation loop pattern
- `src/config/mobius_config.h` - Reference config struct pattern
- `src/config/effect_config.h:51` - Where to add TurbulenceConfig
- `src/render/post_effect.h:29` - Where to add shader + uniform locations
- `src/render/post_effect.cpp:39` - LoadPostEffectShaders, add turbulence shader
- `src/render/render_pipeline.cpp:474-478` - Where to add RenderPass (before kaleidoscope)
- `src/ui/imgui_effects.cpp:43-52` - Reference UI section pattern (Möbius)
- `src/config/preset.cpp:100-104` - Where to add JSON serialization

---

## Phase 1: Shader

**Goal**: Create the turbulence cascade fragment shader with depth accumulation.

**Build**:
- Create `shaders/turbulence.fs`
- Implement accumulation loop: for each octave, apply sine-based shifts, rotate, sample texture, accumulate with depth weight
- Uniforms: `time`, `octaves`, `strength`, `animSpeed`, `rotationPerOctave`
- Smooth UV remap using `sin()` to avoid hard boundaries (match Möbius pattern)

**Done when**: Shader compiles and produces visible swirl distortion when tested with hardcoded values.

---

## Phase 2: Config

**Goal**: Define the configuration struct with sensible defaults.

**Build**:
- Create `src/config/turbulence_config.h`
- Struct `TurbulenceConfig` with fields:
  - `enabled` (bool, default false)
  - `octaves` (int, default 4, range 1-8)
  - `strength` (float, default 0.5, range 0-2)
  - `animSpeed` (float, default 0.3, range 0-2)
  - `rotationPerOctave` (float, default 0.5 radians, range 0-2π)
- Include in `effect_config.h` and add `TurbulenceConfig turbulence` field to `EffectConfig`

**Done when**: Project compiles with new config struct accessible from `EffectConfig`.

---

## Phase 3: PostEffect Integration

**Goal**: Load shader and cache uniform locations.

**Build**:
- `post_effect.h`: Add `Shader turbulenceShader` and uniform location fields (`turbulenceTimeLoc`, `turbulenceOctavesLoc`, `turbulenceStrengthLoc`, `turbulenceAnimSpeedLoc`, `turbulenceRotPerOctaveLoc`)
- `post_effect.h`: Add `float turbulenceTime` for animation state
- `post_effect.cpp` `LoadPostEffectShaders`: Load `shaders/turbulence.fs`
- `post_effect.cpp` `GetShaderUniformLocations`: Cache all turbulence uniform locations
- `post_effect.cpp` `PostEffectUninit`: Unload turbulence shader
- Initialize `turbulenceTime = 0.0f` in `PostEffectInit`

**Done when**: Shader loads without errors, uniform locations are cached.

---

## Phase 4: Render Pipeline

**Goal**: Apply turbulence as a render pass before kaleidoscope.

**Build**:
- `render_pipeline.cpp`: Add `SetupTurbulence` function (set all uniforms from config)
- `render_pipeline.cpp` `RenderPipelineApplyFeedback`: Update `pe->turbulenceTime += deltaTime`
- `render_pipeline.cpp` `RenderPipelineApplyOutput`: Add conditional RenderPass for turbulence before the kaleidoscope pass (after Möbius, line ~478)

**Done when**: Enabling turbulence in code produces visible swirl effect.

---

## Phase 5: UI

**Goal**: Add Effects panel section for turbulence controls.

**Build**:
- `imgui_effects.cpp`: Add `static bool sectionTurbulence = false`
- Add section between Möbius and Kaleidoscope with `DrawSectionBegin("Turbulence", Theme::GLOW_ORANGE, &sectionTurbulence)`
- Checkbox for `enabled`
- `ImGui::SliderInt` for octaves (1-8)
- `ModulatableSlider` for strength with path `"turbulence.strength"`
- `ImGui::SliderFloat` for animSpeed (0-2)
- `ModulatableSliderAngleDeg` for rotationPerOctave with path `"turbulence.rotationPerOctave"`

**Done when**: UI controls appear and modify effect parameters in real-time.

---

## Phase 6: Preset Serialization

**Goal**: Save/load turbulence config in presets.

**Build**:
- `preset.cpp`: Add `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(TurbulenceConfig, enabled, octaves, strength, animSpeed, rotationPerOctave)`
- Add `turbulence` to the `EffectConfig` serialization macro (between `mobius` and `kaleidoscope`)

**Done when**: Presets save and restore turbulence settings correctly.
