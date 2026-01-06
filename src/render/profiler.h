#ifndef PROFILER_H
#define PROFILER_H

#include <stdbool.h>

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

#endif // PROFILER_H
