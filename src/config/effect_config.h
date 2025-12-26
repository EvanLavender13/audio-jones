#ifndef EFFECT_CONFIG_H
#define EFFECT_CONFIG_H

#include "lfo_config.h"
#include "render/physarum.h"

struct EffectConfig {
    bool circular = false;           // Circular waveform mode (false = linear)
    float halfLife = 0.5f;           // Trail persistence (seconds)
    int baseBlurScale = 1;           // Base blur sampling distance (pixels)
    int beatBlurScale = 2;           // Additional blur on beats (pixels)
    int chromaticMaxOffset = 12;     // Max RGB channel offset on beats (pixels, 0 = disabled)
    float feedbackZoom = 0.98f;      // Zoom per frame (0.9-1.0, lower = faster inward)
    float feedbackRotation = 0.005f; // Rotation per frame (radians)
    float feedbackDesaturate = 0.05f;// Fade toward dark gray per frame (0.0-0.2)
    float warpStrength = 0.0f;       // Domain warp intensity (0 = disabled)
    float warpScale = 5.0f;          // Noise frequency (lower = larger swirls)
    int warpOctaves = 3;             // Detail layers (1-5)
    float warpLacunarity = 2.0f;     // Frequency multiplier per octave
    float warpSpeed = 0.5f;          // Animation rate
    int kaleidoSegments = 1;         // Mirror segments (1 = disabled, 4/6/8/12 common)
    float gamma = 1.0f;              // Display gamma correction (1.0 = disabled)

    // Voronoi effect
    float voronoiScale = 15.0f;      // Cell count across screen (5-50)
    float voronoiIntensity = 0.0f;   // Blend amount (0 = disabled)
    float voronoiSpeed = 0.5f;       // Animation rate
    float voronoiEdgeWidth = 0.05f;  // Edge thickness

    // LFO automation
    LFOConfig rotationLFO;

    // Physarum simulation
    PhysarumConfig physarum;
};

#endif // EFFECT_CONFIG_H
