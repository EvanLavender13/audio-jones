// Arc strobe effect module
// FFT-driven Lissajous web — octave-mapped line segments with strobe pulsing
// and gradient coloring

#ifndef ARC_STROBE_H
#define ARC_STROBE_H

#include "config/dual_lissajous_config.h"
#include "raylib.h"
#include "render/blend_mode.h"
#include "render/color_config.h"
#include <stdbool.h>

struct ArcStrobeConfig {
  bool enabled = false;

  // Lissajous motion (dual-harmonic with overridden defaults)
  DualLissajousConfig lissajous = {
      .amplitude = 0.5f,
      .freqX1 = 2.0f, // Spatial ratio — creates Lissajous shape
      .freqY1 = 3.0f,
      .offsetY2 = -2.80f, // ~-160 deg
  };

  // Shape
  float orbitOffset = 2.0f;    // Parameter skip to Q (0.01-10.0)
  float lineThickness = 0.01f; // Segment width subtracted from SDF (0.001-0.05)

  // Glow — fixed tight width, glowIntensity is brightness multiplier
  float glowIntensity = 2.0f; // Brightness multiplier (0.5-10.0)

  // Strobe — additive accent on top of FFT brightness
  float strobeSpeed = 0.3f;  // Sweep rate (0.0-3.0)
  float strobeDecay = 20.0f; // Flash sharpness (5.0-40.0)
  float strobeBoost =
      1.0f; // Strobe flash brightness added on top of FFT (0.0-5.0)

  // FFT mapping
  float baseFreq = 220.0f;    // Lowest visible frequency in Hz (20-880)
  int numOctaves = 5;         // Octave count (1-8)
  int segmentsPerOctave = 24; // Segments per octave (4-48)
  float gain = 5.0f;          // FFT magnitude amplifier (1-20)
  float curve = 2.0f;         // Contrast exponent on magnitude (0.5-4.0)
  float baseBright = 0.05f;   // Ember level for quiet semitones (0.0-0.5)

  // Color
  ColorConfig gradient = {.mode = COLOR_MODE_GRADIENT};

  // Blend compositing
  EffectBlendMode blendMode = EFFECT_BLEND_SCREEN;
  float blendIntensity = 1.0f;
};

typedef struct ColorLUT ColorLUT;

typedef struct ArcStrobeEffect {
  Shader shader;
  ColorLUT *gradientLUT;
  float strobeTime; // CPU-accumulated strobe time
  int resolutionLoc;
  int fftTextureLoc;
  int sampleRateLoc;
  int phaseLoc;
  int amplitudeLoc;
  int orbitOffsetLoc;
  int lineThicknessLoc;
  int freqX1Loc;
  int freqY1Loc;
  int freqX2Loc;
  int freqY2Loc;
  int offsetX2Loc;
  int offsetY2Loc;
  int glowIntensityLoc;
  int strobeSpeedLoc;
  int strobeTimeLoc;
  int strobeDecayLoc;
  int strobeBoostLoc;
  int baseFreqLoc;
  int numOctavesLoc;
  int segmentsPerOctaveLoc;
  int gainLoc;
  int curveLoc;
  int baseBrightLoc;
  int gradientLUTLoc;
} ArcStrobeEffect;

// Returns true on success, false if shader fails to load
bool ArcStrobeEffectInit(ArcStrobeEffect *e, const ArcStrobeConfig *cfg);

// Binds all uniforms including fftTexture, advances Lissajous phase and strobe
void ArcStrobeEffectSetup(ArcStrobeEffect *e, ArcStrobeConfig *cfg,
                         float deltaTime, Texture2D fftTexture);

// Unloads shader and frees LUT
void ArcStrobeEffectUninit(ArcStrobeEffect *e);

// Returns default config
ArcStrobeConfig ArcStrobeConfigDefault(void);

// Registers modulatable params with the modulation engine
void ArcStrobeRegisterParams(ArcStrobeConfig *cfg);

#endif // ARC_STROBE_H
