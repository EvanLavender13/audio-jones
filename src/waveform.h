#ifndef WAVEFORM_H
#define WAVEFORM_H

#include "raylib.h"
#include <stdint.h>

#define WAVEFORM_SAMPLES 1024
#define WAVEFORM_EXTENDED (WAVEFORM_SAMPLES * 2)
#define INTERPOLATION_MULT 1
#define MAX_WAVEFORMS 8

#ifdef __cplusplus
extern "C" {
#endif

// Per-waveform configuration
typedef struct {
    float amplitudeScale;  // Height relative to min(width, height)
    float thickness;       // Line thickness in pixels
    float smoothness;      // Smoothing window radius (0 = none, higher = smoother)
    float radius;          // Base radius as fraction of min(width, height)
    float rotationSpeed;   // Radians per update tick (can be negative)
    float rotation;        // Current rotation angle in radians
    Color color;           // Waveform color
} WaveformConfig;

// Rendering context (screen geometry)
typedef struct {
    int screenW, screenH;
    int centerX, centerY;
    float minDim;          // min(screenW, screenH) for scaling
} RenderContext;

// Initialize a waveform config with default values
WaveformConfig WaveformConfigDefault(void);

// Process raw audio into normalized waveform (no smoothing yet)
void ProcessWaveformBase(const float* audioBuffer, uint32_t framesRead, float* waveform);

// Apply per-waveform smoothing and create palindrome for circular display
void ProcessWaveformSmooth(const float* waveform, float* waveformExtended, float smoothness);

// Draw waveform in linear oscilloscope style
void DrawWaveformLinear(const float* samples, int count, RenderContext* ctx, WaveformConfig* cfg);

// Draw waveform in circular format
void DrawWaveformCircular(float* samples, int count, RenderContext* ctx, WaveformConfig* cfg);

#ifdef __cplusplus
}
#endif

#endif // WAVEFORM_H
