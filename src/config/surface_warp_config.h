// src/config/surface_warp_config.h
#ifndef SURFACE_WARP_CONFIG_H
#define SURFACE_WARP_CONFIG_H

struct SurfaceWarpConfig {
    bool enabled = false;
    float intensity = 0.5f;      // Range: 0.0-2.0, hill steepness
    float angle = 0.0f;          // Range: -PI to PI, base warp direction
    float rotationSpeed = 0.0f;  // Range: -PI to PI, direction rotation rate (rad/s)
    float scrollSpeed = 0.5f;    // Range: -2.0 to 2.0, wave drift speed
    float depthShade = 0.3f;     // Range: 0.0-1.0, valley darkening amount
};

#endif // SURFACE_WARP_CONFIG_H
