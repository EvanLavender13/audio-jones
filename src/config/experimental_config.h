#ifndef EXPERIMENTAL_CONFIG_H
#define EXPERIMENTAL_CONFIG_H

struct FlowFieldConfig {
    float zoomBase = 0.995f;
    float zoomRadial = 0.0f;
    float rotBase = 0.0f;
    float rotRadial = 0.0f;
    float dxBase = 0.0f;
    float dxRadial = 0.0f;
    float dyBase = 0.0f;
    float dyRadial = 0.0f;
};

struct CompositeConfig {
    float gamma = 1.0f;
};

struct ExperimentalConfig {
    float halfLife = 0.5f;           // Trail persistence in seconds
    FlowFieldConfig flowField;       // Spatial UV flow field parameters
    float injectionOpacity = 0.3f;   // Waveform blend strength (0.05 min, 1 = full)
    CompositeConfig composite;       // Display-only post-processing
};

#endif // EXPERIMENTAL_CONFIG_H
