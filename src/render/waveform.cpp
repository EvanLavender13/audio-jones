#include "waveform.h"
#include "thick_line.h"
#include "draw_utils.h"
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

void DrawWaveformLinear(const float* samples, int count, RenderContext* ctx, const Drawable* d, uint64_t globalTick, float opacity)
{
    const float centerY = d->base.y * ctx->screenH;
    const float xStep = (float)ctx->screenW / count;
    const float amplitude = ctx->minDim * d->waveform.amplitudeScale;

    // Calculate color offset from rotation (color moves, waveform stays still)
    // Negate so positive speed scrolls color rightward
    const float effectiveRotation = d->base.rotationOffset + (d->base.rotationSpeed * (float)globalTick);
    float colorOffset = fmodf(-effectiveRotation / (2.0f * PI), 1.0f);
    if (colorOffset < 0.0f) {
        colorOffset += 1.0f;
    }

    ThickLineBegin((float)d->waveform.thickness);
    for (int i = 0; i < count; i++) {
        float t = (float)i / (count - 1) + colorOffset;
        if (t >= 1.0f) {
            t -= 1.0f;
        }
        const Color vertColor = ColorFromConfig(&d->base.color, t, opacity);
        const Vector2 pos = { i * xStep, centerY - samples[i] * amplitude };
        ThickLineVertex(pos, vertColor);
    }
    ThickLineEnd(false);
}

void DrawWaveformCircular(const float* samples, int count, RenderContext* ctx, const Drawable* d, uint64_t globalTick, float opacity)
{
    const float centerX = d->base.x * ctx->screenW;
    const float centerY = d->base.y * ctx->screenH;
    const float baseRadius = ctx->minDim * d->waveform.radius;
    const float amplitude = ctx->minDim * d->waveform.amplitudeScale;
    const float angleStep = (2.0f * PI) / count;

    // Calculate effective rotation: offset + (speed * globalTick)
    // Same-speed waveforms stay synchronized regardless of when speed was set
    const float effectiveRotation = d->base.rotationOffset + (d->base.rotationSpeed * (float)globalTick);

    ThickLineBegin((float)d->waveform.thickness);
    for (int i = 0; i < count; i++) {
        const float t = (float)i / count;
        const Color vertColor = ColorFromConfig(&d->base.color, t, opacity);

        const float angle = i * angleStep + effectiveRotation - PI / 2;
        const float sample = samples[i % count];

        float radius = baseRadius + sample * (amplitude * 0.5f);
        if (radius < 10.0f) {
            radius = 10.0f;
        }

        const Vector2 pos = { centerX + cosf(angle) * radius, centerY + sinf(angle) * radius };
        ThickLineVertex(pos, vertColor);
    }
    ThickLineEnd(true);
}
