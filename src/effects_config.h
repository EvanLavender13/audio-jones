#ifndef EFFECTS_CONFIG_H
#define EFFECTS_CONFIG_H

struct EffectsConfig {
    float halfLife = 0.5f;        // Trail persistence (seconds)
    int baseBlurScale = 1;        // Base blur sampling distance (pixels)
    int beatBlurScale = 2;        // Additional blur scale on beats (pixels)
    float beatSensitivity = 1.0f; // Beat detection threshold
};

#endif // EFFECTS_CONFIG_H
