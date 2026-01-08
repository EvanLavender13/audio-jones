#ifndef CONFORMAL_WARP_CONFIG_H
#define CONFORMAL_WARP_CONFIG_H

struct ConformalWarpConfig {
    bool enabled = false;
    float powerMapN = 2.0f;       // Power exponent (0.5-8.0, snapped to 0.5 increments)
    float rotationSpeed = 0.0f;   // Pattern rotation (radians/frame)
    float focalAmplitude = 0.0f;  // Lissajous center offset
    float focalFreqX = 1.0f;
    float focalFreqY = 1.5f;
};

#endif // CONFORMAL_WARP_CONFIG_H
