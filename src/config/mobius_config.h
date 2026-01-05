#ifndef MOBIUS_CONFIG_H
#define MOBIUS_CONFIG_H

// Iterated Möbius with depth accumulation
// Applies animated Möbius transforms iteratively and accumulates weighted samples
struct MobiusConfig {
    bool enabled = false;
    int iterations = 6;           // Number of iterative transform steps (1-12)
    float animSpeed = 0.3f;       // Animation speed multiplier (0.0-2.0)
    float poleMagnitude = 0.1f;   // Magnitude of c coefficient, controls distortion strength (0.0-0.5)
    float rotationSpeed = 0.3f;   // Rotation speed of a coefficient (0.0-2.0)
    float uvScale = 0.4f;         // UV remap scale (0.3-0.49), higher shows more of texture
};

#endif // MOBIUS_CONFIG_H
