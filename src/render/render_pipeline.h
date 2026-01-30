#ifndef RENDER_PIPELINE_H
#define RENDER_PIPELINE_H

#include "profiler.h"
#include "render_context.h"
#include <stdbool.h>
#include <stdint.h>

typedef struct PostEffect PostEffect;
typedef struct DrawableState DrawableState;
typedef struct Drawable Drawable;

// Renders all drawables at configured opacity
void RenderPipelineDrawablesFull(PostEffect *pe, DrawableState *state,
                                 Drawable *drawables, int count,
                                 RenderContext *renderCtx);

// Full render frame: feedback → drawables → output
void RenderPipelineExecute(PostEffect *pe, DrawableState *state,
                           Drawable *drawables, int count,
                           RenderContext *renderCtx, float deltaTime,
                           const float *fftMagnitude,
                           const float *waveformHistory, int waveformWriteIndex,
                           Profiler *profiler);

// Apply feedback stage effects (voronoi, feedback, blur)
// Updates accumTexture with processed frame
void RenderPipelineApplyFeedback(PostEffect *pe, float deltaTime,
                                 const float *fftMagnitude);

// Apply output stage effects and draw to screen
// Applies trail boost, kaleidoscope, chromatic, FXAA, gamma
void RenderPipelineApplyOutput(PostEffect *pe, uint64_t globalTick);

#endif // RENDER_PIPELINE_H
