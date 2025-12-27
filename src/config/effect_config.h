#ifndef EFFECT_CONFIG_H
#define EFFECT_CONFIG_H

#include "render/physarum.h"

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

struct EffectConfig {
    bool circular = false;           // Circular waveform mode (false = linear)
    float halfLife = 0.5f;           // Trail persistence (seconds)
    float blurScale = 1.0f;          // Blur sampling distance (pixels)
    float chromaticOffset = 0.0f;    // RGB channel offset (pixels, 0 = disabled)
    float kaleidoRotationSpeed = 0.002f; // Kaleidoscope rotation rate (radians/tick)
    float feedbackDesaturate = 0.05f;// Fade toward dark gray per frame (0.0-0.2)
    FlowFieldConfig flowField;       // Spatial UV flow field parameters
    int kaleidoSegments = 1;         // Mirror segments (1 = disabled, 4/6/8/12 common)
    float gamma = 1.0f;              // Display gamma correction (1.0 = disabled)

    // Voronoi effect
    float voronoiScale = 15.0f;      // Cell count across screen (5-50)
    float voronoiIntensity = 0.0f;   // Blend amount (0 = disabled)
    float voronoiSpeed = 0.5f;       // Animation rate
    float voronoiEdgeWidth = 0.05f;  // Edge thickness

    // Physarum simulation
    PhysarumConfig physarum;
};

#endif // EFFECT_CONFIG_H
