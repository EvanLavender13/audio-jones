#ifndef EFFECTS_CONFIG_H
#define EFFECTS_CONFIG_H

typedef struct EffectsConfig {
    float halfLife;        // Trail persistence (seconds)
    int baseBlurScale;     // Base blur sampling distance (pixels)
    int beatBlurScale;     // Additional blur scale on beats (pixels)
    float beatSensitivity; // Beat detection threshold
} EffectsConfig;

#define EFFECTS_CONFIG_DEFAULT { 0.5f, 1, 2, 1.0f }

#endif // EFFECTS_CONFIG_H
