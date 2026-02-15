#ifndef TONE_WARP_H
#define TONE_WARP_H

#include "raylib.h"
#include <stdbool.h>

// Tone Warp: audio-reactive radial displacement
// Maps FFT semitones to screen radius with standard audio params.
// Angular segments create bidirectional push/pull patterns.
struct ToneWarpConfig {
  bool enabled = false;
  float intensity = 0.1f;       // Displacement strength (0.0 - 1.0)
  int numOctaves = 5;           // FFT octave count (1 - 8)
  float baseFreq = 55.0f;       // Lowest frequency Hz (27.5 - 440.0)
  float gain = 2.0f;            // FFT gain (0.1 - 10.0)
  float curve = 0.7f;           // Contrast curve (0.1 - 3.0)
  float baseBright = 0.0f;      // Base brightness floor (0.0 - 1.0)
  float maxRadius = 0.7f;       // Screen radius at octave ceiling (0.1 - 1.0)
  int segments = 4;             // Angular push/pull divisions (1 - 16)
  float pushPullBalance = 0.5f; // Pull <-> push bias (0.0 - 1.0)
  float pushPullSmoothness = 0.0f; // Hard <-> smooth edges (0.0 - 1.0)
  float phaseSpeed = 0.0f;         // Auto-rotate speed (radians/second)
};

#define TONE_WARP_CONFIG_FIELDS                                                \
  enabled, intensity, numOctaves, baseFreq, gain, curve, baseBright,           \
      maxRadius, segments, pushPullBalance, pushPullSmoothness, phaseSpeed

typedef struct ToneWarpEffect {
  Shader shader;
  int resolutionLoc;
  int fftTextureLoc;
  int intensityLoc;
  int sampleRateLoc;
  int baseFreqLoc;
  int numOctavesLoc;
  int gainLoc;
  int curveLoc;
  int baseBrightLoc;
  int maxRadiusLoc;
  int segmentsLoc;
  int pushPullBalanceLoc;
  int pushPullSmoothnessLoc;
  int phaseOffsetLoc;
  float phaseAccum; // Auto-rotation accumulator
} ToneWarpEffect;

// Returns true on success, false if shader fails to load
bool ToneWarpEffectInit(ToneWarpEffect *e);

// Accumulates phase, binds FFT texture, sets all uniforms
void ToneWarpEffectSetup(ToneWarpEffect *e, const ToneWarpConfig *cfg,
                         float deltaTime, int screenWidth, int screenHeight,
                         Texture2D fftTexture);

// Unloads shader
void ToneWarpEffectUninit(ToneWarpEffect *e);

// Returns default config
ToneWarpConfig ToneWarpConfigDefault(void);

// Registers modulatable params with the modulation engine
void ToneWarpRegisterParams(ToneWarpConfig *cfg);

#endif // TONE_WARP_H
