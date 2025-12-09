#include "fft.h"
#include "audio.h"
#include <kiss_fftr.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Hann window (shared, initialized once)
static float hannWindow[SPECTRAL_FFT_SIZE];
static bool hannInitialized = false;

static void InitHannWindow(void)
{
    if (hannInitialized) return;
    for (int i = 0; i < SPECTRAL_FFT_SIZE; i++) {
        hannWindow[i] = 0.5f * (1.0f - cosf(2.0f * 3.14159265359f * (float)i / (float)SPECTRAL_FFT_SIZE));
    }
    hannInitialized = true;
}

struct SpectralProcessor {
    kiss_fftr_cfg fftConfig;
    float sampleBuffer[SPECTRAL_FFT_SIZE];
    int sampleCount;
    float windowedSamples[SPECTRAL_FFT_SIZE];
    kiss_fft_cpx spectrum[SPECTRAL_BIN_COUNT];
    float magnitude[SPECTRAL_BIN_COUNT];
};

SpectralProcessor* SpectralProcessorInit(void)
{
    InitHannWindow();

    SpectralProcessor* sp = (SpectralProcessor*)malloc(sizeof(SpectralProcessor));
    if (sp == NULL) return NULL;

    sp->fftConfig = kiss_fftr_alloc(SPECTRAL_FFT_SIZE, 0, NULL, NULL);
    if (sp->fftConfig == NULL) {
        free(sp);
        return NULL;
    }

    sp->sampleCount = 0;
    memset(sp->sampleBuffer, 0, sizeof(sp->sampleBuffer));
    memset(sp->windowedSamples, 0, sizeof(sp->windowedSamples));
    memset(sp->spectrum, 0, sizeof(sp->spectrum));
    memset(sp->magnitude, 0, sizeof(sp->magnitude));

    return sp;
}

void SpectralProcessorUninit(SpectralProcessor* sp)
{
    if (sp == NULL) return;
    if (sp->fftConfig != NULL) {
        kiss_fftr_free(sp->fftConfig);
    }
    free(sp);
}

void SpectralProcessorFeed(SpectralProcessor* sp, const float* samples, int frameCount)
{
    if (sp == NULL || samples == NULL) return;

    // Accumulate mono samples (stereo to mono conversion)
    for (int i = 0; i < frameCount && sp->sampleCount < SPECTRAL_FFT_SIZE; i++) {
        float mono = (samples[(size_t)i * AUDIO_CHANNELS] +
                      samples[(size_t)i * AUDIO_CHANNELS + 1]) * 0.5f;
        sp->sampleBuffer[sp->sampleCount++] = mono;
    }
}

bool SpectralProcessorUpdate(SpectralProcessor* sp)
{
    if (sp == NULL) return false;

    // Only process when buffer is full
    if (sp->sampleCount < SPECTRAL_FFT_SIZE) return false;

    // Apply Hann window
    for (int i = 0; i < SPECTRAL_FFT_SIZE; i++) {
        sp->windowedSamples[i] = sp->sampleBuffer[i] * hannWindow[i];
    }

    // Execute real-to-complex FFT
    kiss_fftr(sp->fftConfig, sp->windowedSamples, sp->spectrum);

    // Compute magnitude spectrum
    for (int k = 0; k < SPECTRAL_BIN_COUNT; k++) {
        float re = sp->spectrum[k].r;
        float im = sp->spectrum[k].i;
        sp->magnitude[k] = sqrtf(re * re + im * im);
    }

    // Overlapping window: keep 75%, hop 25% (512 samples at ~94Hz update rate)
    int keep = SPECTRAL_FFT_SIZE * 3 / 4;  // 1536 samples
    int hop = SPECTRAL_FFT_SIZE - keep;     // 512 samples
    memmove(sp->sampleBuffer, sp->sampleBuffer + hop, (size_t)keep * sizeof(float));
    sp->sampleCount = keep;

    return true;
}

const float* SpectralProcessorGetMagnitude(const SpectralProcessor* sp)
{
    if (sp == NULL) return NULL;
    return sp->magnitude;
}

int SpectralProcessorGetBinCount(const SpectralProcessor* sp)
{
    (void)sp;
    return SPECTRAL_BIN_COUNT;
}

float SpectralProcessorGetBinFrequency(int bin, float sampleRate)
{
    return (float)bin * sampleRate / (float)SPECTRAL_FFT_SIZE;
}
