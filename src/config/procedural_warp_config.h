#ifndef PROCEDURAL_WARP_CONFIG_H
#define PROCEDURAL_WARP_CONFIG_H

struct ProceduralWarpConfig {
    float warp = 0.0f;       // 0-2, amplitude (0 = disabled)
    float warpSpeed = 1.0f;  // 0.1-2.0, time multiplier
    float warpScale = 1.0f;  // 0.1-100, spatial frequency (lower = larger waves)
};

#endif // PROCEDURAL_WARP_CONFIG_H
