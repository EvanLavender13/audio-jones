# Strange Attractor Flow

GPU compute shader particle system where agents trace trajectories through chaotic dynamical systems (Lorenz, Rössler, Aizawa, Thomas). Particles follow deterministic ODEs with RK4 integration, projected orthographically to screen, depositing colored trails.

## Current State

Existing simulation infrastructure to reuse:
- `src/simulation/trail_map.h:7-21` - Ping-pong diffusion/decay buffer
- `src/render/blend_compositor.h` - Trail-to-accum compositing
- `src/render/color_config.h` - HSV/gradient color modes
- `src/simulation/curl_flow.h` - Pattern reference for agent struct, config, API

Integration points:
- `src/render/post_effect.h:84-86` - Add AttractorFlow* member
- `src/config/effect_config.h:40-44` - Add AttractorFlowConfig member
- `src/render/render_pipeline.cpp:178-195` - Add ApplyAttractorFlowPass
- `src/ui/imgui_effects.cpp:132-155` - Add UI section

---

## Phase 1: Config and Header

**Goal**: Define data structures and API declarations.

**Create**:
- `src/simulation/attractor_flow.h` - AttractorType enum, AttractorAgent struct (32-byte aligned: x,y,z,hue,age,pad[3]), AttractorFlowConfig struct with defaults, AttractorFlow struct with GPU handles and uniform locations, API declarations

**Modify**:
- `src/config/effect_config.h` - Add `#include "simulation/attractor_flow.h"`, add `AttractorFlowConfig attractorFlow;` member

**Done when**: Project compiles with new header included.

---

## Phase 2: Compute Shader

**Goal**: Create the agent update shader with Lorenz attractor.

**Create**:
- `shaders/attractor_agents.glsl` - Compute shader with:
  - Agent SSBO at binding 0
  - TrailMap image at binding 1
  - Lorenz derivative function
  - RK4 integration step
  - Orthographic projection to screen
  - Trail deposit with HSV color
  - Respawn logic when position exceeds bounds

**Done when**: Shader file exists and contains complete Lorenz implementation.

---

## Phase 3: Core Implementation

**Goal**: Implement AttractorFlow lifecycle and update logic.

**Create**:
- `src/simulation/attractor_flow.cpp` - Implement:
  - `AttractorFlowSupported` - Check OpenGL 4.3
  - `AttractorFlowInit` - Load shader, create SSBO, create TrailMap, cache uniform locations
  - `AttractorFlowUninit` - Cleanup all resources
  - `AttractorFlowUpdate` - Enable shader, set uniforms, bind buffers, dispatch compute, memory barrier
  - `AttractorFlowProcessTrails` - Delegate to TrailMapProcess
  - `AttractorFlowResize` - Resize TrailMap, reset agents
  - `AttractorFlowReset` - Clear trails, reinitialize agents
  - `AttractorFlowApplyConfig` - Handle agent count changes, color changes
  - `AttractorFlowDrawDebug` - Draw trail texture overlay
  - `AttractorFlowBeginTrailMapDraw` / `EndTrailMapDraw` - Delegate to TrailMap

**Done when**: All functions implemented following curl_flow.cpp patterns.

---

## Phase 4: Pipeline Integration

**Goal**: Wire AttractorFlow into the render pipeline.

**Modify**:
- `src/render/post_effect.h` - Add forward decl and `AttractorFlow* attractorFlow;` member
- `src/render/post_effect.cpp` - Add include, init in PostEffectInit, uninit in PostEffectUninit, resize in PostEffectResize
- `src/render/render_pipeline.cpp` - Add include, add `SetupAttractorFlowTrailBoost` function, add `ApplyAttractorFlowPass` function, call pass in `RenderPipelineApplyFeedback`, add trail boost in `RenderPipelineApplyOutput`
- `src/main.cpp` - Add `RenderDrawablesToAttractor` function (for visual layering, not perturbation), call after other drawable injections

**Done when**: Enabling attractorFlow.enabled shows Lorenz butterfly on screen.

---

## Phase 5: Additional Attractors

**Goal**: Add Rössler, Aizawa, and Thomas attractor systems.

**Modify**:
- `shaders/attractor_agents.glsl` - Add derivative functions for Rössler, Aizawa, Thomas with hardcoded classic parameters. Add `uniform int attractorType` and switch dispatch in main.
- `src/simulation/attractor_flow.cpp` - Add `attractorTypeLoc` uniform location, set in Update

**Done when**: All 4 attractors selectable via config and produce correct shapes.

---

## Phase 6: UI Panel

**Goal**: Add ImGui controls for attractor configuration.

**Modify**:
- `src/ui/imgui_effects.cpp` - Add `static bool sectionAttractorFlow` state. Add "Attractor Flow" section with:
  - Enabled checkbox
  - Attractor type combo (Lorenz/Rössler/Aizawa/Thomas)
  - Agent count slider (10K-500K)
  - Lorenz params: sigma, rho, beta sliders
  - Time scale slider
  - Attractor scale slider
  - Deposit, decay, diffusion sliders
  - Boost intensity, blend mode
  - ColorConfig widget
  - Debug overlay checkbox

**Done when**: All parameters controllable via UI, changes take effect immediately.

---

## Phase 7: Preset Serialization

**Goal**: Save and load attractor settings with presets.

**Modify**:
- `src/config/preset.cpp` - Add AttractorFlowConfig serialization following existing CurlFlowConfig pattern

**Done when**: Attractor settings persist across save/load cycles.
