# Flow Field Feedback Migration

Replace the standard pipeline's zoom/rotation/warp feedback shader with an 8-parameter spatial flow field from the experimental pipeline. Remove domain warp, add edge fade, register all flow field params for modulation, then delete the experimental pipeline.

## Current State

**Standard pipeline feedback:**
- `shaders/feedback.fs:10-19` - Uniforms: zoom, rotation, desaturate, warp params
- `shaders/feedback.fs:60-90` - Transform: warp → rotate → zoom → desaturate
- `src/config/effect_config.h:12-19` - 7 feedback/warp params in EffectConfig
- `src/render/post_effect.h:38-45` - 7 uniform location fields
- `src/render/render_pipeline.cpp:8-20` - SetWarpUniforms helper
- `src/render/render_pipeline.cpp:58-67` - SetupFeedback callback

**Experimental pipeline flow field (to migrate):**
- `shaders/experimental/feedback_exp.fs:14-22` - 8 flow field uniforms
- `shaders/experimental/feedback_exp.fs:39-52` - Spatially-varying UV transform
- `shaders/experimental/feedback_exp.fs:55-61` - Edge fade with smoothstep
- `src/config/experimental_config.h:4-13` - FlowFieldConfig struct

**Modulation registration:**
- `src/automation/param_registry.cpp:11-19` - PARAM_TABLE with ID/range pairs
- `src/automation/param_registry.cpp:25-33` - Pointer targets array

**UI patterns:**
- `src/ui/imgui_effects.cpp:39-48` - Collapsible section with DrawSectionBegin/End
- `src/ui/imgui_experimental.cpp:33-59` - Flow field sliders with ModulatableSlider

---

## Phase 1: Config & Shader

**Goal**: Define FlowFieldConfig struct and rewrite feedback shader with flow field logic.

**Build**:
- Add `FlowFieldConfig` struct to `effect_config.h` (copy from `experimental_config.h:4-13`):
  - zoomBase (0.995f), zoomRadial (0.0f)
  - rotBase (0.0f), rotRadial (0.0f)
  - dxBase (0.0f), dxRadial (0.0f)
  - dyBase (0.0f), dyRadial (0.0f)
- Add `FlowFieldConfig flowField;` member to EffectConfig
- Remove from EffectConfig: feedbackZoom, feedbackRotation, warpStrength, warpScale, warpOctaves, warpLacunarity, warpSpeed
- Rewrite `shaders/feedback.fs`:
  - Uniforms: 8 flow field params + resolution + desaturate
  - Aspect-corrected radius calculation (from `feedback_exp.fs:29-37`)
  - Spatially-varying params: `zoom = zoomBase + rad * zoomRadial`, etc.
  - Transform order: zoom → rotate → translate (from `feedback_exp.fs:46-52`)
  - Edge fade with smoothstep (from `feedback_exp.fs:55-61`)
  - Keep desaturate logic (from current `feedback.fs:88-89`)

**Done when**: Project compiles. Shader loads without errors (runtime errors expected until Phase 2).

---

## Phase 2: Render Integration

**Goal**: Wire flow field config values to shader uniforms.

**Build**:
- Update `post_effect.h`:
  - Remove: feedbackZoomLoc, feedbackRotationLoc, warpStrengthLoc, warpScaleLoc, warpOctavesLoc, warpLacunarityLoc, warpTimeLoc
  - Add: feedbackResolutionLoc, feedbackZoomBaseLoc, feedbackZoomRadialLoc, feedbackRotBaseLoc, feedbackRotRadialLoc, feedbackDxBaseLoc, feedbackDxRadialLoc, feedbackDyBaseLoc, feedbackDyRadialLoc
  - Remove: warpTime field
- Update `post_effect.cpp`:
  - GetShaderUniformLocations: Replace 7 old queries with 9 new (8 flow + resolution)
  - SetResolutionUniforms: Add feedback shader resolution
  - Remove warpTime initialization
- Update `render_pipeline.cpp`:
  - Delete SetWarpUniforms function
  - Rewrite SetupFeedback: Set 8 flow field uniforms + resolution + desaturate
  - Remove warpTime accumulation from RenderPipelineApplyFeedback

**Done when**: Application runs. Flow field defaults produce visible feedback (zoomBase=0.995 creates slow inward motion).

---

## Phase 3: Modulation Registration

**Goal**: Register all 8 flow field parameters for audio/LFO modulation.

**Build**:
- Update `param_registry.cpp`:
  - Remove "feedback.rotation" entry
  - Add 8 entries to PARAM_TABLE:
    - "flowField.zoomBase": {0.98f, 1.02f}
    - "flowField.zoomRadial": {-0.02f, 0.02f}
    - "flowField.rotBase": {-0.01f, 0.01f}
    - "flowField.rotRadial": {-0.01f, 0.01f}
    - "flowField.dxBase": {-0.02f, 0.02f}
    - "flowField.dxRadial": {-0.02f, 0.02f}
    - "flowField.dyBase": {-0.02f, 0.02f}
    - "flowField.dyRadial": {-0.02f, 0.02f}
  - Add 8 pointers to targets array: &effects->flowField.zoomBase, etc.

**Done when**: Modulation popup shows 8 flow field targets. Assigning LFO to flowField.rotBase produces visible rotation oscillation.

---

## Phase 4: UI Panel

**Goal**: Replace Domain Warp section with Flow Field section in Effects panel.

**Build**:
- Update `imgui_effects.cpp`:
  - Remove sectionWarp static bool
  - Add sectionFlowField static bool
  - Remove Zoom, Rotation sliders from Core Effects section
  - Remove entire Domain Warp section (DrawSectionBegin block)
  - Add Flow Field section with Theme::GLOW_MAGENTA accent:
    - 8 ModulatableSlider widgets for flow field params
    - Keep Desat slider (move into section or keep in Core Effects)
  - Slider formats: "%.4f" for zoom/rot, "%.4f" for dx/dy
  - Add tooltips explaining base vs radial behavior

**Done when**: Flow Field collapsible section appears in Effects panel. All 8 sliders modify feedback visuals. Modulation indicators (diamonds) appear on all sliders.

---

## Phase 5: Experimental Pipeline Cleanup

**Goal**: Delete all experimental pipeline code and files.

**Build**:
- Delete shader files:
  - `shaders/experimental/feedback_exp.fs`
  - `shaders/experimental/blend_inject.fs`
  - `shaders/experimental/composite.fs`
  - `shaders/experimental/` directory (if empty)
- Delete source files:
  - `src/render/experimental_effect.h`
  - `src/render/experimental_effect.cpp`
  - `src/config/experimental_config.h`
  - `src/ui/imgui_experimental.cpp`
- Update `src/main.cpp`:
  - Remove #include "render/experimental_effect.h"
  - Remove ExperimentalEffect* experimentalEffect from AppContext
  - Remove useExperimentalPipeline, prevUseExperimentalPipeline flags
  - Remove ExperimentalEffectInit/Uninit/Resize calls
  - Remove RenderExperimentalPipeline function
  - Remove pipeline toggle detection block
  - Remove experimental render branch (always use RenderStandardPipeline)
  - Remove ImGuiDrawExperimentalPanel call
- Update `src/ui/imgui_panels.h`:
  - Remove ExperimentalConfig forward declaration
  - Remove ImGuiDrawExperimentalPanel declaration
- Update `CMakeLists.txt`:
  - Remove experimental_effect.cpp from sources
  - Remove imgui_experimental.cpp from sources

**Done when**: Clean build succeeds. `grep -ri "experimental" src/` returns zero matches. Application runs with only standard pipeline.

---

## Phase 6: Validation

**Goal**: Verify feature parity and preset compatibility.

**Build**:
- Test default behavior: zoomBase=0.995, all radial=0 creates slow inward spiral
- Test tunnel effect: zoomRadial=0.01 (edges zoom faster than center)
- Test vortex: rotRadial=-0.005 (rotation increases toward center)
- Test horizontal flow: dxBase=0.01 (content drifts right)
- Test edge fade: High rotation values should fade at edges, not streak
- Test aspect ratio: Resize to 16:9 and 4:3, verify flow field remains circular
- Test modulation: Assign bass to flowField.zoomBase, verify audio-reactive zoom
- Test preset save/load: Create preset with flow field values, reload, verify persistence
- Test legacy preset: Load old preset (if any exist with feedbackZoom), verify defaults applied

**Done when**: All visual tests pass. No edge artifacts. Modulation works on all 8 params. Presets save/load correctly.
