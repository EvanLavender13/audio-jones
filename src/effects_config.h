#ifndef EFFECTS_CONFIG_H
#define EFFECTS_CONFIG_H

struct EffectsConfig {
    float halfLife = 0.5f;           // Trail persistence (seconds)
    int baseBlurScale = 1;           // Base blur sampling distance (pixels)
    int beatBlurScale = 2;           // Additional blur on beats (pixels)
    float beatSensitivity = 1.0f;    // Beat detection threshold multiplier
    int chromaticMaxOffset = 12;     // Max RGB channel offset on beats (pixels, 0 = disabled)
};

#endif // EFFECTS_CONFIG_H
