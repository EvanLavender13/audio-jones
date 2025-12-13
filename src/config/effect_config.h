#ifndef EFFECT_CONFIG_H
#define EFFECT_CONFIG_H

struct EffectConfig {
    float halfLife = 0.5f;           // Trail persistence (seconds)
    int baseBlurScale = 1;           // Base blur sampling distance (pixels)
    int beatBlurScale = 2;           // Additional blur on beats (pixels)
    int chromaticMaxOffset = 12;     // Max RGB channel offset on beats (pixels, 0 = disabled)
    float feedbackZoom = 0.98f;      // Zoom per frame (0.9-1.0, lower = faster inward)
    float feedbackRotation = 0.005f; // Rotation per frame (radians)
    float feedbackDesaturate = 0.05f;// Fade toward dark gray per frame (0.0-0.2)
};

#endif // EFFECT_CONFIG_H
