#include "waveform.h"
#include <math.h>

WaveformConfig WaveformConfigDefault(void)
{
    WaveformConfig config = {
        .amplitudeScale = 0.35f,
        .thickness = 2.0f,
        .hueOffset = 0.0f
    };
    return config;
}

void ProcessWaveform(float* audioBuffer, uint32_t framesRead,
                     float* waveform, float* waveformExtended)
{
    // Copy samples, zero-pad if fewer than expected
    int copyCount = (framesRead > WAVEFORM_SAMPLES) ? WAVEFORM_SAMPLES : (int)framesRead;
    for (int i = 0; i < copyCount; i++) {
        waveform[i] = audioBuffer[i];
    }
    for (int i = copyCount; i < WAVEFORM_SAMPLES; i++) {
        waveform[i] = 0.0f;
    }

    // Normalize: scale so peak amplitude reaches 1.0
    float maxAbs = 0.0f;
    for (int i = 0; i < copyCount; i++) {
        float absVal = fabsf(waveform[i]);
        if (absVal > maxAbs) maxAbs = absVal;
    }
    if (maxAbs > 0.0f) {
        for (int i = 0; i < copyCount; i++) {
            waveform[i] /= maxAbs;
        }
    }

    // Create palindrome: original + mirrored for seamless circular display
    for (int i = 0; i < WAVEFORM_SAMPLES; i++) {
        waveformExtended[i] = waveform[i];
        waveformExtended[WAVEFORM_SAMPLES + i] = waveform[WAVEFORM_SAMPLES - 1 - i];
    }

    // Smooth join points
    float avg = (waveform[WAVEFORM_SAMPLES - 1] + waveform[0]) * 0.5f;
    waveformExtended[WAVEFORM_SAMPLES - 1] = avg;
    waveformExtended[WAVEFORM_SAMPLES] = avg;
}

// Cubic interpolation between four points
static float CubicInterp(float y0, float y1, float y2, float y3, float t)
{
    float a0 = y3 - y2 - y0 + y1;
    float a1 = y0 - y1 - a0;
    float a2 = y2 - y0;
    float a3 = y1;
    return a0 * t * t * t + a1 * t * t + a2 * t + a3;
}

void DrawWaveformLinear(float* samples, int count, int width, int centerY,
                        int amplitude, Color color, float thickness)
{
    float xStep = (float)width / count;

    for (int i = 0; i < count - 1; i++) {
        Vector2 start = { i * xStep, centerY - samples[i] * amplitude };
        Vector2 end = { (i + 1) * xStep, centerY - samples[i + 1] * amplitude };
        DrawLineEx(start, end, thickness, color);
    }
}

void DrawWaveformCircularRainbow(float* samples, int count, int centerX, int centerY,
                                  float baseRadius, float amplitude, float rotation,
                                  float hueOffset, float thickness)
{
    int numPoints = count * INTERPOLATION_MULT;
    float angleStep = (2.0f * PI) / numPoints;

    for (int i = 0; i < numPoints; i++) {
        int next = (i + 1) % numPoints;

        float angle1 = i * angleStep + rotation - PI / 2;
        float angle2 = next * angleStep + rotation - PI / 2;

        // Cubic interpolation for point 1
        int idx1 = (i / INTERPOLATION_MULT) % count;
        float frac1 = (float)(i % INTERPOLATION_MULT) / INTERPOLATION_MULT;
        int i0 = (idx1 - 1 + count) % count;
        int i1 = idx1;
        int i2 = (idx1 + 1) % count;
        int i3 = (idx1 + 2) % count;
        float sample1 = CubicInterp(samples[i0], samples[i1], samples[i2], samples[i3], frac1);

        // Cubic interpolation for point 2
        int idx2 = (next / INTERPOLATION_MULT) % count;
        float frac2 = (float)(next % INTERPOLATION_MULT) / INTERPOLATION_MULT;
        int n0 = (idx2 - 1 + count) % count;
        int n1 = idx2;
        int n2 = (idx2 + 1) % count;
        int n3 = (idx2 + 2) % count;
        float sample2 = CubicInterp(samples[n0], samples[n1], samples[n2], samples[n3], frac2);

        float radius1 = baseRadius + sample1 * (amplitude * 0.5f);
        float radius2 = baseRadius + sample2 * (amplitude * 0.5f);
        if (radius1 < 10.0f) radius1 = 10.0f;
        if (radius2 < 10.0f) radius2 = 10.0f;

        Vector2 start = { centerX + cosf(angle1) * radius1, centerY + sinf(angle1) * radius1 };
        Vector2 end = { centerX + cosf(angle2) * radius2, centerY + sinf(angle2) * radius2 };

        // Rainbow color
        float hue = (float)i / numPoints + hueOffset;
        hue = hue - floorf(hue);
        Color color = ColorFromHSV(hue * 360.0f, 1.0f, 1.0f);

        DrawLineEx(start, end, thickness, color);
    }
}
