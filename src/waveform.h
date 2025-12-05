#ifndef WAVEFORM_H
#define WAVEFORM_H

#include "raylib.h"
#include <stdint.h>

#define WAVEFORM_SAMPLES 1024
#define WAVEFORM_EXTENDED (WAVEFORM_SAMPLES * 2)
#define INTERPOLATION_MULT 10

// Process raw audio into display-ready waveform
// Normalizes to peak amplitude, creates mirrored buffer for seamless circular display
void ProcessWaveform(float* audioBuffer, uint32_t framesRead,
                     float* waveform, float* waveformExtended);

// Convert HSV to RGB (h: 0-1, s: 0-1, v: 0-1)
Color HsvToRgb(float h, float s, float v);

// Draw waveform in linear oscilloscope style
void DrawWaveformLinear(float* samples, int count, int width, int centerY, int amplitude, Color color);

// Draw waveform in circular format with rainbow color sweep
void DrawWaveformCircularRainbow(float* samples, int count, int centerX, int centerY,
                                  float baseRadius, float amplitude, float rotation, float hueOffset);

#endif // WAVEFORM_H
