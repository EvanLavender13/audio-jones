#ifndef FFT_H
#define FFT_H

#include <stdbool.h>
#include <kiss_fftr.h>

#define FFT_SIZE 2048
#define FFT_BIN_COUNT (FFT_SIZE / 2 + 1)  // 1025 bins
#define FFT_HOP_SIZE (FFT_SIZE / 4)       // 512 samples (75% overlap)

typedef struct FFTProcessor {
    kiss_fftr_cfg fftConfig;
    float sampleBuffer[FFT_SIZE];
    int sampleCount;
    float windowedSamples[FFT_SIZE];
    kiss_fft_cpx spectrum[FFT_BIN_COUNT];
    float magnitude[FFT_BIN_COUNT];
} FFTProcessor;

// Lifecycle (init-in-place pattern for embedding in structs)
bool FFTProcessorInit(FFTProcessor* fft);
void FFTProcessorUninit(FFTProcessor* fft);

// Feed audio samples (stereo interleaved, converted to mono internally)
// Returns number of frames consumed (may be less than frameCount if buffer fills)
int FFTProcessorFeed(FFTProcessor* fft, const float* samples, int frameCount);

// Process FFT when enough samples accumulated (returns true if spectrum updated)
bool FFTProcessorUpdate(FFTProcessor* fft);

#endif
