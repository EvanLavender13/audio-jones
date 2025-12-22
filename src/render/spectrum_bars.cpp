#include "spectrum_bars.h"
#include "gradient.h"
#include "analysis/fft.h"
#include <stdlib.h>
#include <math.h>

#define SAMPLE_RATE 48000.0f
#define MIN_FREQ 20.0f
#define MAX_FREQ 20000.0f

typedef struct {
    int binStart;
    int binEnd;
} BandRange;

struct SpectrumBars {
    float smoothedBands[SPECTRUM_BAND_COUNT];
    BandRange bandRanges[SPECTRUM_BAND_COUNT];
};

static void ComputeBandRanges(BandRange* ranges)
{
    const float binResolution = SAMPLE_RATE / FFT_SIZE;
    const float logMin = log2f(MIN_FREQ);
    const float logMax = log2f(MAX_FREQ);

    for (int i = 0; i < SPECTRUM_BAND_COUNT; i++) {
        const float t0 = (float)i / SPECTRUM_BAND_COUNT;
        const float t1 = (float)(i + 1) / SPECTRUM_BAND_COUNT;
        const float f0 = exp2f(logMin + t0 * (logMax - logMin));
        const float f1 = exp2f(logMin + t1 * (logMax - logMin));

        ranges[i].binStart = (int)(f0 / binResolution);
        ranges[i].binEnd = (int)(f1 / binResolution);

        // Ensure at least one bin per band
        if (ranges[i].binEnd <= ranges[i].binStart) {
            ranges[i].binEnd = ranges[i].binStart + 1;
        }

        // Clamp to valid bin range
        if (ranges[i].binStart < 0) {
            ranges[i].binStart = 0;
        }
        if (ranges[i].binEnd > FFT_BIN_COUNT) {
            ranges[i].binEnd = FFT_BIN_COUNT;
        }
    }
}

SpectrumBars* SpectrumBarsInit(void)
{
    SpectrumBars* sb = (SpectrumBars*)calloc(1, sizeof(SpectrumBars));
    if (sb == NULL) {
        return NULL;
    }

    ComputeBandRanges(sb->bandRanges);

    for (int i = 0; i < SPECTRUM_BAND_COUNT; i++) {
        sb->smoothedBands[i] = 0.0f;
    }

    return sb;
}

void SpectrumBarsUninit(SpectrumBars* sb)
{
    free(sb);
}

void SpectrumBarsProcess(SpectrumBars* sb,
                         const float* magnitude,
                         int binCount,
                         const SpectrumConfig* config)
{
    if (sb == NULL || magnitude == NULL) {
        return;
    }

    for (int i = 0; i < SPECTRUM_BAND_COUNT; i++) {
        const BandRange* range = &sb->bandRanges[i];

        // Find peak magnitude in this band
        float peak = 0.0f;
        for (int bin = range->binStart; bin < range->binEnd && bin < binCount; bin++) {
            if (magnitude[bin] > peak) {
                peak = magnitude[bin];
            }
        }

        // Convert to dB
        const float dbValue = 20.0f * log10f(peak + 1e-10f);

        // Normalize to 0-1 using minDb/maxDb (guard against zero/negative range)
        float dbRange = config->maxDb - config->minDb;
        if (dbRange < 1.0f) {
            dbRange = 1.0f;
        }
        float normalized = (dbValue - config->minDb) / dbRange;
        if (normalized < 0.0f) {
            normalized = 0.0f;
        }
        if (normalized > 1.0f) {
            normalized = 1.0f;
        }

        // Exponential smoothing: high smoothing = slow decay
        sb->smoothedBands[i] = sb->smoothedBands[i] * config->smoothing +
                               normalized * (1.0f - config->smoothing);
    }
}

// Compute color for a band at position t (0-1) across the spectrum
// Uses ping-pong interpolation (0→1→0) for seamless wrapping at endpoints
static Color GetBandColor(const SpectrumConfig* config, float t)
{
    if (config->color.mode == COLOR_MODE_RAINBOW) {
        const float interp = 1.0f - fabsf(2.0f * t - 1.0f);
        float hue = config->color.rainbowHue + interp * config->color.rainbowRange;
        hue = fmodf(hue, 360.0f);
        if (hue < 0.0f) {
            hue += 360.0f;
        }
        return ColorFromHSV(hue, config->color.rainbowSat, config->color.rainbowVal);
    }
    if (config->color.mode == COLOR_MODE_GRADIENT) {
        return GradientEvaluate(config->color.gradientStops, config->color.gradientStopCount, t);
    }
    return config->color.solid;
}

void SpectrumBarsDrawCircular(const SpectrumBars* sb,
                              const RenderContext* ctx,
                              const SpectrumConfig* config,
                              uint64_t globalTick)
{
    if (sb == NULL || ctx == NULL || config == NULL) {
        return;
    }

    const float baseRadius = ctx->minDim * config->innerRadius;
    const float maxBarHeight = ctx->minDim * config->barHeight;
    const float angleStep = (2.0f * PI) / SPECTRUM_BAND_COUNT;
    const float barArc = angleStep * config->barWidth;

    // Calculate effective rotation
    const float effectiveRotation = config->rotationOffset + (config->rotationSpeed * (float)globalTick);

    for (int i = 0; i < SPECTRUM_BAND_COUNT; i++) {
        const float t = (float)i / SPECTRUM_BAND_COUNT;
        const Color barColor = GetBandColor(config, t);

        const float angle = i * angleStep + effectiveRotation - PI / 2;
        const float barHeight = sb->smoothedBands[i] * maxBarHeight;

        // Calculate bar corners (trapezoid centered on radius)
        const float halfHeight = barHeight * 0.5f;
        const float innerR = baseRadius - halfHeight;
        const float outerR = baseRadius + halfHeight;
        const float halfArc = barArc * 0.5f;

        // Inner edge
        const Vector2 innerLeft = {
            ctx->centerX + cosf(angle - halfArc) * innerR,
            ctx->centerY + sinf(angle - halfArc) * innerR
        };
        const Vector2 innerRight = {
            ctx->centerX + cosf(angle + halfArc) * innerR,
            ctx->centerY + sinf(angle + halfArc) * innerR
        };

        // Outer edge
        const Vector2 outerLeft = {
            ctx->centerX + cosf(angle - halfArc) * outerR,
            ctx->centerY + sinf(angle - halfArc) * outerR
        };
        const Vector2 outerRight = {
            ctx->centerX + cosf(angle + halfArc) * outerR,
            ctx->centerY + sinf(angle + halfArc) * outerR
        };

        // Draw as two triangles forming a quad (CCW winding for raylib)
        DrawTriangle(innerLeft, outerRight, outerLeft, barColor);
        DrawTriangle(innerLeft, innerRight, outerRight, barColor);
    }
}

void SpectrumBarsDrawLinear(const SpectrumBars* sb,
                            const RenderContext* ctx,
                            const SpectrumConfig* config,
                            uint64_t globalTick)
{
    if (sb == NULL || ctx == NULL || config == NULL) {
        return;
    }

    const float totalWidth = (float)ctx->screenW;
    const float slotWidth = totalWidth / SPECTRUM_BAND_COUNT;
    const float barWidth = slotWidth * config->barWidth;
    const float maxBarHeight = ctx->minDim * config->barHeight;
    const float barGap = (slotWidth - barWidth) * 0.5f;

    // Calculate color offset from rotation (color moves, bars stay still)
    const float effectiveRotation = config->rotationOffset + (config->rotationSpeed * (float)globalTick);
    float colorOffset = fmodf(-effectiveRotation / (2.0f * PI), 1.0f);
    if (colorOffset < 0.0f) {
        colorOffset += 1.0f;
    }

    for (int i = 0; i < SPECTRUM_BAND_COUNT; i++) {
        float t = (float)i / SPECTRUM_BAND_COUNT + colorOffset;
        if (t >= 1.0f) {
            t -= 1.0f;
        }
        const Color barColor = GetBandColor(config, t);

        const float barHeight = sb->smoothedBands[i] * maxBarHeight;
        const float x = i * slotWidth + barGap;
        const float y = ctx->centerY - barHeight * 0.5f;

        DrawRectangle((int)x, (int)y, (int)barWidth, (int)barHeight, barColor);
    }
}
