#ifndef RENDER_PIPELINE_H
#define RENDER_PIPELINE_H

#include <stdint.h>
#include <stdbool.h>
#include "render_context.h"

// Profiler constants
#define PROFILER_HISTORY_SIZE 64

// Pipeline zones for CPU timing instrumentation
typedef enum ProfileZoneId {
    ZONE_PRE_FEEDBACK = 0,
    ZONE_FEEDBACK,
    ZONE_PHYSARUM_TRAILS,
    ZONE_CURL_TRAILS,
    ZONE_ATTRACTOR_TRAILS,
    ZONE_POST_FEEDBACK,
    ZONE_OUTPUT,
    ZONE_COUNT
} ProfileZoneId;

// Per-zone timing state with rolling history
typedef struct ProfileZone {
    const char* name;
    double startTime;
    float lastMs;
    float history[PROFILER_HISTORY_SIZE];
    int historyIndex;
} ProfileZone;

// CPU profiler state
typedef struct Profiler {
    ProfileZone zones[ZONE_COUNT];
    double frameStartTime;
    bool enabled;
} Profiler;

// Profiler lifecycle
void ProfilerInit(Profiler* profiler);
void ProfilerFrameBegin(Profiler* profiler);
void ProfilerFrameEnd(Profiler* profiler);
void ProfilerBeginZone(Profiler* profiler, ProfileZoneId zone);
void ProfilerEndZone(Profiler* profiler, ProfileZoneId zone);

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
                           const float* fftMagnitude, Profiler* profiler);

// Apply feedback stage effects (voronoi, feedback, blur)
// Updates accumTexture with processed frame
void RenderPipelineApplyFeedback(PostEffect* pe, float deltaTime, const float* fftMagnitude);

// Apply output stage effects and draw to screen
// Applies trail boost, kaleidoscope, chromatic, FXAA, gamma
void RenderPipelineApplyOutput(PostEffect* pe, uint64_t globalTick);

#endif // RENDER_PIPELINE_H
