#ifndef ANALYSIS_PIPELINE_H
#define ANALYSIS_PIPELINE_H

#include "audio/audio.h"
#include "audio_features.h"
#include "bands.h"
#include "beat.h"
#include "fft.h"

#define WAVEFORM_HISTORY_SIZE 2048

typedef struct AnalysisPipeline {
  FFTProcessor fft;
  BeatDetector beat;
  BandEnergies bands;
  AudioFeatures features;
  float audioBuffer[AUDIO_MAX_FRAMES_PER_UPDATE * AUDIO_CHANNELS];
  uint32_t lastFramesRead;
  float waveformHistory[WAVEFORM_HISTORY_SIZE];
  int waveformWriteIndex;
  float waveformEnvelope; // Low-pass filtered envelope for cymatics
} AnalysisPipeline;

bool AnalysisPipelineInit(AnalysisPipeline *pipeline);
void AnalysisPipelineUninit(AnalysisPipeline *pipeline);

void AnalysisPipelineProcess(AnalysisPipeline *pipeline, AudioCapture *capture,
                             float deltaTime);

// Update waveform history for cymatics (call every frame for smooth gradients)
void AnalysisPipelineUpdateWaveformHistory(AnalysisPipeline *pipeline);

#endif // ANALYSIS_PIPELINE_H
