#include "beat.h"
#include <math.h>
#include <string.h>

// Exponential decay rate: fraction remaining after 1 second
static const float INTENSITY_DECAY_RATE = 0.001f;

// Kick drum frequency bin range (47-140 Hz at 48kHz/2048 FFT = 23.4 Hz/bin)
static const int KICK_BIN_START = 2; // ~47 Hz
static const int KICK_BIN_END = 6;   // ~140 Hz

// Compute spectral flux in kick frequency band
// Returns flux (positive magnitude changes in kick range)
static float ComputeKickBandFlux(const float *magnitude,
                                 const float *prevMagnitude, int binCount) {
  float flux = 0.0f;
  for (int k = KICK_BIN_START; k <= KICK_BIN_END && k < binCount; k++) {
    const float diff = magnitude[k] - prevMagnitude[k];
    if (diff > 0.0f) {
      flux += diff;
    }
  }
  return flux;
}

// Compute rolling average and standard deviation from flux history
static void UpdateFluxStatistics(BeatDetector *bd) {
  float fluxSum = 0.0f;
  for (int i = 0; i < BEAT_HISTORY_SIZE; i++) {
    fluxSum += bd->fluxHistory[i];
  }
  bd->fluxAverage = fluxSum / (float)BEAT_HISTORY_SIZE;

  float varianceSum = 0.0f;
  for (int i = 0; i < BEAT_HISTORY_SIZE; i++) {
    const float diff = bd->fluxHistory[i] - bd->fluxAverage;
    varianceSum += diff * diff;
  }
  bd->fluxStdDev = sqrtf(varianceSum / (float)BEAT_HISTORY_SIZE);
}

void BeatDetectorInit(BeatDetector *bd) {
  memset(bd->magnitude, 0, sizeof(bd->magnitude));
  memset(bd->prevMagnitude, 0, sizeof(bd->prevMagnitude));

  memset(bd->fluxHistory, 0, sizeof(bd->fluxHistory));
  bd->historyIndex = 0;
  bd->fluxAverage = 0.0f;
  bd->fluxStdDev = 0.0f;

  bd->beatDetected = false;
  bd->beatIntensity = 0.0f;
  bd->timeSinceLastBeat = BEAT_DEBOUNCE_SEC;

  memset(bd->graphHistory, 0, sizeof(bd->graphHistory));
  bd->graphIndex = 0;
}

void BeatDetectorProcess(BeatDetector *bd, const float *magnitude, int binCount,
                         float deltaTime) {
  bd->beatDetected = false;

  if (magnitude == NULL || binCount == 0) {
    bd->beatIntensity *= powf(INTENSITY_DECAY_RATE, deltaTime);
    return;
  }

  memcpy(bd->prevMagnitude, bd->magnitude, sizeof(bd->magnitude));

  const int copyCount = (binCount < FFT_BIN_COUNT) ? binCount : FFT_BIN_COUNT;
  memcpy(bd->magnitude, magnitude, (size_t)copyCount * sizeof(float));

  const float flux =
      ComputeKickBandFlux(bd->magnitude, bd->prevMagnitude, copyCount);

  bd->fluxHistory[bd->historyIndex] = flux;
  bd->historyIndex = (bd->historyIndex + 1) % BEAT_HISTORY_SIZE;

  UpdateFluxStatistics(bd);

  bd->timeSinceLastBeat += deltaTime;

  const float threshold = bd->fluxAverage + 2.0f * bd->fluxStdDev;

  if (flux > threshold && bd->timeSinceLastBeat >= BEAT_DEBOUNCE_SEC &&
      bd->fluxAverage > 0.001f) {
    bd->beatDetected = true;
    bd->timeSinceLastBeat = 0.0f;

    const float excess = (flux - bd->fluxAverage) / (bd->fluxStdDev + 0.0001f);
    bd->beatIntensity = fminf(1.0f, excess / 4.0f);
  }

  if (!bd->beatDetected) {
    bd->beatIntensity *= powf(INTENSITY_DECAY_RATE, deltaTime);
  }

  bd->graphHistory[bd->graphIndex] = bd->beatIntensity;
  bd->graphIndex = (bd->graphIndex + 1) % BEAT_GRAPH_SIZE;
}

bool BeatDetectorGetBeat(const BeatDetector *bd) { return bd->beatDetected; }

float BeatDetectorGetIntensity(const BeatDetector *bd) {
  return bd->beatIntensity;
}
