#ifndef BANDS_H
#define BANDS_H

// Frequency band bin ranges (48kHz sample rate, 2048 FFT size = 23.4 Hz/bin)
// Matches MilkDrop band definitions
#define BAND_BASS_START   1     // Skip DC
#define BAND_BASS_END     10    // 20-250 Hz
#define BAND_MID_START    11
#define BAND_MID_END      170   // 250-4000 Hz
#define BAND_TREB_START   171
#define BAND_TREB_END     853   // 4000-20000 Hz

// Attack/release time constants (seconds)
#define BAND_ATTACK_TIME   0.010f   // 10ms - captures transients
#define BAND_RELEASE_TIME  0.150f   // 150ms - prevents jitter

typedef struct BandEnergies {
    // Raw RMS energy per band (unsmoothed)
    float bass;
    float mid;
    float treb;

    // Attack/release smoothed values
    float bassSmooth;
    float midSmooth;
    float trebSmooth;

    // Running averages for normalization (slow decay)
    float bassAvg;
    float midAvg;
    float trebAvg;
} BandEnergies;

// Initialize band energies to zero
void BandEnergiesInit(BandEnergies* bands);

// Process magnitude spectrum to extract band energies
// magnitude: FFT magnitude bins from FFTProcessorGetMagnitude()
// binCount: number of bins
// dt: time since last call in seconds
void BandEnergiesProcess(BandEnergies* bands, const float* magnitude,
                         int binCount, float dt);

#endif // BANDS_H
