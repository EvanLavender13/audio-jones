#include "waveform.h"
#include <math.h>

// Compute color for a segment at position t (0-1) along the waveform
// When loop=true, uses ping-pong interpolation for seamless circular wrapping
static Color GetSegmentColor(WaveformConfig* cfg, float t, bool loop)
{
    if (cfg->colorMode == COLOR_MODE_RAINBOW) {
        const float interp = loop ? (1.0f - fabsf(2.0f * t - 1.0f)) : t;
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
        const int removeIdx = i - smoothness - 1;
        if (removeIdx >= 0) {
            windowSum -= waveform[removeIdx];
            windowCount--;
        }

        // Add element entering window
        const int addIdx = i + smoothness;
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
        const float left = stereo[(size_t)i * 2];
        const float right = stereo[(size_t)i * 2 + 1];

        switch (mode) {
            case CHANNEL_LEFT:
                mono[i] = left;
                break;
            case CHANNEL_RIGHT:
                mono[i] = right;
                break;
            case CHANNEL_MAX: {
                const float absL = fabsf(left);
                const float absR = fabsf(right);
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
        const int sampleCount = (int)framesRead * 2;
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
        const float absVal = fabsf(waveform[i]);
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
    const float a0 = y3 - y2 - y0 + y1;
    const float a1 = y0 - y1 - a0;
    const float a2 = y2 - y0;
    const float a3 = y1;
    return a0 * t * t * t + a1 * t * t + a2 * t + a3;
}

void DrawWaveformLinear(const float* samples, int count, RenderContext* ctx, WaveformConfig* cfg, uint64_t globalTick)
{
    const float xStep = (float)ctx->screenW / count;
    const float amplitude = ctx->minDim * cfg->amplitudeScale;
    const float jointRadius = cfg->thickness * 0.5f;

    // Convert rotation speed (radians) to sample offset
    // 2π radians = full rotation = full sample array shift
    // Negate so positive speed scrolls right (matching intuition)
    const float effectiveShift = cfg->rotationOffset + (cfg->rotationSpeed * (float)globalTick);
    const float rawOffset = -effectiveShift * count / (2.0f * PI);
    // Wrap to [0, count) range
    int sampleOffset = (int)fmodf(rawOffset, (float)count);
    if (sampleOffset < 0) {
        sampleOffset += count;
    }

    for (int i = 0; i < count - 1; i++) {
        const int idx = (i + sampleOffset) % count;
        const int idxNext = (i + 1 + sampleOffset) % count;
        const float t = (float)idx / (count - 1);
        const Color segColor = GetSegmentColor(cfg, t, false);
        const Vector2 start = { i * xStep, ctx->centerY - samples[idx] * amplitude };
        const Vector2 end = { (i + 1) * xStep, ctx->centerY - samples[idxNext] * amplitude };
        DrawLineEx(start, end, cfg->thickness, segColor);
        DrawCircleV(start, jointRadius, segColor);
    }
    // Final vertex
    const int lastIdx = (count - 1 + sampleOffset) % count;
    const float tLast = (float)lastIdx / (count - 1);
    const Color lastColor = GetSegmentColor(cfg, tLast, false);
    const Vector2 last = { (count - 1) * xStep, ctx->centerY - samples[lastIdx] * amplitude };
    DrawCircleV(last, jointRadius, lastColor);
}

void DrawWaveformCircular(float* samples, int count, RenderContext* ctx, WaveformConfig* cfg, uint64_t globalTick)
{
    const float baseRadius = ctx->minDim * cfg->radius;
    const float amplitude = ctx->minDim * cfg->amplitudeScale;
    const int numPoints = count * INTERPOLATION_MULT;
    const float angleStep = (2.0f * PI) / numPoints;

    // Calculate effective rotation: offset + (speed * globalTick)
    // Same-speed waveforms stay synchronized regardless of when speed was set
    const float effectiveRotation = cfg->rotationOffset + (cfg->rotationSpeed * (float)globalTick);

    for (int i = 0; i < numPoints; i++) {
        const int next = (i + 1) % numPoints;
        const float t = (float)i / numPoints;
        const Color segColor = GetSegmentColor(cfg, t, true);

        const float angle1 = i * angleStep + effectiveRotation - PI / 2;
        const float angle2 = next * angleStep + effectiveRotation - PI / 2;

        // Cubic interpolation for point 1
        const int idx1 = (i / INTERPOLATION_MULT) % count;
        const float frac1 = (float)(i % INTERPOLATION_MULT) / INTERPOLATION_MULT;
        const int i0 = (idx1 - 1 + count) % count;
        const int i1 = idx1;
        const int i2 = (idx1 + 1) % count;
        const int i3 = (idx1 + 2) % count;
        const float sample1 = CubicInterp(samples[i0], samples[i1], samples[i2], samples[i3], frac1);

        // Cubic interpolation for point 2
        const int idx2 = (next / INTERPOLATION_MULT) % count;
        const float frac2 = (float)(next % INTERPOLATION_MULT) / INTERPOLATION_MULT;
        const int n0 = (idx2 - 1 + count) % count;
        const int n1 = idx2;
        const int n2 = (idx2 + 1) % count;
        const int n3 = (idx2 + 2) % count;
        const float sample2 = CubicInterp(samples[n0], samples[n1], samples[n2], samples[n3], frac2);

        float radius1 = baseRadius + sample1 * (amplitude * 0.5f);
        float radius2 = baseRadius + sample2 * (amplitude * 0.5f);
        if (radius1 < 10.0f) {
            radius1 = 10.0f;
        }
        if (radius2 < 10.0f) {
            radius2 = 10.0f;
        }

        const Vector2 start = { ctx->centerX + cosf(angle1) * radius1, ctx->centerY + sinf(angle1) * radius1 };
        const Vector2 end = { ctx->centerX + cosf(angle2) * radius2, ctx->centerY + sinf(angle2) * radius2 };

        DrawLineEx(start, end, cfg->thickness, segColor);
    }
}
