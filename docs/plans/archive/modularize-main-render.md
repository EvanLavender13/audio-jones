# Modularize: main.cpp Render Functions

Move 8 `Render*` functions from `main.cpp` to `render_pipeline.cpp` to consolidate rendering orchestration.

## Current State

- `src/main.cpp` - 320 lines, 12 functions
- `src/render/render_pipeline.cpp` - 376 lines, 18 functions

**Problem:** main.cpp accumulates `RenderDrawablesTo*` functions each time a new simulation is added. These belong with the render pipeline.

## Functions to Move

| Function | Line | Purpose |
|----------|------|---------|
| `RenderDrawablesFull` | 123 | Draw all drawables at full opacity |
| `RenderDrawablesWithPhase` | 131 | Draw with feedbackPhase blending |
| `RenderDrawablesToPhysarum` | 138 | Route drawables to physarum trail map |
| `RenderDrawablesToCurlFlow` | 151 | Route drawables to curl flow trail map |
| `RenderDrawablesToAttractor` | 164 | Route drawables to attractor flow trail map |
| `RenderDrawablesPreFeedback` | 178 | Draw before feedback shader |
| `RenderDrawablesPostFeedback` | 187 | Draw after feedback shader |
| `RenderStandardPipeline` | 195 | Orchestrate full render frame |

## Signature Changes

Current signatures take `AppContext*`:
```cpp
static void RenderDrawablesToPhysarum(AppContext* ctx, RenderContext* renderCtx)
```

New signatures take explicit params to avoid `AppContext` dependency in render module:
```cpp
void RenderPipelineDrawablesToPhysarum(PostEffect* pe, DrawableState* state,
                                       Drawable* drawables, int count,
                                       RenderContext* renderCtx);
```

## Implementation Phases

### Phase 1: Add New Functions to render_pipeline

Add to `src/render/render_pipeline.h`:
```cpp
// Drawable routing
void RenderPipelineDrawablesFull(DrawableState* state, Drawable* drawables, int count);
void RenderPipelineDrawablesWithPhase(PostEffect* pe, DrawableState* state,
                                      Drawable* drawables, int count);
void RenderPipelineDrawablesToPhysarum(PostEffect* pe, DrawableState* state,
                                       Drawable* drawables, int count,
                                       RenderContext* renderCtx);
void RenderPipelineDrawablesToCurlFlow(PostEffect* pe, DrawableState* state,
                                       Drawable* drawables, int count,
                                       RenderContext* renderCtx);
void RenderPipelineDrawablesToAttractor(PostEffect* pe, DrawableState* state,
                                        Drawable* drawables, int count,
                                        RenderContext* renderCtx);
void RenderPipelineDrawablesPreFeedback(PostEffect* pe, DrawableState* state,
                                        Drawable* drawables, int count);
void RenderPipelineDrawablesPostFeedback(PostEffect* pe, DrawableState* state,
                                         Drawable* drawables, int count);

// Frame execution
void RenderPipelineExecute(PostEffect* pe, DrawableState* state,
                           Drawable* drawables, int count,
                           RenderContext* renderCtx);
```

Add implementations to `src/render/render_pipeline.cpp`:
- Copy function bodies from main.cpp
- Replace `ctx->` accessors with explicit params
- Add required includes: `drawable.h`, simulation headers

### Phase 2: Update main.cpp

1. Remove the 8 static `Render*` functions
2. Add `#include "render/render_pipeline.h"` if not present
3. Replace `RenderStandardPipeline(ctx, &renderCtx)` call in main loop with:
   ```cpp
   RenderPipelineExecute(&ctx->postEffect, &ctx->drawableState,
                         ctx->drawables, ctx->drawableCount, &renderCtx);
   ```

### Phase 3: Build and Verify

1. Run build: `cmake.exe --build build`
2. Verify no compile errors
3. Run application to confirm visual output unchanged

## After Extraction

**main.cpp (~215 lines):**
- `AppContext` struct definition
- `AppContextInit` / `AppContextUninit`
- `UpdateVisuals`
- `main()` with simplified render call

**render_pipeline.cpp (~475 lines):**
- All shader setup functions (existing)
- All simulation pass functions (existing)
- Drawable routing functions (new)
- `RenderPipelineExecute` as single entry point (new)

## Notes

- `UpdateVisuals` stays in main.cpp - it processes audio data at 20Hz, separate from per-frame rendering
- `AppContext` struct stays in main.cpp - render_pipeline receives decomposed params, not the full context
- Run `/sync-architecture` after completion to update docs
