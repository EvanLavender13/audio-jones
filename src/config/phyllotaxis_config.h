#ifndef PHYLLOTAXIS_CONFIG_H
#define PHYLLOTAXIS_CONFIG_H

#include <stdbool.h>

typedef struct PhyllotaxisConfig {
    bool enabled = false;
    float scale = 0.06f;           // Seed spacing (0.02-0.15), smaller = more seeds
    float angleSpeed = 0.0f;       // Divergence angle drift (radians/frame)
    float phaseSpeed = 0.0f;       // Per-cell pulse animation (radians/frame)
    float cellRadius = 0.8f;       // Effect region size per cell (0.1-1.5)
    float isoFrequency = 5.0f;     // Ring density for center iso (1.0-20.0)

    // Sub-effect intensities (0.0-1.0)
    float uvDistortIntensity = 0.0f;      // UV displacement toward cell center
    float flatFillIntensity = 0.0f;       // Stained glass cell fill
    float centerIsoIntensity = 0.0f;      // Concentric rings from seed centers
    float edgeGlowIntensity = 0.0f;       // Brightness at cell edges
} PhyllotaxisConfig;

#endif // PHYLLOTAXIS_CONFIG_H
