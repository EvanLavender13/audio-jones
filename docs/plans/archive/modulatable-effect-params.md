# Modulatable Effect Parameters

Convert hard-coded effect automations (beat-triggered blur/chromatic, tick-based kaleido rotation) to user-controllable modulatable parameters. Remove dedicated rotation LFO in favor of the unified modulation system.

> **Implementation note**: `kaleidoRotationSpeed` was kept as a regular slider (not modulatable) during implementation. The param registration and ModulatableSlider calls for this field were omitted.

## Current State

Hard-coded automations to remove:
- `src/render/post_effect.cpp:197` - blur: `baseBlurScale + beatIntensity * beatBlurScale`
- `src/render/post_effect.cpp:399` - chromatic: `beatIntensity * chromaticMaxOffset`
- `src/render/post_effect.cpp:398` - kaleido: `0.002f * globalTick` (no user control)
- `src/render/post_effect.cpp:198-201` - feedback rotation LFO processing

Files to modify:
- `src/config/effect_config.h` - struct field changes
- `src/automation/param_registry.cpp` - register new params
- `src/ui/imgui_effects.cpp` - replace sliders, remove LFO section
- `src/render/post_effect.cpp` - remove automation logic
- `src/render/post_effect.h` - remove LFO state, change blur type
- `shaders/blur_h.fs` - int to float uniform
- `shaders/blur_v.fs` - int to float uniform
- `src/config/preset.cpp` - update serialization macro

---

## Phase 1: Config Struct Changes

**Goal**: Update EffectConfig fields and PostEffect state.

**Build**:
- `src/config/effect_config.h`:
  - Remove: `int baseBlurScale`, `int beatBlurScale`, `int chromaticMaxOffset`, `LFOConfig rotationLFO`
  - Add: `float blurScale = 1.0f`, `float chromaticOffset = 0.0f`, `float kaleidoRotationSpeed = 0.002f`
- `src/render/post_effect.h`:
  - Remove: `LFOState rotationLFOState`
  - Change: `int currentBlurScale` to `float currentBlurScale`

**Done when**: Code compiles with field access errors in post_effect.cpp and imgui_effects.cpp (expected).

---

## Phase 2: Remove Hard-Coded Automation

**Goal**: Simplify post_effect.cpp to read config values directly.

**Build**:
- `src/render/post_effect.cpp` in `PostEffectApplyFeedbackStage()`:
  - Line 197: `pe->currentBlurScale = pe->effects.blurScale;`
  - Lines 198-201: Delete LFO processing block, just `pe->currentRotation = pe->effects.feedbackRotation;`
- `src/render/post_effect.cpp` in `ApplyOutputPipeline()`:
  - Line 398: `pe->currentKaleidoRotation = pe->effects.kaleidoRotationSpeed * (float)globalTick;`
  - Line 399: `pe->currentChromaticOffset = pe->effects.chromaticOffset;`
- `src/render/post_effect.cpp` in `PostEffectInit()`:
  - Remove `LFOStateInit(&pe->rotationLFOState);`
- `src/render/post_effect.cpp` in `SetupBlurH()` and `SetupBlurV()`:
  - Change `SHADER_UNIFORM_INT` to `SHADER_UNIFORM_FLOAT`

**Done when**: Code compiles, but blur shader may not work correctly yet (uniform type mismatch).

---

## Phase 3: Shader Updates

**Goal**: Change blur shaders to accept float uniform.

**Build**:
- `shaders/blur_h.fs`:
  - Line 11: `uniform float blurScale;`
  - Line 18: `if (blurScale < 0.01)` (float comparison)
  - Line 23: Remove `float()` cast
- `shaders/blur_v.fs`:
  - Line 13: `uniform float blurScale;`
  - Line 22: `if (blurScale < 0.01)`
  - Line 25: Remove `float()` cast

**Done when**: Run app, blur effect works with float values.

---

## Phase 4: Parameter Registration

**Goal**: Register new params with ModEngine.

**Build**:
- `src/automation/param_registry.cpp`:
  - Add to PARAM_TABLE:
    ```
    {"effects.blurScale",       {0.0f, 10.0f}},
    {"effects.chromaticOffset", {0.0f, 50.0f}},
    {"kaleido.rotationSpeed",   {-0.01f, 0.01f}},
    {"feedback.rotation",       {-0.1f, 0.1f}},
    ```
  - Add to `ParamRegistryInit()`:
    ```
    ModEngineRegisterParam("effects.blurScale", &effects->blurScale, 0.0f, 10.0f);
    ModEngineRegisterParam("effects.chromaticOffset", &effects->chromaticOffset, 0.0f, 50.0f);
    ModEngineRegisterParam("kaleido.rotationSpeed", &effects->kaleidoRotationSpeed, -0.01f, 0.01f);
    ModEngineRegisterParam("feedback.rotation", &effects->feedbackRotation, -0.1f, 0.1f);
    ```

**Done when**: App runs without warnings about missing param IDs.

---

## Phase 5: UI Updates

**Goal**: Replace sliders with ModulatableSlider, remove LFO section.

**Build**:
- `src/ui/imgui_effects.cpp`:
  - Line 27: Replace `ImGui::SliderInt("Blur", ...)` with `ModulatableSlider("Blur", &e->blurScale, "effects.blurScale", "%.1f px", modSources);`
  - Line 29: Remove `ImGui::SliderInt("Bloom", ...)` entirely
  - Line 30: Replace `ImGui::SliderInt("Chroma", ...)` with `ModulatableSlider("Chroma", &e->chromaticOffset, "effects.chromaticOffset", "%.0f px", modSources);`
  - Line 32: Replace `SliderAngleDeg("Rotation", ...)` with `ModulatableSlider("Rotation", &e->feedbackRotation, "feedback.rotation", "%.4f rad/f", modSources);`
  - After line 34 (Kaleido segments): Add `ModulatableSlider("Kaleido Spin", &e->kaleidoRotationSpeed, "kaleido.rotationSpeed", "%.4f", modSources);`
  - Lines 97-106: Delete entire "Rotation LFO" section
  - Line 13: Remove `static bool sectionLFO = false;`

**Done when**: UI shows modulatable sliders with diamond indicators, no LFO section.

---

## Phase 6: Preset Serialization

**Goal**: Update JSON field list.

**Build**:
- `src/config/preset.cpp`:
  - Update `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT` for EffectConfig:
    - Remove: `baseBlurScale`, `beatBlurScale`, `chromaticMaxOffset`, `rotationLFO`
    - Add: `blurScale`, `chromaticOffset`, `kaleidoRotationSpeed`

**Done when**: Save/load presets works. Old presets load with defaults for new fields.
