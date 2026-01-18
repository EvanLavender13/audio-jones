#ifndef DOMAIN_WARP_CONFIG_H
#define DOMAIN_WARP_CONFIG_H

struct DomainWarpConfig {
    bool enabled = false;
    float warpStrength = 0.1f;   // 0.0 to 0.5
    float warpScale = 4.0f;      // 1.0 to 10.0
    int warpIterations = 2;      // 1 to 3
    float falloff = 0.5f;        // 0.3 to 0.8
    float driftSpeed = 0.0f;     // radians/frame, CPU accumulated
    float driftAngle = 0.0f;     // direction of drift (radians)
};

#endif // DOMAIN_WARP_CONFIG_H
