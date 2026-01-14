#ifndef CHLADNI_WARP_CONFIG_H
#define CHLADNI_WARP_CONFIG_H

// Chladni Warp: UV displacement using Chladni plate equations
// Gradient of Chladni function displaces toward/along nodal lines
struct ChladniWarpConfig {
    bool enabled = false;
    float n = 4.0f;            // X-axis frequency mode (1.0-12.0)
    float m = 3.0f;            // Y-axis frequency mode (1.0-12.0)
    float plateSize = 1.0f;    // Plate dimension L (0.5-2.0)
    float strength = 0.1f;     // UV displacement magnitude (0.0-0.5)
    int warpMode = 0;          // 0=toward, 1=along, 2=intensity
    float animRate = 0.0f;     // Animation rate (radians/second, 0.0-2.0)
    float animRange = 0.0f;    // Amplitude of n/m oscillation (0.0-5.0)
    bool preFold = false;      // Enable abs(uv) symmetry
};

#endif // CHLADNI_WARP_CONFIG_H
