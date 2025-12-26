#ifndef MOD_SOURCES_H
#define MOD_SOURCES_H

#include "analysis/bands.h"
#include "analysis/beat.h"
#include "imgui.h"

typedef enum {
    MOD_SOURCE_BASS = 0,
    MOD_SOURCE_MID,
    MOD_SOURCE_TREB,
    MOD_SOURCE_BEAT,
    MOD_SOURCE_LFO1,
    MOD_SOURCE_LFO2,
    MOD_SOURCE_LFO3,
    MOD_SOURCE_LFO4,
    MOD_SOURCE_COUNT
} ModSource;

typedef struct ModSources {
    float values[MOD_SOURCE_COUNT];
} ModSources;

void ModSourcesInit(ModSources* sources);
void ModSourcesUpdate(ModSources* sources, const BandEnergies* bands,
                      const BeatDetector* beat, const float lfoOutputs[4]);
const char* ModSourceGetName(ModSource source);
ImU32 ModSourceGetColor(ModSource source);

#endif // MOD_SOURCES_H
