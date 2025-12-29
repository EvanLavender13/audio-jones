#include "drawable.h"
#include <string.h>

// Render single waveform drawable
static void DrawableRenderWaveform(const DrawableState* state,
                                   RenderContext* ctx,
                                   const Drawable* d,
                                   int index,
                                   uint64_t tick,
                                   float opacity)
{
    if (d->path == PATH_CIRCULAR) {
        DrawWaveformCircular(
            (float*)state->waveformExtended[index],
            WAVEFORM_EXTENDED,
            ctx,
            d,
            tick,
            opacity);
    } else {
        DrawWaveformLinear(
            state->waveformExtended[index],
            WAVEFORM_SAMPLES,
            ctx,
            d,
            tick,
            opacity);
    }
}

// Render single spectrum drawable
static void DrawableRenderSpectrum(const DrawableState* state,
                                   RenderContext* ctx,
                                   const Drawable* d,
                                   uint64_t tick,
                                   float opacity)
{
    if (d->path == PATH_CIRCULAR) {
        SpectrumBarsDrawCircular(state->spectrumBars, ctx, d, tick, opacity);
    } else {
        SpectrumBarsDrawLinear(state->spectrumBars, ctx, d, tick, opacity);
    }
}

void DrawableStateInit(DrawableState* state)
{
    memset(state, 0, sizeof(DrawableState));
    state->spectrumBars = SpectrumBarsInit();
}

void DrawableStateUninit(DrawableState* state)
{
    if (state->spectrumBars != NULL) {
        SpectrumBarsUninit(state->spectrumBars);
        state->spectrumBars = NULL;
    }
}

void DrawableProcessWaveforms(DrawableState* state,
                              const float* audioBuffer,
                              uint32_t framesRead,
                              const Drawable* drawables,
                              int count,
                              ChannelMode channelMode)
{
    // Process base waveform from audio
    ProcessWaveformBase(audioBuffer, framesRead, state->waveform, channelMode);

    // Apply per-drawable smoothing (increment index for all waveforms to match render order)
    int waveformIndex = 0;
    for (int i = 0; i < count && waveformIndex < MAX_WAVEFORMS; i++) {
        if (drawables[i].type != DRAWABLE_WAVEFORM) {
            continue;
        }
        if (drawables[i].base.enabled) {
            ProcessWaveformSmooth(
                state->waveform,
                state->waveformExtended[waveformIndex],
                drawables[i].waveform.smoothness);
        }
        waveformIndex++;
    }

    state->globalTick++;
}

void DrawableProcessSpectrum(DrawableState* state,
                             const float* magnitude,
                             int binCount,
                             const Drawable* drawables,
                             int count)
{
    // Find first enabled spectrum drawable
    for (int i = 0; i < count; i++) {
        if (drawables[i].type == DRAWABLE_SPECTRUM && drawables[i].base.enabled) {
            SpectrumBarsProcess(state->spectrumBars, magnitude, binCount, &drawables[i]);
            return;
        }
    }
}

// Core render loop: fixedOpacity < 0 uses phase-based opacity, >= 0 uses that value
static void DrawableRenderCore(const DrawableState* state,
                               RenderContext* ctx,
                               const Drawable* drawables,
                               int count,
                               uint64_t tick,
                               bool isPreFeedback,
                               float fixedOpacity)
{
    const float opacityThreshold = 0.001f;
    int waveformIndex = 0;

    for (int i = 0; i < count; i++) {
        if (drawables[i].type == DRAWABLE_WAVEFORM) {
            if (!drawables[i].base.enabled) {
                waveformIndex++;
                continue;
            }
        } else if (!drawables[i].base.enabled) {
            continue;
        }

        float opacity;
        if (fixedOpacity >= 0.0f) {
            opacity = fixedOpacity;
        } else {
            opacity = isPreFeedback
                ? (1.0f - drawables[i].base.feedbackPhase)
                : drawables[i].base.feedbackPhase;
            if (opacity < opacityThreshold) {
                if (drawables[i].type == DRAWABLE_WAVEFORM) {
                    waveformIndex++;
                }
                continue;
            }
        }

        switch (drawables[i].type) {
            case DRAWABLE_WAVEFORM:
                DrawableRenderWaveform(state, ctx, &drawables[i], waveformIndex, tick, opacity);
                waveformIndex++;
                break;
            case DRAWABLE_SPECTRUM:
                DrawableRenderSpectrum(state, ctx, &drawables[i], tick, opacity);
                break;
            case DRAWABLE_SHAPE:
                if (drawables[i].shape.textured) {
                    ShapeDrawTextured(ctx, &drawables[i], tick, opacity);
                } else {
                    ShapeDrawSolid(ctx, &drawables[i], tick, opacity);
                }
                break;
        }
    }
}

void DrawableRenderAll(const DrawableState* state,
                       RenderContext* ctx,
                       const Drawable* drawables,
                       int count,
                       uint64_t tick,
                       bool isPreFeedback)
{
    DrawableRenderCore(state, ctx, drawables, count, tick, isPreFeedback, -1.0f);
}

void DrawableRenderFull(const DrawableState* state,
                        RenderContext* ctx,
                        const Drawable* drawables,
                        int count,
                        uint64_t tick)
{
    DrawableRenderCore(state, ctx, drawables, count, tick, false, 1.0f);
}

bool DrawableValidate(const Drawable* drawables, int count)
{
    int spectrumCount = 0;
    int waveformCount = 0;
    int shapeCount = 0;

    for (int i = 0; i < count; i++) {
        switch (drawables[i].type) {
            case DRAWABLE_SPECTRUM: spectrumCount++; break;
            case DRAWABLE_WAVEFORM: waveformCount++; break;
            case DRAWABLE_SHAPE: shapeCount++; break;
        }
    }

    // Enforce limits
    if (spectrumCount > 1) {
        return false;
    }
    if (waveformCount > MAX_WAVEFORMS) {
        return false;
    }
    if (shapeCount > MAX_SHAPES) {
        return false;
    }

    return true;
}

uint64_t DrawableGetTick(const DrawableState* state)
{
    return state->globalTick;
}
