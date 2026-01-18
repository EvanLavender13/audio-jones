#ifndef DOMAIN_WARP_CONFIG_H
#define DOMAIN_WARP_CONFIG_H

struct DomainWarpConfig {
    bool enabled = false;
    float strength = 0.05f;   // Warp magnitude (0.0 to 0.3)
    int octaves = 3;          // Fractal octaves (1 to 6)
    float lacunarity = 2.0f;  // Frequency multiplier per octave (1.5 to 3.0)
    float persistence = 0.5f; // Amplitude decay per octave (0.3 to 0.7)
    float scale = 5.0f;       // Base noise frequency
    float driftSpeed = 0.0f;  // Animation speed (radians/frame)
};

#endif // DOMAIN_WARP_CONFIG_H
