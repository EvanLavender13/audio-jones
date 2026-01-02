#ifndef RENDER_PIPELINE_H
#define RENDER_PIPELINE_H

#include <stdint.h>
#include <stdbool.h>
#include "render_context.h"

typedef struct PostEffect PostEffect;
typedef struct DrawableState DrawableState;
typedef struct Drawable Drawable;

// Drawable routing - draws at full opacity (for simulation trail maps)
void RenderPipelineDrawablesFull(DrawableState* state, Drawable* drawables, int count,
                                 RenderContext* renderCtx);

// Drawable routing - draws with per-drawable feedbackPhase opacity
// isPreFeedback: true = draw at (1-feedbackPhase), false = draw at feedbackPhase
void RenderPipelineDrawablesWithPhase(DrawableState* state, Drawable* drawables, int count,
                                      RenderContext* renderCtx, bool isPreFeedback);

// Routes drawables to physarum trail map for agent sensing
void RenderPipelineDrawablesToPhysarum(PostEffect* pe, DrawableState* state,
                                       Drawable* drawables, int count,
                                       RenderContext* renderCtx);

// Routes drawables to curl flow trail map for agent sensing
void RenderPipelineDrawablesToCurlFlow(PostEffect* pe, DrawableState* state,
                                       Drawable* drawables, int count,
                                       RenderContext* renderCtx);

// Routes drawables to attractor flow trail map for visual layering
void RenderPipelineDrawablesToAttractor(PostEffect* pe, DrawableState* state,
                                        Drawable* drawables, int count,
                                        RenderContext* renderCtx);

// Renders drawables before feedback shader at opacity (1 - feedbackPhase)
void RenderPipelineDrawablesPreFeedback(PostEffect* pe, DrawableState* state,
                                        Drawable* drawables, int count,
                                        RenderContext* renderCtx);

// Renders drawables after feedback shader at opacity feedbackPhase
void RenderPipelineDrawablesPostFeedback(PostEffect* pe, DrawableState* state,
                                         Drawable* drawables, int count,
                                         RenderContext* renderCtx);

// Full render frame: pre-feedback → feedback → simulations → post-feedback → composite
void RenderPipelineExecute(PostEffect* pe, DrawableState* state,
                           Drawable* drawables, int count,
                           RenderContext* renderCtx, float deltaTime,
                           const float* fftMagnitude);

// Apply feedback stage effects (voronoi, feedback, blur)
// Updates accumTexture with processed frame
void RenderPipelineApplyFeedback(PostEffect* pe, float deltaTime, const float* fftMagnitude);

// Apply output stage effects and draw to screen
// Applies trail boost, kaleidoscope, chromatic, FXAA, gamma
void RenderPipelineApplyOutput(PostEffect* pe, uint64_t globalTick);

#endif // RENDER_PIPELINE_H
