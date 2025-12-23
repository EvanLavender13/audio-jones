#include "waveform.h"
#include "gradient.h"
#include <math.h>

static float FindPeakAmplitude(const float* data, int count)
{
    float peak = 0.0f;
    for (int i = 0; i < count; i++) {
        const float absVal = fabsf(data[i]);
        if (absVal > peak) {
            peak = absVal;
        }
    }
    return peak;
}

static Color GetSegmentColor(WaveformConfig* cfg, float t)
{
    if (cfg->color.mode == COLOR_MODE_RAINBOW) {
        const float interp = 1.0f - fabsf(2.0f * t - 1.0f);
        float hue = cfg->color.rainbowHue + interp * cfg->color.rainbowRange;
        hue = fmodf(hue, 360.0f);
        if (hue < 0.0f) {
            hue += 360.0f;
        }
        return ColorFromHSV(hue, cfg->color.rainbowSat, cfg->color.rainbowVal);
    }
    if (cfg->color.mode == COLOR_MODE_GRADIENT) {
        const float interp = 1.0f - fabsf(2.0f * t - 1.0f);
        return GradientEvaluate(cfg->color.gradientStops, cfg->color.gradientStopCount, interp);
    }
    return cfg->color.solid;
}

// Single pass of sliding window moving average - O(N) complexity
static void SmoothWaveformPass(const float* waveform, float* smoothed, int count, int windowRadius)
{
    // Initialize window sum for first element
    float windowSum = 0.0f;
    int windowCount = 0;
    for (int j = -windowRadius; j <= windowRadius; j++) {
        if (j >= 0 && j < count) {
            windowSum += waveform[j];
            windowCount++;
        }
    }
    smoothed[0] = windowSum / windowCount;

    // Slide window across data
    for (int i = 1; i < count; i++) {
        // Remove element leaving window
        const int removeIdx = i - windowRadius - 1;
        if (removeIdx >= 0) {
            windowSum -= waveform[removeIdx];
            windowCount--;
        }

        // Add element entering window
        const int addIdx = i + windowRadius;
        if (addIdx < count) {
            windowSum += waveform[addIdx];
            windowCount++;
        }

        smoothed[i] = windowSum / windowCount;
    }
}

static void SmoothWaveform(float* waveform, int count, int smoothness)
{
    if (smoothness <= 0 || count <= 0) {
        return;
    }

    const float originalPeak = FindPeakAmplitude(waveform, count);

    const int passCount = 3;
    const int windowRadius = (smoothness + passCount - 1) / passCount;

    static float smoothed[WAVEFORM_EXTENDED];

    for (int pass = 0; pass < passCount; pass++) {
        SmoothWaveformPass(waveform, smoothed, count, windowRadius);
        for (int i = 0; i < count; i++) {
            waveform[i] = smoothed[i];
        }
    }

    const float newPeak = FindPeakAmplitude(waveform, count);

    if (newPeak > 0.0001f && originalPeak > 0.0001f) {
        const float scale = originalPeak / newPeak;
        for (int i = 0; i < count; i++) {
            waveform[i] *= scale;
        }
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

    // Instant normalization for volume-independent display
    float peak = 0.0f;
    for (int i = 0; i < copyCount; i++) {
        const float absVal = fabsf(waveform[i]);
        if (absVal > peak) {
            peak = absVal;
        }
    }
    if (peak > 0.0001f) {
        const float gain = 1.0f / peak;
        for (int i = 0; i < copyCount; i++) {
            waveform[i] *= gain;
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
    const float yOffset = cfg->verticalOffset * ctx->screenH;

    // Calculate color offset from rotation (color moves, waveform stays still)
    // Negate so positive speed scrolls color rightward
    const float effectiveRotation = cfg->rotationOffset + (cfg->rotationSpeed * (float)globalTick);
    float colorOffset = fmodf(-effectiveRotation / (2.0f * PI), 1.0f);
    if (colorOffset < 0.0f) {
        colorOffset += 1.0f;
    }

    for (int i = 0; i < count - 1; i++) {
        // t ranges 0→1 across the waveform, offset by colorOffset for animation
        float t = (float)i / (count - 1) + colorOffset;
        if (t >= 1.0f) {
            t -= 1.0f;
        }
        const Color segColor = GetSegmentColor(cfg, t);
        const Vector2 start = { i * xStep, ctx->centerY - samples[i] * amplitude - yOffset };
        const Vector2 end = { (i + 1) * xStep, ctx->centerY - samples[i + 1] * amplitude - yOffset };
        DrawLineEx(start, end, cfg->thickness, segColor);
        DrawCircleV(start, jointRadius, segColor);
    }
    // Final vertex
    float tLast = 1.0f + colorOffset;
    if (tLast >= 1.0f) {
        tLast -= 1.0f;
    }
    const Color lastColor = GetSegmentColor(cfg, tLast);
    const Vector2 last = { (count - 1) * xStep, ctx->centerY - samples[count - 1] * amplitude - yOffset };
    DrawCircleV(last, jointRadius, lastColor);
}

void DrawWaveformCircular(float* samples, int count, RenderContext* ctx, WaveformConfig* cfg, uint64_t globalTick)
{
    const float baseRadius = ctx->minDim * cfg->radius;
    const float amplitude = ctx->minDim * cfg->amplitudeScale;
    const int numPoints = count * INTERPOLATION_MULT;
    const float angleStep = (2.0f * PI) / numPoints;
    const float jointRadius = cfg->thickness * 0.5f;

    // Calculate effective rotation: offset + (speed * globalTick)
    // Same-speed waveforms stay synchronized regardless of when speed was set
    const float effectiveRotation = cfg->rotationOffset + (cfg->rotationSpeed * (float)globalTick);

    for (int i = 0; i < numPoints; i++) {
        const int next = (i + 1) % numPoints;
        const float t = (float)i / numPoints;
        const Color segColor = GetSegmentColor(cfg, t);

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
        DrawCircleV(start, jointRadius, segColor);
    }
}
