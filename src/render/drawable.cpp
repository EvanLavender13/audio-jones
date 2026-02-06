#include "drawable.h"
#include "draw_utils.h"
#include <math.h>
#include <string.h>

// Render single waveform drawable
static void DrawableRenderWaveform(const DrawableState *state,
                                   RenderContext *ctx, const Drawable *d,
                                   int index, uint64_t tick, float opacity) {
  if (d->path == PATH_CIRCULAR) {
    DrawWaveformCircular((float *)state->waveformExtended[index],
                         WAVEFORM_EXTENDED, ctx, d, tick, opacity);
  } else {
    DrawWaveformLinear(state->waveformExtended[index], WAVEFORM_SAMPLES, ctx, d,
                       tick, opacity);
  }
}

// Render single spectrum drawable
static void DrawableRenderSpectrum(const DrawableState *state,
                                   RenderContext *ctx, const Drawable *d,
                                   int spectrumIndex, uint64_t tick,
                                   float opacity) {
  SpectrumBars *bars = state->spectrumBars[spectrumIndex];
  if (bars == NULL) {
    return;
  }
  if (d->path == PATH_CIRCULAR) {
    SpectrumBarsDrawCircular(bars, ctx, d, tick, opacity);
  } else {
    SpectrumBarsDrawLinear(bars, ctx, d, tick, opacity);
  }
}

void DrawableStateInit(DrawableState *state) {
  memset(state, 0, sizeof(DrawableState));
  // spectrumBars pointers initialized to NULL by memset (lazy allocation)
}

void DrawableStateUninit(DrawableState *state) {
  for (int i = 0; i < MAX_DRAWABLES; i++) {
    if (state->spectrumBars[i] != NULL) {
      SpectrumBarsUninit(state->spectrumBars[i]);
      state->spectrumBars[i] = NULL;
    }
  }
}

void DrawableProcessWaveforms(DrawableState *state, const float *audioBuffer,
                              uint32_t framesRead, const Drawable *drawables,
                              int count, ChannelMode channelMode) {
  // Process base waveform from audio
  ProcessWaveformBase(audioBuffer, framesRead, state->waveform, channelMode);

  // Apply per-drawable EMA temporal smoothing, then spatial smoothing
  int waveformIndex = 0;
  for (int i = 0; i < count && waveformIndex < MAX_DRAWABLES; i++) {
    if (drawables[i].type != DRAWABLE_WAVEFORM) {
      continue;
    }
    if (!drawables[i].base.enabled) {
      // Reset smoothed buffer so re-enable starts fresh
      memset(state->smoothedWaveform[waveformIndex], 0,
             sizeof(state->smoothedWaveform[waveformIndex]));
      waveformIndex++;
      continue;
    }
    const float alpha = drawables[i].waveform.waveformMotionScale;
    float *smoothed = state->smoothedWaveform[waveformIndex];
    for (int s = 0; s < WAVEFORM_SAMPLES; s++) {
      smoothed[s] += alpha * (state->waveform[s] - smoothed[s]);
    }
    // Rescale smoothed buffer to match raw waveform peak amplitude
    // (EMA on phase-shifting signals averages toward zero without this)
    float rawPeak = 0.0f;
    float smoothedPeak = 0.0f;
    for (int s = 0; s < WAVEFORM_SAMPLES; s++) {
      const float ra =
          state->waveform[s] > 0.0f ? state->waveform[s] : -state->waveform[s];
      const float sa = smoothed[s] > 0.0f ? smoothed[s] : -smoothed[s];
      if (ra > rawPeak) {
        rawPeak = ra;
      }
      if (sa > smoothedPeak) {
        smoothedPeak = sa;
      }
    }
    if (smoothedPeak > 0.0001f && rawPeak > 0.0001f) {
      const float scale = rawPeak / smoothedPeak;
      for (int s = 0; s < WAVEFORM_SAMPLES; s++) {
        smoothed[s] *= scale;
      }
    }
    ProcessWaveformSmooth(smoothed, state->waveformExtended[waveformIndex],
                          drawables[i].waveform.smoothness);
    waveformIndex++;
  }

  state->globalTick++;
}

void DrawableProcessSpectrum(DrawableState *state, const float *magnitude,
                             int binCount, const Drawable *drawables,
                             int count) {
  // Process all enabled spectrum drawables with independent smoothing
  int spectrumIndex = 0;
  for (int i = 0; i < count && spectrumIndex < MAX_DRAWABLES; i++) {
    if (drawables[i].type != DRAWABLE_SPECTRUM) {
      continue;
    }
    // Lazy allocate SpectrumBars on first use
    if (state->spectrumBars[spectrumIndex] == NULL) {
      state->spectrumBars[spectrumIndex] = SpectrumBarsInit();
    }
    if (drawables[i].base.enabled) {
      SpectrumBarsProcess(state->spectrumBars[spectrumIndex], magnitude,
                          binCount, &drawables[i]);
    }
    spectrumIndex++;
  }
}

// Check common draw conditions (enabled, interval, opacity threshold)
// Returns opacity if drawable should render, -1.0f otherwise
static float DrawableShouldRender(DrawableState *state, const Drawable *d,
                                  int drawableIndex, uint64_t tick) {
  const float opacityThreshold = 0.001f;

  if (!d->base.enabled) {
    return -1.0f;
  }

  const uint8_t interval = d->base.drawInterval;
  const uint64_t lastTick = state->lastDrawTick[drawableIndex];
  if (interval > 0 && lastTick > 0 && lastTick < tick &&
      (tick - lastTick) < interval) {
    return -1.0f;
  }

  const float opacity = d->base.opacity;
  if (opacity < opacityThreshold) {
    return -1.0f;
  }

  return opacity;
}

static void DrawableRenderParametricTrail(RenderContext *ctx, Drawable *d,
                                          uint64_t tick, float opacity) {
  (void)tick;
  ParametricTrailData &trail = d->parametricTrail;

  // Compute cursor position via dual-harmonic Lissajous
  const float deltaTime = GetFrameTime();
  float offsetX, offsetY;
  DualLissajousUpdate(&trail.lissajous, deltaTime, 0.0f, &offsetX, &offsetY);
  const float x = d->base.x + offsetX;
  const float y = d->base.y + offsetY;

  // Draw gate check
  bool shouldDraw = true;
  if (trail.gateFreq > 0.0f) {
    const float gatePhase = fmodf((float)GetTime() * trail.gateFreq, 1.0f);
    shouldDraw = gatePhase < 0.5f;
  }

  if (!shouldDraw) {
    return;
  }

  // Convert to screen coordinates
  const Vector2 pos = {x * ctx->screenW, y * ctx->screenH};
  const float t = fmodf(trail.lissajous.phase, 1.0f);
  const Color color = ColorFromConfig(&d->base.color, t, opacity);

  // Map shape type to polygon sides
  int sides;
  switch (trail.shapeType) {
  case TRAIL_SHAPE_TRIANGLE:
    sides = 3;
    break;
  case TRAIL_SHAPE_SQUARE:
    sides = 4;
    break;
  case TRAIL_SHAPE_PENTAGON:
    sides = 5;
    break;
  case TRAIL_SHAPE_HEXAGON:
    sides = 6;
    break;
  case TRAIL_SHAPE_CIRCLE:
  default:
    sides = 32;
    break;
  }

  const float radius = trail.size * 0.5f;
  const float rotation = d->rotationAccum * RAD2DEG;

  if (trail.filled) {
    DrawPoly(pos, sides, radius, rotation, color);
  } else {
    DrawPolyLinesEx(pos, sides, radius, rotation, trail.strokeThickness, color);
  }
}

void DrawableRenderFull(DrawableState *state, RenderContext *ctx,
                        const Drawable *drawables, int count, uint64_t tick) {
  // Two-pass rendering: shapes first, then waveforms/spectrum.
  // Matches MilkDrop draw order so textured shapes sample accumulated waveform
  // trails.

  // Pass 1: Render shapes only
  for (int i = 0; i < count; i++) {
    if (drawables[i].type != DRAWABLE_SHAPE) {
      continue;
    }

    const float opacity = DrawableShouldRender(state, &drawables[i], i, tick);
    if (opacity < 0.0f) {
      continue;
    }

    if (drawables[i].shape.textured) {
      ShapeDrawTextured(ctx, &drawables[i], tick, opacity);
    } else {
      ShapeDrawSolid(ctx, &drawables[i], tick, opacity);
    }
    state->lastDrawTick[i] = tick;
  }

  // Pass 2: Render waveforms and spectrum
  int waveformIndex = 0;
  int spectrumIndex = 0;

  for (int i = 0; i < count; i++) {
    // Track indices for all waveforms/spectrum regardless of draw state
    const int thisWaveformIndex =
        (drawables[i].type == DRAWABLE_WAVEFORM) ? waveformIndex++ : -1;
    const int thisSpectrumIndex =
        (drawables[i].type == DRAWABLE_SPECTRUM) ? spectrumIndex++ : -1;

    if (drawables[i].type == DRAWABLE_SHAPE) {
      continue;
    }

    const float opacity = DrawableShouldRender(state, &drawables[i], i, tick);
    if (opacity < 0.0f) {
      continue;
    }

    switch (drawables[i].type) {
    case DRAWABLE_WAVEFORM:
      DrawableRenderWaveform(state, ctx, &drawables[i], thisWaveformIndex, tick,
                             opacity);
      break;
    case DRAWABLE_SPECTRUM:
      DrawableRenderSpectrum(state, ctx, &drawables[i], thisSpectrumIndex, tick,
                             opacity);
      break;
    case DRAWABLE_SHAPE:
      break; // Already rendered in pass 1
    case DRAWABLE_PARAMETRIC_TRAIL:
      // Cast to non-const: DrawableRenderParametricTrail updates trail state
      DrawableRenderParametricTrail(ctx, const_cast<Drawable *>(&drawables[i]),
                                    tick, opacity);
      break;
    }
    state->lastDrawTick[i] = tick;
  }
}

bool DrawableValidate(const Drawable *drawables, int count) {
  (void)drawables;
  return count <= MAX_DRAWABLES;
}

uint64_t DrawableGetTick(const DrawableState *state) {
  return state->globalTick;
}

int DrawableCountByType(const Drawable *drawables, int count,
                        DrawableType type) {
  int result = 0;
  for (int i = 0; i < count; i++) {
    if (drawables[i].type == type) {
      result++;
    }
  }
  return result;
}

bool DrawableHasType(const Drawable *drawables, int count, DrawableType type) {
  for (int i = 0; i < count; i++) {
    if (drawables[i].type == type) {
      return true;
    }
  }
  return false;
}

void DrawableTickRotations(Drawable *drawables, int count, float deltaTime) {
  for (int i = 0; i < count; i++) {
    drawables[i].rotationAccum += drawables[i].base.rotationSpeed * deltaTime;
    if (drawables[i].type == DRAWABLE_WAVEFORM) {
      drawables[i].colorShiftAccum +=
          drawables[i].waveform.colorShiftSpeed * deltaTime;
    } else if (drawables[i].type == DRAWABLE_SPECTRUM) {
      drawables[i].colorShiftAccum +=
          drawables[i].spectrum.colorShiftSpeed * deltaTime;
    }
  }
}
