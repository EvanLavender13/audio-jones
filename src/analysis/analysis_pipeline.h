#ifndef ANALYSIS_PIPELINE_H
#define ANALYSIS_PIPELINE_H

#include "fft.h"
#include "beat.h"
#include "bands.h"
#include "audio/audio.h"

typedef struct AnalysisPipeline {
    FFTProcessor fft;
    BeatDetector beat;
    BandEnergies bands;
    float audioBuffer[AUDIO_MAX_FRAMES_PER_UPDATE * AUDIO_CHANNELS];
    float peakLevel;
    uint32_t lastFramesRead;
} AnalysisPipeline;

bool AnalysisPipelineInit(AnalysisPipeline* pipeline);
void AnalysisPipelineUninit(AnalysisPipeline* pipeline);

void AnalysisPipelineProcess(AnalysisPipeline* pipeline,
                             AudioCapture* capture,
                             float deltaTime);

#endif // ANALYSIS_PIPELINE_H
