#ifndef RADIAL_PULSE_H
#define RADIAL_PULSE_H

#include "raylib.h"
#include <stdbool.h>

// Radial Pulse with angular modulation
// Creates pulsating radial waves with segmented angular distortion.
// Supports petal shapes, spiral twisting, and multi-octave layering.
// depthBlend samples color at each octave for layered effect; disable for
// single final sample.
struct RadialPulseConfig {
  bool enabled = false;
  float radialFreq = 8.0f;     // Radial wave frequency
  float radialAmp = 0.05f;     // Radial wave amplitude
  int segments = 6;            // Angular segment count
  float angularAmp = 0.1f;     // Angular distortion amplitude
  float petalAmp = 0.0f;       // Petal shape amplitude
  float phaseSpeed = 1.0f;     // Animation rate (radians/second)
  float spiralTwist = 0.0f;    // Spiral twist per radius
  int octaves = 1;             // Number of cascade octaves
  float octaveRotation = 0.0f; // Rotation per octave in radians
  bool depthBlend = false;     // true=sample each octave, false=sample once
};

typedef struct RadialPulseEffect {
  Shader shader;
  int radialFreqLoc;
  int radialAmpLoc;
  int segmentsLoc;
  int angularAmpLoc;
  int petalAmpLoc;
  int phaseLoc;
  int spiralTwistLoc;
  int octavesLoc;
  int octaveRotationLoc;
  int depthBlendLoc;
  float time; // Animation accumulator
} RadialPulseEffect;

// Returns true on success, false if shader fails to load
bool RadialPulseEffectInit(RadialPulseEffect *e);

// Accumulates time, sets all uniforms
void RadialPulseEffectSetup(RadialPulseEffect *e, const RadialPulseConfig *cfg,
                            float deltaTime);

// Unloads shader
void RadialPulseEffectUninit(RadialPulseEffect *e);

// Returns default config
RadialPulseConfig RadialPulseConfigDefault(void);

// Registers modulatable params with the modulation engine
void RadialPulseRegisterParams(RadialPulseConfig *cfg);

#endif // RADIAL_PULSE_H
