# Gamma Post Effect

Add gamma correction as the final step in the regular rendering pipeline (after FXAA), mirroring the experimental pipeline's approach.

## Current State

- `src/render/post_effect.cpp:353-415` - Output pipeline: trail boost → kaleido → chromatic → FXAA → screen
- `src/render/post_effect.h:11-61` - PostEffect struct with 8 shaders, no gamma
- `src/config/effect_config.h:7-34` - EffectConfig struct, no gamma field
- `src/config/preset.cpp:79-84` - JSON serialization macro for EffectConfig
- `src/ui/imgui_effects.cpp:24-34` - Core Effects section UI controls
- `shaders/experimental/composite.fs` - Reference gamma shader implementation

## Phase 1: Add Gamma Shader and Config

**Goal**: Create shader file and add gamma parameter to config.

**Build**:
- Create `shaders/gamma.fs` - Copy from `shaders/experimental/composite.fs`, same implementation: `pow(color, vec3(1.0/gamma))`
- Modify `src/config/effect_config.h` - Add `float gamma = 1.0f` field to EffectConfig struct
- Modify `src/config/preset.cpp` - Add `gamma` to the NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT macro for EffectConfig

**Done when**: Shader file exists, config compiles with gamma field, preset serialization includes gamma.

---

## Phase 2: Integrate Gamma into Post Effect Pipeline

**Goal**: Load gamma shader and apply it as final output step.

**Build**:
- Modify `src/render/post_effect.h`:
  - Add `Shader gammaShader` to PostEffect struct
  - Add `int gammaGammaLoc` for uniform location
- Modify `src/render/post_effect.cpp`:
  - Load shader in `LoadPostEffectShaders()`
  - Get uniform location in `GetShaderUniformLocations()`
  - Unload shader in `PostEffectUninit()`
  - Apply gamma after FXAA in `ApplyOutputPipeline()` - only when gamma != 1.0 (optimization)

**Done when**: Gamma shader loads, applies after FXAA, visual output matches experimental pipeline behavior.

---

## Phase 3: Add UI Control

**Goal**: Expose gamma slider in Effects panel.

**Build**:
- Modify `src/ui/imgui_effects.cpp` - Add `ImGui::SliderFloat("Gamma", &e->gamma, 0.5f, 2.5f, "%.2f")` in Core Effects section (after Kaleido slider)

**Done when**: Slider appears in UI, adjusting it changes display gamma in real-time.
