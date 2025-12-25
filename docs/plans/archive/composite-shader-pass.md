# Composite Shader Pass

Add a display-only shader pass that runs after the feedback loop but does not feed back into the accumulator. This completes the MilkDrop-equivalent two-pass architecture where effects can be applied to the final output without compounding each frame. Initial effect: gamma correction.

## Current State

- `src/render/experimental_effect.cpp:181-188` - `ExperimentalEffectToScreen()` draws `expAccumTexture` directly with no shader
- `src/render/experimental_effect.cpp:7-20` - `LoadExperimentalShaders()` loads feedback and blend shaders
- `src/render/experimental_effect.cpp:22-37` - `GetShaderUniformLocations()` caches uniform locations
- `src/config/experimental_config.h:4-19` - Config structs with nested `FlowFieldConfig`
- `src/ui/imgui_experimental.cpp:26-69` - UI sections using `DrawSectionBegin/End` pattern
- `shaders/chromatic.fs` - Reference for simple post-process shader structure

## Phase 1: Shader and Config

**Goal**: Add composite shader file and config struct without changing runtime behavior.

**Create**:
- `shaders/experimental/composite.fs` - Fragment shader with `uniform float gamma`, applies `pow(color, vec3(1.0 / gamma))`

**Modify**:
- `src/config/experimental_config.h` - Add `CompositeConfig` struct with `float gamma = 1.0f`, add field to `ExperimentalConfig`

**Done when**: Project builds successfully, no behavior change.

---

## Phase 2: Shader Loading and Cleanup

**Goal**: Load composite shader and cache uniform location.

**Modify**:
- `src/render/experimental_effect.h` - Add `Shader compositeShader` and `int compositeGammaLoc` fields to struct
- `src/render/experimental_effect.cpp`:
  - `LoadExperimentalShaders()` - Load `composite.fs`, add validation check, update return condition
  - `GetShaderUniformLocations()` - Cache gamma uniform location
  - `ExperimentalEffectUninit()` - Unload composite shader

**Done when**: Shader loads without warnings in log, project runs unchanged.

---

## Phase 3: Apply Composite Pass

**Goal**: Apply composite shader when drawing to screen.

**Modify**:
- `src/render/experimental_effect.cpp` - Replace `ExperimentalEffectToScreen()` body:
  - Wrap draw in `BeginShaderMode()`/`EndShaderMode()`
  - Set gamma uniform before draw

**Done when**:
- Gamma=1.0 produces identical output to pre-change (passthrough)
- Manually setting gamma=2.0 in code visibly brightens midtones
- Feedback loop behavior unchanged (trails/flow work normally)

---

## Phase 4: UI Integration

**Goal**: Expose gamma slider in Experimental panel.

**Modify**:
- `src/ui/imgui_experimental.cpp`:
  - Add `static bool sectionComposite = false`
  - Add Composite section after Injection using `DrawSectionBegin("Composite", Theme::GLOW_CYAN, &sectionComposite)`
  - Add gamma slider with range 0.5-2.5

**Done when**:
- Composite section appears collapsed by default
- Gamma slider updates in real-time
- Tooltip explains effect clearly

---

## Validation Checklist

- [ ] Gamma=1.0 produces pixel-identical output to main branch
- [ ] Gamma=2.0 brightens midtones without affecting black/white points
- [ ] Gamma=0.5 darkens midtones appropriately
- [ ] Feedback loop unaffected by gamma changes (trails decay same, flow works)
- [ ] No performance regression (<0.1ms overhead)
- [ ] Shader loads without warnings
- [ ] UI section collapses/expands correctly
