# Render Pipeline Modularization

Split `render_pipeline.cpp` (632 lines) into three focused modules: profiler, shader setup, and pipeline orchestration.

## Current State

- `src/render/render_pipeline.h:8-44` - Profiler types and function declarations
- `src/render/render_pipeline.cpp:16-77` - Profiler implementation (~60 lines)
- `src/render/render_pipeline.cpp:79-378` - 17 shader setup functions (~300 lines)
- `src/render/render_pipeline.cpp:380-631` - Pipeline logic (~250 lines)

**Callers:**
- `src/main.cpp:97` - `ProfilerInit()`
- `src/ui/imgui_analysis.cpp` - reads `Profiler*` for overlay
- All `Setup*` functions are `static`, called only by `RenderPass()`

## Phase 1: Extract Profiler Module

**Goal**: Move profiler types and implementation to standalone module.

**Build**:
- Create `src/render/profiler.h` with:
  - `PROFILER_HISTORY_SIZE` constant
  - `ProfileZoneId` enum
  - `ProfileZone` struct
  - `Profiler` struct
  - Function declarations: Init, Uninit, FrameBegin, FrameEnd, BeginZone, EndZone
- Create `src/render/profiler.cpp` with:
  - `ZONE_NAMES` static array
  - All `Profiler*` function implementations (lines 25-77)
- Update `src/render/render_pipeline.h`:
  - Add `#include "profiler.h"`
  - Remove profiler type definitions and declarations
- Update `src/main.cpp`:
  - Add `#include "render/profiler.h"`
- Update `src/ui/imgui_analysis.cpp`:
  - Add `#include "render/profiler.h"`
- Update `CMakeLists.txt`:
  - Add `src/render/profiler.cpp` to sources

**Done when**: Build succeeds, profiler overlay displays timing data.

---

## Phase 2: Extract Shader Setup Module

**Goal**: Move shader uniform binding functions to dedicated module.

**Build**:
- Create `src/render/shader_setup.h` with:
  - Forward declaration: `typedef struct PostEffect PostEffect;`
  - `RenderPipelineShaderSetupFn` typedef
  - `TransformEffectEntry` struct
  - Function declarations for all 17 setup functions
  - `GetTransformEffect()` declaration
- Create `src/render/shader_setup.cpp` with:
  - `#include "shader_setup.h"`
  - `#include "post_effect.h"`
  - `#include "blend_compositor.h"`
  - `#include "simulation/physarum.h"` (and other sim headers)
  - Move all `Setup*` functions (lines 150-372)
  - Move `GetTransformEffect()` (lines 96-118)
- Update `src/render/render_pipeline.cpp`:
  - Add `#include "shader_setup.h"`
  - Remove setup function implementations
  - Remove `TransformEffectEntry` struct and typedef
- Update `CMakeLists.txt`:
  - Add `src/render/shader_setup.cpp` to sources

**Done when**: Build succeeds, all visual effects render correctly.

---

## Phase 3: Clean Up render_pipeline

**Goal**: Verify final state and update documentation.

**Build**:
- Verify `render_pipeline.cpp` contains only:
  - `BlitTexture()` helper
  - `RenderPass()` helper
  - `ApplyPhysarumPass()`, `ApplyCurlFlowPass()`, `ApplyAttractorFlowPass()`
  - `UpdateFFTTexture()`
  - Public API: `RenderPipelineApplyFeedback`, `RenderPipelineDrawablesFull`, `RenderPipelineExecute`, `RenderPipelineApplyOutput`
- Run `/sync-architecture` to update module documentation
- Verify line count reduction (~270 lines remaining)

**Done when**: Build succeeds, architecture docs updated, render_pipeline.cpp under 300 lines.
