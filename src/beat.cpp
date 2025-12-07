#include "beat.h"
#include "audio.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>

// Exponential decay rate: fraction remaining after 1 second
static const float INTENSITY_DECAY_RATE = 0.001f;

// Precomputed Hann window coefficients
static float hannWindow[BEAT_FFT_SIZE];
static bool hannInitialized = false;

static void InitHannWindow(void)
{
    if (hannInitialized) return;
    for (int i = 0; i < BEAT_FFT_SIZE; i++) {
        hannWindow[i] = 0.5f * (1.0f - cosf(2.0f * 3.14159265359f * (float)i / (float)BEAT_FFT_SIZE));
    }
    hannInitialized = true;
}

void BeatDetectorInit(BeatDetector* bd)
{
    InitHannWindow();

    // Allocate FFT configuration (forward transform)
    bd->fftConfig = kiss_fftr_alloc(BEAT_FFT_SIZE, 0, NULL, NULL);

    bd->sampleCount = 0;
    memset(bd->sampleBuffer, 0, sizeof(bd->sampleBuffer));
    memset(bd->windowedSamples, 0, sizeof(bd->windowedSamples));
    memset(bd->spectrum, 0, sizeof(bd->spectrum));
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

void BeatDetectorUninit(BeatDetector* bd)
{
    if (bd->fftConfig) {
        kiss_fftr_free(bd->fftConfig);
        bd->fftConfig = NULL;
    }
}

void BeatDetectorProcess(BeatDetector* bd, const float* samples, int frameCount,
                         float deltaTime, float sensitivity)
{
    bd->beatDetected = false;

    if (frameCount == 0) {
        return;
    }

    // Accumulate mono samples into buffer
    for (int i = 0; i < frameCount && bd->sampleCount < BEAT_FFT_SIZE; i++) {
        float mono = (samples[(size_t)i * AUDIO_CHANNELS] +
                      samples[(size_t)i * AUDIO_CHANNELS + 1]) * 0.5f;
        bd->sampleBuffer[bd->sampleCount++] = mono;
    }

    // Process FFT when buffer is full
    if (bd->sampleCount >= BEAT_FFT_SIZE) {
        // Apply Hann window
        for (int i = 0; i < BEAT_FFT_SIZE; i++) {
            bd->windowedSamples[i] = bd->sampleBuffer[i] * hannWindow[i];
        }

        // Execute real-to-complex FFT
        kiss_fftr(bd->fftConfig, bd->windowedSamples, bd->spectrum);

        // Save previous magnitude for flux calculation
        memcpy(bd->prevMagnitude, bd->magnitude, sizeof(bd->magnitude));

        // Compute magnitude spectrum
        for (int k = 0; k < BEAT_SPECTRUM_SIZE; k++) {
            float re = bd->spectrum[k].r;
            float im = bd->spectrum[k].i;
            bd->magnitude[k] = sqrtf(re * re + im * im);
        }

        // Compute spectral flux AND energy in kick frequencies (20-200 Hz)
        // At 48kHz with 2048 FFT: bin resolution = 23.4 Hz
        // Bin 1 = ~23 Hz, Bin 8 = ~188 Hz
        const int kickBinStart = 1;   // Skip DC
        const int kickBinEnd = 8;     // Up to ~200 Hz
        float flux = 0.0f;
        float bassEnergy = 0.0f;
        for (int k = kickBinStart; k <= kickBinEnd; k++) {
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

        // Shift buffer: keep second half for overlap
        int overlap = BEAT_FFT_SIZE / 2;
        memmove(bd->sampleBuffer, bd->sampleBuffer + overlap, (size_t)overlap * sizeof(float));
        bd->sampleCount = overlap;
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
