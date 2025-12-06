#ifndef BEAT_H
#define BEAT_H

#include <stdbool.h>

#define BEAT_HISTORY_SIZE 43      // ~860ms rolling average at 20Hz update rate
#define BEAT_GRAPH_SIZE 64        // Number of samples in beat graph display
#define BEAT_DEBOUNCE_SEC 0.15f   // Minimum seconds between beats

typedef enum {
    BEAT_ALGO_LOWPASS,  // Simple IIR low-pass filter + energy threshold
    // Future: BEAT_ALGO_FFT, BEAT_ALGO_ONSET, etc.
} BeatAlgorithm;

typedef struct BeatDetector {
    BeatAlgorithm algorithm;
    // Detection state
    float energyHistory[BEAT_HISTORY_SIZE];
    int historyIndex;
    float averageEnergy;
    float currentEnergy;
    float lowPassState;

    // Beat state
    bool beatDetected;
    float beatIntensity;
    float timeSinceLastBeat;

    // Parameters
    float sensitivity;

    // Visualization
    float graphHistory[BEAT_GRAPH_SIZE];
    int graphIndex;
} BeatDetector;

// Initialize beat detector with default values
void BeatDetectorInit(BeatDetector* bd);

// Process audio samples and update beat detection state
// samples: interleaved stereo float samples
// frameCount: number of frames (sample pairs)
// deltaTime: time since last call in seconds
void BeatDetectorProcess(BeatDetector* bd, const float* samples, int frameCount, float deltaTime);

// Returns true if a beat was detected this frame
bool BeatDetectorGetBeat(const BeatDetector* bd);

// Returns current beat intensity (0.0-1.0, decays after beat)
float BeatDetectorGetIntensity(const BeatDetector* bd);

#endif // BEAT_H
