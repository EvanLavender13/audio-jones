#ifndef POINCARE_DISK_CONFIG_H
#define POINCARE_DISK_CONFIG_H

// Poincare Disk: hyperbolic tiling via Mobius translation and fundamental domain folding
// Creates Escher-like repeating patterns within a circular boundary
struct PoincareDiskConfig {
    bool enabled = false;
    int tileP = 4;                   // Angle at origin vertex (pi/P), range 2-12
    int tileQ = 4;                   // Angle at second vertex (pi/Q), range 2-12
    int tileR = 4;                   // Angle at third vertex (pi/R), range 2-12
    float translationX = 0.0f;       // Mobius translation center X (-0.9 to 0.9)
    float translationY = 0.0f;       // Mobius translation center Y (-0.9 to 0.9)
    float translationSpeed = 0.0f;   // Circular motion angular velocity (radians/second)
    float translationAmplitude = 0.0f; // Circular motion radius (0.0-0.9)
    float diskScale = 1.0f;          // Disk size relative to screen (0.5-2.0)
    float rotationSpeed = 0.0f;      // Euclidean rotation speed (radians/frame)
};

#endif // POINCARE_DISK_CONFIG_H
