#include "fft.h"
#include "audio/audio.h"
#include <kiss_fftr.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Hann window (shared, initialized once)
static float hannWindow[FFT_SIZE];
static bool hannInitialized = false;

static void InitHannWindow(void)
{
    if (hannInitialized) return;
    for (int i = 0; i < FFT_SIZE; i++) {
        hannWindow[i] = 0.5f * (1.0f - cosf(2.0f * 3.14159265359f * (float)i / (float)FFT_SIZE));
    }
    hannInitialized = true;
}

struct FFTProcessor {
    kiss_fftr_cfg fftConfig;
    float sampleBuffer[FFT_SIZE];
    int sampleCount;
    float windowedSamples[FFT_SIZE];
    kiss_fft_cpx spectrum[FFT_BIN_COUNT];
    float magnitude[FFT_BIN_COUNT];
};

FFTProcessor* FFTProcessorInit(void)
{
    InitHannWindow();

    FFTProcessor* fft = (FFTProcessor*)malloc(sizeof(FFTProcessor));
    if (fft == NULL) return NULL;

    fft->fftConfig = kiss_fftr_alloc(FFT_SIZE, 0, NULL, NULL);
    if (fft->fftConfig == NULL) {
        free(fft);
        return NULL;
    }

    fft->sampleCount = 0;
    memset(fft->sampleBuffer, 0, sizeof(fft->sampleBuffer));
    memset(fft->windowedSamples, 0, sizeof(fft->windowedSamples));
    memset(fft->spectrum, 0, sizeof(fft->spectrum));
    memset(fft->magnitude, 0, sizeof(fft->magnitude));

    return fft;
}

void FFTProcessorUninit(FFTProcessor* fft)
{
    if (fft == NULL) return;
    if (fft->fftConfig != NULL) {
        kiss_fftr_free(fft->fftConfig);
    }
    free(fft);
}

void FFTProcessorFeed(FFTProcessor* fft, const float* samples, int frameCount)
{
    if (fft == NULL || samples == NULL) return;

    // Accumulate mono samples (stereo to mono conversion)
    for (int i = 0; i < frameCount && fft->sampleCount < FFT_SIZE; i++) {
        float mono = (samples[(size_t)i * AUDIO_CHANNELS] +
                      samples[(size_t)i * AUDIO_CHANNELS + 1]) * 0.5f;
        fft->sampleBuffer[fft->sampleCount++] = mono;
    }
}

bool FFTProcessorUpdate(FFTProcessor* fft)
{
    if (fft == NULL) return false;

    // Only process when buffer is full
    if (fft->sampleCount < FFT_SIZE) return false;

    // Apply Hann window
    for (int i = 0; i < FFT_SIZE; i++) {
        fft->windowedSamples[i] = fft->sampleBuffer[i] * hannWindow[i];
    }

    // Execute real-to-complex FFT
    kiss_fftr(fft->fftConfig, fft->windowedSamples, fft->spectrum);

    // Compute magnitude spectrum
    for (int k = 0; k < FFT_BIN_COUNT; k++) {
        float re = fft->spectrum[k].r;
        float im = fft->spectrum[k].i;
        fft->magnitude[k] = sqrtf(re * re + im * im);
    }

    // Overlapping window: keep 75%, hop 25% (512 samples at ~94Hz update rate)
    int keep = FFT_SIZE * 3 / 4;  // 1536 samples
    int hop = FFT_SIZE - keep;     // 512 samples
    memmove(fft->sampleBuffer, fft->sampleBuffer + hop, (size_t)keep * sizeof(float));
    fft->sampleCount = keep;

    return true;
}

const float* FFTProcessorGetMagnitude(const FFTProcessor* fft)
{
    if (fft == NULL) return NULL;
    return fft->magnitude;
}

int FFTProcessorGetBinCount(const FFTProcessor* fft)
{
    (void)fft;
    return FFT_BIN_COUNT;
}

float FFTProcessorGetBinFrequency(int bin, float sampleRate)
{
    return (float)bin * sampleRate / (float)FFT_SIZE;
}
