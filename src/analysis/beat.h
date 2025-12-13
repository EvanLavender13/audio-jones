#ifndef BEAT_H
#define BEAT_H

#include <stdbool.h>

#define BEAT_SPECTRUM_SIZE 1025  // Matches FFT_BIN_COUNT
#define BEAT_HISTORY_SIZE 80     // ~850ms rolling average at 94Hz FFT rate
#define BEAT_GRAPH_SIZE 64       // Number of samples in beat graph display
#define BEAT_DEBOUNCE_SEC 0.15f  // Minimum seconds between beats

typedef struct BeatDetector {
    // Magnitude buffers (for spectral flux calculation)
    float magnitude[BEAT_SPECTRUM_SIZE];
    float prevMagnitude[BEAT_SPECTRUM_SIZE];

    // Spectral flux history (onset strength)
    float fluxHistory[BEAT_HISTORY_SIZE];
    int historyIndex;
    float fluxAverage;
    float fluxStdDev;

    // Bass energy history (sustained low-frequency power)
    float bassHistory[BEAT_HISTORY_SIZE];
    float bassAverage;

    // Beat state
    bool beatDetected;
    float beatIntensity;
    float timeSinceLastBeat;

    // Visualization
    float graphHistory[BEAT_GRAPH_SIZE];
    int graphIndex;
} BeatDetector;

// Initialize beat detector state
void BeatDetectorInit(BeatDetector* bd);

// Process magnitude spectrum from FFTProcessor
// magnitude: FFT magnitude bins from FFTProcessorGetMagnitude()
// binCount: number of bins (should be BEAT_SPECTRUM_SIZE)
// deltaTime: time since last call in seconds
void BeatDetectorProcess(BeatDetector* bd, const float* magnitude, int binCount,
                         float deltaTime);

// Returns true if a beat was detected this frame
bool BeatDetectorGetBeat(const BeatDetector* bd);

// Returns current beat intensity (0.0-1.0, decays after beat)
float BeatDetectorGetIntensity(const BeatDetector* bd);

#endif // BEAT_H
