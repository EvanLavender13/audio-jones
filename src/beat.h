#ifndef BEAT_H
#define BEAT_H

#include <stdbool.h>
#include <kiss_fftr.h>

#define BEAT_FFT_SIZE 2048            // FFT window size (43ms at 48kHz)
#define BEAT_SPECTRUM_SIZE (BEAT_FFT_SIZE / 2 + 1)  // Real FFT output bins
#define BEAT_HISTORY_SIZE 43          // ~860ms rolling average at 20Hz update rate
#define BEAT_GRAPH_SIZE 64            // Number of samples in beat graph display
#define BEAT_DEBOUNCE_SEC 0.15f       // Minimum seconds between beats

typedef struct BeatDetector {
    // FFT state
    kiss_fftr_cfg fftConfig;
    float sampleBuffer[BEAT_FFT_SIZE];    // Accumulated samples for FFT
    int sampleCount;                       // Current samples in buffer
    float windowedSamples[BEAT_FFT_SIZE]; // Windowed input for FFT
    kiss_fft_cpx spectrum[BEAT_SPECTRUM_SIZE];
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

// Initialize beat detector (allocates FFT config)
// Returns false if FFT allocation fails
bool BeatDetectorInit(BeatDetector* bd);

// Free beat detector resources
void BeatDetectorUninit(BeatDetector* bd);

// Process audio samples and update beat detection state
// samples: interleaved stereo float samples
// frameCount: number of frames (sample pairs)
// deltaTime: time since last call in seconds
// sensitivity: threshold multiplier (standard deviations above mean)
void BeatDetectorProcess(BeatDetector* bd, const float* samples, int frameCount,
                         float deltaTime, float sensitivity);

// Returns true if a beat was detected this frame
bool BeatDetectorGetBeat(const BeatDetector* bd);

// Returns current beat intensity (0.0-1.0, decays after beat)
float BeatDetectorGetIntensity(const BeatDetector* bd);

#endif // BEAT_H
