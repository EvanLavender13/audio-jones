#ifndef MULTI_INVERSION_CONFIG_H
#define MULTI_INVERSION_CONFIG_H

// Multi-Inversion Blend
// Chains circle inversions with depth-weighted accumulation
// Animated centers follow phase-offset Lissajous paths
struct MultiInversionConfig {
    bool enabled = false;
    int iterations = 6;             // Inversion depth (1-12)
    float radius = 0.5f;            // Base inversion radius (0.1-1.0)
    float radiusScale = 0.9f;       // Per-iteration radius multiplier (0.5-1.5)
    float focalAmplitude = 0.15f;   // Lissajous center offset magnitude (0.0-0.3)
    float focalFreqX = 1.0f;        // Lissajous X frequency (0.1-5.0)
    float focalFreqY = 1.3f;        // Lissajous Y frequency (0.1-5.0)
    float phaseOffset = 0.5f;       // Per-iteration phase offset along Lissajous (0.0-2.0)
    float animSpeed = 0.3f;         // Animation speed multiplier (0.0-2.0)
};

#endif // MULTI_INVERSION_CONFIG_H
