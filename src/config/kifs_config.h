#ifndef KIFS_CONFIG_H
#define KIFS_CONFIG_H

// Kaleidoscopic IFS: Fractal folding via iterated mirror/rotate/scale
struct KifsConfig {
    bool enabled = false;
    int iterations = 4;           // Folding iterations (1-8)
    int segments = 6;             // Mirror segments per iteration (1-12)
    float scale = 2.0f;           // Per-iteration scale factor (0.5-4.0)
    float offsetX = 1.0f;         // X translation after fold (UV units)
    float offsetY = 1.0f;         // Y translation after fold (UV units)
    float rotationSpeed = 0.002f; // Rotation rate (radians/frame)
    float twistAngle = 0.0f;      // Per-iteration rotation offset (radians)
};

#endif // KIFS_CONFIG_H
