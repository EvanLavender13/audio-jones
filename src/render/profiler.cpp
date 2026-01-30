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

    // Read previous frame's completed GPU queries
    const int readIdx = 1 - profiler->writeIdx;
    for (int i = 0; i < ZONE_COUNT; i++) {
        const GLuint query = profiler->queries[i][readIdx];
        GLuint64 elapsed = 0;
        glGetQueryObjectui64v(query, GL_QUERY_RESULT, &elapsed);
        const float ms = elapsed / 1000000.0f;  // ns to ms
        profiler->zones[i].lastMs = ms;
        profiler->zones[i].history[profiler->zones[i].historyIndex] = ms;

        // EMA smoothing for stable UI display
        float* smoothed = &profiler->zones[i].smoothedMs;
        if (*smoothed == 0.0f) {
            *smoothed = ms;  // Initialize on first sample
        } else {
            *smoothed = *smoothed + PROFILER_SMOOTHING * (ms - *smoothed);
        }
    }
}

void ProfilerFrameEnd(Profiler* profiler)
{
    if (profiler == NULL || !profiler->enabled) {
        return;
    }
    // Flip double buffer for next frame
    profiler->writeIdx = 1 - profiler->writeIdx;

    // Advance history ring buffer index
    for (int i = 0; i < ZONE_COUNT; i++) {
        profiler->zones[i].historyIndex = (profiler->zones[i].historyIndex + 1) % PROFILER_HISTORY_SIZE;
    }
}

void ProfilerBeginZone(Profiler* profiler, ProfileZoneId zone)
{
    if (profiler == NULL || !profiler->enabled) {
        return;
    }
    const GLuint query = profiler->queries[zone][profiler->writeIdx];
    glBeginQuery(GL_TIME_ELAPSED, query);
}

void ProfilerEndZone(Profiler* profiler, ProfileZoneId zone)
{
    if (profiler == NULL || !profiler->enabled) {
        return;
    }
    (void)zone;  // GL_TIME_ELAPSED allows only one active query; zone unused
    glEndQuery(GL_TIME_ELAPSED);
}
