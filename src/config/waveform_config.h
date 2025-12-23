#ifndef WAVEFORM_CONFIG_H
#define WAVEFORM_CONFIG_H

#include "render/color_config.h"

// Per-waveform configuration
struct WaveformConfig {
    float amplitudeScale = 0.35f;  // Height relative to min(width, height)
    int thickness = 2;             // Line thickness in pixels
    float smoothness = 5.0f;       // Smoothing window radius (0 = none, higher = smoother)
    float radius = 0.25f;          // Base radius as fraction of min(width, height)
    float verticalOffset = 0.0f;   // Linear mode Y offset as fraction of screen height (-0.5 to 0.5)
    float rotationSpeed = 0.0f;    // Radians per update tick (can be negative)
    float rotationOffset = 0.0f;   // Base rotation offset in radians (for staggered starts)
    ColorConfig color;             // Color configuration (solid or rainbow)
};

#endif // WAVEFORM_CONFIG_H
