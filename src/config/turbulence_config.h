#ifndef TURBULENCE_CONFIG_H
#define TURBULENCE_CONFIG_H

// Turbulence Cascade with depth accumulation
// Stacks sine-based coordinate shifts with rotation to create organic swirl patterns
struct TurbulenceConfig {
    bool enabled = false;
    int octaves = 4;                 // Number of cascade octaves (1-8)
    float strength = 0.5f;           // Distortion intensity (0.0-2.0)
    float animSpeed = 0.3f;          // Animation speed multiplier (0.0-2.0)
    float rotationPerOctave = 0.5f;  // Rotation per octave in radians (±π)
};

#endif // TURBULENCE_CONFIG_H
