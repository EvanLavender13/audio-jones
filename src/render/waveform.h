#ifndef WAVEFORM_H
#define WAVEFORM_H

#include "raylib.h"
#include "render_context.h"
#include "audio/audio_config.h"
#include "config/waveform_config.h"
#include <stdint.h>

#define WAVEFORM_SAMPLES 1024
#define WAVEFORM_EXTENDED (WAVEFORM_SAMPLES * 2)
#define INTERPOLATION_MULT 1
#define MAX_WAVEFORMS 8

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
