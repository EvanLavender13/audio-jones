#ifndef ITERATIVE_MIRROR_CONFIG_H
#define ITERATIVE_MIRROR_CONFIG_H

// Iterative Mirror: Repeated rotation + mirror operations for fractal symmetry
struct IterativeMirrorConfig {
    bool enabled = false;
    int iterations = 4;           // Mirror iterations (1-8)
    float rotationSpeed = 0.002f; // Rotation rate (radians/frame)
    float twistAngle = 0.0f;      // Per-iteration rotation offset (radians)
};

#endif // ITERATIVE_MIRROR_CONFIG_H
