#ifndef CROSS_HATCHING_CONFIG_H
#define CROSS_HATCHING_CONFIG_H

// Cross-Hatching: NPR effect simulating hand-drawn shading via procedural diagonal strokes
// Maps luminance to 4 density layers with Sobel edge outlines
struct CrossHatchingConfig {
    bool enabled = false;
    float density = 10.0f;      // Pixel spacing between hatch lines (4.0-50.0)
    float width = 1.0f;         // Line thickness in pixels (0.5-4.0)
    float threshold = 1.0f;     // Global luminance sensitivity multiplier (0.0-2.0)
    float jitter = 0.0f;        // Sketchy hand-tremor displacement (0.0-1.0)
    float outline = 0.5f;       // Sobel edge outline strength (0.0-1.0)
    float blend = 1.0f;         // Mix: original color (0) -> ink (1) (0.0-1.0)
};

#endif // CROSS_HATCHING_CONFIG_H
