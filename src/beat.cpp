#include "beat.h"
#include "audio.h"
#include <math.h>
#include <string.h>

// IIR low-pass filter coefficient for ~100Hz cutoff at 48kHz
// alpha = 2 * pi * fc / (fs + 2 * pi * fc) where fc=100, fs=48000
static const float LOW_PASS_ALPHA = 0.013f;

// Exponential decay rate: fraction remaining after 1 second
// 0.001 = 0.1% remains after 1 sec (fast decay with smooth tail)
static const float INTENSITY_DECAY_RATE = 0.001f;

void BeatDetectorInit(BeatDetector* bd)
{
    bd->algorithm = BEAT_ALGO_LOWPASS;

    memset(bd->energyHistory, 0, sizeof(bd->energyHistory));
    bd->historyIndex = 0;
    bd->averageEnergy = 0.0f;
    bd->varianceEnergy = 0.0f;
    bd->currentEnergy = 0.0f;
    bd->lowPassState = 0.0f;

    bd->beatDetected = false;
    bd->beatIntensity = 0.0f;
    bd->timeSinceLastBeat = BEAT_DEBOUNCE_SEC;

    memset(bd->graphHistory, 0, sizeof(bd->graphHistory));
    bd->graphIndex = 0;
}

void BeatDetectorProcess(BeatDetector* bd, const float* samples, int frameCount,
                         float deltaTime, float sensitivity)
{
    if (frameCount == 0) {
        bd->beatDetected = false;
        return;
    }

    // Apply low-pass filter and compute energy
    float energy = 0.0f;
    for (int i = 0; i < frameCount; i++) {
        // Mix stereo to mono
        const float sample = (samples[(size_t)i * AUDIO_CHANNELS] + samples[(size_t)i * AUDIO_CHANNELS + 1]) * 0.5f;

        // IIR low-pass filter
        bd->lowPassState = LOW_PASS_ALPHA * sample + (1.0f - LOW_PASS_ALPHA) * bd->lowPassState;

        // Accumulate squared filtered sample
        energy += bd->lowPassState * bd->lowPassState;
    }
    energy /= (float)frameCount;
    bd->currentEnergy = energy;

    // Update energy history circular buffer
    bd->energyHistory[bd->historyIndex] = energy;
    bd->historyIndex = (bd->historyIndex + 1) % BEAT_HISTORY_SIZE;

    // Compute rolling average
    float sum = 0.0f;
    for (int i = 0; i < BEAT_HISTORY_SIZE; i++) {
        sum += bd->energyHistory[i];
    }
    bd->averageEnergy = sum / (float)BEAT_HISTORY_SIZE;

    // Compute variance for adaptive threshold
    float varianceSum = 0.0f;
    for (int i = 0; i < BEAT_HISTORY_SIZE; i++) {
        float diff = bd->energyHistory[i] - bd->averageEnergy;
        varianceSum += diff * diff;
    }
    bd->varianceEnergy = varianceSum / (float)BEAT_HISTORY_SIZE;

    // Update debounce timer
    bd->timeSinceLastBeat += deltaTime;

    // Beat detection
    bd->beatDetected = false;
    float threshold;
    if (bd->algorithm == BEAT_ALGO_ADAPTIVE) {
        // Variance-based threshold: low variance = high threshold, high variance = low threshold
        // Use normalized variance (coefficient of variation squared) for volume-independent measure
        float normVar = (bd->averageEnergy > 0.0001f)
            ? bd->varianceEnergy / (bd->averageEnergy * bd->averageEnergy)
            : 0.0f;
        // Sensitivity acts as baseline, variance adjusts it down for dynamic music
        float c = -15.0f * normVar + sensitivity;
        c = fmaxf(1.0f, c);  // Floor only, let sensitivity control ceiling
        threshold = bd->averageEnergy * c;
    } else {
        threshold = bd->averageEnergy * sensitivity;
    }

    if (energy > threshold && bd->timeSinceLastBeat >= BEAT_DEBOUNCE_SEC && bd->averageEnergy > 0.0001f) {
        bd->beatDetected = true;
        bd->timeSinceLastBeat = 0.0f;

        // Compute intensity as normalized excess over threshold
        const float excess = (energy - bd->averageEnergy) / bd->averageEnergy;
        bd->beatIntensity = fminf(1.0f, excess);
    } else {
        // Exponential decay: smooth falloff with long tail
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
