#include "waveform.h"
#include <math.h>

// Cubic interpolation between four points
static float CubicInterp(float y0, float y1, float y2, float y3, float t)
{
    float a0 = y3 - y2 - y0 + y1;
    float a1 = y0 - y1 - a0;
    float a2 = y2 - y0;
    float a3 = y1;
    return a0 * t * t * t + a1 * t * t + a2 * t + a3;
}

Color HsvToRgb(float h, float s, float v)
{
    float r, g, b;

    int i = (int)(h * 6.0f);
    float f = h * 6.0f - i;
    float p = v * (1.0f - s);
    float q = v * (1.0f - f * s);
    float t = v * (1.0f - (1.0f - f) * s);

    switch (i % 6) {
        case 0: r = v; g = t; b = p; break;
        case 1: r = q; g = v; b = p; break;
        case 2: r = p; g = v; b = t; break;
        case 3: r = p; g = q; b = v; break;
        case 4: r = t; g = p; b = v; break;
        case 5: r = v; g = p; b = q; break;
        default: r = v; g = t; b = p; break;
    }

    return (Color){
        (unsigned char)(r * 255),
        (unsigned char)(g * 255),
        (unsigned char)(b * 255),
        255
    };
}

void DrawWaveformLinear(float* samples, int count, int width, int centerY, int amplitude, Color color)
{
    float xStep = (float)width / count;
    for (int i = 0; i < count - 1; i++) {
        int x1 = (int)(i * xStep);
        int y1 = centerY - (int)(samples[i] * amplitude);
        int x2 = (int)((i + 1) * xStep);
        int y2 = centerY - (int)(samples[i + 1] * amplitude);
        DrawLine(x1, y1, x2, y2, color);
    }
}

void DrawWaveformCircularRainbow(float* samples, int count, int centerX, int centerY,
                                  float baseRadius, float amplitude, float rotation, float hueOffset)
{
    int numPoints = count * INTERPOLATION_MULT;
    float angleStep = (2.0f * PI) / numPoints;

    for (int i = 0; i < numPoints; i++) {
        int next = (i + 1) % numPoints;

        float angle1 = i * angleStep + rotation - PI / 2;
        float angle2 = next * angleStep + rotation - PI / 2;

        // Cubic interpolation for smoother curves
        int idx = (i / INTERPOLATION_MULT) % count;
        float frac = (float)(i % INTERPOLATION_MULT) / INTERPOLATION_MULT;
        int i0 = (idx - 1 + count) % count;
        int i1 = idx;
        int i2 = (idx + 1) % count;
        int i3 = (idx + 2) % count;
        float sample1 = CubicInterp(samples[i0], samples[i1], samples[i2], samples[i3], frac);

        int nextIdx = (next / INTERPOLATION_MULT) % count;
        float nextFrac = (float)(next % INTERPOLATION_MULT) / INTERPOLATION_MULT;
        int n0 = (nextIdx - 1 + count) % count;
        int n1 = nextIdx;
        int n2 = (nextIdx + 1) % count;
        int n3 = (nextIdx + 2) % count;
        float sample2 = CubicInterp(samples[n0], samples[n1], samples[n2], samples[n3], nextFrac);

        // Amplitude is total range - half above baseRadius, half below
        float radius1 = baseRadius + sample1 * (amplitude * 0.5f);
        float radius2 = baseRadius + sample2 * (amplitude * 0.5f);

        // Clamp to prevent negative radii
        if (radius1 < 10.0f) radius1 = 10.0f;
        if (radius2 < 10.0f) radius2 = 10.0f;

        int x1 = centerX + (int)(cosf(angle1) * radius1);
        int y1 = centerY + (int)(sinf(angle1) * radius1);
        int x2 = centerX + (int)(cosf(angle2) * radius2);
        int y2 = centerY + (int)(sinf(angle2) * radius2);

        // Hue cycles around the circle
        float hue = (float)i / numPoints + hueOffset;
        hue = hue - floorf(hue);
        Color color = HsvToRgb(hue, 1.0f, 1.0f);

        DrawLine(x1, y1, x2, y2, color);
    }
}
