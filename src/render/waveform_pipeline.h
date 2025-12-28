#ifndef WAVEFORM_PIPELINE_H
#define WAVEFORM_PIPELINE_H

#include "waveform.h"
#include "render_context.h"
#include "audio/audio_config.h"
#include "config/waveform_config.h"
#include <stdint.h>

typedef struct WaveformPipeline {
    float waveform[WAVEFORM_SAMPLES];
    float waveformExtended[MAX_WAVEFORMS][WAVEFORM_EXTENDED];
    uint64_t globalTick;
} WaveformPipeline;

void WaveformPipelineInit(WaveformPipeline* wp);
void WaveformPipelineUninit(WaveformPipeline* wp);

// Process audio into smoothed waveforms (call at visual update rate)
void WaveformPipelineProcess(WaveformPipeline* wp,
                             const float* audioBuffer,
                             uint32_t framesRead,
                             const WaveformConfig* configs,
                             int configCount,
                             ChannelMode channelMode);

// Draw all waveforms (linear or circular based on mode)
// opacity: 0.0-1.0 alpha multiplier for split-pass rendering
void WaveformPipelineDraw(WaveformPipeline* wp,
                          RenderContext* ctx,
                          const WaveformConfig* configs,
                          int configCount,
                          bool circular,
                          float opacity);

#endif // WAVEFORM_PIPELINE_H
