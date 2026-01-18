#ifndef AUDIO_FEATURES_H
#define AUDIO_FEATURES_H

#include "fft.h"

// Attack/release time constants (seconds)
#define FEATURE_ATTACK_TIME   0.010f   // 10ms - captures transients
#define FEATURE_RELEASE_TIME  0.150f   // 150ms - prevents jitter

typedef struct AudioFeatures {
    // Spectral Flatness: 0 = pure tone, 1 = white noise
    float flatness;
    float flatnessSmooth;
    float flatnessAvg;

    // Spectral Spread: bandwidth around centroid (0-1 normalized)
    float spread;
    float spreadSmooth;
    float spreadAvg;

    // Spectral Rolloff: normalized bin where 85% energy concentrates (0-1)
    float rolloff;
    float rolloffSmooth;
    float rolloffAvg;

    // Full-band Spectral Flux: onset/activity detection (self-calibrated 0-1)
    float flux;
    float fluxSmooth;
    float fluxAvg;

    // Crest Factor: peak/RMS ratio, normalized (0-1, high = punchy)
    float crest;
    float crestSmooth;
    float crestAvg;

    // Internal state for flux calculation
    float prevMagnitude[FFT_BIN_COUNT];
} AudioFeatures;

void AudioFeaturesInit(AudioFeatures* features);

// Extracts 5 spectral characteristics from FFT magnitude and raw samples.
// Call after FFT processing to populate raw, smooth, and avg fields.
void AudioFeaturesProcess(AudioFeatures* features, const float* magnitude,
                          int binCount, const float* samples, int sampleCount,
                          float dt);

#endif // AUDIO_FEATURES_H
