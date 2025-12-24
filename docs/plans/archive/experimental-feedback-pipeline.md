# Experimental Feedback Pipeline

A separate, swappable rendering pipeline for MilkDrop-style feedback experimentation. The waveform becomes a subtle seed (low opacity injection) rather than the dominant foreground. Feedback loop with blur + decay + subtle zoom dominates the visual. Toggle via new UI panel.

See `docs/research/milkdrop-feedback-architecture.md` for background on the MilkDrop approach.

## Current State

Existing rendering uses PostEffect which draws waveforms at full brightness onto the accumulation texture:

- `src/render/post_effect.cpp:368` - `PostEffectBeginAccum()` applies feedback, leaves accumTexture open
- `src/render/post_effect.cpp:390` - `PostEffectEndAccum()` closes texture mode
- `src/main.cpp:176-188` - Waveforms drawn between Begin/End, then `PostEffectToScreen()`
- `src/config/effect_config.h:7` - `EffectConfig` struct with feedback parameters
- `src/ui/imgui_effects.cpp:14` - Effects panel pattern with collapsible sections
- `shaders/feedback.fs` - Current feedback shader (zoom + rotation + domain warp)
- `shaders/blur_v.fs:38-42` - Exponential decay formula

## Phase 1: Core Infrastructure ✓

**Goal**: Build skeleton that compiles and initializes without crashing.

**Build**:
- `src/config/experimental_config.h` - Config struct with: halfLife (float, default 0.5), zoomFactor (float, default 0.995), injectionOpacity (float, default 0.3)
- `src/render/experimental_effect.h` - ExperimentalEffect struct with 3 RenderTexture2D fields, 2 Shader fields, uniform locations, screen dimensions. Function declarations for Init/Uninit/Resize
- `src/render/experimental_effect.cpp` - Implement Init (create 3 HDR textures, load placeholder shaders), Uninit (free resources), Resize (recreate textures)
- Add new sources to CMakeLists.txt

**Done when**: App builds and runs normally. ExperimentalEffect initialized but not used in render loop.

---

## Phase 2: Shaders ✓

**Goal**: Create the two experimental shaders that compile and run.

**Build**:
- `shaders/experimental/feedback_exp.fs` - Fragment shader with uniforms (resolution, halfLife, deltaTime, zoomFactor). Apply 3x3 gaussian blur with fixed 1-pixel sampling, framerate-independent exponential decay, subtle zoom toward center
- `shaders/experimental/blend_inject.fs` - Fragment shader with uniforms (injectionTex, injectionOpacity). Sample both textures, blend: `feedback * (1-opacity) + injection * opacity`
- Update ExperimentalEffectInit to load these shaders and cache uniform locations

**Done when**: Shaders compile without errors (verified by Init not returning NULL).

---

## Phase 3: Pipeline Implementation ✓

**Goal**: Complete render pipeline functions that process textures correctly.

**Build**:
- `ExperimentalEffectBeginAccum(exp, deltaTime)` - Apply feedback shader (expAccumTexture → expTempTexture), then begin injectionTexture render mode with clear
- `ExperimentalEffectEndAccum()` - End injection texture mode, apply blend shader (expTempTexture + injectionTexture → expAccumTexture)
- `ExperimentalEffectToScreen()` - Draw expAccumTexture to screen as fullscreen quad
- `ExperimentalEffectClear()` - Clear all 3 textures to black (for pipeline switching)

**Done when**: Functions implemented. Can manually test by temporarily wiring into main.cpp.

---

## Phase 4: Main Loop Integration ✓

**Goal**: Toggle between PostEffect and ExperimentalEffect pipelines.

**Build**:
- Modify `AppContext` in main.cpp - Add `ExperimentalEffect* experimentalEffect` and `bool useExperimentalPipeline`
- Modify `AppContextInit()` - Initialize experimentalEffect, default toggle to false
- Modify `AppContextUninit()` - Free experimentalEffect
- Modify resize handler - Call ExperimentalEffectResize when window resizes
- Modify render loop - If toggle true: call ExperimentalEffect functions and draw waveforms to injection texture. Else: existing PostEffect code unchanged
- Add toggle state change detection - Clear buffers when switching pipelines

**Done when**: Can set `useExperimentalPipeline = true` in code and see waveforms with feedback trails. Toggle in code switches between pipelines cleanly.

---

## Phase 5: UI Panel ✓

**Goal**: New "Experimental" window with toggle and parameter sliders.

**Build**:
- `src/ui/imgui_experimental.cpp` - `ImGuiDrawExperimentalPanel(ExperimentalConfig* cfg, bool* useExperimental)` function with checkbox toggle and 3 sliders (half-life, zoom, injection opacity)
- Add declaration to `src/ui/imgui_panels.h`
- Call panel from main.cpp UI section
- Move toggle state change detection to work with UI checkbox

**Done when**: "Experimental" window appears in UI. Checkbox toggles pipelines. Sliders adjust parameters in real-time.

---

## Phase 6: Polish ✓

**Goal**: Fix bugs and tune parameters.

**Fixes applied**:
- **Blur bug**: Original shader multiplied texel size by blurScale, causing discrete line duplication instead of smooth blur. Fixed by using fixed 1-pixel texel spacing for proper 3x3 Gaussian kernel
- **Decay model**: Replaced abstract `decayRate` (0-1 per frame) with `halfLife` in seconds, matching the existing pipeline. Uses framerate-independent formula: `exp(-0.693147 * deltaTime / halfLife)`
- **Redundant UI**: Removed `[ACTIVE]/[OFF]` status label - checkbox state is self-explanatory
- **Opacity minimum**: Changed from 0.0 to 0.05 minimum - at 0 nothing renders

**Final parameters**:
- Half-life: 0.1 - 2.0 seconds (default 0.5s)
- Zoom: 0.98 - 1.0 (default 0.995)
- Opacity: 0.05 - 1.0 (default 0.3)

**Done when**: Stable, visually distinct from PostEffect, parameters produce expected results.
