# Post-Effect Pipeline Clarity Refactor

Restructure post-effect rendering pipeline to clarify texture flow and phase boundaries.

## Goal

Make the post-effect pipeline's two-phase architecture explicit: accumulation (feedback loop) vs output-only effects. Eliminate confusing texture pointer juggling and clarify when textures are used for ping-pong vs final output. The refactor prioritizes code readability and future extensibility while maintaining the existing public API surface (except removing unused globalTick parameter).

## Current State

### Pipeline Structure (`src/render/post_effect.cpp`)

The pipeline uses 3 textures with unclear responsibilities:

- `accumTexture` (HDR RGBA32F): Main feedback buffer, persists between frames
- `tempTexture` (HDR RGBA32F): Ping-pong buffer for multi-pass effects
- `outputTexture` (standard): Only used for kaleidoscope (lines 332-341), name implies broader use

### Phase Mixing (`src/render/post_effect.cpp:279-356`)

Two conceptually distinct phases are interleaved:

1. **Accumulation phase** (lines 279-301): Effects that feed back into next frame
   - Physarum simulation (line 294)
   - Voronoi distortion (line 295)
   - Feedback zoom/rotation (line 296)
   - Blur with decay (line 297)
   - Ends with accumTexture ready for new waveforms

2. **Output phase** (lines 310-356): Effects applied only to final frame
   - Trail boost from physarum (lines 315-326)
   - Kaleidoscope mirroring (lines 329-342)
   - Chromatic aberration (lines 344-355)

### Texture Pointer Confusion (`src/render/post_effect.cpp:310-356`)

The `sourceTexture` pointer in `PostEffectToScreen()` is reassigned 3 times:

- Line 312: `sourceTexture = &pe->accumTexture;` (initial)
- Line 325: `sourceTexture = &pe->tempTexture;` (after trail boost)
- Line 341: `sourceTexture = &pe->outputTexture;` (after kaleidoscope)

This creates mental overhead tracking which texture holds current state. The triple-use of tempTexture (ping-pong during accum, trail boost target, chromatic source) is non-obvious.

### Unused Parameter (`src/render/post_effect.h:72`)

`globalTick` parameter in `PostEffectEndAccum()` is explicitly ignored (line 306). Only used by `PostEffectToScreen()` for kaleidoscope rotation.

## Algorithm

### Texture Rename

Rename `outputTexture` to `kaleidoTexture` to reflect actual usage:

```cpp
RenderTexture2D kaleidoTexture;  // Dedicated kaleidoscope output (non-feedback)
```

### Phase Function Extraction

Replace inline implementation with dedicated phase functions:

```cpp
// Apply feedback effects that persist between frames
// Sequence: physarum -> voronoi -> feedback -> blur -> copy to accumTexture
static void ApplyAccumulationPipeline(PostEffect* pe, float deltaTime,
                                       float rotation, int blurScale);

// Apply output-only effects (not fed back into accumulation)
// Sequence: trail boost -> kaleidoscope -> chromatic -> screen
static void ApplyOutputPipeline(PostEffect* pe, uint64_t globalTick);
```

### Decision Table: Which Pipeline?

| Effect | Pipeline | Reason |
|--------|----------|--------|
| Physarum | Accumulation | Agents sense previous frame trails |
| Voronoi | Accumulation | Overlay persists in feedback |
| Feedback | Accumulation | Core recursive effect (zoom/rotate) |
| Blur | Accumulation | Trails decay through feedback |
| Trail Boost | Output | Brightens for display only, no feedback pollution |
| Kaleidoscope | Output | Non-feedback (moved per commit 180c8a6) |
| Chromatic | Output | Beat-reactive aberration, screen-only |

## Architecture Decision

**Chosen Approach: Minimal Rename + Phase Function Extraction**

Split accumulation and output phases into dedicated functions with clear texture flow documentation, rename `outputTexture` to `kaleidoTexture`, and contain pointer tracking logic within `ApplyOutputPipeline()`.

### Alternatives Considered

| Approach | Pros | Cons | Decision |
|----------|------|------|----------|
| **Pragmatic (Chosen)** | Clear phases, honest naming, small scope | Still uses pointer tracking | SELECTED |
| Explicit TextureState enum | Self-documenting, bug-proof | Verbose, 2x implementation time | REJECTED |
| Keep as-is + comments | No changes | Doesn't fix extensibility | REJECTED |

### Benefits

- Implementation time: 1-2 hours (small scope)
- Immediate clarity gains without architectural overhaul
- Maintains raylib/C-style direct texture access patterns
- Easy to add new effects (clear insertion points)
- No performance overhead

## Component Design

### Renamed Texture (`src/render/post_effect.h:14`)

```cpp
typedef struct PostEffect {
    RenderTexture2D accumTexture;     // Feedback buffer (persists between frames)
    RenderTexture2D tempTexture;      // Ping-pong buffer for multi-pass effects
    RenderTexture2D kaleidoTexture;   // Dedicated kaleidoscope output (was outputTexture)
    // ... shaders unchanged
} PostEffect;
```

### ApplyAccumulationPipeline (`src/render/post_effect.cpp`)

```cpp
// Apply feedback effects that persist between frames
// Texture flow: accumTexture <-> tempTexture (ping-pong)
// Result: accumTexture contains processed frame, left in render mode
static void ApplyAccumulationPipeline(PostEffect* pe, float deltaTime,
                                       float rotation, int blurScale)
{
    ApplyPhysarumPass(pe, deltaTime);
    ApplyVoronoiPass(pe);
    ApplyFeedbackPass(pe, rotation);
    ApplyBlurPass(pe, blurScale, deltaTime);

    // Copy final result back to accumTexture for waveform rendering
    BeginTextureMode(pe->accumTexture);
        DrawFullscreenQuad(pe, &pe->tempTexture);
    // Leave accumTexture open for caller (PostEffectBeginAccum)
}
```

### ApplyOutputPipeline (`src/render/post_effect.cpp`)

```cpp
// Apply output-only effects (not fed back into accumulation)
// Texture flow: accumTexture -> tempTexture -> kaleidoTexture -> screen
static void ApplyOutputPipeline(PostEffect* pe, uint64_t globalTick)
{
    RenderTexture2D* currentSource = &pe->accumTexture;

    // Trail boost (optional, writes to tempTexture)
    if (pe->physarum != NULL && pe->effects.physarum.boostIntensity > 0.0f) {
        BeginTextureMode(pe->tempTexture);
        BeginShaderMode(pe->trailBoostShader);
            SetShaderValueTexture(pe->trailBoostShader, pe->trailMapLoc,
                                  pe->physarum->trailMap.texture);
            SetShaderValue(pe->trailBoostShader, pe->trailBoostIntensityLoc,
                           &pe->effects.physarum.boostIntensity, SHADER_UNIFORM_FLOAT);
            DrawFullscreenQuad(pe, currentSource);
        EndShaderMode();
        EndTextureMode();
        currentSource = &pe->tempTexture;
    }

    // Kaleidoscope (optional, writes to kaleidoTexture)
    if (pe->effects.kaleidoSegments > 1) {
        const float rotation = 0.002f * (float)globalTick;

        BeginTextureMode(pe->kaleidoTexture);
        BeginShaderMode(pe->kaleidoShader);
            SetShaderValue(pe->kaleidoShader, pe->kaleidoSegmentsLoc,
                           &pe->effects.kaleidoSegments, SHADER_UNIFORM_INT);
            SetShaderValue(pe->kaleidoShader, pe->kaleidoRotationLoc,
                           &rotation, SHADER_UNIFORM_FLOAT);
            DrawFullscreenQuad(pe, currentSource);
        EndShaderMode();
        EndTextureMode();
        currentSource = &pe->kaleidoTexture;
    }

    // Chromatic aberration (final pass, renders to screen)
    const float chromaticOffset = pe->currentBeatIntensity * pe->effects.chromaticMaxOffset;

    if (pe->effects.chromaticMaxOffset == 0 || chromaticOffset < 0.01f) {
        DrawFullscreenQuad(pe, currentSource);
        return;
    }

    BeginShaderMode(pe->chromaticShader);
        SetShaderValue(pe->chromaticShader, pe->chromaticOffsetLoc,
                       &chromaticOffset, SHADER_UNIFORM_FLOAT);
        DrawFullscreenQuad(pe, currentSource);
    EndShaderMode();
}
```

### Updated Public API (`src/render/post_effect.h`)

```cpp
// Begin rendering to accumulation texture
// deltaTime: frame time in seconds for framerate-independent fade
// beatIntensity: 0.0-1.0 beat intensity for bloom pulse effect
void PostEffectBeginAccum(PostEffect* pe, float deltaTime, float beatIntensity);

// End rendering to accumulation texture
void PostEffectEndAccum(PostEffect* pe);  // CHANGED: Removed unused globalTick

// Draw accumulated texture to screen with output-only effects
// globalTick: shared counter for kaleidoscope rotation
void PostEffectToScreen(PostEffect* pe, uint64_t globalTick);
```

## Integration

### Main Loop Update (`src/main.cpp:175-182`)

```cpp
// Before:
PostEffectEndAccum(ctx->postEffect, ctx->waveformPipeline.globalTick);

// After:
PostEffectEndAccum(ctx->postEffect);
```

### Texture Rename Locations

Find/replace `outputTexture` -> `kaleidoTexture` in `src/render/post_effect.cpp`:

- Line 14 (struct field in header)
- Line 222 (InitRenderTexture)
- Line 224 (NULL check)
- Line 247 (UnloadRenderTexture in Uninit)
- Line 269 (UnloadRenderTexture in Resize)
- Line 272 (InitRenderTexture in Resize)
- Lines 332, 341 (ToScreen usage)

## File Changes

| File | Change |
|------|--------|
| `src/render/post_effect.h` | Rename `outputTexture` -> `kaleidoTexture`, remove `globalTick` from `PostEffectEndAccum()` |
| `src/render/post_effect.cpp` | Extract `ApplyAccumulationPipeline()` and `ApplyOutputPipeline()`, rename texture references, update public function implementations |
| `src/main.cpp` | Remove `globalTick` argument from `PostEffectEndAccum()` call (line 178) |

## Build Sequence

### Phase 1: Texture Rename and API Cleanup (Single Session)

1. Rename `outputTexture` to `kaleidoTexture` in `src/render/post_effect.h:14`
2. Update all references in `src/render/post_effect.cpp`:
   - `InitRenderTexture(&pe->kaleidoTexture, ...)` (line 222)
   - NULL check (line 224)
   - `UnloadRenderTexture(pe->kaleidoTexture)` (lines 247, 269)
   - `InitRenderTexture(&pe->kaleidoTexture, ...)` (line 272)
   - `BeginTextureMode(pe->kaleidoTexture)` (line 332)
   - `sourceTexture = &pe->kaleidoTexture` (line 341)
3. Remove `globalTick` parameter from `PostEffectEndAccum()`:
   - Header signature (line 72)
   - Implementation (line 304-308, remove `(void)globalTick;`)
4. Update call site in `src/main.cpp:178`
5. Build and verify no regressions

### Phase 2: Extract Phase Functions (Single Session)

1. Create `ApplyAccumulationPipeline()` static function before `PostEffectBeginAccum()`:
   - Move physarum/voronoi/feedback/blur calls from lines 294-297
   - Move final blit from lines 299-300
   - Add texture flow comment at top
2. Create `ApplyOutputPipeline()` static function before `PostEffectToScreen()`:
   - Move trail boost block from lines 315-326
   - Move kaleidoscope block from lines 329-342
   - Move chromatic block from lines 344-355
   - Add texture flow comment at top
3. Refactor `PostEffectBeginAccum()` to:
   - Compute blurScale and effectiveRotation (keep existing logic)
   - Call `ApplyAccumulationPipeline(pe, deltaTime, effectiveRotation, blurScale)`
4. Refactor `PostEffectToScreen()` to:
   - Call `ApplyOutputPipeline(pe, globalTick)`
5. Add struct field comment documenting 3-texture purpose
6. Build and test all effects

## Validation

- [ ] Application builds without warnings
- [ ] All existing presets load and render identically
- [ ] Kaleidoscope effect functions (segments > 1)
- [ ] Chromatic aberration reacts to beats
- [ ] Physarum trail boost applies only to output (not fed back)
- [ ] Voronoi distortion feeds back into next frame
- [ ] Feedback zoom/rotation creates tunnel effect
- [ ] No visual regressions at 1080p/60fps
- [ ] `main.cpp` compiles with updated `PostEffectEndAccum()` signature

## Future Extensions

With the phase separation in place, adding new effects becomes straightforward:

**Accumulation effects** (add to `ApplyAccumulationPipeline()`):
- Edge detection, color grading, distortion that should persist in feedback

**Output effects** (add to `ApplyOutputPipeline()`):
- Vignette, film grain, CRT scanlines that should NOT feed back

Each new effect follows the pattern:
1. Add shader and uniform locations to PostEffect struct
2. Create `ApplyNewEffectPass()` helper function
3. Add call at appropriate point in phase function
