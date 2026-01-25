#ifndef PHYLLOTAXIS_WARP_CONFIG_H
#define PHYLLOTAXIS_WARP_CONFIG_H

#include <stdbool.h>

typedef struct PhyllotaxisWarpConfig {
    bool enabled = false;
    float scale = 0.06f;              // Arm spacing (0.02-0.15), smaller = tighter spirals
    float divergenceAngle = 2.39996f; // Golden angle ~2.4 rad, off-values create spokes
    float warpStrength = 0.3f;        // Global displacement multiplier (0.0-1.0)
    float warpFalloff = 5.0f;         // Arm edge sharpness (1.0-20.0), higher = narrower band
    float tangentIntensity = 0.5f;    // Swirl amount perpendicular to radius (0.0-1.0)
    float radialIntensity = 0.2f;     // Breathing amount along radius (0.0-1.0)
    float spinSpeed = 0.0f;           // Rotation rate (radians/second), CPU accumulated
    float crawlSpeed = 0.0f;          // Radial crawl rate (indices/second), CPU accumulated
} PhyllotaxisWarpConfig;

#endif // PHYLLOTAXIS_WARP_CONFIG_H
