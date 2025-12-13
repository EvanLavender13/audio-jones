#include "waveform_pipeline.h"
#include "audio/audio.h"
#include <string.h>

void WaveformPipelineInit(WaveformPipeline* wp)
{
    memset(wp, 0, sizeof(WaveformPipeline));
}

void WaveformPipelineUninit(WaveformPipeline* wp)
{
    (void)wp;
}

void WaveformPipelineProcess(WaveformPipeline* wp,
                             const float* audioBuffer,
                             uint32_t framesRead,
                             const WaveformConfig* configs,
                             int configCount,
                             ChannelMode channelMode)
{
    if (wp == NULL || audioBuffer == NULL || framesRead == 0) {
        return;
    }

    // Waveform uses only the last 1024 frames (most recent audio)
    uint32_t waveformOffset = 0;
    uint32_t waveformFrames = framesRead;
    if (framesRead > AUDIO_BUFFER_FRAMES) {
        waveformOffset = framesRead - AUDIO_BUFFER_FRAMES;
        waveformFrames = AUDIO_BUFFER_FRAMES;
    }

    ProcessWaveformBase(audioBuffer + ((size_t)waveformOffset * AUDIO_CHANNELS),
                        waveformFrames, wp->waveform, channelMode);

    for (int i = 0; i < configCount && i < MAX_WAVEFORMS; i++) {
        ProcessWaveformSmooth(wp->waveform, wp->waveformExtended[i], configs[i].smoothness);
    }

    wp->globalTick++;
}

void WaveformPipelineDraw(WaveformPipeline* wp,
                          RenderContext* ctx,
                          const WaveformConfig* configs,
                          int configCount,
                          bool circular)
{
    if (wp == NULL || ctx == NULL || configs == NULL) {
        return;
    }

    if (circular) {
        for (int i = 0; i < configCount && i < MAX_WAVEFORMS; i++) {
            DrawWaveformCircular(wp->waveformExtended[i], WAVEFORM_EXTENDED, ctx,
                                 (WaveformConfig*)&configs[i], wp->globalTick);
        }
    } else {
        // Linear mode shows only the first waveform - horizontal layout doesn't suit multiple layers
        if (configCount > 0) {
            DrawWaveformLinear(wp->waveformExtended[0], WAVEFORM_SAMPLES, ctx,
                               (WaveformConfig*)&configs[0], wp->globalTick);
        }
    }
}
