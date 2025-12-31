# Agent Simulation Abstraction

Extract shared infrastructure from physarum and curl_flow into a new `src/simulation/` module with reusable TrailMap and shader utilities.

## Current State

Both effects duplicate ~300 lines of identical code:

- `src/render/physarum.cpp:11-18` / `curl_flow.cpp:10-17` - `LoadShaderSource()` (identical)
- `src/render/physarum.cpp:20-50` / `curl_flow.cpp:19-49` - `RGBToHSV()` (identical)
- `src/render/physarum.cpp:86-118` / `curl_flow.cpp:65-97` - `CreateTrailMap()` (identical)
- `src/render/physarum.cpp:120-125` / `curl_flow.cpp:99-104` - `ClearTrailMap()` (identical)
- `src/render/physarum.cpp:170-198` / `curl_flow.cpp:147-175` - `LoadTrailProgram()` (near-identical)
- `src/render/physarum.cpp:344-384` / `curl_flow.cpp:317-357` - `ProcessTrails()` (near-identical)

Both already share `shaders/physarum_trail.glsl` and `shaders/physarum_debug.fs`.

Files that reference physarum/curl_flow and need include updates:
- `src/config/effect_config.h:4-5`
- `src/main.cpp:15-16`
- `src/render/post_effect.h:8-9`
- `src/render/post_effect.cpp:3-4,150-151,167-168,209-210`
- `src/render/render_pipeline.cpp:4-5`
- `src/ui/imgui_effects.cpp` (if exists)

---

## Phase 1: Create Simulation Module Infrastructure

**Goal**: Create `src/simulation/` module with TrailMap struct and shader utilities.

**Build**:

Create `src/simulation/shader_utils.h`:
- Declare `SimLoadShaderSource(const char* path)` - loads shader source with error logging

Create `src/simulation/shader_utils.cpp`:
- Implement `SimLoadShaderSource` using `LoadFileText` and `TraceLog`

Create `src/simulation/trail_map.h`:
- Define `TrailMap` struct containing:
  - `RenderTexture2D primary` - main trail texture (agents write here)
  - `RenderTexture2D temp` - ping-pong buffer for diffusion
  - `unsigned int program` - trail compute shader program
  - Uniform locations: `resolutionLoc`, `diffusionScaleLoc`, `decayFactorLoc`, `applyDecayLoc`, `directionLoc`
  - `int width, height`
- Declare lifecycle functions:
  - `TrailMap* TrailMapInit(int width, int height)`
  - `void TrailMapUninit(TrailMap* tm)`
  - `void TrailMapResize(TrailMap* tm, int width, int height)`
  - `void TrailMapClear(TrailMap* tm)`
  - `void TrailMapProcess(TrailMap* tm, float deltaTime, float decayHalfLife, int diffusionScale)`
  - `bool TrailMapBeginDraw(TrailMap* tm)`
  - `void TrailMapEndDraw(TrailMap* tm)`
  - `Texture2D TrailMapGetTexture(const TrailMap* tm)`

Create `src/simulation/trail_map.cpp`:
- Implement all TrailMap functions
- Extract logic from `physarum.cpp:86-118` (CreateTrailMap), `physarum.cpp:120-125` (ClearTrailMap), `physarum.cpp:170-198` (LoadTrailProgram), `physarum.cpp:344-384` (ProcessTrails)
- Use `shaders/trail_diffusion.glsl` path (renamed shader)

Rename shaders:
- `shaders/physarum_trail.glsl` -> `shaders/trail_diffusion.glsl`
- `shaders/physarum_debug.fs` -> `shaders/trail_debug.fs`

Update `CMakeLists.txt`:
- Add `src/simulation/trail_map.cpp` and `src/simulation/shader_utils.cpp` to sources

**Done when**: Build succeeds with new files (not yet used by effects).

---

## Phase 2: Extract Color Utilities

**Goal**: Move `RGBToHSV` to existing color_config module.

**Build**:

Modify `src/render/color_config.h`:
- Add declaration after line 32: `void ColorConfigRGBToHSV(Color c, float* outH, float* outS, float* outV);`

Create `src/render/color_config.cpp`:
- Implement `ColorConfigRGBToHSV` (extract from `physarum.cpp:20-50`)
- Include `color_config.h`, `<math.h>`

Update `CMakeLists.txt`:
- Add `src/render/color_config.cpp` to RENDER_SOURCES

**Done when**: Build succeeds with new color utility.

---

## Phase 3: Migrate Physarum to Simulation Module

**Goal**: Move physarum to `src/simulation/` and refactor to use shared infrastructure.

**Build**:

Move files:
- `src/render/physarum.h` -> `src/simulation/physarum.h`
- `src/render/physarum.cpp` -> `src/simulation/physarum.cpp`

Modify `src/simulation/physarum.h`:
- Add forward declaration: `typedef struct TrailMap TrailMap;`
- Remove trail-related fields from Physarum struct (lines 38-39, 54-59):
  - Remove `RenderTexture2D trailMap`
  - Remove `RenderTexture2D trailMapTemp`
  - Remove `unsigned int trailProgram`
  - Remove `trailResolutionLoc`, `trailDiffusionScaleLoc`, `trailDecayFactorLoc`, `trailApplyDecayLoc`, `trailDirectionLoc`
- Add field: `TrailMap* trailMap;`

Modify `src/simulation/physarum.cpp`:
- Add includes: `#include "trail_map.h"`, `#include "shader_utils.h"`, `#include "render/color_config.h"`
- Delete static functions: `LoadShaderSource`, `RGBToHSV`, `CreateTrailMap`, `ClearTrailMap`, `LoadTrailProgram`
- In `PhysarumInit`:
  - Replace `CreateTrailMap(&p->trailMap, ...)` with `p->trailMap = TrailMapInit(width, height)`
  - Remove second CreateTrailMap call (trailMapTemp now inside TrailMap)
  - Remove LoadTrailProgram call (now inside TrailMapInit)
- In `PhysarumUninit`:
  - Replace `UnloadRenderTexture(p->trailMap)` and `UnloadRenderTexture(p->trailMapTemp)` with `TrailMapUninit(p->trailMap)`
  - Remove `rlUnloadShaderProgram(p->trailProgram)` (now in TrailMapUninit)
- In `PhysarumUpdate`:
  - Replace `RGBToHSV(...)` with `ColorConfigRGBToHSV(...)`
  - Replace `p->trailMap.texture.id` with `TrailMapGetTexture(p->trailMap).id`
- In `PhysarumProcessTrails`:
  - Replace entire body with: `TrailMapProcess(p->trailMap, deltaTime, p->config.decayHalfLife, p->config.diffusionScale);`
- In `PhysarumResize`:
  - Replace `UnloadRenderTexture` + `CreateTrailMap` calls with `TrailMapResize(p->trailMap, width, height)`
- In `PhysarumReset`:
  - Replace `ClearTrailMap(&p->trailMap)` calls with `TrailMapClear(p->trailMap)`
- In `PhysarumBeginTrailMapDraw`:
  - Replace `BeginTextureMode(p->trailMap)` with `return TrailMapBeginDraw(p->trailMap)`
- In `PhysarumEndTrailMapDraw`:
  - Replace `EndTextureMode()` with `TrailMapEndDraw(p->trailMap)`
- In `PhysarumDrawDebug`:
  - Update trail texture access to use `TrailMapGetTexture`
- In `LoadComputeProgram`:
  - Replace `LoadShaderSource` with `SimLoadShaderSource`

Update include paths in consumers:
- `src/config/effect_config.h:4`: `#include "render/physarum.h"` -> `#include "simulation/physarum.h"`
- `src/main.cpp:15`: Update include
- `src/render/post_effect.h:8`: Update forward declaration comment or include
- `src/render/post_effect.cpp:3`: Update include
- `src/render/render_pipeline.cpp:4`: Update include, update trail texture access

Update `CMakeLists.txt`:
- Move `physarum.cpp` from RENDER_SOURCES to SIMULATION_SOURCES (create new source group)

**Done when**: Build succeeds and physarum effect works visually (trails diffuse and decay correctly).

---

## Phase 4: Migrate CurlFlow to Simulation Module

**Goal**: Move curl_flow to `src/simulation/` and refactor to use shared infrastructure.

**Build**:

Move files:
- `src/render/curl_flow.h` -> `src/simulation/curl_flow.h`
- `src/render/curl_flow.cpp` -> `src/simulation/curl_flow.cpp`

Modify `src/simulation/curl_flow.h`:
- Same changes as physarum.h: add TrailMap forward declaration, replace trail fields with `TrailMap* trailMap`

Modify `src/simulation/curl_flow.cpp`:
- Same refactoring pattern as physarum.cpp
- Delete static functions: `LoadShaderSource`, `RGBToHSV`, `CreateTrailMap`, `ClearTrailMap`, `LoadTrailProgram`
- Update all lifecycle functions to use TrailMap and color utilities

Update include paths in consumers:
- `src/config/effect_config.h:5`: Update include path
- `src/main.cpp:16`: Update include path
- `src/render/post_effect.h:9`: Update forward declaration
- `src/render/post_effect.cpp:4`: Update include
- `src/render/render_pipeline.cpp:5`: Update include, update trail texture access

Update `CMakeLists.txt`:
- Move `curl_flow.cpp` to SIMULATION_SOURCES

**Done when**: Build succeeds and curl_flow effect works visually.

---

## Phase 5: Cleanup

**Goal**: Verify no dead references remain.

**Build**:

Verify no remaining references:
- Grep for `physarum_trail.glsl` - should find only git history
- Grep for `physarum_debug.fs` - should find only git history
- Grep for old include paths `render/physarum.h` and `render/curl_flow.h`

**Done when**: All old references removed, build succeeds.
