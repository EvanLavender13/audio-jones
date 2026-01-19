#ifndef TRIANGLE_FOLD_CONFIG_H
#define TRIANGLE_FOLD_CONFIG_H

// Triangle Fold: Sierpinski-style UV transform creating 3-fold/6-fold gasket patterns
// Each iteration: fold into 60-degree wedge -> scale -> translate -> twist
struct TriangleFoldConfig {
    bool enabled = false;
    int iterations = 3;            // Recursion depth (1-6)
    float scale = 2.0f;            // Expansion per iteration (1.5-2.5)
    float offsetX = 0.5f;          // X translation after fold (0.0-2.0)
    float offsetY = 0.5f;          // Y translation after fold (0.0-2.0)
    float rotationSpeed = 0.0f;    // Animation rotation rate (radians/second)
    float twistSpeed = 0.0f;       // Per-iteration rotation rate (radians/second)
};

#endif // TRIANGLE_FOLD_CONFIG_H
