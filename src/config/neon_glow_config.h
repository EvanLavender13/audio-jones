#ifndef NEON_GLOW_CONFIG_H
#define NEON_GLOW_CONFIG_H

// NeonGlow: Sobel edge detection with colored glow and additive blending
// Creates cyberpunk/Tron wireframe aesthetics
struct NeonGlowConfig {
    bool enabled = false;
    float glowR = 0.0f;                // Glow color red (0.0-1.0)
    float glowG = 1.0f;                // Glow color green (0.0-1.0)
    float glowB = 1.0f;                // Glow color blue (0.0-1.0)
    float edgeThreshold = 0.1f;        // Noise suppression (0.0-0.5)
    float edgePower = 1.0f;            // Edge intensity curve (0.5-3.0)
    float glowIntensity = 2.0f;        // Brightness multiplier (0.5-5.0)
    float glowRadius = 2.0f;           // Blur spread in pixels (0.0-10.0)
    int glowSamples = 5;               // Cross-tap quality, odd (3-9)
    float originalVisibility = 0.0f;   // Original image blend (0.0-1.0)
    int colorMode = 0;                  // 0 = Custom color, 1 = Source-derived
    float saturationBoost = 0.5f;       // Extra saturation for source mode (0.0-1.0)
    float brightnessBoost = 0.5f;       // Extra brightness for source mode (0.0-1.0)
};

#endif // NEON_GLOW_CONFIG_H
