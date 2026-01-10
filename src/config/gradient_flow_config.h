#ifndef GRADIENT_FLOW_CONFIG_H
#define GRADIENT_FLOW_CONFIG_H

struct GradientFlowConfig {
    bool enabled = false;
    float strength = 0.01f;    // Displacement per iteration (0.0 to 0.1)
    int iterations = 8;        // Cascade depth (1 to 32)
    float flowAngle = 0.0f;    // 0 = tangent (along edges), PI/2 = gradient (across edges)
    float edgeWeight = 1.0f;   // Blend between uniform (0) and edge-scaled (1) displacement
};

#endif // GRADIENT_FLOW_CONFIG_H
