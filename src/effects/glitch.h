// Glitch video corruption effect module
// Analog/digital corruption through UV distortion, chromatic aberration, noise

#ifndef GLITCH_H
#define GLITCH_H

#include "raylib.h"
#include <stdbool.h>

// Glitch: Analog/digital video corruption through UV distortion, chromatic
// aberration, noise Modes enable automatically when their primary parameter > 0
struct GlitchConfig {
  bool enabled = false;

  // CRT mode: barrel distortion with edge vignette
  bool crtEnabled = false;
  float curvature = 0.1f; // Barrel strength (0-0.2)
  bool vignetteEnabled = true;

  // Analog mode: horizontal noise distortion with chromatic aberration
  // Enabled when analogIntensity > 0
  float analogIntensity = 0.0f; // Distortion amount (0-1). 0 = disabled.
  float aberration = 5.0f;      // Chromatic channel offset in pixels (0-20)

  // Digital mode: block displacement glitches
  // Enabled when blockThreshold > 0
  float blockThreshold = 0.0f; // Block probability (0-0.9). 0 = disabled.
  float blockOffset = 0.2f;    // Max displacement (0-0.5)

  // VHS mode: tracking bars and scanline noise
  bool vhsEnabled = false;
  float trackingBarIntensity = 0.02f;   // Bar strength (0-0.05)
  float scanlineNoiseIntensity = 0.01f; // Per-line jitter (0-0.02)
  float colorDriftIntensity = 1.0f;     // R/G channel drift (0-2.0)

  // Overlay: applied when any mode is active
  float scanlineAmount = 0.1f; // Scanline darkness (0-0.5)
  float noiseAmount = 0.05f;   // White noise (0-0.3)

  // Datamosh: variable resolution pixelation with diagonal bands
  bool datamoshEnabled = false;
  float datamoshIntensity = 1.0f; // Blend strength (0-1)
  float datamoshMin = 6.0f;       // Min resolution (4-32)
  float datamoshMax = 64.0f;      // Max resolution (16-128)
  float datamoshSpeed = 6.0f;     // Frame change rate (1-30)
  float datamoshBands = 8.0f;     // Diagonal band count (1-32)

  // Row Slice: horizontal displacement bursts
  bool rowSliceEnabled = false;
  float rowSliceIntensity = 0.1f;  // Displacement amount (0-0.5)
  float rowSliceBurstFreq = 4.0f;  // Burst frequency Hz (0.5-20)
  float rowSliceBurstPower = 7.0f; // Burst narrowness (1-15)
  float rowSliceColumns = 32.0f;   // Slice column width (8-128)

  // Column Slice: vertical displacement bursts
  bool colSliceEnabled = false;
  float colSliceIntensity = 0.1f;  // Displacement amount (0-0.5)
  float colSliceBurstFreq = 4.0f;  // Burst frequency Hz (0.5-20)
  float colSliceBurstPower = 7.0f; // Burst narrowness (1-15)
  float colSliceRows = 32.0f;      // Slice row height (8-128)

  // Diagonal Bands: UV displacement along 45-degree stripes
  bool diagonalBandsEnabled = false;
  float diagonalBandCount = 8.0f;     // Number of bands (2-32)
  float diagonalBandDisplace = 0.02f; // Displacement amount (0-0.1)
  float diagonalBandSpeed = 1.0f;     // Animation speed (0-10)

  // Block Mask: random block color tinting
  bool blockMaskEnabled = false;
  float blockMaskIntensity = 0.5f; // Tint strength (0-1)
  int blockMaskMinSize = 1;        // Min block size (1-10)
  int blockMaskMaxSize = 10;       // Max block size (5-20)
  float blockMaskTintR = 1.0f;     // Tint color R (0-1)
  float blockMaskTintG = 0.0f;     // Tint color G (0-1)
  float blockMaskTintB = 0.5f;     // Tint color B (0-1)

  // Temporal Jitter: radial spatial displacement
  bool temporalJitterEnabled = false;
  float temporalJitterAmount = 0.02f; // Jitter strength (0-0.1)
  float temporalJitterGate = 0.3f;    // Probability threshold (0-1)

  // Block Multiply: recursive block UV folding with cross-sampling
  bool blockMultiplyEnabled = false;
  float blockMultiplySize = 40.0f; // Block size (4-64, larger = bigger blocks)
  float blockMultiplyControl = 0.1f;   // Block pattern mix factor (0-1)
  int blockMultiplyIterations = 6;     // Recursive passes (1-8)
  float blockMultiplyIntensity = 1.0f; // Blend with original (0-1)
};

typedef struct GlitchEffect {
  Shader shader;
  int resolutionLoc;
  int timeLoc;
  int frameLoc;
  int crtEnabledLoc;
  int curvatureLoc;
  int vignetteEnabledLoc;
  int analogIntensityLoc;
  int aberrationLoc;
  int blockThresholdLoc;
  int blockOffsetLoc;
  int vhsEnabledLoc;
  int trackingBarIntensityLoc;
  int scanlineNoiseIntensityLoc;
  int colorDriftIntensityLoc;
  int scanlineAmountLoc;
  int noiseAmountLoc;
  int datamoshEnabledLoc;
  int datamoshIntensityLoc;
  int datamoshMinLoc;
  int datamoshMaxLoc;
  int datamoshSpeedLoc;
  int datamoshBandsLoc;
  int rowSliceEnabledLoc;
  int rowSliceIntensityLoc;
  int rowSliceBurstFreqLoc;
  int rowSliceBurstPowerLoc;
  int rowSliceColumnsLoc;
  int colSliceEnabledLoc;
  int colSliceIntensityLoc;
  int colSliceBurstFreqLoc;
  int colSliceBurstPowerLoc;
  int colSliceRowsLoc;
  int diagonalBandsEnabledLoc;
  int diagonalBandCountLoc;
  int diagonalBandDisplaceLoc;
  int diagonalBandSpeedLoc;
  int blockMaskEnabledLoc;
  int blockMaskIntensityLoc;
  int blockMaskMinSizeLoc;
  int blockMaskMaxSizeLoc;
  int blockMaskTintLoc;
  int temporalJitterEnabledLoc;
  int temporalJitterAmountLoc;
  int temporalJitterGateLoc;
  int blockMultiplyEnabledLoc;
  int blockMultiplySizeLoc;
  int blockMultiplyControlLoc;
  int blockMultiplyIterationsLoc;
  int blockMultiplyIntensityLoc;
  float time;
  int frame;
} GlitchEffect;

// Returns true on success, false if shader fails to load
bool GlitchEffectInit(GlitchEffect *e);

// Accumulates time/frame and sets all uniforms
void GlitchEffectSetup(GlitchEffect *e, const GlitchConfig *cfg,
                       float deltaTime);

// Unloads shader
void GlitchEffectUninit(GlitchEffect *e);

// Returns default config
GlitchConfig GlitchConfigDefault(void);

#endif // GLITCH_H
