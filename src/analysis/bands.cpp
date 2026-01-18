#include "bands.h"
#include "audio/audio.h"
#include "fft.h"
#include <math.h>
#include <string.h>

// Running average decay factor (0.999 = ~1 second time constant at 60Hz)
static const float AVG_DECAY = 0.999f;
static const float AVG_ATTACK = 0.001f;

// Minimum denominator to avoid division by zero
static const float MIN_DENOM = 1e-6f;

// Centroid Hz remapping range (musical content typically falls here)
static const float CENTROID_MIN_HZ = 200.0f;
static const float CENTROID_MAX_HZ = 8000.0f;
static const float HZ_PER_BIN = (float)AUDIO_SAMPLE_RATE / (float)FFT_SIZE;

// Compute RMS energy for a range of FFT bins
static float ComputeBandRMS(const float* magnitude, int binStart, int binEnd, int binCount)
{
    // Clamp range to available bins
    if (binStart >= binCount) {
        return 0.0f;
    }
    if (binEnd > binCount) {
        binEnd = binCount;
    }

    float sumSquared = 0.0f;
    const int count = binEnd - binStart;
    if (count <= 0) {
        return 0.0f;
    }

    for (int i = binStart; i < binEnd; i++) {
        sumSquared += magnitude[i] * magnitude[i];
    }

    return sqrtf(sumSquared / (float)count);
}

// Apply attack/release envelope follower
// Returns smoothed value, updates *smoothed in place
static void ApplyEnvelope(float* smoothed, float raw, float dt)
{
    const float tau = (raw > *smoothed) ? BAND_ATTACK_TIME : BAND_RELEASE_TIME;
    const float alpha = 1.0f - expf(-dt / tau);
    *smoothed += alpha * (raw - *smoothed);
}

void BandEnergiesInit(BandEnergies* bands)
{
    memset(bands, 0, sizeof(BandEnergies));
}

void BandEnergiesProcess(BandEnergies* bands, const float* magnitude,
                         int binCount, float dt)
{
    if (magnitude == NULL || binCount == 0) {
        return;
    }

    // Extract raw RMS energy per band
    bands->bass = ComputeBandRMS(magnitude, BAND_BASS_START, BAND_BASS_END, binCount);
    bands->mid = ComputeBandRMS(magnitude, BAND_MID_START, BAND_MID_END, binCount);
    bands->treb = ComputeBandRMS(magnitude, BAND_TREB_START, BAND_TREB_END, binCount);

    // Apply attack/release smoothing
    ApplyEnvelope(&bands->bassSmooth, bands->bass, dt);
    ApplyEnvelope(&bands->midSmooth, bands->mid, dt);
    ApplyEnvelope(&bands->trebSmooth, bands->treb, dt);

    // Compute spectral centroid (weighted average of bin indices, remapped to musical Hz range)
    float weightedSum = 0.0f;
    float totalEnergy = 0.0f;
    for (int i = 1; i < binCount; i++) {
        weightedSum += (float)i * magnitude[i];
        totalEnergy += magnitude[i];
    }
    if (totalEnergy > MIN_DENOM) {
        float binIndex = weightedSum / totalEnergy;
        float centroidHz = binIndex * HZ_PER_BIN;
        float centroidNorm = (centroidHz - CENTROID_MIN_HZ) / (CENTROID_MAX_HZ - CENTROID_MIN_HZ);
        bands->centroid = fminf(fmaxf(centroidNorm, 0.0f), 1.0f);
    } else {
        bands->centroid = 0.0f;
    }
    ApplyEnvelope(&bands->centroidSmooth, bands->centroid, dt);

    // Update running averages for normalization
    bands->bassAvg = bands->bassAvg * AVG_DECAY + bands->bass * AVG_ATTACK;
    bands->midAvg = bands->midAvg * AVG_DECAY + bands->mid * AVG_ATTACK;
    bands->trebAvg = bands->trebAvg * AVG_DECAY + bands->treb * AVG_ATTACK;
    bands->centroidAvg = bands->centroidAvg * AVG_DECAY + bands->centroid * AVG_ATTACK;
}
