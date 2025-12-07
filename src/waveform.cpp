#include "waveform.h"
#include <math.h>

// Compute color for a segment at position t (0-1) along the waveform
// When loop=true, uses ping-pong interpolation for seamless circular wrapping
static Color GetSegmentColor(WaveformConfig* cfg, float t, bool loop)
{
    if (cfg->colorMode == COLOR_MODE_RAINBOW) {
        float interp = loop ? (1.0f - fabsf(2.0f * t - 1.0f)) : t;
        float hue = cfg->rainbowHue + interp * cfg->rainbowRange;
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

// Mix interleaved stereo buffer to mono based on channel mode
static void MixStereoToMono(const float* stereo, uint32_t frameCount, float* mono, ChannelMode mode)
{
    for (uint32_t i = 0; i < frameCount; i++) {
        float left = stereo[i * 2];
        float right = stereo[i * 2 + 1];

        switch (mode) {
            case CHANNEL_LEFT:
                mono[i] = left;
                break;
            case CHANNEL_RIGHT:
                mono[i] = right;
                break;
            case CHANNEL_MAX: {
                float absL = fabsf(left);
                float absR = fabsf(right);
                mono[i] = (absL >= absR) ? left : right;
                break;
            }
            case CHANNEL_MIX:
                mono[i] = (left + right) * 0.5f;
                break;
            case CHANNEL_SIDE:
                mono[i] = left - right;
                break;
            case CHANNEL_INTERLEAVED:
                // Handled specially in ProcessWaveformBase
                mono[i] = stereo[i];
                break;
        }
    }
}

void ProcessWaveformBase(const float* audioBuffer, uint32_t framesRead, float* waveform, ChannelMode mode)
{
    int copyCount;

    if (mode == CHANNEL_INTERLEAVED) {
        // Legacy behavior: copy interleaved samples directly (uses 2x samples)
        int sampleCount = (int)framesRead * 2;
        copyCount = (sampleCount > WAVEFORM_SAMPLES) ? WAVEFORM_SAMPLES : sampleCount;
        for (int i = 0; i < copyCount; i++) {
            waveform[i] = audioBuffer[i];
        }
    } else {
        // Mix stereo to mono first
        copyCount = (framesRead > WAVEFORM_SAMPLES) ? WAVEFORM_SAMPLES : (int)framesRead;
        MixStereoToMono(audioBuffer, copyCount, waveform, mode);
    }

    // Zero-pad remainder
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
        Color segColor = GetSegmentColor(cfg, t, false);
        Vector2 start = { i * xStep, ctx->centerY - samples[i] * amplitude };
        Vector2 end = { (i + 1) * xStep, ctx->centerY - samples[i + 1] * amplitude };
        DrawLineEx(start, end, cfg->thickness, segColor);
        DrawCircleV(start, jointRadius, segColor);
    }
    // Final vertex - use same t as last segment to avoid hue wrap at 360
    float tLast = (float)(count - 2) / (count - 1);
    Color lastColor = GetSegmentColor(cfg, tLast, false);
    Vector2 last = { (count - 1) * xStep, ctx->centerY - samples[count - 1] * amplitude };
    DrawCircleV(last, jointRadius, lastColor);
}

void DrawWaveformCircular(float* samples, int count, RenderContext* ctx, WaveformConfig* cfg, uint64_t globalTick)
{
    float baseRadius = ctx->minDim * cfg->radius;
    float amplitude = ctx->minDim * cfg->amplitudeScale;
    int numPoints = count * INTERPOLATION_MULT;
    float angleStep = (2.0f * PI) / numPoints;

    // Calculate effective rotation: offset + (speed * globalTick)
    // Same-speed waveforms stay synchronized regardless of when speed was set
    float effectiveRotation = cfg->rotationOffset + (cfg->rotationSpeed * (float)globalTick);

    for (int i = 0; i < numPoints; i++) {
        int next = (i + 1) % numPoints;
        float t = (float)i / numPoints;
        Color segColor = GetSegmentColor(cfg, t, true);

        float angle1 = i * angleStep + effectiveRotation - PI / 2;
        float angle2 = next * angleStep + effectiveRotation - PI / 2;

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
