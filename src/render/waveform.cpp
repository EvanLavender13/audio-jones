#include "waveform.h"
#include "draw_utils.h"
#include "thick_line.h"
#include <math.h>
#include <stdlib.h>

#define WAVEFORM_MAX_POINTS 512

static float FindPeakAmplitude(const float *data, int count) {
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
static void SmoothWaveformPass(const float *waveform, float *smoothed,
                               int count, int windowRadius) {
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

static void SmoothWaveform(float *waveform, int count, int smoothness) {
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
static void MixStereoToMono(const float *stereo, uint32_t frameCount,
                            float *mono, ChannelMode mode) {
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

void ProcessWaveformBase(const float *audioBuffer, uint32_t framesRead,
                         float *waveform, ChannelMode mode) {
  int copyCount;

  if (mode == CHANNEL_INTERLEAVED) {
    // Legacy behavior: copy interleaved samples directly (uses 2x samples)
    const int sampleCount = (int)framesRead * 2;
    copyCount =
        (sampleCount > WAVEFORM_SAMPLES) ? WAVEFORM_SAMPLES : sampleCount;
    for (int i = 0; i < copyCount; i++) {
      waveform[i] = audioBuffer[i];
    }
  } else {
    // Mix stereo to mono first
    copyCount =
        (framesRead > WAVEFORM_SAMPLES) ? WAVEFORM_SAMPLES : (int)framesRead;
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

void ProcessWaveformSmooth(const float *waveform, float *waveformExtended,
                           float smoothness) {
  // Copy base waveform to extended buffer
  for (int i = 0; i < WAVEFORM_SAMPLES; i++) {
    waveformExtended[i] = waveform[i];
  }

  // Create palindrome first
  for (int i = 0; i < WAVEFORM_SAMPLES; i++) {
    waveformExtended[WAVEFORM_SAMPLES + i] =
        waveformExtended[WAVEFORM_SAMPLES - 1 - i];
  }

  // Smooth the full palindrome so window blends across join points
  SmoothWaveform(waveformExtended, WAVEFORM_EXTENDED, (int)smoothness);
}

static void AverageSamples(const float *samples, int count, float *averaged,
                           int pointCount) {
  const int groupSize = count / pointCount;
  for (int i = 0; i < pointCount; i++) {
    float sum = 0.0f;
    for (int j = 0; j < groupSize; j++) {
      sum += samples[i * groupSize + j];
    }
    averaged[i] = sum / groupSize;
  }
}

static float ComputeLineLength(float cosA, float sinA, int screenW,
                               int screenH) {
  const float abscos = fabsf(cosA);
  const float abssin = fabsf(sinA);
  float lineLength;
  if (abscos < 0.001f) {
    lineLength = screenH;
  } else if (abssin < 0.001f) {
    lineLength = screenW;
  } else {
    const float lenX = screenW / abscos;
    const float lenY = screenH / abssin;
    lineLength = fminf(lenX, lenY);
  }
  return lineLength * 1.5f;
}

static float ComputeColorOffset(const Drawable *d) {
  const float effectiveColorShift = d->waveform.colorShift + d->colorShiftAccum;
  float colorOffset = fmodf(-effectiveColorShift / (2.0f * PI), 1.0f);
  if (colorOffset < 0.0f) {
    colorOffset += 1.0f;
  }
  return colorOffset;
}

static void DrawDotsLinear(const float *averaged, int pc,
                           const RenderContext *ctx, const Drawable *d,
                           float opacity) {
  const float centerX = d->base.x * ctx->screenW;
  const float centerY = d->base.y * ctx->screenH;
  const float amplitude = ctx->minDim * d->waveform.amplitudeScale;
  const float thickness = d->waveform.thickness;

  const float angle = d->base.rotationAngle + d->rotationAccum;
  const float cosA = cosf(angle);
  const float sinA = sinf(angle);

  const float lineLength =
      ComputeLineLength(cosA, sinA, ctx->screenW, ctx->screenH);
  const float halfLen = lineLength * 0.5f;
  const float colorOffset = ComputeColorOffset(d);

  for (int i = 0; i < pc; i++) {
    const float t_pos = (float)i / (pc - 1);
    const float dist = -halfLen + t_pos * lineLength;
    const float amp = averaged[i] * amplitude;
    const float localX = dist * cosA - amp * sinA;
    const float localY = dist * sinA + amp * cosA;
    const Vector2 pos = {centerX + localX, centerY + localY};

    float t = t_pos + colorOffset;
    if (t >= 1.0f) {
      t -= 1.0f;
    }
    const Color color = ColorFromConfig(&d->base.color, t, opacity);
    DrawCircleV(pos, thickness * 0.5f, color);
  }
}

static void DrawDotsCircular(const float *averaged, int pc,
                             const RenderContext *ctx, const Drawable *d,
                             float opacity) {
  const float centerX = d->base.x * ctx->screenW;
  const float centerY = d->base.y * ctx->screenH;
  const float baseRadius = ctx->minDim * d->waveform.radius;
  const float amplitude = ctx->minDim * d->waveform.amplitudeScale;
  const float thickness = d->waveform.thickness;

  const float effectiveRotation = d->base.rotationAngle + d->rotationAccum;
  const float colorOffset = ComputeColorOffset(d);

  for (int i = 0; i < pc; i++) {
    const float t_pos = (float)i / pc;
    const float angle = t_pos * 2.0f * PI + effectiveRotation - PI / 2;
    float radius = baseRadius + averaged[i] * (amplitude * 0.5f);
    if (radius < 10.0f) {
      radius = 10.0f;
    }
    const Vector2 pos = {centerX + cosf(angle) * radius,
                         centerY + sinf(angle) * radius};

    float t = t_pos + colorOffset;
    if (t >= 1.0f) {
      t -= 1.0f;
    }
    const Color color = ColorFromConfig(&d->base.color, t, opacity);
    DrawCircleV(pos, thickness * 0.5f, color);
  }
}

static void DrawBarsLinear(const float *averaged, int pc,
                           const RenderContext *ctx, const Drawable *d,
                           float opacity) {
  const float centerX = d->base.x * ctx->screenW;
  const float centerY = d->base.y * ctx->screenH;
  const float amplitude = ctx->minDim * d->waveform.amplitudeScale;
  const float thickness = d->waveform.thickness;

  const float angle = d->base.rotationAngle + d->rotationAccum;
  const float cosA = cosf(angle);
  const float sinA = sinf(angle);

  const float lineLength =
      ComputeLineLength(cosA, sinA, ctx->screenW, ctx->screenH);
  const float halfLen = lineLength * 0.5f;
  const float colorOffset = ComputeColorOffset(d);

  const float angleDegs = angle * RAD2DEG;

  for (int i = 0; i < pc; i++) {
    const float t_pos = (float)i / (pc - 1);
    const float dist = -halfLen + t_pos * lineLength;

    const float baseX = centerX + dist * cosA;
    const float baseY = centerY + dist * sinA;

    const float barHeight = averaged[i] * amplitude;
    const float halfBar = barHeight * 0.5f;
    const float barCenterX = baseX - halfBar * sinA;
    const float barCenterY = baseY + halfBar * cosA;

    const Rectangle rect = {barCenterX, barCenterY, thickness,
                            fabsf(barHeight)};
    const Vector2 origin = {thickness * 0.5f, fabsf(barHeight) * 0.5f};

    float t = t_pos + colorOffset;
    if (t >= 1.0f) {
      t -= 1.0f;
    }
    const Color color = ColorFromConfig(&d->base.color, t, opacity);
    DrawRectanglePro(rect, origin, angleDegs, color);
  }
}

static void DrawBarsCircular(const float *averaged, int pc,
                             const RenderContext *ctx, const Drawable *d,
                             float opacity) {
  const float centerX = d->base.x * ctx->screenW;
  const float centerY = d->base.y * ctx->screenH;
  const float baseRadius = ctx->minDim * d->waveform.radius;
  const float amplitude = ctx->minDim * d->waveform.amplitudeScale;
  const float thickness = d->waveform.thickness;

  const float effectiveRotation = d->base.rotationAngle + d->rotationAccum;
  const float colorOffset = ComputeColorOffset(d);

  for (int i = 0; i < pc; i++) {
    const float t_pos = (float)i / pc;
    const float angle = t_pos * 2.0f * PI + effectiveRotation - PI / 2;
    const float barHeight = averaged[i] * amplitude * 0.5f;
    const float barRadius = baseRadius + barHeight * 0.5f;
    const float barCenterX = centerX + cosf(angle) * barRadius;
    const float barCenterY = centerY + sinf(angle) * barRadius;

    const float angleDegs = angle * RAD2DEG + 90.0f;
    const Rectangle rect = {barCenterX, barCenterY, thickness,
                            fabsf(barHeight)};
    const Vector2 origin = {thickness * 0.5f, fabsf(barHeight) * 0.5f};

    float t = t_pos + colorOffset;
    if (t >= 1.0f) {
      t -= 1.0f;
    }
    const Color color = ColorFromConfig(&d->base.color, t, opacity);
    DrawRectanglePro(rect, origin, angleDegs, color);
  }
}

void DrawWaveformLinear(const float *samples, int count, RenderContext *ctx,
                        const Drawable *d, uint64_t globalTick, float opacity) {
  (void)globalTick; // reserved for future sync

  // Dispatch dots/bars styles via averaged samples
  if (d->waveform.style != WAVEFORM_STYLE_LINE) {
    int pc = (int)d->waveform.pointCount;
    if (pc < 2) {
      pc = 2;
    }
    if (pc > WAVEFORM_MAX_POINTS) {
      pc = WAVEFORM_MAX_POINTS;
    }
    if (pc > count) {
      pc = count;
    }
    float averaged[WAVEFORM_MAX_POINTS];
    AverageSamples(samples, count, averaged, pc);
    if (d->waveform.style == WAVEFORM_STYLE_DOTS) {
      DrawDotsLinear(averaged, pc, ctx, d, opacity);
    } else {
      DrawBarsLinear(averaged, pc, ctx, d, opacity);
    }
    return;
  }

  const float centerX = d->base.x * ctx->screenW;
  const float centerY = d->base.y * ctx->screenH;
  const float amplitude = ctx->minDim * d->waveform.amplitudeScale;

  // Geometric rotation
  const float angle = d->base.rotationAngle + d->rotationAccum;
  const float cosA = cosf(angle);
  const float sinA = sinf(angle);

  const float lineLength =
      ComputeLineLength(cosA, sinA, ctx->screenW, ctx->screenH);
  const float halfLen = lineLength * 0.5f;
  const float step = lineLength / (count - 1);
  const float colorOffset = ComputeColorOffset(d);

  ThickLineBegin(d->waveform.thickness);
  for (int i = 0; i < count; i++) {
    // Position along rotated axis
    const float dist = -halfLen + i * step;

    // Perpendicular offset for amplitude
    const float amp = samples[i] * amplitude;

    // Rotated position: dist along axis, amp perpendicular
    const float localX = dist * cosA - amp * sinA;
    const float localY = dist * sinA + amp * cosA;

    const Vector2 pos = {centerX + localX, centerY + localY};

    // Color from position + offset
    float t = (float)i / (count - 1) + colorOffset;
    if (t >= 1.0f) {
      t -= 1.0f;
    }
    const Color vertColor = ColorFromConfig(&d->base.color, t, opacity);
    ThickLineVertex(pos, vertColor);
  }
  ThickLineEnd(false);
}

void DrawWaveformCircular(const float *samples, int count, RenderContext *ctx,
                          const Drawable *d, uint64_t globalTick,
                          float opacity) {
  (void)globalTick; // reserved for future sync

  // Dispatch dots/bars styles via averaged samples
  if (d->waveform.style != WAVEFORM_STYLE_LINE) {
    int pc = (int)d->waveform.pointCount;
    if (pc < 2) {
      pc = 2;
    }
    if (pc > WAVEFORM_MAX_POINTS) {
      pc = WAVEFORM_MAX_POINTS;
    }
    if (pc > count) {
      pc = count;
    }
    float averaged[WAVEFORM_MAX_POINTS];
    AverageSamples(samples, count, averaged, pc);
    if (d->waveform.style == WAVEFORM_STYLE_DOTS) {
      DrawDotsCircular(averaged, pc, ctx, d, opacity);
    } else {
      DrawBarsCircular(averaged, pc, ctx, d, opacity);
    }
    return;
  }

  const float centerX = d->base.x * ctx->screenW;
  const float centerY = d->base.y * ctx->screenH;
  const float baseRadius = ctx->minDim * d->waveform.radius;
  const float amplitude = ctx->minDim * d->waveform.amplitudeScale;
  const float angleStep = (2.0f * PI) / count;

  const float effectiveRotation = d->base.rotationAngle + d->rotationAccum;
  const float colorOffset = ComputeColorOffset(d);

  ThickLineBegin(d->waveform.thickness);
  for (int i = 0; i < count; i++) {
    float t = (float)i / count + colorOffset;
    if (t >= 1.0f) {
      t -= 1.0f;
    }
    const Color vertColor = ColorFromConfig(&d->base.color, t, opacity);

    const float angle = i * angleStep + effectiveRotation - PI / 2;
    const float sample = samples[i % count];

    float radius = baseRadius + sample * (amplitude * 0.5f);
    if (radius < 10.0f) {
      radius = 10.0f;
    }

    const Vector2 pos = {centerX + cosf(angle) * radius,
                         centerY + sinf(angle) * radius};
    ThickLineVertex(pos, vertColor);
  }
  ThickLineEnd(true);
}
