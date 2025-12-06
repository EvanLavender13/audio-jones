#ifndef WAVEFORM_H
#define WAVEFORM_H

#include "raylib.h"
#include <stdint.h>

#define WAVEFORM_SAMPLES 1024
#define WAVEFORM_EXTENDED (WAVEFORM_SAMPLES * 2)
#define INTERPOLATION_MULT 1
#define MAX_WAVEFORMS 8

typedef enum {
    COLOR_MODE_SOLID,
    COLOR_MODE_RAINBOW
} ColorMode;

typedef enum {
    CHANNEL_LEFT,        // Left channel only
    CHANNEL_RIGHT,       // Right channel only
    CHANNEL_MAX,         // Max magnitude of L/R with sign from larger
    CHANNEL_MIX,         // (L+R)/2 mono downmix
    CHANNEL_SIDE,        // L-R stereo difference
    CHANNEL_INTERLEAVED  // Alternating L/R samples (legacy behavior)
} ChannelMode;

// Per-waveform configuration
struct WaveformConfig {
    float amplitudeScale = 0.35f;  // Height relative to min(width, height)
    float thickness = 2.0f;        // Line thickness in pixels
    float smoothness = 5.0f;       // Smoothing window radius (0 = none, higher = smoother)
    float radius = 0.25f;          // Base radius as fraction of min(width, height)
    float rotationSpeed = 0.0f;    // Radians per update tick (can be negative)
    float rotationOffset = 0.0f;   // Base rotation offset in radians (for staggered starts)
    Color color = WHITE;           // Waveform color (solid mode)

    // Color mode
    ColorMode colorMode = COLOR_MODE_SOLID;
    float rainbowHue = 0.0f;       // Starting hue offset (0-360)
    float rainbowRange = 360.0f;   // Hue degrees to span (0-360)
    float rainbowSat = 1.0f;       // Saturation (0-1)
    float rainbowVal = 1.0f;       // Value/brightness (0-1)
};

// Rendering context (screen geometry)
typedef struct {
    int screenW, screenH;
    int centerX, centerY;
    float minDim;          // min(screenW, screenH) for scaling
} RenderContext;

// Process raw audio into normalized waveform (no smoothing yet)
// audioBuffer: interleaved stereo samples (L0, R0, L1, R1, ...)
// framesRead: number of stereo frames (not individual samples)
void ProcessWaveformBase(const float* audioBuffer, uint32_t framesRead, float* waveform, ChannelMode mode);

// Apply per-waveform smoothing and create palindrome for circular display
void ProcessWaveformSmooth(const float* waveform, float* waveformExtended, float smoothness);

// Draw waveform in linear oscilloscope style
void DrawWaveformLinear(const float* samples, int count, RenderContext* ctx, WaveformConfig* cfg);

// Draw waveform in circular format
// globalTick: shared update counter for synchronized rotation
void DrawWaveformCircular(float* samples, int count, RenderContext* ctx, WaveformConfig* cfg, uint64_t globalTick);

#endif // WAVEFORM_H
