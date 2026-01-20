#ifndef CROSS_HATCHING_CONFIG_H
#define CROSS_HATCHING_CONFIG_H

// Cross-Hatching: NPR effect with hand-drawn aesthetic via temporal stutter and varied stroke angles
// Maps luminance to 4 angle-varied layers with Sobel edge outlines
struct CrossHatchingConfig {
    bool enabled = false;
    float width = 1.5f;         // Base line thickness in pixels (0.5-4.0)
    float threshold = 1.0f;     // Global luminance sensitivity multiplier (0.0-2.0)
    float noise = 0.5f;         // Per-pixel irregularity for organic feel (0.0-1.0)
    float outline = 0.5f;       // Sobel edge outline strength (0.0-1.0)
    float blend = 1.0f;         // Mix: original color (0) -> ink (1) (0.0-1.0)
};

#endif // CROSS_HATCHING_CONFIG_H
