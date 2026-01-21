#ifndef GRADIENT_FLOW_CONFIG_H
#define GRADIENT_FLOW_CONFIG_H

struct GradientFlowConfig {
    bool enabled = false;
    float strength = 0.01f;    // Displacement per iteration (0.0 to 0.1)
    int iterations = 8;        // Cascade depth (1 to 8)
    float edgeWeight = 1.0f;   // Blend between uniform (0) and edge-scaled (1) displacement
    bool randomDirection = false;  // Randomize tangent direction per pixel for crunchy look
};

#endif // GRADIENT_FLOW_CONFIG_H
