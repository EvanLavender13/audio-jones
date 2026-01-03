#ifndef WAVEFORM_H
#define WAVEFORM_H

#include "raylib.h"
#include "render_context.h"
#include "audio/audio_config.h"
#include "config/drawable_config.h"
#include <stdint.h>

#define WAVEFORM_SAMPLES 1024
#define WAVEFORM_EXTENDED (WAVEFORM_SAMPLES * 2)
#define MAX_WAVEFORMS 8

// Process raw audio into normalized waveform (no smoothing yet)
// audioBuffer: interleaved stereo samples (L0, R0, L1, R1, ...)
// framesRead: number of stereo frames (not individual samples)
void ProcessWaveformBase(const float* audioBuffer, uint32_t framesRead, float* waveform, ChannelMode mode);

// Apply per-waveform smoothing and create palindrome for circular display
void ProcessWaveformSmooth(const float* waveform, float* waveformExtended, float smoothness);

// Draw waveform in linear oscilloscope style
// globalTick: shared update counter for synchronized horizontal shift
// opacity: 0.0-1.0 alpha multiplier for split-pass rendering
void DrawWaveformLinear(const float* samples, int count, RenderContext* ctx, const Drawable* d, uint64_t globalTick, float opacity);

// Draw waveform in circular format
// globalTick: shared update counter for synchronized rotation
// opacity: 0.0-1.0 alpha multiplier for split-pass rendering
void DrawWaveformCircular(const float* samples, int count, RenderContext* ctx, const Drawable* d, uint64_t globalTick, float opacity);

#endif // WAVEFORM_H
