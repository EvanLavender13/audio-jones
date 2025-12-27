# Refactor Ping-Pong Buffer Management

Replace confusing pointer-comparison conditionals with index-based ping-pong array and callback-based helper function.

## Current State

- `src/render/post_effect.h:12-14` - Three textures with misleading names (`tempTexture`, `kaleidoTexture`)
- `src/render/post_effect.cpp:395-397` - Ternary pointer comparison for `fxaaInput`
- `src/render/post_effect.cpp:420-422` - Ternary pointer comparison for `gammaInput`
- `src/render/post_effect.cpp:72-132` - 11 BeginTextureMode blocks with repetitive boilerplate

---

## Phase 1: Add Helper Function and Buffer Array

**Goal**: Add infrastructure without changing behavior.

**Build**:

Modify `src/render/post_effect.h`:
- Replace `tempTexture` and `kaleidoTexture` with `RenderTexture2D pingPong[2]`
- Keep `accumTexture` as-is (persistent feedback buffer)

Modify `src/render/post_effect.cpp`:

Add after `SetWarpUniforms()`:
```c
typedef void (*ShaderSetupFn)(PostEffect* pe);

static void RenderPass(PostEffect* pe,
                       RenderTexture2D* source,
                       RenderTexture2D* dest,
                       Shader shader,
                       ShaderSetupFn setup)
{
    BeginTextureMode(*dest);
    if (shader.id != 0) {
        BeginShaderMode(shader);
        if (setup != NULL) { setup(pe); }
    }
    DrawFullscreenQuad(pe, source);
    if (shader.id != 0) {
        EndShaderMode();
    }
    EndTextureMode();
}
```

Update lifecycle functions:
- `PostEffectInit()`: Change `tempTexture`/`kaleidoTexture` init to `pingPong[0]`/`pingPong[1]`
- `PostEffectUninit()`: Unload `pingPong[0]` and `pingPong[1]`
- `PostEffectResize()`: Resize `pingPong[0]` and `pingPong[1]`

**Done when**: Build succeeds, application starts, all effects render identically.

---

## Phase 2: Refactor Output Pipeline

**Goal**: Eliminate pointer comparison conditionals with index-based ping-pong.

**Build**:

Rewrite `ApplyOutputPipeline()` (lines 357-442):

```
Entry state:
  source = &accumTexture (read from feedback result)
  writeIdx = 0 (next write goes to pingPong[0])

Trail boost (if enabled):
  Read: source
  Write: pingPong[writeIdx]
  source = &pingPong[writeIdx]
  writeIdx = 1 - writeIdx

Kaleidoscope (if enabled):
  Read: source
  Write: pingPong[writeIdx]
  source = &pingPong[writeIdx]
  writeIdx = 1 - writeIdx

Chromatic (always, shader conditional):
  Read: source
  Write: pingPong[writeIdx]
  source = &pingPong[writeIdx]
  writeIdx = 1 - writeIdx

FXAA + Gamma:
  If gamma enabled:
    FXAA: Read source, Write pingPong[writeIdx]
    source = &pingPong[writeIdx]
    Gamma: Read source, Write to screen
  Else:
    FXAA: Read source, Write to screen
```

Add setup callback functions for each shader:
- `SetupTrailBoost(PostEffect* pe)` - sets trailMap texture, intensity, blend mode
- `SetupKaleido(PostEffect* pe)` - sets segments, rotation (needs globalTick passed differently)
- `SetupChromatic(PostEffect* pe)` - sets offset
- `SetupGamma(PostEffect* pe)` - sets gamma value

Delete:
- Lines 395-397: `fxaaInput` ternary selection
- Lines 420-422: `gammaInput` ternary selection

**Done when**: All output effect combinations render correctly:
- No effects
- Trail boost only
- Kaleidoscope only
- Trail boost + kaleidoscope
- All effects + gamma
- All effects without gamma

---

## Phase 3: Refactor Feedback Stage

**Goal**: Apply helper function to reduce feedback stage boilerplate.

**Build**:

Add setup callback functions:
- `SetupVoronoi(PostEffect* pe)` - sets scale, intensity, time, speed, edgeWidth
- `SetupFeedback(PostEffect* pe)` - sets zoom, rotation, desaturate + calls SetWarpUniforms
- `SetupBlurH(PostEffect* pe)` - sets blurScale
- `SetupBlurV(PostEffect* pe)` - sets blurScale, halfLife, deltaTime

Refactor using RenderPass helper:
- `ApplyVoronoiPass()`: Two RenderPass calls (voronoi shader, then plain copy)
- `ApplyFeedbackPass()`: One RenderPass call
- `ApplyBlurPass()`: Two RenderPass calls (H then V)
- Final copy in `PostEffectApplyFeedbackStage()`: One RenderPass call

Note: Some setup functions need parameters (rotation, blurScale, deltaTime). Either:
- Store temporaries in PostEffect struct before calling
- Or keep inline SetShaderValue for those specific cases

**Done when**: Feedback effects render correctly, code is shorter.

---

## Phase 4: Cleanup

**Goal**: Verify no regressions, remove dead code.

**Build**:

Search and verify:
- No remaining references to `tempTexture` or `kaleidoTexture`
- All `pingPong[0]` and `pingPong[1]` uses are correct

Test matrix:
- Each effect individually enabled
- All effects disabled (passthrough)
- All effects enabled
- Window resize (no crashes, no leaks)
- Physarum enabled/disabled combinations

**Done when**: Full visual test passes, no compiler warnings.
