#ifndef WAVEFORM_CONFIG_H
#define WAVEFORM_CONFIG_H

#include "render/color_config.h"

// Per-waveform configuration
struct WaveformConfig {
    float x = 0.5f;                // Center X position (0.0 = left, 1.0 = right)
    float y = 0.5f;                // Center Y position (0.0 = top, 1.0 = bottom)
    float amplitudeScale = 0.35f;  // Height relative to min(width, height)
    int thickness = 2;             // Line thickness in pixels
    float smoothness = 5.0f;       // Smoothing window radius (0 = none, higher = smoother)
    float radius = 0.25f;          // Base radius as fraction of min(width, height)
    float rotationSpeed = 0.0f;    // Radians per update tick (can be negative)
    float rotationOffset = 0.0f;   // Base rotation offset in radians (for staggered starts)
    float feedbackPhase = 1.0f;    // 0.0 = integrated into feedback, 1.0 = crisp on top
    ColorConfig color;             // Color configuration (solid or rainbow)
};

#endif // WAVEFORM_CONFIG_H
