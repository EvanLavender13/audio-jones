#ifndef MOD_SOURCES_H
#define MOD_SOURCES_H

#include "analysis/bands.h"
#include "analysis/beat.h"
#include "config/lfo_config.h"
#include "imgui.h"

typedef enum {
    MOD_SOURCE_BASS = 0,
    MOD_SOURCE_MID,
    MOD_SOURCE_TREB,
    MOD_SOURCE_BEAT,
    MOD_SOURCE_LFO1,  // 4
    MOD_SOURCE_LFO2,  // 5
    MOD_SOURCE_LFO3,  // 6
    MOD_SOURCE_LFO4,  // 7
    MOD_SOURCE_LFO5,  // 8
    MOD_SOURCE_LFO6,  // 9
    MOD_SOURCE_LFO7,  // 10
    MOD_SOURCE_LFO8,  // 11
    MOD_SOURCE_CENTROID,  // 12 (moved from 8)
    MOD_SOURCE_COUNT
} ModSource;

typedef struct ModSources {
    float values[MOD_SOURCE_COUNT];
} ModSources;

void ModSourcesInit(ModSources* sources);
void ModSourcesUpdate(ModSources* sources, const BandEnergies* bands,
                      const BeatDetector* beat, const float lfoOutputs[NUM_LFOS]);
const char* ModSourceGetName(ModSource source);
ImU32 ModSourceGetColor(ModSource source);

#endif // MOD_SOURCES_H
