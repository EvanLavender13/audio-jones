#ifndef FFT_H
#define FFT_H

#include <stdbool.h>

#define FFT_SIZE 2048
#define FFT_BIN_COUNT (FFT_SIZE / 2 + 1)  // 1025 bins

typedef struct FFTProcessor FFTProcessor;

// Lifecycle
FFTProcessor* FFTProcessorInit(void);
void FFTProcessorUninit(FFTProcessor* fft);

// Feed audio samples (stereo interleaved, converted to mono internally)
// Returns number of frames consumed (may be less than frameCount if buffer fills)
int FFTProcessorFeed(FFTProcessor* fft, const float* samples, int frameCount);

// Process FFT when enough samples accumulated (returns true if spectrum updated)
bool FFTProcessorUpdate(FFTProcessor* fft);

// Access results (valid after Update returns true)
const float* FFTProcessorGetMagnitude(const FFTProcessor* fft);
int FFTProcessorGetBinCount(const FFTProcessor* fft);
float FFTProcessorGetBinFrequency(int bin, float sampleRate);

#endif
