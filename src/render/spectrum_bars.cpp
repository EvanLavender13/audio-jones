#include "spectrum_bars.h"
#include "analysis/fft.h"
#include "draw_utils.h"
#include <math.h>
#include <stdlib.h>

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

static void ComputeBandRanges(BandRange *ranges) {
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

SpectrumBars *SpectrumBarsInit(void) {
  SpectrumBars *sb = (SpectrumBars *)calloc(1, sizeof(SpectrumBars));
  if (sb == NULL) {
    return NULL;
  }

  ComputeBandRanges(sb->bandRanges);

  for (int i = 0; i < SPECTRUM_BAND_COUNT; i++) {
    sb->smoothedBands[i] = 0.0f;
  }

  return sb;
}

void SpectrumBarsUninit(SpectrumBars *sb) { free(sb); }

void SpectrumBarsProcess(SpectrumBars *sb, const float *magnitude, int binCount,
                         const Drawable *d) {
  if (sb == NULL || magnitude == NULL || d == NULL) {
    return;
  }

  for (int i = 0; i < SPECTRUM_BAND_COUNT; i++) {
    const BandRange *range = &sb->bandRanges[i];

    // Find peak magnitude in this band
    float peak = 0.0f;
    for (int bin = range->binStart; bin < range->binEnd && bin < binCount;
         bin++) {
      if (magnitude[bin] > peak) {
        peak = magnitude[bin];
      }
    }

    // Convert to dB
    const float dbValue = 20.0f * log10f(peak + 1e-10f);

    // Normalize to 0-1 using minDb/maxDb (guard against zero/negative range)
    float dbRange = d->spectrum.maxDb - d->spectrum.minDb;
    if (dbRange < 1.0f) {
      dbRange = 1.0f;
    }
    float normalized = (dbValue - d->spectrum.minDb) / dbRange;
    if (normalized < 0.0f) {
      normalized = 0.0f;
    }
    if (normalized > 1.0f) {
      normalized = 1.0f;
    }

    // Exponential smoothing: high smoothing = slow decay
    sb->smoothedBands[i] = sb->smoothedBands[i] * d->spectrum.smoothing +
                           normalized * (1.0f - d->spectrum.smoothing);
  }
}

void SpectrumBarsDrawCircular(
    const SpectrumBars *sb, const RenderContext *ctx, const Drawable *d,
    uint64_t globalTick, // NOLINT(misc-unused-parameters)
    float opacity) {
  if (sb == NULL || ctx == NULL || d == NULL) {
    return;
  }

  const float centerX = d->base.x * ctx->screenW;
  const float centerY = d->base.y * ctx->screenH;
  const float baseRadius = ctx->minDim * d->spectrum.innerRadius;
  const float maxBarHeight = ctx->minDim * d->spectrum.barHeight;
  const float angleStep = (2.0f * PI) / SPECTRUM_BAND_COUNT;
  const float barArc = angleStep * d->spectrum.barWidth;

  const float effectiveRotation = d->base.rotationAngle + d->rotationAccum;

  // Color offset from color shift (independent of physical rotation)
  const float effectiveColorShift = d->spectrum.colorShift + d->colorShiftAccum;
  float colorOffset = fmodf(-effectiveColorShift / (2.0f * PI), 1.0f);
  if (colorOffset < 0.0f) {
    colorOffset += 1.0f;
  }

  for (int i = 0; i < SPECTRUM_BAND_COUNT; i++) {
    float t = (float)i / SPECTRUM_BAND_COUNT + colorOffset;
    if (t >= 1.0f) {
      t -= 1.0f;
    }
    const Color barColor = ColorFromConfig(&d->base.color, t, opacity);

    const float angle = i * angleStep + effectiveRotation - PI / 2;
    const float barHeight = sb->smoothedBands[i] * maxBarHeight;

    // Calculate bar corners (trapezoid centered on radius)
    const float halfHeight = barHeight * 0.5f;
    const float innerR = baseRadius - halfHeight;
    const float outerR = baseRadius + halfHeight;
    const float halfArc = barArc * 0.5f;

    // Inner edge
    const Vector2 innerLeft = {centerX + cosf(angle - halfArc) * innerR,
                               centerY + sinf(angle - halfArc) * innerR};
    const Vector2 innerRight = {centerX + cosf(angle + halfArc) * innerR,
                                centerY + sinf(angle + halfArc) * innerR};

    // Outer edge
    const Vector2 outerLeft = {centerX + cosf(angle - halfArc) * outerR,
                               centerY + sinf(angle - halfArc) * outerR};
    const Vector2 outerRight = {centerX + cosf(angle + halfArc) * outerR,
                                centerY + sinf(angle + halfArc) * outerR};

    // Draw as two triangles forming a quad (CCW winding for raylib)
    DrawTriangle(innerLeft, outerRight, outerLeft, barColor);
    DrawTriangle(innerLeft, innerRight, outerRight, barColor);
  }
}

void SpectrumBarsDrawLinear(
    const SpectrumBars *sb, const RenderContext *ctx, const Drawable *d,
    uint64_t globalTick, // NOLINT(misc-unused-parameters)
    float opacity) {
  if (sb == NULL || ctx == NULL || d == NULL) {
    return;
  }

  const float centerX = d->base.x * ctx->screenW;
  const float centerY = d->base.y * ctx->screenH;
  const float maxBarHeight = ctx->minDim * d->spectrum.barHeight;

  // Geometric rotation (same approach as waveform linear)
  const float angle = d->base.rotationAngle + d->rotationAccum;
  const float cosA = cosf(angle);
  const float sinA = sinf(angle);

  const float abscos = fabsf(cosA);
  const float abssin = fabsf(sinA);
  float lineLength;
  if (abscos < 0.001f) {
    lineLength = ctx->screenH;
  } else if (abssin < 0.001f) {
    lineLength = ctx->screenW;
  } else {
    const float lenX = ctx->screenW / abscos;
    const float lenY = ctx->screenH / abssin;
    lineLength = fminf(lenX, lenY);
  }

  // Overextend past viewport to cover bar height at endpoints
  lineLength *= 1.2f;

  const float halfLen = lineLength * 0.5f;
  const float slotWidth = lineLength / SPECTRUM_BAND_COUNT;
  const float barWidth = slotWidth * d->spectrum.barWidth;
  const float barGap = (slotWidth - barWidth) * 0.5f;

  // Color offset from color shift (independent of geometry)
  const float effectiveColorShift = d->spectrum.colorShift + d->colorShiftAccum;
  float colorOffset = fmodf(-effectiveColorShift / (2.0f * PI), 1.0f);
  if (colorOffset < 0.0f) {
    colorOffset += 1.0f;
  }

  // Axis direction: (cosA, sinA), perpendicular: (-sinA, cosA)
  for (int i = 0; i < SPECTRUM_BAND_COUNT; i++) {
    float t = (float)i / SPECTRUM_BAND_COUNT + colorOffset;
    if (t >= 1.0f) {
      t -= 1.0f;
    }
    const Color barColor = ColorFromConfig(&d->base.color, t, opacity);

    const float barHeight = sb->smoothedBands[i] * maxBarHeight;
    const float halfH = barHeight * 0.5f;

    // Bar start/end along the rotated axis
    const float axisStart = -halfLen + i * slotWidth + barGap;
    const float axisEnd = axisStart + barWidth;

    // Four corners: axis position +/- perpendicular half-height
    const Vector2 p0 = {centerX + axisStart * cosA + halfH * sinA,
                        centerY + axisStart * sinA - halfH * cosA};
    const Vector2 p1 = {centerX + axisEnd * cosA + halfH * sinA,
                        centerY + axisEnd * sinA - halfH * cosA};
    const Vector2 p2 = {centerX + axisEnd * cosA - halfH * sinA,
                        centerY + axisEnd * sinA + halfH * cosA};
    const Vector2 p3 = {centerX + axisStart * cosA - halfH * sinA,
                        centerY + axisStart * sinA + halfH * cosA};

    DrawTriangle(p0, p2, p1, barColor);
    DrawTriangle(p0, p3, p2, barColor);
  }
}
