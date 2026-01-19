#ifndef PHYLLOTAXIS_CONFIG_H
#define PHYLLOTAXIS_CONFIG_H

#include <stdbool.h>

typedef struct PhyllotaxisConfig {
    bool enabled = false;
    float scale = 0.06f;           // Seed spacing (0.02-0.15), smaller = more seeds
    float angleSpeed = 0.0f;       // Divergence angle drift (radians/frame)
    float phaseSpeed = 0.0f;       // Per-cell pulse animation (radians/frame)
    float cellRadius = 0.8f;       // Effect region size per cell (0.1-1.5)
    float isoFrequency = 5.0f;     // Ring density for iso effects (1.0-20.0)

    // Sub-effect intensities (0.0-1.0)
    float uvDistortIntensity = 0.0f;      // UV displacement toward cell center
    float organicFlowIntensity = 0.0f;    // UV distort with edge mask
    float edgeIsoIntensity = 0.0f;        // Iso rings from cell borders
    float centerIsoIntensity = 0.0f;      // Concentric rings from seed centers
    float flatFillIntensity = 0.0f;       // Stained glass cell fill
    float edgeGlowIntensity = 0.0f;       // Black cells with colored edges
    float ratioIntensity = 0.0f;          // Edge/center distance ratio
    float determinantIntensity = 0.0f;    // 2D cross product shading
    float edgeDetectIntensity = 0.0f;     // Highlight where edge closer than center
} PhyllotaxisConfig;

#endif // PHYLLOTAXIS_CONFIG_H
