#ifndef VORONOI_CONFIG_H
#define VORONOI_CONFIG_H

#include <stdbool.h>

typedef struct VoronoiConfig {
    bool enabled = false;
    float intensity = 0.5f;   // Blend amount (0.1-1.0, enabled handles "off")
    float scale = 15.0f;      // Cell count across screen (5-50)
    float speed = 0.5f;       // Animation rate (0.1-2.0)
    float edgeWidth = 0.05f;  // Edge thickness (0.01-0.1)
} VoronoiConfig;

#endif // VORONOI_CONFIG_H
