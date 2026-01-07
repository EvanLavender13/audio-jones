#ifndef VORONOI_CONFIG_H
#define VORONOI_CONFIG_H

#include <stdbool.h>

typedef struct VoronoiConfig {
    bool enabled = false;
    float scale = 15.0f;       // Cell count across screen (5-50)
    float speed = 0.5f;        // Animation rate (0.1-2.0)
    float edgeFalloff = 0.3f;  // Edge softness for multiple effects (0.1-1.0)
    float isoFrequency = 10.0f; // Ring density for iso effects (1.0-50.0)

    // Effect intensities (0.0-1.0)
    float uvDistortIntensity = 0.0f;      // UV displacement magnitude
    float edgeIsoIntensity = 0.0f;        // Concentric rings from edges
    float centerIsoIntensity = 0.0f;      // Concentric rings from centers
    float flatFillIntensity = 0.0f;       // Stained glass cell fill
    float edgeDarkenIntensity = 0.0f;     // Leadframe darkening at edges
    float angleShadeIntensity = 0.0f;     // Shading by border-center angle
    float determinantIntensity = 0.0f;    // 2D cross product shading
    float ratioIntensity = 0.0f;          // Edge/center distance ratio
    float edgeDetectIntensity = 0.0f;     // Edge detection glow
} VoronoiConfig;

#endif // VORONOI_CONFIG_H
