#include "mod_sources.h"
#include "ui/theme.h"
#include <math.h>

void ModSourcesInit(ModSources* sources)
{
    for (int i = 0; i < MOD_SOURCE_COUNT; i++) {
        sources->values[i] = 0.0f;
    }
}

void ModSourcesUpdate(ModSources* sources, const BandEnergies* bands,
                      const BeatDetector* beat, const float lfoOutputs[4])
{
    // Normalize by running average (self-calibrating)
    // Output 0-1 where 1.0 = 2x the running average
    const float MIN_AVG = 1e-6f;

    float norm = bands->bassSmooth / fmaxf(bands->bassAvg, MIN_AVG);
    sources->values[MOD_SOURCE_BASS] = fminf(norm / 2.0f, 1.0f);

    norm = bands->midSmooth / fmaxf(bands->midAvg, MIN_AVG);
    sources->values[MOD_SOURCE_MID] = fminf(norm / 2.0f, 1.0f);

    norm = bands->trebSmooth / fmaxf(bands->trebAvg, MIN_AVG);
    sources->values[MOD_SOURCE_TREB] = fminf(norm / 2.0f, 1.0f);

    // Beat intensity (already 0-1)
    sources->values[MOD_SOURCE_BEAT] = beat->beatIntensity;

    // LFOs: remap from -1..1 to 0..1
    for (int i = 0; i < 4; i++) {
        sources->values[MOD_SOURCE_LFO1 + i] = (lfoOutputs[i] + 1.0f) * 0.5f;
    }
}

const char* ModSourceGetName(ModSource source)
{
    switch (source) {
        case MOD_SOURCE_BASS: return "Bass";
        case MOD_SOURCE_MID:  return "Mid";
        case MOD_SOURCE_TREB: return "Treb";
        case MOD_SOURCE_BEAT: return "Beat";
        case MOD_SOURCE_LFO1: return "LFO1";
        case MOD_SOURCE_LFO2: return "LFO2";
        case MOD_SOURCE_LFO3: return "LFO3";
        case MOD_SOURCE_LFO4: return "LFO4";
        default: return "???";
    }
}

ImU32 ModSourceGetColor(ModSource source)
{
    switch (source) {
        case MOD_SOURCE_BASS: return Theme::BAND_CYAN_U32;
        case MOD_SOURCE_MID:  return Theme::BAND_WHITE_U32;
        case MOD_SOURCE_TREB: return Theme::BAND_MAGENTA_U32;
        case MOD_SOURCE_BEAT: return Theme::ACCENT_ORANGE_U32;
        case MOD_SOURCE_LFO1:
        case MOD_SOURCE_LFO2:
        case MOD_SOURCE_LFO3:
        case MOD_SOURCE_LFO4: {
            // Interpolate cyan -> magenta by LFO index
            const int idx = source - MOD_SOURCE_LFO1;
            const float t = idx / 3.0f;
            const int r = (int)(0 + t * 255);
            const int g = (int)(230 - t * 210);
            const int b = (int)(242 - t * 95);
            return IM_COL32(r, g, b, 255);
        }
        default: return Theme::TEXT_SECONDARY_U32;
    }
}
