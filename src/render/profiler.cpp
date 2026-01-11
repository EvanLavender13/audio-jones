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
    profiler->enabled = true;
}

void ProfilerUninit(Profiler* profiler)
{
    (void)profiler;
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
    profiler->zones[zone].startTime = GetTime();
}

void ProfilerEndZone(Profiler* profiler, ProfileZoneId zone)
{
    if (profiler == NULL || !profiler->enabled) {
        return;
    }
    ProfileZone* z = &profiler->zones[zone];
    const double delta = GetTime() - z->startTime;
    z->lastMs = (float)(delta * 1000.0);
    z->history[z->historyIndex] = z->lastMs;
}
