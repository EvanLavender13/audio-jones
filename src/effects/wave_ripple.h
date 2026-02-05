#ifndef WAVE_RIPPLE_H
#define WAVE_RIPPLE_H

#include "raylib.h"
#include <stdbool.h>

#include "config/dual_lissajous_config.h"

// Wave Ripple: pseudo-3D radial wave displacement
// Summed sine waves create height field; gradient displaces UVs for parallax
struct WaveRippleConfig {
  bool enabled = false;
  int octaves = 2;        // Wave octaves (1-4)
  float strength = 0.02f; // UV displacement strength (0.0-0.5)
  float speed = 1.0f;     // Animation rate (radians/second, 0.0-5.0)
  float frequency = 8.0f; // Base wave frequency (1.0-20.0)
  float steepness =
      0.0f;           // Gerstner asymmetry: 0=sine, 1=sharp crests (0.0-1.0)
  float decay = 5.0f; // Amplitude falloff with distance (0.0-50.0)
  float centerHole = 0.0f; // Radius of calm center (0.0-0.5 UV space)
  float originX = 0.5f;    // Wave origin X in UV space (0.0-1.0)
  float originY = 0.5f;    // Wave origin Y in UV space (0.0-1.0)
  DualLissajousConfig originLissajous = {
      .amplitude = 0.0f, // Disabled by default
      .freqX1 = 1.0f,    // Origin X oscillation frequency
      .freqY1 = 1.0f,    // Origin Y oscillation frequency
  };
  bool shadeEnabled = false;   // Height-based brightness modulation
  float shadeIntensity = 0.2f; // Shade strength (0.0-0.5)
};

typedef struct WaveRippleEffect {
  Shader shader;
  int timeLoc;
  int octavesLoc;
  int strengthLoc;
  int frequencyLoc;
  int steepnessLoc;
  int decayLoc;
  int centerHoleLoc;
  int originLoc;
  int shadeEnabledLoc;
  int shadeIntensityLoc;
  float time; // Animation accumulator
} WaveRippleEffect;

// Returns true on success, false if shader fails to load
bool WaveRippleEffectInit(WaveRippleEffect *e);

// Accumulates time, computes origin via Lissajous if enabled, sets all uniforms
// Non-const config: DualLissajousUpdate mutates internal phase state
void WaveRippleEffectSetup(WaveRippleEffect *e, WaveRippleConfig *cfg,
                           float deltaTime);

// Unloads shader
void WaveRippleEffectUninit(WaveRippleEffect *e);

// Returns default config
WaveRippleConfig WaveRippleConfigDefault(void);

// Registers modulatable params with the modulation engine
void WaveRippleRegisterParams(WaveRippleConfig *cfg);

#endif // WAVE_RIPPLE_H
