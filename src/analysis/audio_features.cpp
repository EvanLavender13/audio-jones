#include "audio_features.h"
#include "smoothing.h"
#include <math.h>
#include <string.h>

// Epsilon for log calculations to avoid log(0)
static const float LOG_EPSILON = 1e-10f;

// Rolloff threshold (85% of total energy)
static const float ROLLOFF_THRESHOLD = 0.85f;

// Crest factor normalization (typical max ~6:1 for music)
static const float CREST_NORMALIZE = 6.0f;

void AudioFeaturesInit(AudioFeatures* features)
{
    memset(features, 0, sizeof(AudioFeatures));
}

void AudioFeaturesProcess(AudioFeatures* features, const float* magnitude,
                          int binCount, const float* samples, int sampleCount,
                          float dt)
{
    if (magnitude == NULL || binCount == 0) {
        return;
    }

    // --- Spectral Flatness ---
    // Ratio of geometric mean to arithmetic mean
    float logSum = 0.0f;
    float arithmeticSum = 0.0f;
    for (int k = 1; k < binCount; k++) {
        logSum += logf(magnitude[k] + LOG_EPSILON);
        arithmeticSum += magnitude[k];
    }
    const int N = binCount - 1;  // Skip DC bin
    if (arithmeticSum > MIN_DENOM) {
        float geometricMean = expf(logSum / (float)N);
        float arithmeticMean = arithmeticSum / (float)N;
        features->flatness = geometricMean / arithmeticMean;
    } else {
        features->flatness = 0.0f;
    }
    ApplyEnvelope(&features->flatnessSmooth, features->flatness, dt, FEATURE_ATTACK_TIME, FEATURE_RELEASE_TIME);
    UpdateRunningAvg(&features->flatnessAvg, features->flatness);

    // --- Spectral Spread ---
    // Standard deviation around centroid
    float weightedSum = 0.0f;
    float totalMag = 0.0f;
    for (int k = 1; k < binCount; k++) {
        weightedSum += (float)k * magnitude[k];
        totalMag += magnitude[k];
    }
    if (totalMag > MIN_DENOM) {
        float centroid = weightedSum / totalMag;
        float varianceSum = 0.0f;
        for (int k = 1; k < binCount; k++) {
            float diff = (float)k - centroid;
            varianceSum += magnitude[k] * diff * diff;
        }
        float spread = sqrtf(varianceSum / totalMag);
        // Normalize by N/2 for 0-1 range
        features->spread = fminf(spread / (float)(binCount / 2), 1.0f);
    } else {
        features->spread = 0.0f;
    }
    ApplyEnvelope(&features->spreadSmooth, features->spread, dt, FEATURE_ATTACK_TIME, FEATURE_RELEASE_TIME);
    UpdateRunningAvg(&features->spreadAvg, features->spread);

    // --- Spectral Rolloff ---
    // Find bin where 85% of energy is concentrated
    float totalEnergy = 0.0f;
    for (int k = 1; k < binCount; k++) {
        totalEnergy += magnitude[k] * magnitude[k];
    }
    float threshold = ROLLOFF_THRESHOLD * totalEnergy;
    float cumulative = 0.0f;
    int rolloffBin = binCount - 1;
    for (int k = 1; k < binCount; k++) {
        cumulative += magnitude[k] * magnitude[k];
        if (cumulative >= threshold) {
            rolloffBin = k;
            break;
        }
    }
    features->rolloff = (float)rolloffBin / (float)(binCount - 1);
    ApplyEnvelope(&features->rolloffSmooth, features->rolloff, dt, FEATURE_ATTACK_TIME, FEATURE_RELEASE_TIME);
    UpdateRunningAvg(&features->rolloffAvg, features->rolloff);

    // --- Full-band Spectral Flux ---
    // Positive-only difference from previous frame
    float flux = 0.0f;
    for (int k = 1; k < binCount; k++) {
        float diff = magnitude[k] - features->prevMagnitude[k];
        if (diff > 0.0f) {
            flux += diff;
        }
        features->prevMagnitude[k] = magnitude[k];
    }
    // Self-calibrate: normalize by running average
    UpdateRunningAvg(&features->fluxAvg, flux);
    if (features->fluxAvg > MIN_DENOM) {
        features->flux = fminf(flux / (features->fluxAvg * 3.0f), 1.0f);
    } else {
        features->flux = 0.0f;
    }
    ApplyEnvelope(&features->fluxSmooth, features->flux, dt, FEATURE_ATTACK_TIME, FEATURE_RELEASE_TIME);

    // --- Crest Factor ---
    // Peak-to-RMS ratio of time-domain signal
    if (samples != NULL && sampleCount > 0) {
        float peak = 0.0f;
        float sumSquared = 0.0f;
        for (int i = 0; i < sampleCount; i++) {
            float absSample = fabsf(samples[i]);
            if (absSample > peak) {
                peak = absSample;
            }
            sumSquared += samples[i] * samples[i];
        }
        float rms = sqrtf(sumSquared / (float)sampleCount);
        if (rms > MIN_DENOM) {
            float crestRaw = peak / rms;
            features->crest = fminf(crestRaw / CREST_NORMALIZE, 1.0f);
        } else {
            features->crest = 0.0f;
        }
    } else {
        features->crest = 0.0f;
    }
    ApplyEnvelope(&features->crestSmooth, features->crest, dt, FEATURE_ATTACK_TIME, FEATURE_RELEASE_TIME);
    UpdateRunningAvg(&features->crestAvg, features->crest);
}
