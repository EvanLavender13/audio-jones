# Staged Rendering Pipeline

Refactor PostEffect to use explicit 3-stage architecture matching MilkDrop/Experimental patterns. Makes the implicit staging in `BeginAccum/EndAccum` explicit via separate API methods.

## Current State

PostEffect has implicit staging buried in the implementation:

- `src/render/post_effect.cpp:156-167` - `ApplyAccumulationPipeline()` runs physarum → voronoi → feedback → blur, then leaves `accumTexture` open
- `src/render/post_effect.cpp:323-343` - `PostEffectBeginAccum()` calls pipeline, waveforms drawn by caller
- `src/render/post_effect.cpp:345-348` - `PostEffectEndAccum()` just closes texture mode
- `src/render/post_effect.cpp:350-419` - `ApplyOutputPipeline()` runs composite effects
- `src/main.cpp:234-248` - Awkward physarum trailMap injection with EndTextureMode/BeginTextureMode gymnastics

## Phase 1: Refactor PostEffect API

**Goal**: Replace `BeginAccum/EndAccum` with 3 explicit stage methods.

**Build**:

Modify `src/render/post_effect.h`:
- Remove `PostEffectBeginAccum()` and `PostEffectEndAccum()` declarations
- Add new stage methods:
  - `PostEffectApplyFeedbackStage(PostEffect* pe, float deltaTime, float beatIntensity, const float* fftMagnitude)` - Runs physarum → voronoi → feedback → blur, closes texture mode when done
  - `PostEffectBeginDrawStage(PostEffect* pe)` - Opens `accumTexture` for waveform drawing
  - `PostEffectEndDrawStage(void)` - Closes `accumTexture`
- Keep `PostEffectToScreen()` unchanged (already the composite stage)

Modify `src/render/post_effect.cpp`:
- Rename `ApplyAccumulationPipeline()` to `PostEffectApplyFeedbackStage()` (make public)
- Add `EndTextureMode()` at end of feedback stage (currently leaves texture open)
- Remove final `BeginTextureMode(pe->accumTexture)` from feedback stage
- Add `PostEffectBeginDrawStage()`: just `BeginTextureMode(pe->accumTexture)`
- Add `PostEffectEndDrawStage()`: just `EndTextureMode()`
- Remove `PostEffectBeginAccum()` and `PostEffectEndAccum()`

**Done when**: PostEffect exposes 3 explicit stage methods. Build succeeds.

---

## Phase 2: Update Main Loop Integration

**Goal**: Update `main.cpp` to use the new staged API.

**Build**:

Modify `src/main.cpp` (lines 234-248) to replace:
```c
PostEffectBeginAccum(ctx->postEffect, deltaTime, beatIntensity, fftMagnitude);
    if (ctx->postEffect->physarum != NULL) {
        EndTextureMode();
        if (PhysarumBeginTrailMapDraw(ctx->postEffect->physarum)) {
            RenderWaveforms(ctx, &renderCtx);
            PhysarumEndTrailMapDraw(ctx->postEffect->physarum);
        }
        BeginTextureMode(ctx->postEffect->accumTexture);
    }
    RenderWaveforms(ctx, &renderCtx);
PostEffectEndAccum();
```

With staged API:
```c
// STAGE 1: Feedback/Warp (physarum, voronoi, feedback shader, blur)
PostEffectApplyFeedbackStage(ctx->postEffect, deltaTime, beatIntensity,
                              ctx->analysis.fft.magnitude);

// STAGE 2: Draw waveforms to physarum trailMap AND feedback buffer
if (ctx->postEffect->physarum != NULL &&
    PhysarumBeginTrailMapDraw(ctx->postEffect->physarum)) {
    RenderWaveforms(ctx, &renderCtx);
    PhysarumEndTrailMapDraw(ctx->postEffect->physarum);
}
PostEffectBeginDrawStage(ctx->postEffect);
    RenderWaveforms(ctx, &renderCtx);
PostEffectEndDrawStage();

// STAGE 3: Composite (trail boost, kaleidoscope, chromatic, FXAA)
// ... existing BeginDrawing/PostEffectToScreen code unchanged
```

**Done when**: App builds and runs. Visual output identical to before. Staging is now explicit in caller code.
