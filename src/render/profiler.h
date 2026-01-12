#ifndef PROFILER_H
#define PROFILER_H

#include <stdbool.h>
#include "external/glad.h"

// Profiler constants
#define PROFILER_HISTORY_SIZE 64
#define PROFILER_SMOOTHING 0.05f  // EMA factor: 0.05 = 5% new value per frame (slower, calmer UI)

// Pipeline zones for GPU timing instrumentation
typedef enum ProfileZoneId {
    ZONE_FEEDBACK = 0,
    ZONE_SIMULATION,
    ZONE_DRAWABLES,
    ZONE_OUTPUT,
    ZONE_COUNT
} ProfileZoneId;

// Per-zone timing state with rolling history
typedef struct ProfileZone {
    const char* name;
    float lastMs;
    float smoothedMs;  // EMA-smoothed value for stable UI display
    float history[PROFILER_HISTORY_SIZE];
    int historyIndex;
} ProfileZone;

// GPU profiler state with double-buffered timestamp queries
typedef struct Profiler {
    ProfileZone zones[ZONE_COUNT];
    GLuint queries[ZONE_COUNT][2];  // Double-buffered query IDs per zone
    int writeIdx;                    // Current write buffer (0 or 1)
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

#endif // PROFILER_H
