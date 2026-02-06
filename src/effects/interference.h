// Interference effect module
// Overlapping wave emitters create ripple patterns via constructive/destructive
// interference

#ifndef INTERFERENCE_H
#define INTERFERENCE_H

#include "config/dual_lissajous_config.h"
#include "raylib.h"
#include "render/blend_mode.h"
#include "render/color_config.h"
#include <stdbool.h>

struct InterferenceConfig {
  bool enabled = false;

  // Sources
  int sourceCount = 3;           // Number of wave emitters (1-8)
  float baseRadius = 0.4f;       // Distance of sources from center (0.0-1.0)
  DualLissajousConfig lissajous; // Source motion pattern

  // Wave properties
  float waveFreq = 30.0f; // Ripple density (5.0-100.0)
  float waveSpeed = 2.0f; // Animation speed (0.0-10.0)

  // Falloff
  int falloffType = 3;          // 0=None, 1=Inverse, 2=InvSquare, 3=Gaussian
  float falloffStrength = 1.0f; // Attenuation rate (0.0-5.0)

  // Boundaries (mirror sources at screen edges)
  bool boundaries = false;
  float reflectionGain = 0.5f; // Mirror source attenuation (0.0-1.0)

  // Visualization
  int visualMode = 0;      // 0=Raw, 1=Absolute, 2=Contour
  int contourCount = 8;    // Band count for contour mode (2-20)
  float visualGain = 1.5f; // Output intensity (0.5-5.0)

  // Color
  int colorMode = 0; // 0=Intensity, 1=PerSource, 2=Chromatic
  float chromaSpread =
      0.03f; // RGB wavelength spread for Chromatic mode (0.0-0.1)
  ColorConfig color = {.mode = COLOR_MODE_GRADIENT};

  // Blend compositing
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f;
};

typedef struct ColorLUT ColorLUT;

typedef struct InterferenceEffect {
  Shader shader;
  ColorLUT *colorLUT;
  float time;
  int resolutionLoc;
  int timeLoc;
  int sourcesLoc;
  int phasesLoc;
  int sourceCountLoc;
  int waveFreqLoc;
  int falloffTypeLoc;
  int falloffStrengthLoc;
  int boundariesLoc;
  int reflectionGainLoc;
  int visualModeLoc;
  int contourCountLoc;
  int visualGainLoc;
  int chromaSpreadLoc;
  int colorModeLoc;
  int colorLUTLoc;
} InterferenceEffect;

// Returns true on success, false if shader fails to load
bool InterferenceEffectInit(InterferenceEffect *e,
                            const InterferenceConfig *cfg);

// Binds all uniforms and advances time accumulator.
// Non-const cfg because Lissajous mutates phase each frame.
void InterferenceEffectSetup(InterferenceEffect *e, InterferenceConfig *cfg,
                             float deltaTime);

// Unloads shader and frees LUT
void InterferenceEffectUninit(InterferenceEffect *e);

// Returns default config
InterferenceConfig InterferenceConfigDefault(void);

// Registers modulatable params with the modulation engine
void InterferenceRegisterParams(InterferenceConfig *cfg);

#endif // INTERFERENCE_H
