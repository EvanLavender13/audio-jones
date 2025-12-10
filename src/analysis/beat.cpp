#include "beat.h"
#include <math.h>
#include <string.h>

// Exponential decay rate: fraction remaining after 1 second
static const float INTENSITY_DECAY_RATE = 0.001f;

// Kick drum frequency bin range (20-200 Hz at 48kHz/2048 FFT = 23.4 Hz/bin)
static const int KICK_BIN_START = 1;   // Skip DC, ~23 Hz
static const int KICK_BIN_END = 8;     // Up to ~188 Hz

// Compute spectral flux and bass energy from magnitude spectrum
// Returns flux (positive magnitude changes) via return value, bassEnergy via out param
static float ComputeSpectralFlux(const float* magnitude, const float* prevMagnitude,
                                  int binCount, float* bassEnergy)
{
    float flux = 0.0f;
    float energy = 0.0f;
    for (int k = KICK_BIN_START; k <= KICK_BIN_END && k < binCount; k++) {
        const float diff = magnitude[k] - prevMagnitude[k];
        if (diff > 0.0f) {
            flux += diff;
        }
        energy += magnitude[k] * magnitude[k];
    }
    *bassEnergy = sqrtf(energy);  // RMS-like measure
    return flux;
}

void BeatDetectorInit(BeatDetector* bd)
{
    memset(bd->magnitude, 0, sizeof(bd->magnitude));
    memset(bd->prevMagnitude, 0, sizeof(bd->prevMagnitude));

    memset(bd->fluxHistory, 0, sizeof(bd->fluxHistory));
    bd->historyIndex = 0;
    bd->fluxAverage = 0.0f;
    bd->fluxStdDev = 0.0f;

    memset(bd->bassHistory, 0, sizeof(bd->bassHistory));
    bd->bassAverage = 0.0f;

    bd->beatDetected = false;
    bd->beatIntensity = 0.0f;
    bd->timeSinceLastBeat = BEAT_DEBOUNCE_SEC;

    memset(bd->graphHistory, 0, sizeof(bd->graphHistory));
    bd->graphIndex = 0;
}

void BeatDetectorProcess(BeatDetector* bd, const float* magnitude, int binCount,
                         float deltaTime, float sensitivity)
{
    bd->beatDetected = false;

    if (magnitude == NULL || binCount == 0) {
        // Still decay intensity when no new data
        bd->beatIntensity *= powf(INTENSITY_DECAY_RATE, deltaTime);
        return;
    }

    // Save previous magnitude for flux calculation
    memcpy(bd->prevMagnitude, bd->magnitude, sizeof(bd->magnitude));

    // Copy new magnitude (clamp to buffer size)
    const int copyCount = (binCount < BEAT_SPECTRUM_SIZE) ? binCount : BEAT_SPECTRUM_SIZE;
    memcpy(bd->magnitude, magnitude, (size_t)copyCount * sizeof(float));

    // Compute spectral flux and bass energy in kick frequencies
    float bassEnergy = 0.0f;
    const float flux = ComputeSpectralFlux(bd->magnitude, bd->prevMagnitude, copyCount, &bassEnergy);

    // Update flux and bass history (shared index)
    bd->fluxHistory[bd->historyIndex] = flux;
    bd->bassHistory[bd->historyIndex] = bassEnergy;
    bd->historyIndex = (bd->historyIndex + 1) % BEAT_HISTORY_SIZE;

    // Compute rolling averages
    float fluxSum = 0.0f;
    float bassSum = 0.0f;
    for (int i = 0; i < BEAT_HISTORY_SIZE; i++) {
        fluxSum += bd->fluxHistory[i];
        bassSum += bd->bassHistory[i];
    }
    bd->fluxAverage = fluxSum / (float)BEAT_HISTORY_SIZE;
    bd->bassAverage = bassSum / (float)BEAT_HISTORY_SIZE;

    // Compute flux standard deviation
    float varianceSum = 0.0f;
    for (int i = 0; i < BEAT_HISTORY_SIZE; i++) {
        const float diff = bd->fluxHistory[i] - bd->fluxAverage;
        varianceSum += diff * diff;
    }
    bd->fluxStdDev = sqrtf(varianceSum / (float)BEAT_HISTORY_SIZE);

    // Update debounce timer
    bd->timeSinceLastBeat += deltaTime;

    // Beat detection: flux exceeds N standard deviations above mean
    const float threshold = bd->fluxAverage + sensitivity * bd->fluxStdDev;

    if (flux > threshold && bd->timeSinceLastBeat >= BEAT_DEBOUNCE_SEC && bd->fluxAverage > 0.001f) {
        bd->beatDetected = true;
        bd->timeSinceLastBeat = 0.0f;

        // Compute intensity as normalized excess over threshold
        const float excess = (flux - bd->fluxAverage) / (bd->fluxStdDev + 0.0001f);
        bd->beatIntensity = fminf(1.0f, excess / (sensitivity * 2.0f));
    }

    // Exponential decay when no beat
    if (!bd->beatDetected) {
        bd->beatIntensity *= powf(INTENSITY_DECAY_RATE, deltaTime);
    }

    // Update graph history
    bd->graphHistory[bd->graphIndex] = bd->beatIntensity;
    bd->graphIndex = (bd->graphIndex + 1) % BEAT_GRAPH_SIZE;
}

bool BeatDetectorGetBeat(const BeatDetector* bd)
{
    return bd->beatDetected;
}

float BeatDetectorGetIntensity(const BeatDetector* bd)
{
    return bd->beatIntensity;
}
