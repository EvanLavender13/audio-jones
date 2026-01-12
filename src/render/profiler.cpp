#include "profiler.h"
#include "raylib.h"
#include <string.h>

static const char* ZONE_NAMES[ZONE_COUNT] = {
    "Feedback",
    "Simulation",
    "Drawables",
    "Output"
};

void ProfilerInit(Profiler* profiler)
{
    if (profiler == NULL) {
        return;
    }
    memset(profiler, 0, sizeof(Profiler));
    for (int i = 0; i < ZONE_COUNT; i++) {
        profiler->zones[i].name = ZONE_NAMES[i];
    }

    // Allocate double-buffered GPU timestamp queries
    glGenQueries(ZONE_COUNT * 2, &profiler->queries[0][0]);
    profiler->writeIdx = 0;

    // Run dummy queries to initialize all query objects (prevents first-frame read errors)
    for (int i = 0; i < ZONE_COUNT * 2; i++) {
        glBeginQuery(GL_TIME_ELAPSED, profiler->queries[i / 2][i % 2]);
        glEndQuery(GL_TIME_ELAPSED);
    }

    profiler->enabled = true;
}

void ProfilerUninit(Profiler* profiler)
{
    if (profiler == NULL) {
        return;
    }
    glDeleteQueries(ZONE_COUNT * 2, &profiler->queries[0][0]);
}

void ProfilerFrameBegin(Profiler* profiler)
{
    if (profiler == NULL || !profiler->enabled) {
        return;
    }
    profiler->frameStartTime = GetTime();
}

void ProfilerFrameEnd(Profiler* profiler)
{
    if (profiler == NULL || !profiler->enabled) {
        return;
    }
    for (int i = 0; i < ZONE_COUNT; i++) {
        profiler->zones[i].historyIndex = (profiler->zones[i].historyIndex + 1) % PROFILER_HISTORY_SIZE;
    }
}

void ProfilerBeginZone(Profiler* profiler, ProfileZoneId zone)
{
    if (profiler == NULL || !profiler->enabled) {
        return;
    }
    // GPU timing replaces CPU timing - actual query calls added in Phase 2
    (void)zone;
}

void ProfilerEndZone(Profiler* profiler, ProfileZoneId zone)
{
    if (profiler == NULL || !profiler->enabled) {
        return;
    }
    // GPU timing replaces CPU timing - actual query calls added in Phase 2
    (void)zone;
}
