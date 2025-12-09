#ifndef WAVEFORM_H
#define WAVEFORM_H

#include "raylib.h"
#include "color_config.h"
#include "render_context.h"
#include <stdint.h>

#define WAVEFORM_SAMPLES 1024
#define WAVEFORM_EXTENDED (WAVEFORM_SAMPLES * 2)
#define INTERPOLATION_MULT 10
#define MAX_WAVEFORMS 8

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
    int thickness = 2;             // Line thickness in pixels
    float smoothness = 5.0f;       // Smoothing window radius (0 = none, higher = smoother)
    float radius = 0.25f;          // Base radius as fraction of min(width, height)
    float rotationSpeed = 0.0f;    // Radians per update tick (can be negative)
    float rotationOffset = 0.0f;   // Base rotation offset in radians (for staggered starts)
    ColorConfig color;             // Color configuration (solid or rainbow)
};

// Process raw audio into normalized waveform (no smoothing yet)
// audioBuffer: interleaved stereo samples (L0, R0, L1, R1, ...)
// framesRead: number of stereo frames (not individual samples)
void ProcessWaveformBase(const float* audioBuffer, uint32_t framesRead, float* waveform, ChannelMode mode);

// Apply per-waveform smoothing and create palindrome for circular display
void ProcessWaveformSmooth(const float* waveform, float* waveformExtended, float smoothness);

// Draw waveform in linear oscilloscope style
// globalTick: shared update counter for synchronized horizontal shift
void DrawWaveformLinear(const float* samples, int count, RenderContext* ctx, WaveformConfig* cfg, uint64_t globalTick);

// Draw waveform in circular format
// globalTick: shared update counter for synchronized rotation
void DrawWaveformCircular(float* samples, int count, RenderContext* ctx, WaveformConfig* cfg, uint64_t globalTick);

#endif // WAVEFORM_H
