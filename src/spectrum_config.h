#ifndef SPECTRUM_CONFIG_H
#define SPECTRUM_CONFIG_H

#include "color_config.h"

#define SPECTRUM_BAND_COUNT 32

struct SpectrumConfig {
    bool enabled = false;
    float innerRadius = 0.15f;    // Circular: base radius (fraction of minDim)
    float barHeight = 0.25f;      // Max bar height (fraction of minDim)
    float barWidth = 0.8f;        // Bar width (0.5-1.0, fraction of arc/slot)
    float smoothing = 0.8f;       // Per-band smoothing (0.0-0.95, higher = slower decay)
    float minDb = 10.0f;          // Floor threshold (dB) - raw FFT magnitudes
    float maxDb = 50.0f;          // Ceiling threshold (dB) - raw FFT magnitudes
    float rotationSpeed = 0.0f;   // Radians per update tick
    float rotationOffset = 0.0f;  // Base rotation offset in radians
    ColorConfig color;            // Shared color config
};

#endif // SPECTRUM_CONFIG_H
