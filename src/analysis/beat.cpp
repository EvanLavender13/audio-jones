#include "beat.h"
#include <math.h>
#include <string.h>

// Exponential decay rate: fraction remaining after 1 second
static const float INTENSITY_DECAY_RATE = 0.001f;

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
    int copyCount = (binCount < BEAT_SPECTRUM_SIZE) ? binCount : BEAT_SPECTRUM_SIZE;
    memcpy(bd->magnitude, magnitude, (size_t)copyCount * sizeof(float));

    // Compute spectral flux AND energy in kick frequencies (20-200 Hz)
    // At 48kHz with 2048 FFT: bin resolution = 23.4 Hz
    // Bin 1 = ~23 Hz, Bin 8 = ~188 Hz
    const int kickBinStart = 1;   // Skip DC
    const int kickBinEnd = 8;     // Up to ~200 Hz
    float flux = 0.0f;
    float bassEnergy = 0.0f;
    for (int k = kickBinStart; k <= kickBinEnd && k < copyCount; k++) {
        float diff = bd->magnitude[k] - bd->prevMagnitude[k];
        if (diff > 0.0f) {
            flux += diff;
        }
        bassEnergy += bd->magnitude[k] * bd->magnitude[k];
    }
    bassEnergy = sqrtf(bassEnergy);  // RMS-like measure

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
        float diff = bd->fluxHistory[i] - bd->fluxAverage;
        varianceSum += diff * diff;
    }
    bd->fluxStdDev = sqrtf(varianceSum / (float)BEAT_HISTORY_SIZE);

    // Update debounce timer
    bd->timeSinceLastBeat += deltaTime;

    // Beat detection: flux exceeds N standard deviations above mean
    float threshold = bd->fluxAverage + sensitivity * bd->fluxStdDev;

    if (flux > threshold && bd->timeSinceLastBeat >= BEAT_DEBOUNCE_SEC && bd->fluxAverage > 0.001f) {
        bd->beatDetected = true;
        bd->timeSinceLastBeat = 0.0f;

        // Compute intensity as normalized excess over threshold
        float excess = (flux - bd->fluxAverage) / (bd->fluxStdDev + 0.0001f);
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
