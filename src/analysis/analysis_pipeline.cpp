#include "analysis_pipeline.h"
#include <math.h>
#include <string.h>

static void NormalizeAudioBuffer(float* buffer, uint32_t sampleCount, float* peakLevel)
{
    const float ATTACK = 0.3f;
    const float RELEASE = 0.999f;
    const float MIN_PEAK = 0.0001f;

    float bufferPeak = 0.0f;
    for (uint32_t i = 0; i < sampleCount; i++) {
        float absVal = fabsf(buffer[i]);
        if (absVal > bufferPeak) {
            bufferPeak = absVal;
        }
    }

    if (bufferPeak > *peakLevel) {
        *peakLevel += ATTACK * (bufferPeak - *peakLevel);
    } else {
        *peakLevel *= RELEASE;
    }

    if (*peakLevel < MIN_PEAK) {
        *peakLevel = MIN_PEAK;
    }

    float gain = 1.0f / *peakLevel;
    for (uint32_t i = 0; i < sampleCount; i++) {
        buffer[i] *= gain;
    }
}

bool AnalysisPipelineInit(AnalysisPipeline* pipeline)
{
    if (pipeline == NULL) {
        return false;
    }

    if (!FFTProcessorInit(&pipeline->fft)) {
        return false;
    }

    BeatDetectorInit(&pipeline->beat);
    BandEnergiesInit(&pipeline->bands);

    memset(pipeline->audioBuffer, 0, sizeof(pipeline->audioBuffer));
    pipeline->peakLevel = 0.01f;
    pipeline->lastFramesRead = 0;

    return true;
}

void AnalysisPipelineUninit(AnalysisPipeline* pipeline)
{
    if (pipeline == NULL) {
        return;
    }
    FFTProcessorUninit(&pipeline->fft);
}

void AnalysisPipelineProcess(AnalysisPipeline* pipeline,
                             AudioCapture* capture,
                             float beatSensitivity,
                             float deltaTime)
{
    if (pipeline == NULL || capture == NULL) {
        return;
    }

    const uint32_t available = AudioCaptureAvailable(capture);
    if (available == 0) {
        BeatDetectorProcess(&pipeline->beat, NULL, 0, deltaTime, beatSensitivity);
        return;
    }

    uint32_t framesToRead = available;
    if (framesToRead > AUDIO_MAX_FRAMES_PER_UPDATE) {
        framesToRead = AUDIO_MAX_FRAMES_PER_UPDATE;
    }

    pipeline->lastFramesRead = AudioCaptureRead(capture, pipeline->audioBuffer, framesToRead);
    if (pipeline->lastFramesRead == 0) {
        BeatDetectorProcess(&pipeline->beat, NULL, 0, deltaTime, beatSensitivity);
        return;
    }

    NormalizeAudioBuffer(pipeline->audioBuffer, pipeline->lastFramesRead * AUDIO_CHANNELS, &pipeline->peakLevel);

    uint32_t offset = 0;
    bool hadFFTUpdate = false;
    while (offset < pipeline->lastFramesRead) {
        int consumed = FFTProcessorFeed(&pipeline->fft, pipeline->audioBuffer + offset * AUDIO_CHANNELS, pipeline->lastFramesRead - offset);
        offset += consumed;
        if (FFTProcessorUpdate(&pipeline->fft)) {
            hadFFTUpdate = true;
            BeatDetectorProcess(&pipeline->beat, pipeline->fft.magnitude, FFT_BIN_COUNT, deltaTime, beatSensitivity);
            BandEnergiesProcess(&pipeline->bands, pipeline->fft.magnitude, FFT_BIN_COUNT, deltaTime);
        }
    }

    if (!hadFFTUpdate) {
        BeatDetectorProcess(&pipeline->beat, NULL, 0, deltaTime, beatSensitivity);
    }
}
