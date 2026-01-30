#include "bands.h"
#include "audio/audio.h"
#include "fft.h"
#include "smoothing.h"
#include <math.h>
#include <string.h>

// Centroid Hz remapping range (musical content typically falls here)
static const float CENTROID_MIN_HZ = 200.0f;
static const float CENTROID_MAX_HZ = 8000.0f;
static const float HZ_PER_BIN = (float)AUDIO_SAMPLE_RATE / (float)FFT_SIZE;

// Compute RMS energy for a range of FFT bins
static float ComputeBandRMS(const float *magnitude, int binStart, int binEnd,
                            int binCount) {
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

void BandEnergiesInit(BandEnergies *bands) {
  memset(bands, 0, sizeof(BandEnergies));
}

void BandEnergiesProcess(BandEnergies *bands, const float *magnitude,
                         int binCount, float dt) {
  if (magnitude == NULL || binCount == 0) {
    return;
  }

  // Extract raw RMS energy per band
  bands->bass =
      ComputeBandRMS(magnitude, BAND_BASS_START, BAND_BASS_END, binCount);
  bands->mid =
      ComputeBandRMS(magnitude, BAND_MID_START, BAND_MID_END, binCount);
  bands->treb =
      ComputeBandRMS(magnitude, BAND_TREB_START, BAND_TREB_END, binCount);

  // Apply attack/release smoothing
  ApplyEnvelope(&bands->bassSmooth, bands->bass, dt, BAND_ATTACK_TIME,
                BAND_RELEASE_TIME);
  ApplyEnvelope(&bands->midSmooth, bands->mid, dt, BAND_ATTACK_TIME,
                BAND_RELEASE_TIME);
  ApplyEnvelope(&bands->trebSmooth, bands->treb, dt, BAND_ATTACK_TIME,
                BAND_RELEASE_TIME);

  // Compute spectral centroid (weighted average of bin indices, remapped to
  // musical Hz range)
  float weightedSum = 0.0f;
  float totalEnergy = 0.0f;
  for (int i = 1; i < binCount; i++) {
    weightedSum += (float)i * magnitude[i];
    totalEnergy += magnitude[i];
  }
  if (totalEnergy > MIN_DENOM) {
    const float binIndex = weightedSum / totalEnergy;
    const float centroidHz = binIndex * HZ_PER_BIN;
    const float centroidNorm =
        (centroidHz - CENTROID_MIN_HZ) / (CENTROID_MAX_HZ - CENTROID_MIN_HZ);
    bands->centroid = fminf(fmaxf(centroidNorm, 0.0f), 1.0f);
  } else {
    bands->centroid = 0.0f;
  }
  ApplyEnvelope(&bands->centroidSmooth, bands->centroid, dt, BAND_ATTACK_TIME,
                BAND_RELEASE_TIME);

  // Update running averages for normalization
  UpdateRunningAvg(&bands->bassAvg, bands->bass);
  UpdateRunningAvg(&bands->midAvg, bands->mid);
  UpdateRunningAvg(&bands->trebAvg, bands->treb);
  UpdateRunningAvg(&bands->centroidAvg, bands->centroid);
}
