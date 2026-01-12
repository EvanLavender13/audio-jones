#ifndef DUOTONE_CONFIG_H
#define DUOTONE_CONFIG_H

// Duotone: Maps luminance to two-color gradient
// Extracts luminance via BT.601 weights, interpolates between shadow and highlight colors
struct DuotoneConfig {
    bool enabled = false;
    float shadowColor[3] = { 0.1f, 0.0f, 0.2f };     // Dark region color (purple)
    float highlightColor[3] = { 1.0f, 0.9f, 0.6f };  // Bright region color (warm yellow)
    float intensity = 0.0f;                           // Blend: 0 = original, 1 = full duotone
};

#endif // DUOTONE_CONFIG_H
