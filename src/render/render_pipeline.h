#ifndef RENDER_PIPELINE_H
#define RENDER_PIPELINE_H

#include <stdint.h>
#include <stdbool.h>
#include "render_context.h"

// Profiler constants
#define PROFILER_HISTORY_SIZE 64

// Pipeline zones for CPU timing instrumentation
typedef enum ProfileZoneId {
    ZONE_FEEDBACK = 0,
    ZONE_PHYSARUM,
    ZONE_CURL_FLOW,
    ZONE_ATTRACTOR,
    ZONE_DRAWABLES,
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
void ProfilerUninit(Profiler* profiler);
void ProfilerFrameBegin(Profiler* profiler);
void ProfilerFrameEnd(Profiler* profiler);
void ProfilerBeginZone(Profiler* profiler, ProfileZoneId zone);
void ProfilerEndZone(Profiler* profiler, ProfileZoneId zone);

typedef struct PostEffect PostEffect;
typedef struct DrawableState DrawableState;
typedef struct Drawable Drawable;

// Renders all drawables at configured opacity
void RenderPipelineDrawablesFull(PostEffect* pe, DrawableState* state,
                                 Drawable* drawables, int count,
                                 RenderContext* renderCtx);

// Full render frame: feedback → drawables → output
void RenderPipelineExecute(PostEffect* pe, DrawableState* state,
                           Drawable* drawables, int count,
                           RenderContext* renderCtx, float deltaTime,
                           const float* fftMagnitude, Profiler* profiler);

// Apply feedback stage effects (voronoi, feedback, blur) and simulation updates
// Updates accumTexture with processed frame
void RenderPipelineApplyFeedback(PostEffect* pe, float deltaTime, const float* fftMagnitude,
                                 Profiler* profiler);

// Apply output stage effects and draw to screen
// Applies trail boost, kaleidoscope, chromatic, FXAA, gamma
void RenderPipelineApplyOutput(PostEffect* pe, uint64_t globalTick);

#endif // RENDER_PIPELINE_H
