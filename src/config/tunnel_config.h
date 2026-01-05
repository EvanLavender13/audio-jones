#ifndef TUNNEL_CONFIG_H
#define TUNNEL_CONFIG_H

// Tunnel Effect with multi-depth accumulation
// Polar coordinate remapping creates infinite tunnel illusion with Lissajous winding
struct TunnelConfig {
    bool enabled = false;
    float speed = 0.5f;              // Depth motion speed (-2.0 to 2.0)
    float rotationSpeed = 0.0f;      // Angular rotation in radians/sec (-π to π)
    float twist = 0.0f;              // Spiral twist per depth unit in radians (-π to π)
    int layers = 4;                  // Accumulation depth samples (1-8)
    float depthSpacing = 0.1f;       // Distance between depth layers (0.0-0.5)
    float windingAmplitude = 0.0f;   // Lissajous path winding intensity (0.0-0.5)
    float windingFreqX = 1.0f;       // Winding X frequency (0.1-5.0)
    float windingFreqY = 1.3f;       // Winding Y frequency (0.1-5.0)
    float focalAmplitude = 0.0f;     // Center drift amount (0.0-0.2)
    float focalFreqX = 0.7f;         // Focal X frequency (0.1-5.0)
    float focalFreqY = 1.1f;         // Focal Y frequency (0.1-5.0)
    float animSpeed = 1.0f;          // Global time multiplier (0.0-2.0)
};

#endif // TUNNEL_CONFIG_H
