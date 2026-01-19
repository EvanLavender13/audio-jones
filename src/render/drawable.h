#ifndef DRAWABLE_H
#define DRAWABLE_H

#include "config/drawable_config.h"
#include "render/waveform.h"
#include "render/spectrum_bars.h"
#include "render/shape.h"
#include "render/render_context.h"
#include "audio/audio_config.h"
#include <stdint.h>

#define MAX_DRAWABLES 16

typedef struct DrawableState {
    float waveform[WAVEFORM_SAMPLES];
    float waveformExtended[MAX_DRAWABLES][WAVEFORM_EXTENDED];
    uint64_t globalTick;
    uint64_t lastDrawTick[MAX_DRAWABLES] = {};
    SpectrumBars* spectrumBars[MAX_DRAWABLES];
} DrawableState;

// Lifecycle
void DrawableStateInit(DrawableState* state);
void DrawableStateUninit(DrawableState* state);

// Process audio data into drawable buffers
void DrawableProcessWaveforms(DrawableState* state,
                              const float* audioBuffer,
                              uint32_t framesRead,
                              const Drawable* drawables,
                              int count,
                              ChannelMode channelMode);

void DrawableProcessSpectrum(DrawableState* state,
                             const float* magnitude,
                             int binCount,
                             const Drawable* drawables,
                             int count);

// Render all enabled drawables at their configured opacity
void DrawableRenderFull(DrawableState* state,
                        RenderContext* ctx,
                        const Drawable* drawables,
                        int count,
                        uint64_t tick);

// Validate drawable array (enforces count <= MAX_DRAWABLES)
bool DrawableValidate(const Drawable* drawables, int count);

// Query drawable array by type
int DrawableCountByType(const Drawable* drawables, int count, DrawableType type);
bool DrawableHasType(const Drawable* drawables, int count, DrawableType type);

// Get current tick from waveform pipeline
uint64_t DrawableGetTick(const DrawableState* state);

// Accumulate rotation speeds into rotation offsets (call once per frame)
void DrawableTickRotations(Drawable* drawables, int count, float deltaTime);

#endif // DRAWABLE_H
