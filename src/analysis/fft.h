#ifndef FFT_H
#define FFT_H

#include <stdbool.h>

#define SPECTRAL_FFT_SIZE 2048
#define SPECTRAL_BIN_COUNT (SPECTRAL_FFT_SIZE / 2 + 1)  // 1025 bins

typedef struct SpectralProcessor SpectralProcessor;

// Lifecycle
SpectralProcessor* SpectralProcessorInit(void);
void SpectralProcessorUninit(SpectralProcessor* sp);

// Feed audio samples (stereo interleaved, converted to mono internally)
void SpectralProcessorFeed(SpectralProcessor* sp, const float* samples, int frameCount);

// Process FFT when enough samples accumulated (returns true if spectrum updated)
bool SpectralProcessorUpdate(SpectralProcessor* sp);

// Access results (valid after Update returns true)
const float* SpectralProcessorGetMagnitude(const SpectralProcessor* sp);
int SpectralProcessorGetBinCount(const SpectralProcessor* sp);
float SpectralProcessorGetBinFrequency(int bin, float sampleRate);

#endif
