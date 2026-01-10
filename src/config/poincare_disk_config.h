#ifndef POINCARE_DISK_CONFIG_H
#define POINCARE_DISK_CONFIG_H

// Poincare Disk: hyperbolic tiling via Mobius translation and fundamental domain folding
// Creates Escher-like repeating patterns within a circular boundary
struct PoincareDiskConfig {
    bool enabled = false;
    int tileP = 4;                   // Angle at origin vertex (pi/P), range 2-12
    int tileQ = 4;                   // Angle at second vertex (pi/Q), range 2-12
    int tileR = 4;                   // Angle at third vertex (pi/R), range 2-12
    float translationX = 0.0f;       // Mobius translation X in disk (-0.9 to 0.9)
    float translationY = 0.0f;       // Mobius translation Y in disk (-0.9 to 0.9)
    float translationSpeed = 0.0f;   // Lissajous animation speed (radians/second)
    float diskScale = 1.0f;          // Disk size relative to screen (0.5-2.0)
    float rotationSpeed = 0.0f;      // Euclidean rotation speed (radians/frame)
    float translationAmplitude = 0.0f; // Lissajous motion amplitude (0.0-0.9)
    float translationFreqX = 1.0f;   // X oscillation frequency (0.1-5.0)
    float translationFreqY = 1.3f;   // Y oscillation frequency (0.1-5.0)
};

#endif // POINCARE_DISK_CONFIG_H
