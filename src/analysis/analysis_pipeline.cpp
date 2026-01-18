#include "analysis_pipeline.h"
#include <math.h>
#include <string.h>

// Instant peak normalization for volume-independent analysis (matches waveform.cpp)
static void NormalizeAudioBuffer(float* buffer, uint32_t sampleCount)
{
    const float MIN_PEAK = 0.0001f;

    float peak = 0.0f;
    for (uint32_t i = 0; i < sampleCount; i++) {
        const float absVal = fabsf(buffer[i]);
        if (absVal > peak) {
            peak = absVal;
        }
    }

    if (peak > MIN_PEAK) {
        const float gain = 1.0f / peak;
        for (uint32_t i = 0; i < sampleCount; i++) {
            buffer[i] *= gain;
        }
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
    AudioFeaturesInit(&pipeline->features);

    memset(pipeline->audioBuffer, 0, sizeof(pipeline->audioBuffer));
    pipeline->lastFramesRead = 0;

    memset(pipeline->waveformHistory, 0, sizeof(pipeline->waveformHistory));
    for (int i = 0; i < WAVEFORM_HISTORY_SIZE; i++) {
        pipeline->waveformHistory[i] = 0.5f;
    }
    pipeline->waveformWriteIndex = 0;
    pipeline->waveformEnvelope = 0.0f;

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

    NormalizeAudioBuffer(pipeline->audioBuffer, pipeline->lastFramesRead * AUDIO_CHANNELS);

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
            AudioFeaturesProcess(&pipeline->features, pipeline->fft.magnitude, FFT_BIN_COUNT,
                                 pipeline->audioBuffer, pipeline->lastFramesRead * AUDIO_CHANNELS,
                                 audioHopTime);
        }
    }

    if (!hadFFTUpdate) {
        BeatDetectorProcess(&pipeline->beat, NULL, 0, deltaTime);
    }
}

void AnalysisPipelineUpdateWaveformHistory(AnalysisPipeline* pipeline)
{
    if (pipeline == NULL) {
        return;
    }

    // Find peak amplitude in this update (preserves dynamics better than average)
    float peakSigned = 0.0f;
    if (pipeline->lastFramesRead > 0) {
        const uint32_t frameCount = pipeline->lastFramesRead;
        float peak = 0.0f;
        for (uint32_t i = 0; i < frameCount; i++) {
            const float left = pipeline->audioBuffer[i * AUDIO_CHANNELS];
            const float right = pipeline->audioBuffer[i * AUDIO_CHANNELS + 1];
            const float mono = (left + right) * 0.5f;
            if (fabsf(mono) > peak) {
                peak = fabsf(mono);
                peakSigned = mono;
            }
        }
    }
    // When no audio, peakSigned stays 0, causing envelope to decay toward silence

    // Smooth the envelope to prevent flicker (~2Hz response)
    const float ALPHA = 0.1f;
    pipeline->waveformEnvelope += ALPHA * (peakSigned - pipeline->waveformEnvelope);

    // Dead zone - snap to silence when near zero to prevent residual flicker
    if (fabsf(pipeline->waveformEnvelope) < 0.01f) {
        pipeline->waveformEnvelope = 0.0f;
    }

    // Store one smoothed value per update
    const float stored = pipeline->waveformEnvelope * 0.5f + 0.5f;
    pipeline->waveformHistory[pipeline->waveformWriteIndex] = stored;
    pipeline->waveformWriteIndex = (pipeline->waveformWriteIndex + 1) % WAVEFORM_HISTORY_SIZE;
}
