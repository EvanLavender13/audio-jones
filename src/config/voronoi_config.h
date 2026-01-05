#ifndef VORONOI_CONFIG_H
#define VORONOI_CONFIG_H

#include <stdbool.h>

typedef enum VoronoiMode {
    VORONOI_GLASS_BLOCKS = 0,
    VORONOI_ORGANIC_FLOW = 1,
    VORONOI_EDGE_WARP = 2
} VoronoiMode;

typedef struct VoronoiConfig {
    bool enabled = false;
    VoronoiMode mode = VORONOI_GLASS_BLOCKS;
    float scale = 15.0f;       // Cell count across screen (5-50)
    float strength = 0.1f;     // UV displacement magnitude (0.0-0.5)
    float speed = 0.5f;        // Animation rate (0.1-2.0)
    float edgeFalloff = 0.3f;  // Distortion gradient sharpness (0.1-1.0)
} VoronoiConfig;

#endif // VORONOI_CONFIG_H
