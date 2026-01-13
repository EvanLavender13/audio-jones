#ifndef DUOTONE_CONFIG_H
#define DUOTONE_CONFIG_H

// Duotone: Maps luminance to two-color gradient
// Extracts luminance via BT.601 weights, interpolates between shadow and highlight colors
struct DuotoneConfig {
    bool enabled = false;
    float shadowR = 0.1f;      // Dark region color (purple)
    float shadowG = 0.0f;
    float shadowB = 0.2f;
    float highlightR = 1.0f;   // Bright region color (warm yellow)
    float highlightG = 0.9f;
    float highlightB = 0.6f;
    float intensity = 0.0f;    // Blend: 0 = original, 1 = full duotone
};

#endif // DUOTONE_CONFIG_H
