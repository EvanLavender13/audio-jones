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
        const float absVal = fabsf(buffer[i]);
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

    const float gain = 1.0f / *peakLevel;
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
                             float deltaTime)
{
    if (pipeline == NULL || capture == NULL) {
        return;
    }

    const uint32_t available = AudioCaptureAvailable(capture);
    if (available == 0) {
        BeatDetectorProcess(&pipeline->beat, NULL, 0, deltaTime);
        return;
    }

    uint32_t framesToRead = available;
    if (framesToRead > AUDIO_MAX_FRAMES_PER_UPDATE) {
        framesToRead = AUDIO_MAX_FRAMES_PER_UPDATE;
    }

    pipeline->lastFramesRead = AudioCaptureRead(capture, pipeline->audioBuffer, framesToRead);
    if (pipeline->lastFramesRead == 0) {
        BeatDetectorProcess(&pipeline->beat, NULL, 0, deltaTime);
        return;
    }

    NormalizeAudioBuffer(pipeline->audioBuffer, pipeline->lastFramesRead * AUDIO_CHANNELS, &pipeline->peakLevel);

    // Audio time per FFT hop (not frame time) for consistent beat detection timing
    // NOLINTNEXTLINE(bugprone-integer-division) - both operands explicitly cast to float
    const float audioHopTime = (float)FFT_HOP_SIZE / (float)AUDIO_SAMPLE_RATE;

    uint32_t offset = 0;
    bool hadFFTUpdate = false;
    while (offset < pipeline->lastFramesRead) {
        const int consumed = FFTProcessorFeed(&pipeline->fft, pipeline->audioBuffer + (size_t)offset * AUDIO_CHANNELS, pipeline->lastFramesRead - offset);
        offset += consumed;
        if (FFTProcessorUpdate(&pipeline->fft)) {
            hadFFTUpdate = true;
            BeatDetectorProcess(&pipeline->beat, pipeline->fft.magnitude, FFT_BIN_COUNT, audioHopTime);
            BandEnergiesProcess(&pipeline->bands, pipeline->fft.magnitude, FFT_BIN_COUNT, audioHopTime);
        }
    }

    if (!hadFFTUpdate) {
        BeatDetectorProcess(&pipeline->beat, NULL, 0, deltaTime);
    }
}
