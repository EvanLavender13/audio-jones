#ifndef MANDELBOX_CONFIG_H
#define MANDELBOX_CONFIG_H

// Mandelbox: Box fold + sphere fold fractal transform
// Each iteration: box fold -> sphere fold -> scale -> translate -> twist
struct MandelboxConfig {
    bool enabled = false;
    int iterations = 4;            // Fold/scale/translate cycles (1-6)
    float boxLimit = 1.0f;         // Box fold boundary (0.5-2.0)
    float sphereMin = 0.25f;       // Inner sphere radius for strong inversion (0.1-0.5)
    float sphereMax = 1.0f;        // Outer sphere radius (0.5-2.0)
    float scale = 2.0f;            // Expansion factor per iteration (1.5-2.5)
    float offsetX = 1.0f;          // X translation after fold (0.0-2.0)
    float offsetY = 1.0f;          // Y translation after fold (0.0-2.0)
    float rotationSpeed = 0.002f;  // Animation rotation rate (radians/frame)
    float twistSpeed = 0.0f;       // Per-iteration rotation rate (radians/frame)
    float boxIntensity = 1.0f;     // Box fold contribution (0.0-1.0)
    float sphereIntensity = 1.0f;  // Sphere fold contribution (0.0-1.0)
};

#endif // MANDELBOX_CONFIG_H
