#include "beat.h"
#include "audio.h"
#include <math.h>
#include <string.h>

// IIR low-pass filter coefficient for ~200Hz cutoff at 48kHz
// alpha = 2 * pi * fc / (fs + 2 * pi * fc) where fc=200, fs=48000
static const float LOW_PASS_ALPHA = 0.026f;

// Intensity decay rate per second
static const float INTENSITY_DECAY = 4.0f;

void BeatDetectorInit(BeatDetector* bd)
{
    bd->algorithm = BEAT_ALGO_LOWPASS;

    memset(bd->energyHistory, 0, sizeof(bd->energyHistory));
    bd->historyIndex = 0;
    bd->averageEnergy = 0.0f;
    bd->currentEnergy = 0.0f;
    bd->lowPassState = 0.0f;

    bd->beatDetected = false;
    bd->beatIntensity = 0.0f;
    bd->timeSinceLastBeat = BEAT_DEBOUNCE_SEC;

    bd->sensitivity = 1.5f;

    memset(bd->graphHistory, 0, sizeof(bd->graphHistory));
    bd->graphIndex = 0;
}

void BeatDetectorProcess(BeatDetector* bd, const float* samples, int frameCount, float deltaTime)
{
    if (frameCount == 0) {
        bd->beatDetected = false;
        return;
    }

    // Apply low-pass filter and compute energy
    float energy = 0.0f;
    for (int i = 0; i < frameCount; i++) {
        // Mix stereo to mono
        float sample = (samples[i * AUDIO_CHANNELS] + samples[i * AUDIO_CHANNELS + 1]) * 0.5f;

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

    // Update debounce timer
    bd->timeSinceLastBeat += deltaTime;

    // Beat detection
    bd->beatDetected = false;
    float threshold = bd->averageEnergy * bd->sensitivity;

    if (energy > threshold && bd->timeSinceLastBeat >= BEAT_DEBOUNCE_SEC && bd->averageEnergy > 0.0001f) {
        bd->beatDetected = true;
        bd->timeSinceLastBeat = 0.0f;

        // Compute intensity as normalized excess over threshold
        float excess = (energy - bd->averageEnergy) / bd->averageEnergy;
        bd->beatIntensity = fminf(1.0f, excess);
    } else {
        // Decay intensity
        bd->beatIntensity -= INTENSITY_DECAY * deltaTime;
        if (bd->beatIntensity < 0.0f) {
            bd->beatIntensity = 0.0f;
        }
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
