#include "waveform.h"
#include <math.h>

WaveformConfig WaveformConfigDefault(void)
{
    WaveformConfig config = {
        .amplitudeScale = 0.35f,
        .thickness = 2.0f,
        .smoothness = 5.0f,
        .radius = 0.25f,
        .rotationSpeed = 0.0f,
        .rotation = 0.0f,
        .color = WHITE,
        .colorMode = COLOR_MODE_SOLID,
        .rainbowHue = 0.0f,
        .rainbowRange = 360.0f,
        .rainbowSat = 1.0f,
        .rainbowVal = 1.0f
    };
    return config;
}

// Compute color for a segment at position t (0-1) along the waveform
// Uses ping-pong interpolation for seamless circular wrapping
static Color GetSegmentColor(WaveformConfig* cfg, float t)
{
    if (cfg->colorMode == COLOR_MODE_RAINBOW) {
        // Ping-pong: 0->1->0 maps to start->end->start for seamless loop
        float pingPong = 1.0f - fabsf(2.0f * t - 1.0f);
        float hue = cfg->rainbowHue + pingPong * cfg->rainbowRange;
        hue = fmodf(hue, 360.0f);
        if (hue < 0.0f) {
            hue += 360.0f;
        }
        return ColorFromHSV(hue, cfg->rainbowSat, cfg->rainbowVal);
    }
    return cfg->color;
}

// Sliding window moving average - O(N) complexity
static void SmoothWaveform(float* waveform, int count, int smoothness)
{
    if (smoothness <= 0 || count <= 0) {
        return;
    }

    // Temporary buffer for smoothed values
    static float smoothed[WAVEFORM_EXTENDED];

    // Initialize window sum for first element
    float windowSum = 0.0f;
    int windowCount = 0;
    for (int j = -smoothness; j <= smoothness; j++) {
        if (j >= 0 && j < count) {
            windowSum += waveform[j];
            windowCount++;
        }
    }
    smoothed[0] = windowSum / windowCount;

    // Slide window across data
    for (int i = 1; i < count; i++) {
        // Remove element leaving window
        int removeIdx = i - smoothness - 1;
        if (removeIdx >= 0) {
            windowSum -= waveform[removeIdx];
            windowCount--;
        }

        // Add element entering window
        int addIdx = i + smoothness;
        if (addIdx < count) {
            windowSum += waveform[addIdx];
            windowCount++;
        }

        smoothed[i] = windowSum / windowCount;
    }

    // Copy back to original buffer
    for (int i = 0; i < count; i++) {
        waveform[i] = smoothed[i];
    }
}

void ProcessWaveformBase(const float* audioBuffer, uint32_t framesRead, float* waveform)
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
        if (absVal > maxAbs) {
            maxAbs = absVal;
        }
    }
    if (maxAbs > 0.0f) {
        for (int i = 0; i < copyCount; i++) {
            waveform[i] /= maxAbs;
        }
    }
}

void ProcessWaveformSmooth(const float* waveform, float* waveformExtended, float smoothness)
{
    // Copy base waveform to extended buffer
    for (int i = 0; i < WAVEFORM_SAMPLES; i++) {
        waveformExtended[i] = waveform[i];
    }

    // Create palindrome first
    for (int i = 0; i < WAVEFORM_SAMPLES; i++) {
        waveformExtended[WAVEFORM_SAMPLES + i] = waveformExtended[WAVEFORM_SAMPLES - 1 - i];
    }

    // Smooth the full palindrome so window blends across join points
    SmoothWaveform(waveformExtended, WAVEFORM_EXTENDED, (int)smoothness);
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

void DrawWaveformLinear(const float* samples, int count, RenderContext* ctx, WaveformConfig* cfg)
{
    float xStep = (float)ctx->screenW / count;
    float amplitude = ctx->minDim * cfg->amplitudeScale;
    float jointRadius = cfg->thickness * 0.5f;

    for (int i = 0; i < count - 1; i++) {
        float t = (float)i / (count - 1);
        Color segColor = GetSegmentColor(cfg, t);
        Vector2 start = { i * xStep, ctx->centerY - samples[i] * amplitude };
        Vector2 end = { (i + 1) * xStep, ctx->centerY - samples[i + 1] * amplitude };
        DrawLineEx(start, end, cfg->thickness, segColor);
        DrawCircleV(start, jointRadius, segColor);
    }
    // Final vertex
    float tLast = 1.0f;
    Color lastColor = GetSegmentColor(cfg, tLast);
    Vector2 last = { (count - 1) * xStep, ctx->centerY - samples[count - 1] * amplitude };
    DrawCircleV(last, jointRadius, lastColor);
}

void DrawWaveformCircular(float* samples, int count, RenderContext* ctx, WaveformConfig* cfg)
{
    float baseRadius = ctx->minDim * cfg->radius;
    float amplitude = ctx->minDim * cfg->amplitudeScale;
    int numPoints = count * INTERPOLATION_MULT;
    float angleStep = (2.0f * PI) / numPoints;

    for (int i = 0; i < numPoints; i++) {
        int next = (i + 1) % numPoints;
        float t = (float)i / numPoints;
        Color segColor = GetSegmentColor(cfg, t);

        float angle1 = i * angleStep + cfg->rotation - PI / 2;
        float angle2 = next * angleStep + cfg->rotation - PI / 2;

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
        if (radius1 < 10.0f) {
            radius1 = 10.0f;
        }
        if (radius2 < 10.0f) {
            radius2 = 10.0f;
        }

        Vector2 start = { ctx->centerX + cosf(angle1) * radius1, ctx->centerY + sinf(angle1) * radius1 };
        Vector2 end = { ctx->centerX + cosf(angle2) * radius2, ctx->centerY + sinf(angle2) * radius2 };

        DrawLineEx(start, end, cfg->thickness, segColor);
    }
}
